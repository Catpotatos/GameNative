package com.winlator.xserver.extensions;

import static com.winlator.xserver.XClientRequestHandler.RESPONSE_CODE_SUCCESS;

import android.util.SparseArray;

import com.winlator.renderer.GPUImage;
import com.winlator.renderer.Texture;
import com.winlator.renderer.VulkanRenderer;
import com.winlator.renderer.XServerRenderer;
import com.winlator.xconnector.XInputStream;
import com.winlator.xconnector.XOutputStream;
import com.winlator.xconnector.XStreamLock;
import com.winlator.xenvironment.components.VortekRendererComponent;
import com.winlator.xserver.Bitmask;
import com.winlator.xserver.Drawable;
import com.winlator.xserver.Pixmap;
import com.winlator.xserver.Window;
import com.winlator.xserver.XClient;
import com.winlator.xserver.XLock;
import com.winlator.xserver.XServer;
import com.winlator.xserver.errors.BadImplementation;
import com.winlator.xserver.errors.BadMatch;
import com.winlator.xserver.errors.BadPixmap;
import com.winlator.xserver.errors.BadWindow;
import com.winlator.xserver.errors.XRequestError;
import com.winlator.xserver.events.PresentCompleteNotify;
import com.winlator.xserver.events.PresentIdleNotify;

import java.io.IOException;

public class PresentExtension implements Extension {
    public static final byte MAJOR_OPCODE = -103;
    public enum Kind { PIXMAP, MSC_NOTIFY }
    public enum Mode { COPY, FLIP, SKIP }

    private final SparseArray<Event> events = new SparseArray<>();
    private SyncExtension syncExtension;
    private byte firstEventId = 0;
    private byte firstErrorId = 0;

    // Target FPS for the back-pressure limiter. 0 disables limiting (notifies fire
    // immediately). Set from XServerScreen when the user toggles the FPS cap.
    private volatile int frameRateLimit = 0;

    private static class PendingIdle {
        Window window; Pixmap pixmap; int serial; int idleFence;
        long targetNs;
        int  vsyncSkips;    // vsyncs left to skip before firing (for fps < refresh)
        // Optional deferred PresentCompleteNotify. VKD3D-Proton (DX12) gates buffer
        // reuse on present *completion* rather than the X idle fence, so for the
        // copy path we pace the completion too (sent when this entry fires).
        // NOT CURRENTLY WORKING IN SOME DX12 GAMES that are frame dependant (e.g fighting games)
        boolean sendComplete = false;
        XClient client; Kind kind; Mode mode; long msc;
        PendingIdle(Window w, Pixmap p, int s, int f, long t, int sk) {
            window = w; pixmap = p; serial = s; idleFence = f; targetNs = t; vsyncSkips = sk;
        }
    }

    // IMPORTANT: a single window can have multiple pixmaps in flight (DXVK/VKD3D
    // swapchains commonly use 2–3 images). Keep every pending idle entry in a
    // queue keyed by targetNs so no fence/idle notify gets overwritten.
    private final java.util.concurrent.PriorityBlockingQueue<PendingIdle> pendingIdles =
        new java.util.concurrent.PriorityBlockingQueue<>(11,
            java.util.Comparator.comparingLong(p -> p.targetNs));

    private volatile android.view.Choreographer choreographer = null;
    private volatile boolean choreographerChecked = false;
    private final Object choreographerLock = new Object();

    private Thread cpuPacerThread = null;
    private volatile boolean pacerRunning = false;
    private final java.util.concurrent.ExecutorService idleFlushExecutor =
        java.util.concurrent.Executors.newSingleThreadExecutor(r -> {
            Thread t = new Thread(r, "PresentIdleFlush");
            t.setDaemon(true);
            t.setPriority(Thread.MAX_PRIORITY);
            return t;
        });
    private final java.util.concurrent.PriorityBlockingQueue<PendingIdle> cpuQueue =
        new java.util.concurrent.PriorityBlockingQueue<>(11,
            java.util.Comparator.comparingLong(p -> p.targetNs));

    private static final long FIRE_EARLY_NS = 700_000L; // 0.7 ms
    // Keep UI thread pacing callback bounded to avoid ANR under heavy backlog.
    private static final int MAX_IDLE_NOTIFIES_PER_VSYNC = 192;

    public void setFrameRateLimit(int limit) {
        final int sanitized = Math.max(0, limit);
        final int previous = this.frameRateLimit;
        this.frameRateLimit = sanitized;
        com.winlator.renderer.FramePacingLogger.updatePresentExtLimit(sanitized);
        if (previous == sanitized) return;

        // Per-window timing anchors are stale relative to the new cadence — let
        // the next present re-anchor against the freshly chosen interval.
        windowTimings.clear();

        // Any idles queued under the previous cadence can keep swapchain images
        // artificially busy across a limiter transition; flush them now so apps
        // never block waiting on stale fences after quick-menu FPS changes.
        flushQueuedIdlesAsync();

        if (choreographer != null && !pendingIdles.isEmpty()) postChoreographerCallback();
        // Wake the pacer so it re-evaluates the (now flushed/re-anchored) queue
        // immediately. IMPORTANT: use unpark, NOT interrupt — interrupting would
        // satisfy the loop's exit condition and permanently kill the pacer thread,
        // which would stall every future IdleNotify and freeze the guest.
        Thread pacer = cpuPacerThread;
        if (pacer != null) java.util.concurrent.locks.LockSupport.unpark(pacer);
    }

    private void flushQueuedIdlesAsync() {
        idleFlushExecutor.execute(() -> {
            java.util.ArrayList<PendingIdle> drained = new java.util.ArrayList<>();
            pendingIdles.drainTo(drained);
            cpuQueue.drainTo(drained);
            for (PendingIdle p : drained) {
                firePending(p);
            }
        });
    }

    // Delivers a paced present: first the (optionally deferred) CompleteNotify,
    // then the IdleNotify. Used by every fire site so completion- and idle-gated
    // clients are both released on the limiter's cadence.
    private void firePending(PendingIdle p) {
        if (p.sendComplete) {
            long ustNow = System.nanoTime() / 1000;
            sendCompleteNotify(p.window, p.serial, p.kind, p.mode, ustNow, p.msc);
            if (p.client != null) flushClientOutput(p.client);
        }
        sendIdleNotify(p.window, p.pixmap, p.serial, p.idleFence);
    }

    public void close() {
        pacerRunning = false;
        if (cpuPacerThread != null) {
            java.util.concurrent.locks.LockSupport.unpark(cpuPacerThread);
            cpuPacerThread = null;
        }
        idleFlushExecutor.shutdownNow();
    }

    private android.view.Choreographer tryGetChoreographer(VulkanRenderer renderer) {
        if (choreographerChecked) return choreographer;
        synchronized (choreographerLock) {
            if (choreographerChecked) return choreographer;
            choreographerChecked = true;
            try {
                if (renderer != null && renderer.xServerView != null) {
                    choreographer = android.view.Choreographer.getInstance();
                }
            } catch (Exception ignored) {
                android.util.Log.w("PresentExtension", "Choreographer unavailable, using CPU pacer");
            }
            if (choreographer == null) {
                startCpuPacer();
            }
            return choreographer;
        }
    }

    private void startCpuPacer() {
        if (cpuPacerThread != null) return;
        pacerRunning = true;
        cpuPacerThread = new Thread(() -> {
            // Loop lifetime is governed by `pacerRunning`, NOT the interrupt flag,
            // so a stray interrupt/unpark only wakes the pacer to re-check work —
            // it can never silently kill the thread and stall the guest.
            while (pacerRunning) {
                PendingIdle p = cpuQueue.peek();
                if (p == null) {
                    // No work: sleep until unparked by a new offer / rate change,
                    // with a timeout as a safety net against missed wakeups.
                    java.util.concurrent.locks.LockSupport.parkNanos(5_000_000L);
                    continue;
                }
                long now = System.nanoTime();
                if (now >= p.targetNs) {
                    PendingIdle head = cpuQueue.poll();
                    // poll() may race with flushQueuedIdlesAsync()'s drainTo and
                    // return null; that's fine, the entry was already delivered.
                    if (head != null)
                        firePending(head);
                } else {
                    // Park exactly until this entry is due. A newly offered earlier
                    // entry unparks us so we re-peek and retarget without oversleeping.
                    java.util.concurrent.locks.LockSupport.parkNanos(p.targetNs - now);
                }
            }
        }, "PresentPacer-CPU");
        cpuPacerThread.setDaemon(true);
        cpuPacerThread.setPriority(Thread.MAX_PRIORITY);
        cpuPacerThread.start();
    }

    private volatile boolean choreographerPosted = false;
    private final android.view.Choreographer.FrameCallback vsyncCallback = frameTimeNs -> {
        choreographerPosted = false;
        int processed = 0;
        while (processed < MAX_IDLE_NOTIFIES_PER_VSYNC) {
            PendingIdle p = pendingIdles.peek();
            if (p == null || frameTimeNs < p.targetNs) break;
            pendingIdles.poll();
            firePending(p);
            processed++;
        }
        if (!pendingIdles.isEmpty()) postChoreographerCallback();
    };

    private void postChoreographerCallback() {
        if (choreographer == null || choreographerPosted) return;
        choreographerPosted = true;
        choreographer.postFrameCallback(vsyncCallback);
    }

    private static class WindowTiming { long nextIdleNs = 0; }
    private final java.util.concurrent.ConcurrentHashMap<Integer, WindowTiming> windowTimings =
        new java.util.concurrent.ConcurrentHashMap<>();

    private void scheduleIdleNotify(Window window, Pixmap pixmap, int serial,
                                     int idleFence, int targetFps, VulkanRenderer renderer) {
        if (targetFps <= 0) {
            sendIdleNotify(window, pixmap, serial, idleFence);
            return;
        }
        final long fireTime = nextFireTime(window, targetFps);
        final PendingIdle entry = new PendingIdle(window, pixmap, serial, idleFence, fireTime, 0);
        enqueuePaced(entry, renderer);
    }

    // Like scheduleIdleNotify but also defers the PresentCompleteNotify to the
    // same paced instant. Required for completion-gated clients (VKD3D-Proton):
    // sending CompleteNotify immediately lets such clients reuse the source
    // buffer right away, defeating the idle-notify back-pressure.
    private void schedulePacedPresent(XClient client, Window window, Pixmap pixmap, int serial,
                                       int idleFence, int targetFps, VulkanRenderer renderer,
                                       Kind kind, Mode mode, long msc) {
        final long fireTime = nextFireTime(window, targetFps);
        final PendingIdle entry = new PendingIdle(window, pixmap, serial, idleFence, fireTime, 0);
        entry.sendComplete = true;
        entry.client = client;
        entry.kind = kind;
        entry.mode = mode;
        entry.msc = msc;
        enqueuePaced(entry, renderer);
    }

    // Computes the next paced fire instant for this window, striding forward by
    // exactly one frame interval so successive presents (with N images in flight)
    // drain at the user-selected rate; re-anchors to "now" if we've fallen behind.
    private long nextFireTime(Window window, int targetFps) {
        final long frameNs = 1_000_000_000L / targetFps;
        final long now = System.nanoTime();
        WindowTiming wt = windowTimings.computeIfAbsent(window.id, k -> new WindowTiming());
        synchronized (wt) {
            if (wt.nextIdleNs <= now - frameNs) {
                wt.nextIdleNs = now + frameNs;
            } else {
                wt.nextIdleNs += frameNs;
            }
            return wt.nextIdleNs - FIRE_EARLY_NS;
        }
    }

    private void enqueuePaced(PendingIdle entry, VulkanRenderer renderer) {
        android.view.Choreographer ch = tryGetChoreographer(renderer);
        if (ch != null) {
            // FIFO queue (not a per-window map): every in-flight pixmap is
            // tracked independently so its fence is signalled in due time.
            pendingIdles.offer(entry);
            postChoreographerCallback();
        } else {
            cpuQueue.offer(entry);
            // Wake the pacer so a closer deadline is honoured immediately.
            Thread pacer = cpuPacerThread;
            if (pacer != null) java.util.concurrent.locks.LockSupport.unpark(pacer);
        }
    }

    private static abstract class ClientOpcodes {
        static final byte QUERY_VERSION = 0;
        static final byte PRESENT_PIXMAP = 1;
        static final byte SELECT_INPUT = 3;
    }

    private static class Event {
        Window window;
        XClient client;
        int id;
        Bitmask mask;
    }

    @Override
    public String getName() { return "Present"; }

    @Override
    public byte getMajorOpcode() { return MAJOR_OPCODE; }

    @Override
    public int getNumEvents() { return 2; }

    @Override
    public int getNumErrors() { return 0; }

    @Override
    public void setFirstEventId(byte id) { this.firstEventId = id; }

    @Override
    public void setFirstErrorId(byte id) { this.firstErrorId = id; }

    @Override
    public byte getFirstEventId() { return firstEventId; }

    @Override
    public byte getFirstErrorId() { return firstErrorId; }

    private void sendIdleNotify(Window window, Pixmap pixmap, int serial, int idleFence) {
        if (idleFence != 0 && syncExtension != null) syncExtension.setTriggered(idleFence);
        synchronized (events) {
            for (int i = 0; i < events.size(); i++) {
                Event e = events.valueAt(i);
                if (e.window == window && e.mask.isSet(PresentIdleNotify.getEventMask())) {
                    e.client.sendEvent(new PresentIdleNotify(e.id, window, pixmap, serial, idleFence));
                }
            }
        }
    }

    private void sendCompleteNotify(Window window, int serial, Kind kind, Mode mode, long ust, long msc) {
        synchronized (events) {
            for (int i = 0; i < events.size(); i++) {
                Event e = events.valueAt(i);
                if (e.window == window && e.mask.isSet(PresentCompleteNotify.getEventMask())) {
                    e.client.sendEvent(new PresentCompleteNotify(e.id, window, serial, kind, mode, ust, msc));
                }
            }
        }
    }

    private void flushClientOutput(XClient client) {
        try {
            try (XStreamLock ignored = client.getOutputStream().lock()) {
            }
        } catch (Exception ignored) {}
    }

    private static void queryVersion(XClient client, XInputStream inputStream, XOutputStream outputStream) throws IOException, XRequestError {
        inputStream.skip(8);

        try (XStreamLock lock = outputStream.lock()) {
            outputStream.writeByte(RESPONSE_CODE_SUCCESS);
            outputStream.writeByte((byte)0);
            outputStream.writeShort(client.getSequenceNumber());
            outputStream.writeInt(0);
            outputStream.writeInt(1);
            outputStream.writeInt(0);
            outputStream.writePad(16);
        }
    }

    private void presentPixmap(XClient client, XInputStream inputStream, XOutputStream outputStream) throws IOException, XRequestError {
        int windowId = inputStream.readInt();
        int pixmapId = inputStream.readInt();
        int serial = inputStream.readInt();
        inputStream.skip(8);
        short xOff = inputStream.readShort();
        short yOff = inputStream.readShort();
        inputStream.skip(8);
        int idleFence = inputStream.readInt();
        inputStream.skip(client.getRemainingRequestLength());

        final Window window = client.xServer.windowManager.getWindow(windowId);
        if (window == null) throw new BadWindow(windowId);

        final Pixmap pixmap = client.xServer.pixmapManager.getPixmap(pixmapId);
        if (pixmap == null) throw new BadPixmap(pixmapId);

        Drawable content = window.getContent();
        int contentDepth = content.visual.depth;
        int pixmapDepth = pixmap.drawable.visual.depth;
        boolean depthCompat = (contentDepth == pixmapDepth) ||
            ((contentDepth == 24 || contentDepth == 32) && (pixmapDepth == 24 || pixmapDepth == 32));
        if (!depthCompat) throw new BadMatch();

        final XServerRenderer xr = client.xServer.getRenderer();
        final VulkanRenderer vr = (xr instanceof VulkanRenderer) ? (VulkanRenderer) xr : null;
        final int targetFps = this.frameRateLimit;

        long ust = System.nanoTime() / 1000;
        long msc = ust / (targetFps > 0 ? (1_000_000L / targetFps) : (1_000_000L / 60));

        synchronized (content.renderLock) {
            boolean isNative = vr != null && vr.isNativeMode();

            if (isNative && pixmap.drawable.isDirectScanout()) {
                content.setTexture(pixmap.drawable.getTexture());
                content.setDirectScanout(true);
                sendCompleteNotify(window, serial, Kind.PIXMAP, Mode.FLIP, ust, msc);
                flushClientOutput(client);
                if (window.attributes.isMapped()) {
                    vr.onUpdateWindowContent(window);
                }
                // Scanout is a real FLIP: idle is naturally gated by the next flip,
                // so idle-notify back-pressure alone is correct here.
                if (targetFps > 0) scheduleIdleNotify(window, pixmap, serial, idleFence, targetFps, vr);
                else sendIdleNotify(window, pixmap, serial, idleFence);
                com.winlator.renderer.FramePacingLogger.recordXPresent(true, false);
            } else if (vr != null && window.attributes.isMapped()) {
                if (targetFps > 0) {
                    // COPY path: pace BOTH completion and idle so completion-gated
                    // clients (VKD3D/DX12) are throttled, not just idle-gated DXVK.
                    vr.onUpdateWindowContentDirect(window, pixmap.drawable, xOff, yOff);
                    schedulePacedPresent(client, window, pixmap, serial, idleFence, targetFps, vr,
                        Kind.PIXMAP, Mode.COPY, msc);
                } else {
                    sendCompleteNotify(window, serial, Kind.PIXMAP, Mode.COPY, ust, msc);
                    flushClientOutput(client);
                    vr.onUpdateWindowContentDirect(window, pixmap.drawable, xOff, yOff);
                    sendIdleNotify(window, pixmap, serial, idleFence);
                }
                com.winlator.renderer.FramePacingLogger.recordXPresent(false, true);
            } else {
                content.copyArea((short)0, (short)0, xOff, yOff,
                    pixmap.drawable.width, pixmap.drawable.height, pixmap.drawable);
                if (targetFps > 0) {
                    schedulePacedPresent(client, window, pixmap, serial, idleFence, targetFps, vr,
                        Kind.PIXMAP, Mode.COPY, msc);
                } else {
                    sendCompleteNotify(window, serial, Kind.PIXMAP, Mode.COPY, ust, msc);
                    flushClientOutput(client);
                    sendIdleNotify(window, pixmap, serial, idleFence);
                }
                com.winlator.renderer.FramePacingLogger.recordXPresent(false, true);
            }
        }
    }

    private void selectInput(XClient client, XInputStream inputStream, XOutputStream outputStream) throws IOException, XRequestError {
        int eventId = inputStream.readInt();
        int windowId = inputStream.readInt();
        Bitmask mask = new Bitmask(inputStream.readInt());

        Window window = client.xServer.windowManager.getWindow(windowId);
        if (window == null) throw new BadWindow(windowId);

        if (GPUImage.isSupported() && !mask.isEmpty()) {
            Drawable content = window.getContent();
            final Texture oldTexture = content.getTexture();
            if (oldTexture != null && !(oldTexture instanceof GPUImage)) {
                XServerRenderer r = client.xServer.getRenderer();
                if (r != null)
                    r.getRendererView().queueEvent(() -> VortekRendererComponent.destroyTexture(oldTexture));
            }
            if (!(content.getTexture() instanceof GPUImage))
                content.setTexture(new GPUImage(content.width, content.height));
        }

        synchronized (events) {
            Event event = events.get(eventId);
            if (event != null) {
                if (event.window != window || event.client != client) throw new BadMatch();
                if (!mask.isEmpty()) event.mask = mask;
                else events.remove(eventId);
            } else {
                event = new Event();
                event.id = eventId;
                event.window = window;
                event.client = client;
                event.mask = mask;
                events.put(eventId, event);
            }
        }
    }

    @Override
    public void handleRequest(XClient client, XInputStream inputStream, XOutputStream outputStream) throws IOException, XRequestError {
        int opcode = client.getRequestData();
        if (syncExtension == null) syncExtension = client.xServer.getExtension(SyncExtension.MAJOR_OPCODE);

        switch (opcode) {
            case ClientOpcodes.QUERY_VERSION:
                queryVersion(client, inputStream, outputStream);
                break;
            case ClientOpcodes.PRESENT_PIXMAP:
                try (XLock lock = client.xServer.lock(XServer.Lockable.WINDOW_MANAGER)) {
                    presentPixmap(client, inputStream, outputStream);
                }
                break;
            case ClientOpcodes.SELECT_INPUT:
                try (XLock lock = client.xServer.lock(XServer.Lockable.WINDOW_MANAGER)) {
                    selectInput(client, inputStream, outputStream);
                }
                break;
            default:
                throw new BadImplementation();
        }
    }
}
