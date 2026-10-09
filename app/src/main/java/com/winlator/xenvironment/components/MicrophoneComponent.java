package com.winlator.xenvironment.components;

import android.Manifest;
import android.bluetooth.BluetoothAdapter;
import android.bluetooth.BluetoothHeadset;
import android.bluetooth.BluetoothManager;
import android.bluetooth.BluetoothProfile;
import android.content.BroadcastReceiver;
import android.content.Context;
import android.content.Intent;
import android.content.IntentFilter;
import android.content.pm.PackageManager;
import android.media.AudioDeviceCallback;
import android.media.AudioDeviceInfo;
import android.media.AudioFormat;
import android.media.AudioManager;
import android.media.AudioRecord;
import android.media.MediaRecorder;
import android.os.Build;
import android.os.Process;
import android.system.ErrnoException;
import android.system.Os;
import android.system.OsConstants;

import androidx.core.content.ContextCompat;

import com.winlator.xenvironment.EnvironmentComponent;

import java.io.File;
import java.io.FileDescriptor;
import java.io.InterruptedIOException;
import java.util.concurrent.atomic.AtomicBoolean;

import timber.log.Timber;

/**
 * Exposes the Android microphone to Wine/Proton as a real PulseAudio *source*.
 *
 * How it works
 * ------------
 * {@link PulseAudioComponent} loads PulseAudio's stock {@code module-pipe-source}, which creates a
 * FIFO and registers a capture device ("source") that reads raw PCM from it. This component captures
 * from the Android microphone with {@link AudioRecord} and feeds that PCM into the FIFO.
 *
 * Wine's {@code winepulse.drv} enumerates every PulseAudio source it finds, so the microphone shows
 * up inside the container as a normal Windows recording device with no Wine-side patching needed.
 *
 * Bluetooth routing (opt-in per container, never on XR builds)
 * -----------------
 * When disabled, no Bluetooth API is touched and only built-in, wired/aux and USB mics are used.
 * Android does not list a {@code TYPE_BLUETOOTH_SCO} input device from
 * {@code AudioManager.getDevices(GET_DEVICES_INPUTS)} until a SCO link is actually negotiated, so
 * simply watching device add/remove events and calling {@code setPreferredDevice()} never finds it.
 * Instead we:
 *   1. Track real headset connection with {@link BluetoothHeadset} (stable across reconnects, unlike
 *      {@link AudioDeviceCallback} which churns repeatedly during SCO setup).
 *   2. When a headset is connected, switch to {@code MODE_IN_COMMUNICATION}, start the SCO link and
 *      wait for {@code ACTION_SCO_AUDIO_STATE_UPDATED} to confirm it.
 *   3. Select the now-visible {@code TYPE_BLUETOOTH_SCO} device as the preferred input and open the
 *      recorder at 16 kHz or 8 kHz, upsampling to the FIFO's fixed 48 kHz format ourselves.
 * If SCO does not come up, the SCO recorder cannot be opened, or the headset disconnects, the session
 * is torn down (restoring the previous audio mode) and the built-in mic is used. The SCO session is
 * also released while paused.
 *
 * Safety / performance notes
 * --------------------------
 * - The FIFO is opened with {@code O_NONBLOCK}: if nothing inside the container is recording, the
 *   pipe fills up and writes fail fast with EAGAIN. We simply drop the chunk. We never block, never
 *   grow an unbounded buffer and never accumulate latency.
 * - Chunks are 20 ms (1920 bytes) which is below {@code PIPE_BUF} (4096), so every write is atomic:
 *   there is no risk of handing PulseAudio a torn frame.
 * - All work happens on one dedicated thread at {@code THREAD_PRIORITY_AUDIO}. It is driven by
 *   {@link AudioRecord#read} which blocks in the kernel, so it costs ~0% CPU and cannot stutter the
 *   game's render/emulation threads.
 * - Capture is fully released while the app is backgrounded (see {@link #pause()}/{@link #resume()}),
 *   so the mic indicator and the power cost only exist while the game is actually in the foreground.
 * - If the {@code RECORD_AUDIO} permission is missing, or {@code module-pipe-source.so} is not
 *   shipped, this component silently does nothing: audio output is never affected. Missing Bluetooth
 *   permissions only disable headset switching; the built-in mic keeps working.
 */
public class MicrophoneComponent extends EnvironmentComponent {
    private static final String TAG = "MicrophoneComponent";

    /** Must stay in sync with the module arguments written by {@link PulseAudioComponent}. */
    public static final int SAMPLE_RATE = 48000;
    public static final int CHANNEL_COUNT = 1;

    private static final int BYTES_PER_FRAME = 2 * CHANNEL_COUNT; // s16le
    private static final int CHUNK_FRAMES = SAMPLE_RATE / 50;     // 20 ms
    private static final int CHUNK_BYTES = CHUNK_FRAMES * BYTES_PER_FRAME; // 1920 (< PIPE_BUF)

    /** How long we wait for PulseAudio to create the FIFO before giving up. */
    private static final long FIFO_WAIT_TIMEOUT_MS = 10_000;
    private static final long FIFO_POLL_INTERVAL_MS = 100;

    /** How long we wait for the Bluetooth SCO link to come up before giving up on it for this attempt. */
    private static final long SCO_CONNECT_TIMEOUT_MS = 3_000;
    private static final long SCO_POLL_INTERVAL_MS = 50;

    /** Minimum gap between attempts to (re)bind the BluetoothHeadset profile proxy. */
    private static final long BLUETOOTH_PROXY_RETRY_INTERVAL_MS = 5_000;

    /**
     * Try common HFP/SCO capture rates, wideband first, then resample to the FIFO's fixed 48 kHz.
     * The requested AudioRecord rate does not prove the headset's negotiated codec or native rate.
     */
    private static final int[] BLUETOOTH_SCO_SAMPLE_RATES = {16000, 8000};
    private static final int[] BUILTIN_SAMPLE_RATES = {SAMPLE_RATE};

    /** Built-in, wired and USB mics presets, top one is used first, then fallbacks. */
    private static final int[] INPUT_PRESETS_BUILTIN = {
        MediaRecorder.AudioSource.VOICE_COMMUNICATION, // echo cancel + noise suppression when available
        MediaRecorder.AudioSource.MIC,
        MediaRecorder.AudioSource.UNPROCESSED,
        MediaRecorder.AudioSource.VOICE_RECOGNITION,
        MediaRecorder.AudioSource.DEFAULT
    };

    /**
     * Bluetooth SCO captures still route through the headset;
     * top one is used first, then fallbacks.
     */
    private static final int[] INPUT_PRESETS_BLUETOOTH = {
        MediaRecorder.AudioSource.VOICE_COMMUNICATION,
        MediaRecorder.AudioSource.VOICE_RECOGNITION,
        MediaRecorder.AudioSource.MIC,
        MediaRecorder.AudioSource.DEFAULT
    };

    private final File fifoFile;
    /** User opt-in for Bluetooth headset mics. When false, no Bluetooth APIs are touched at all. */
    private final boolean bluetoothEnabled;
    private final AtomicBoolean running = new AtomicBoolean(false);
    private final AtomicBoolean paused = new AtomicBoolean(false);
    /** Set whenever the input route may have changed; the capture loop then rebuilds the recorder. */
    private final AtomicBoolean routingDirty = new AtomicBoolean(true);

    /** True while BluetoothHeadset (the source of truth) reports a connected headset. */
    private final AtomicBoolean bluetoothHeadsetConnected = new AtomicBoolean(false);
    /** True once the SCO link is actually up, per ACTION_SCO_AUDIO_STATE_UPDATED. */
    private final AtomicBoolean bluetoothScoActive = new AtomicBoolean(false);
    /** Whether *we* started SCO/changed the audio mode, so we know what to clean up on teardown. */
    private final AtomicBoolean weOwnScoSession = new AtomicBoolean(false);

    private volatile AudioManager audioManager;
    private volatile AudioDeviceCallback audioDeviceCallback;
    private volatile BroadcastReceiver scoStateReceiver;
    private volatile BroadcastReceiver headsetConnectionReceiver;
    private volatile BluetoothAdapter bluetoothAdapter;
    private volatile BluetoothProfile bluetoothHeadsetProxy;
    private volatile boolean bluetoothProxyBound = false;
    private volatile boolean bluetoothProxyRequestInFlight = false;
    private volatile long lastBluetoothProxyRetryAtMs = 0;
    private volatile int previousAudioMode = AudioManager.MODE_NORMAL;

    private volatile Thread worker;
    /**
     * The sample rate the current {@link AudioRecord} was opened at (lower than {@link #SAMPLE_RATE}
     * for Bluetooth SCO); captureLoop resamples to SAMPLE_RATE before writing.
     */
    private volatile int recorderSampleRate = SAMPLE_RATE;

    // ---------------------------------------------------------------------------------------
    // Runtime controls (driven from the in-game quick menu). Static so the UI can reach them
    // without plumbing a component reference through XServerScreen (dex register limit).
    // ---------------------------------------------------------------------------------------

    public static final int MIN_GAIN_PERCENT = 0;
    public static final int MAX_GAIN_PERCENT = 300;

    /** Software gain applied to captured PCM before it reaches PulseAudio (1.0 = unchanged). */
    private static volatile float softwareGain = 1f;
    /** When true, silence is written instead of mic audio (stream stays alive for the game). */
    private static volatile boolean muted = false;
    /** Human readable description of the active input, for UI/debugging. */
    private static volatile String activeInputDescription = null;

    public static void setGainPercent(int percent) {
        int clamped = Math.max(MIN_GAIN_PERCENT, Math.min(MAX_GAIN_PERCENT, percent));
        softwareGain = clamped / 100f;
        Timber.tag(TAG).d("Mic software gain set to %d%%", clamped);
    }

    public static int getGainPercent() {
        return Math.round(softwareGain * 100f);
    }

    public static void setMuted(boolean value) {
        if (muted != value) Timber.tag(TAG).d("Mic muted=%s", value);
        muted = value;
    }

    public static boolean isMuted() {
        return muted;
    }

    /** e.g. "VOICE_COMMUNICATION @ 48000 Hz via BUILTIN_MIC", or null when not capturing. */
    public static String getActiveInputDescription() {
        return activeInputDescription;
    }

    /**
     * Notified whenever {@link #activeInputDescription} changes (input opened/closed/switched).
     * Lets the quick menu reflect the active input without polling. Listeners run on the thread
     * publishing the change and must be thread-safe and non-blocking.
     */
    private static final java.util.concurrent.CopyOnWriteArrayList<Runnable> activeInputListeners =
        new java.util.concurrent.CopyOnWriteArrayList<>();

    public static void addActiveInputListener(Runnable listener) {
        activeInputListeners.add(listener);
    }

    public static void removeActiveInputListener(Runnable listener) {
        activeInputListeners.remove(listener);
    }

    private static void setActiveInputDescription(String description) {
        if (java.util.Objects.equals(activeInputDescription, description)) return;
        activeInputDescription = description;
        for (Runnable listener : activeInputListeners) {
            try {
                listener.run();
            } catch (Throwable ignored) {
            }
        }
    }

    /** The recorder currently feeding the FIFO; activeInputDescription belongs to it. */
    private volatile AudioRecord activeRecorder;

    public MicrophoneComponent(File fifoFile, boolean bluetoothEnabled) {
        this.fifoFile = fifoFile;
        this.bluetoothEnabled = bluetoothEnabled;
    }

    @Override
    public void start() {
        if (fifoFile == null) return;
        if (!running.compareAndSet(false, true)) return;

        paused.set(false);
        routingDirty.set(true);
        loadPersistedControls();

        Thread thread = new Thread(this::captureLoop, "GN-MicrophoneBridge");
        thread.setDaemon(true);
        worker = thread;
        thread.start();
        Timber.tag(TAG).d("Started (fifo=%s, bluetooth=%s)", fifoFile, bluetoothEnabled);
    }

    @Override
    public void stop() {
        if (!running.compareAndSet(true, false)) return;

        Thread thread = worker;
        worker = null;
        if (thread != null) {
            thread.interrupt();
            wakeWorker();
            try {
                thread.join(1500);
            } catch (InterruptedException e) {
                Thread.currentThread().interrupt();
            }
        }
        // The worker's finally block owns all cleanup, so this also covers early-exit paths.
        Timber.tag(TAG).d("Stopped");
    }

    private static void loadPersistedControls() {
        try {
            setGainPercent(app.gamenative.PrefManager.INSTANCE.getMicGainPercent());
        } catch (Throwable t) {
            Timber.tag(TAG).w("Could not load mic preferences: %s", t.getMessage());
        }
        // Mute is intentionally session-only so a game never starts with a silently dead mic.
        muted = false;
    }

    /** Releases the microphone while the activity is in the background. */
    public void pause() {
        paused.set(true);
    }

    /** Re-acquires the microphone when the activity comes back to the foreground. */
    public void resume() {
        paused.set(false);
        wakeWorker();
    }

    /** Wakes the capture thread out of its idle wait (see the paused branch in captureLoop). */
    private synchronized void wakeWorker() {
        notifyAll();
    }

    private Context getContext() {
        // The same component can be transferred to a new activity's XEnvironment, so register and
        // unregister receivers on the stable application context rather than on that activity.
        Context context = environment != null ? environment.getContext() : null;
        return context != null ? context.getApplicationContext() : null;
    }

    private boolean isGranted(String permission) {
        Context context = getContext();
        return context != null
            && ContextCompat.checkSelfPermission(context, permission) == PackageManager.PERMISSION_GRANTED;
    }

    private void captureLoop() {
        Process.setThreadPriority(Process.THREAD_PRIORITY_AUDIO);

        FileDescriptor fd = null;
        AudioRecord recorder = null;
        byte[] readBuffer = new byte[CHUNK_BYTES];
        final byte[] resampleBuffer = new byte[CHUNK_BYTES];
        int activeSampleRate = SAMPLE_RATE;
        int activeChunkBytes = CHUNK_BYTES;
        long droppedChunks = 0;

        try {
            if (!isGranted(Manifest.permission.RECORD_AUDIO)) {
                Timber.tag(TAG).w("RECORD_AUDIO permission not granted, microphone bridge disabled");
                return;
            }

            registerAudioDeviceCallback();
            if (bluetoothEnabled) {
                registerScoStateReceiver();
                registerBluetoothHeadsetProxy();
            }

            fd = awaitAndOpenFifo();
            if (fd == null) return;

            while (running.get() && !Thread.currentThread().isInterrupted()) {
                if (paused.get()) {
                    recorder = releaseRecorder(recorder);
                    // Don't keep the phone in call mode / SCO while backgrounded; renegotiated on resume.
                    teardownBluetoothScoSession();
                    synchronized (this) {
                        // Idle without polling until resume() or stop() wakes us.
                        while (running.get() && paused.get()) wait();
                    }
                    continue;
                }

                if (routingDirty.getAndSet(false) && recorder != null) {
                    recorder = releaseRecorder(recorder);
                    continue;
                }

                maybeRetryBluetoothProxyRegistration();

                if (recorder == null) {
                    recorder = createRecorder();
                    if (recorder == null) {
                        // Another app may hold exclusive access; back off instead of spinning.
                        sleep(1000);
                        continue;
                    }
                    activeSampleRate = recorderSampleRate;
                    activeChunkBytes = (activeSampleRate / 50) * BYTES_PER_FRAME; // 20 ms at this rate
                    if (readBuffer.length != activeChunkBytes) readBuffer = new byte[activeChunkBytes];
                }

                int read = recorder.read(readBuffer, 0, activeChunkBytes);
                if (read <= 0) {
                    if (read == AudioRecord.ERROR_INVALID_OPERATION || read == AudioRecord.ERROR_BAD_VALUE
                        || read == AudioRecord.ERROR_DEAD_OBJECT) {
                        // Typically a device disconnect (USB/BT mic unplugged). Rebuild the stream.
                        Timber.tag(TAG).w("AudioRecord.read() failed (%d), reopening", read);
                        recorder = releaseRecorder(recorder);
                        sleep(250);
                    }
                    continue;
                }

                byte[] chunk = readBuffer;
                int chunkLength = read;
                if (activeSampleRate != SAMPLE_RATE) {
                    chunkLength = resampleTo48k(readBuffer, read, activeSampleRate, resampleBuffer);
                    chunk = resampleBuffer;
                    if (chunkLength <= 0) continue;
                }

                applyGainAndMute(chunk, chunkLength);

                if (!writeChunk(fd, chunk, chunkLength)) {
                    // Nobody inside the container is recording right now, the pipe is full.
                    if ((++droppedChunks % 500) == 0) {
                        Timber.tag(TAG).d("No PulseAudio consumer, dropped %d chunks so far", droppedChunks);
                    }
                }
            }
        } catch (InterruptedException e) {
            Thread.currentThread().interrupt();
        } catch (Throwable t) {
            Timber.tag(TAG).e(t, "Microphone bridge terminated");
        } finally {
            // Runs for every exit path (permission missing, FIFO timeout, stop(), error), so nothing
            // registered above can outlive the worker.
            releaseRecorder(recorder);
            teardownBluetoothScoSession();
            closeQuietly(fd);
            unregisterAudioDeviceCallback();
            unregisterScoStateReceiver();
            unregisterBluetoothHeadsetProxy();
            setActiveInputDescription(null);
            running.set(false);
        }
    }

    /**
     * PulseAudio creates the FIFO from {@code module-pipe-source}'s init. Wait for it, then open the
     * write end non-blocking. The module keeps the FIFO open O_RDWR, so opening never blocks and we
     * can never see EOF/EPIPE while the server is alive.
     */
    private FileDescriptor awaitAndOpenFifo() {
        long deadline = System.currentTimeMillis() + FIFO_WAIT_TIMEOUT_MS;

        while (running.get() && System.currentTimeMillis() < deadline) {
            if (fifoFile.exists()) {
                try {
                    return Os.open(fifoFile.getAbsolutePath(),
                        OsConstants.O_WRONLY | OsConstants.O_NONBLOCK, 0);
                } catch (ErrnoException e) {
                    if (e.errno != OsConstants.ENXIO && e.errno != OsConstants.ENOENT) {
                        Timber.tag(TAG).e("Unable to open mic FIFO: %s", e.getMessage());
                        return null;
                    }
                    // Reader side not ready yet, retry.
                }
            }
            if (!sleep(FIFO_POLL_INTERVAL_MS)) return null;
        }

        Timber.tag(TAG).w("PulseAudio mic source FIFO never appeared (%s); is module-pipe-source.so shipped?",
            fifoFile);
        return null;
    }

    /** @return true when the whole chunk was handed to PulseAudio, false when it was dropped. */
    private boolean writeChunk(FileDescriptor fd, byte[] buffer, int length) throws ErrnoException {
        int offset = 0;
        while (offset < length) {
            try {
                int written = Os.write(fd, buffer, offset, length - offset);
                if (written <= 0) return false;
                offset += written;
            } catch (ErrnoException e) {
                // On Linux EAGAIN == EWOULDBLOCK: the pipe is full, nothing is recording.
                if (e.errno == OsConstants.EAGAIN) return false;
                if (e.errno == OsConstants.EINTR) continue;
                throw e;
            } catch (InterruptedIOException e) {
                return false;
            }
        }
        return true;
    }

    private AudioRecord createRecorder() {
        if (bluetoothEnabled && bluetoothHeadsetConnected.get() && hasBluetoothHeadsetPermissions()) {
            AudioDeviceInfo bluetoothInput = negotiateBluetoothScoAndFindDevice();
            if (bluetoothInput == null) {
                Timber.tag(TAG).w("Bluetooth headset connected but SCO link did not come up; using built-in mic");
            } else {
                AudioRecord recorder = openRecorder(INPUT_PRESETS_BLUETOOTH, BLUETOOTH_SCO_SAMPLE_RATES, bluetoothInput);
                if (recorder != null) return recorder;
                Timber.tag(TAG).w("Bluetooth SCO input could not be opened; using built-in mic");
            }
        }

        // Built-in / wired / USB path: never leave the phone in call mode or SCO behind us.
        teardownBluetoothScoSession();
        AudioRecord recorder = openRecorder(INPUT_PRESETS_BUILTIN, BUILTIN_SAMPLE_RATES, null);
        if (recorder == null) Timber.tag(TAG).w("No usable microphone input could be opened");
        return recorder;
    }

    /**
     * Tries every sample rate / audio source combination in order and returns the first recorder that
     * actually starts. {@code preferredDevice} is the Bluetooth SCO input, or null for the default route.
     */
    private AudioRecord openRecorder(int[] presets, int[] sampleRates, AudioDeviceInfo preferredDevice) {
        boolean bluetooth = preferredDevice != null;

        for (int rate : sampleRates) {
            int chunkBytesAtRate = (rate / 50) * BYTES_PER_FRAME;
            int minBufferSize = AudioRecord.getMinBufferSize(
                rate, AudioFormat.CHANNEL_IN_MONO, AudioFormat.ENCODING_PCM_16BIT);
            if (minBufferSize <= 0) {
                if (bluetooth) {
                    Timber.tag(TAG).d("Bluetooth SCO rate %d Hz not supported by HAL, skipping", rate);
                    continue; // this SCO rate isn't supported by the HAL, try the next
                }
                minBufferSize = chunkBytesAtRate * 4;
            }
            int bufferSize = Math.max(minBufferSize, chunkBytesAtRate * 4);

            for (int source : presets) {
                String sourceName = audioSourceName(source);
                AudioRecord recorder = null;
                try {
                    recorder = new AudioRecord(source, rate, AudioFormat.CHANNEL_IN_MONO,
                        AudioFormat.ENCODING_PCM_16BIT, bufferSize);

                    if (recorder.getState() != AudioRecord.STATE_INITIALIZED) {
                        Timber.tag(TAG).d("Audio source %s @ %d Hz rejected: not initialized", sourceName, rate);
                    } else if (bluetooth && !recorder.setPreferredDevice(preferredDevice)) {
                        Timber.tag(TAG).d("Audio source %s @ %d Hz rejected: SCO input not accepted", sourceName, rate);
                    } else {
                        recorder.startRecording();
                        if (recorder.getRecordingState() == AudioRecord.RECORDSTATE_RECORDING) {
                            AudioDeviceInfo routed = recorder.getRoutedDevice();
                            if (bluetooth && routed != null && routed.getType() != AudioDeviceInfo.TYPE_BLUETOOTH_SCO) {
                                // Started, but the framework routed it elsewhere: not a real headset capture.
                                Timber.tag(TAG).d("Audio source %s @ %d Hz rejected: routed to %s", sourceName, rate, describeDevice(routed));
                            } else {
                                String routedDesc = describeDevice(routed);
                                Timber.tag(TAG).i("Capturing at %d Hz mono: audioSource=%s (%d), bluetooth=%s, routed=%s",
                                    rate, sourceName, source, bluetooth, routedDesc);
                                recorderSampleRate = rate;
                                activeRecorder = recorder;
                                setActiveInputDescription(sourceName + " @ " + rate + " Hz via " + routedDesc);
                                return recorder;
                            }
                        } else {
                            Timber.tag(TAG).d("Audio source %s @ %d Hz rejected: startRecording() did not start", sourceName, rate);
                        }
                    }
                } catch (SecurityException e) {
                    Timber.tag(TAG).w("RECORD_AUDIO denied while opening AudioRecord");
                    releaseRecorder(recorder);
                    return null;
                } catch (Exception e) {
                    Timber.tag(TAG).d("Audio source %s @ %d Hz unavailable: %s", sourceName, rate, e.getMessage());
                }
                releaseRecorder(recorder);
            }
        }
        return null;
    }

    private static String audioSourceName(int source) {
        switch (source) {
            case MediaRecorder.AudioSource.DEFAULT: return "DEFAULT";
            case MediaRecorder.AudioSource.MIC: return "MIC";
            case MediaRecorder.AudioSource.VOICE_RECOGNITION: return "VOICE_RECOGNITION";
            case MediaRecorder.AudioSource.VOICE_COMMUNICATION: return "VOICE_COMMUNICATION";
            case MediaRecorder.AudioSource.UNPROCESSED: return "UNPROCESSED";
            default: return "SOURCE_" + source;
        }
    }

    private static String deviceTypeName(int type) {
        switch (type) {
            case AudioDeviceInfo.TYPE_BUILTIN_MIC: return "BUILTIN_MIC";
            case AudioDeviceInfo.TYPE_BLUETOOTH_SCO: return "BLUETOOTH_SCO";
            case AudioDeviceInfo.TYPE_WIRED_HEADSET: return "WIRED_HEADSET";
            case AudioDeviceInfo.TYPE_USB_DEVICE: return "USB_DEVICE";
            case AudioDeviceInfo.TYPE_USB_HEADSET: return "USB_HEADSET";
            case AudioDeviceInfo.TYPE_USB_ACCESSORY: return "USB_ACCESSORY";
            case AudioDeviceInfo.TYPE_LINE_ANALOG: return "LINE_ANALOG";
            case AudioDeviceInfo.TYPE_BLE_HEADSET: return "BLE_HEADSET";
            default: return "TYPE_" + type;
        }
    }

    /** Shown in the quick menu, e.g. {@code BLUETOOTH_SCO 'WH-1000XM4'}. */
    private static String describeDevice(AudioDeviceInfo device) {
        if (device == null) return "default";
        CharSequence product = device.getProductName();
        return deviceTypeName(device.getType()) + (product != null && product.length() > 0 ? " '" + product + "'" : "");
    }


    /** In-place software gain (with clipping) or silence, on s16le mono PCM. */
    private static void applyGainAndMute(byte[] buffer, int length) {
        if (muted) {
            java.util.Arrays.fill(buffer, 0, length, (byte) 0);
            return;
        }
        float gain = softwareGain;
        if (gain == 1f) return;
        for (int i = 0; i + 1 < length; i += 2) {
            int sample = (short) ((buffer[i] & 0xFF) | (buffer[i + 1] << 8));
            int scaled = Math.round(sample * gain);
            if (scaled > Short.MAX_VALUE) scaled = Short.MAX_VALUE;
            else if (scaled < Short.MIN_VALUE) scaled = Short.MIN_VALUE;
            buffer[i] = (byte) (scaled & 0xFF);
            buffer[i + 1] = (byte) ((scaled >> 8) & 0xFF);
        }
    }

    /**
     * Linear-interpolation upsampler from {@code fromRate} (8/16 kHz Bluetooth SCO) to
     * {@link #SAMPLE_RATE}. Good enough for voice-band speech and far cheaper than a polyphase
     * resampler. Short reads are handled by scaling the output length to the input length, so
     * timing is preserved.
     *
     * @param out reused across calls; must hold at least {@link #CHUNK_BYTES}.
     * @return the number of bytes written to {@code out}.
     */
    private static int resampleTo48k(byte[] in, int inLength, int fromRate, byte[] out) {
        int inFrames = inLength / BYTES_PER_FRAME;
        if (inFrames <= 0) return 0;
        int outFrames = (int) Math.min((long) inFrames * SAMPLE_RATE / fromRate, out.length / BYTES_PER_FRAME);
        double step = (double) fromRate / SAMPLE_RATE;

        for (int i = 0; i < outFrames; i++) {
            double srcPos = i * step;
            int idx0 = Math.min((int) srcPos, inFrames - 1);
            int idx1 = Math.min(idx0 + 1, inFrames - 1);
            double frac = srcPos - idx0;

            int off0 = idx0 * BYTES_PER_FRAME;
            int off1 = idx1 * BYTES_PER_FRAME;
            short s0 = (short) ((in[off0] & 0xFF) | (in[off0 + 1] << 8));
            short s1 = (short) ((in[off1] & 0xFF) | (in[off1 + 1] << 8));
            short sample = (short) (s0 + (s1 - s0) * frac);

            int outOff = i * BYTES_PER_FRAME;
            out[outOff] = (byte) (sample & 0xFF);
            out[outOff + 1] = (byte) ((sample >> 8) & 0xFF);
        }
        return outFrames * BYTES_PER_FRAME;
    }

    private AudioRecord releaseRecorder(AudioRecord recorder) {
        if (recorder == null) return null;
        if (recorder == activeRecorder) {
            activeRecorder = null;
            setActiveInputDescription(null);
        }
        try {
            if (recorder.getRecordingState() == AudioRecord.RECORDSTATE_RECORDING) recorder.stop();
        } catch (Exception ignored) {
        }
        try {
            recorder.release();
        } catch (Exception ignored) {
        }
        return null;
    }

    private void closeQuietly(FileDescriptor fd) {
        if (fd == null) return;
        try {
            Os.close(fd);
        } catch (ErrnoException ignored) {
        }
    }

    private boolean sleep(long millis) {
        try {
            Thread.sleep(millis);
            return true;
        } catch (InterruptedException e) {
            Thread.currentThread().interrupt();
            return false;
        }
    }

    // ---------------------------------------------------------------------------------------
    // Bluetooth headset detection (source of truth for "is there actually a BT mic to use?")
    // ---------------------------------------------------------------------------------------

    /**
     * Android 12+: BLUETOOTH_CONNECT is required. Legacy targetSdk (< 31) builds on Android 12+ can
     * still hit framework paths that enforce the old BLUETOOTH permission as well. Below Android 12
     * only BLUETOOTH is needed (install-time, granted whenever it is declared).
     */
    private boolean hasBluetoothHeadsetPermissions() {
        boolean hasLegacy = isGranted(Manifest.permission.BLUETOOTH);
        if (Build.VERSION.SDK_INT < Build.VERSION_CODES.S) return hasLegacy;
        if (!isGranted(Manifest.permission.BLUETOOTH_CONNECT)) return false;
        return getAppTargetSdk() >= Build.VERSION_CODES.S || hasLegacy;
    }

    private int getAppTargetSdk() {
        Context context = getContext();
        if (context == null || context.getApplicationInfo() == null) return Build.VERSION.SDK_INT;
        return context.getApplicationInfo().targetSdkVersion;
    }

    private synchronized void registerBluetoothHeadsetProxy() {
        if (bluetoothProxyBound || bluetoothProxyRequestInFlight) return;
        if (!hasBluetoothHeadsetPermissions()) {
            Timber.tag(TAG).i("Bluetooth permissions missing; Bluetooth mic switching disabled, using built-in mic");
            return;
        }
        Context context = getContext();
        if (context == null) return;

        BluetoothManager manager = (BluetoothManager) context.getSystemService(Context.BLUETOOTH_SERVICE);
        BluetoothAdapter adapter = manager != null ? manager.getAdapter() : null;
        if (adapter == null) {
            Timber.tag(TAG).i("No BluetoothAdapter on this device; Bluetooth mic switching unavailable");
            return;
        }
        bluetoothAdapter = adapter;

        BluetoothProfile.ServiceListener listener = new BluetoothProfile.ServiceListener() {
            @Override
            public void onServiceConnected(int profile, BluetoothProfile proxy) {
                bluetoothProxyRequestInFlight = false;
                if (!running.get()) {
                    // Bound after stop(): release it straight away instead of leaking the proxy.
                    try {
                        adapter.closeProfileProxy(BluetoothProfile.HEADSET, proxy);
                    } catch (Exception ignored) {
                    }
                    return;
                }
                bluetoothHeadsetProxy = proxy;
                bluetoothProxyBound = true;
                Timber.tag(TAG).d("BluetoothHeadset profile proxy bound");
                updateBluetoothHeadsetConnectedState(proxy);
            }

            @Override
            public void onServiceDisconnected(int profile) {
                // Bluetooth was turned off. Keep the proxy: the framework rebinds it (and calls
                // onServiceConnected again) when Bluetooth comes back, so requesting a new one would leak this one.
                if (bluetoothHeadsetConnected.compareAndSet(true, false)) {
                    routingDirty.set(true);
                }
            }
        };

        try {
            bluetoothProxyRequestInFlight = adapter.getProfileProxy(context, listener, BluetoothProfile.HEADSET);
        } catch (Exception e) {
            Timber.tag(TAG).w("Failed to bind BluetoothHeadset profile: %s", e.getMessage());
            bluetoothProxyRequestInFlight = false;
        }

        if (headsetConnectionReceiver == null) {
            BroadcastReceiver receiver = new BroadcastReceiver() {
                @Override
                public void onReceive(Context ctx, Intent intent) {
                    BluetoothProfile proxy = bluetoothHeadsetProxy;
                    if (proxy != null) updateBluetoothHeadsetConnectedState(proxy);
                }
            };
            try {
                ContextCompat.registerReceiver(context, receiver,
                    new IntentFilter(BluetoothHeadset.ACTION_CONNECTION_STATE_CHANGED), ContextCompat.RECEIVER_EXPORTED);
                headsetConnectionReceiver = receiver;
            } catch (Exception e) {
                Timber.tag(TAG).w("Failed to register BluetoothHeadset state receiver: %s", e.getMessage());
            }
        }
    }

    /**
     * The proxy bind can fail (permission granted after the component started, adapter not ready,
     * etc). Cheaply re-attempt from the capture loop until it succeeds, so a permission granted
     * mid-session is picked up without restarting the game.
     */
    private void maybeRetryBluetoothProxyRegistration() {
        if (!bluetoothEnabled || bluetoothProxyBound || bluetoothProxyRequestInFlight) return;
        long now = System.currentTimeMillis();
        if (now - lastBluetoothProxyRetryAtMs < BLUETOOTH_PROXY_RETRY_INTERVAL_MS) return;
        lastBluetoothProxyRetryAtMs = now;
        if (hasBluetoothHeadsetPermissions()) registerBluetoothHeadsetProxy();
    }

    private void updateBluetoothHeadsetConnectedState(BluetoothProfile proxy) {
        boolean connected;
        try {
            connected = !proxy.getConnectedDevices().isEmpty();
        } catch (SecurityException e) {
            connected = false;
        }
        if (bluetoothHeadsetConnected.compareAndSet(!connected, connected)) {
            Timber.tag(TAG).d("Bluetooth headset connected=%s", connected);
            routingDirty.set(true);
        }
    }

    private synchronized void unregisterBluetoothHeadsetProxy() {
        Context context = getContext();

        BroadcastReceiver receiver = headsetConnectionReceiver;
        headsetConnectionReceiver = null;
        if (context != null && receiver != null) {
            try {
                context.unregisterReceiver(receiver);
            } catch (Exception ignored) {
            }
        }

        BluetoothAdapter adapter = bluetoothAdapter;
        BluetoothProfile proxy = bluetoothHeadsetProxy;
        bluetoothAdapter = null;
        bluetoothHeadsetProxy = null;
        bluetoothProxyBound = false;
        bluetoothProxyRequestInFlight = false;
        bluetoothHeadsetConnected.set(false);
        if (adapter != null && proxy != null) {
            try {
                adapter.closeProfileProxy(BluetoothProfile.HEADSET, proxy);
            } catch (Exception ignored) {
            }
        }
    }

    // ---------------------------------------------------------------------------------------
    // SCO link negotiation
    // ---------------------------------------------------------------------------------------

    private void registerScoStateReceiver() {
        Context context = getContext();
        if (context == null || scoStateReceiver != null) return;

        BroadcastReceiver receiver = new BroadcastReceiver() {
            @Override
            public void onReceive(Context ctx, Intent intent) {
                int state = intent.getIntExtra(AudioManager.EXTRA_SCO_AUDIO_STATE,
                    AudioManager.SCO_AUDIO_STATE_ERROR);
                boolean connected = state == AudioManager.SCO_AUDIO_STATE_CONNECTED;
                boolean previous = bluetoothScoActive.getAndSet(connected);
                if (previous == connected) return;
                Timber.tag(TAG).d("Bluetooth SCO state changed: %s -> %s", previous, connected);
                // CONNECTED is the expected result of our own negotiation (polled there), so only a
                // link drop under a live headset capture needs the recorder rebuilt.
                if (!connected && weOwnScoSession.get() && activeRecorder != null) routingDirty.set(true);
            }
        };

        try {
            ContextCompat.registerReceiver(context, receiver,
                new IntentFilter(AudioManager.ACTION_SCO_AUDIO_STATE_UPDATED), ContextCompat.RECEIVER_EXPORTED);
            scoStateReceiver = receiver;
        } catch (Exception e) {
            Timber.tag(TAG).w("Failed to register SCO state receiver: %s", e.getMessage());
        }
    }

    private void unregisterScoStateReceiver() {
        Context context = getContext();
        BroadcastReceiver receiver = scoStateReceiver;
        scoStateReceiver = null;
        if (context == null || receiver == null) return;
        try {
            context.unregisterReceiver(receiver);
        } catch (Exception ignored) {
        }
    }

    /**
     * Starts (or reuses) the Bluetooth SCO link and blocks the capture thread briefly until it comes
     * up, then returns the now-visible {@code TYPE_BLUETOOTH_SCO} input device. Returns null if the
     * link never came up within the timeout; callers must fall back to the built-in path.
     */
    private AudioDeviceInfo negotiateBluetoothScoAndFindDevice() {
        AudioManager manager = getAudioManager();
        if (manager == null) return null;

        if (bluetoothScoActive.get()) {
            AudioDeviceInfo device = findBluetoothScoInputDevice(manager);
            if (device != null) return device;
        }

        try {
            if (weOwnScoSession.compareAndSet(false, true)) {
                previousAudioMode = manager.getMode();
                manager.setMode(AudioManager.MODE_IN_COMMUNICATION);
            }
            manager.startBluetoothSco();
            manager.setBluetoothScoOn(true);
        } catch (Exception e) {
            Timber.tag(TAG).w("Failed to start Bluetooth SCO: %s", e.getMessage());
            return null;
        }

        long deadline = System.currentTimeMillis() + SCO_CONNECT_TIMEOUT_MS;
        while (running.get() && !paused.get() && System.currentTimeMillis() < deadline) {
            if (bluetoothScoActive.get()) {
                AudioDeviceInfo device = findBluetoothScoInputDevice(manager);
                if (device != null) return device;
            }
            if (!bluetoothHeadsetConnected.get()) return null; // disconnected mid-negotiation
            if (!sleep(SCO_POLL_INTERVAL_MS)) return null;
        }
        return null;
    }

    private AudioDeviceInfo findBluetoothScoInputDevice(AudioManager manager) {
        for (AudioDeviceInfo input : manager.getDevices(AudioManager.GET_DEVICES_INPUTS)) {
            if (input.getType() == AudioDeviceInfo.TYPE_BLUETOOTH_SCO) return input;
        }
        return null;
    }

    /** Stops SCO and restores the previous audio mode, but only if we were the ones who changed them. */
    private void teardownBluetoothScoSession() {
        if (!weOwnScoSession.compareAndSet(true, false)) return;

        AudioManager manager = getAudioManager();
        if (manager != null) {
            // Separate try blocks: a failure in one call must not skip restoring the audio mode.
            try {
                manager.stopBluetoothSco();
            } catch (Exception ignored) {
            }
            try {
                manager.setBluetoothScoOn(false);
            } catch (Exception ignored) {
            }
            try {
                // Only restore if we are still the one holding the mode; don't stomp on another owner.
                if (manager.getMode() == AudioManager.MODE_IN_COMMUNICATION) manager.setMode(previousAudioMode);
            } catch (Exception ignored) {
            }
        }
        bluetoothScoActive.set(false);
    }

    // ---------------------------------------------------------------------------------------
    // Wired/USB input changes. Bluetooth is deliberately ignored here: BluetoothHeadset above is
    // the authoritative signal, since SCO negotiation itself causes noisy add/remove churn.
    // ---------------------------------------------------------------------------------------

    private void registerAudioDeviceCallback() {
        AudioManager manager = getAudioManager();
        if (manager == null || audioDeviceCallback != null) return;

        AudioDeviceCallback callback = new AudioDeviceCallback() {
            @Override
            public void onAudioDevicesAdded(AudioDeviceInfo[] addedDevices) {
                if (hasNonBluetoothInputDevice(addedDevices)) routingDirty.set(true);
            }

            @Override
            public void onAudioDevicesRemoved(AudioDeviceInfo[] removedDevices) {
                if (hasNonBluetoothInputDevice(removedDevices)) routingDirty.set(true);
            }
        };

        try {
            manager.registerAudioDeviceCallback(callback, null);
            audioDeviceCallback = callback;
        } catch (Exception e) {
            Timber.tag(TAG).w("Failed to register audio device callback: %s", e.getMessage());
        }
    }

    private void unregisterAudioDeviceCallback() {
        AudioManager manager = audioManager;
        AudioDeviceCallback callback = audioDeviceCallback;
        audioDeviceCallback = null;
        if (manager == null || callback == null) return;

        try {
            manager.unregisterAudioDeviceCallback(callback);
        } catch (Exception ignored) {
        }
    }

    private static boolean hasNonBluetoothInputDevice(AudioDeviceInfo[] devices) {
        if (devices == null) return false;
        for (AudioDeviceInfo device : devices) {
            if (device == null || !device.isSource()) continue;
            int type = device.getType();
            if (type == AudioDeviceInfo.TYPE_BLUETOOTH_SCO || type == AudioDeviceInfo.TYPE_BLE_HEADSET) continue;
            return true;
        }
        return false;
    }

    private AudioManager getAudioManager() {
        AudioManager manager = audioManager;
        if (manager != null) return manager;

        Context context = getContext();
        if (context == null) return null;

        manager = (AudioManager) context.getSystemService(Context.AUDIO_SERVICE);
        audioManager = manager;
        return manager;
    }
}
