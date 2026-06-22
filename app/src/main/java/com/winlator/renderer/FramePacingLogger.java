package com.winlator.renderer;

import android.util.Log;
import java.util.concurrent.atomic.AtomicInteger;

/**
 * Global FPS diagnostics: counts frames from every presentation path and
 * reports to logcat every 10 seconds.
 *
 * Presentation paths tracked:
 *  - X Present protocol (PresentExtension.presentPixmap) — back-pressure paced by PresentExtension
 *    • sub-type: direct scanout (AHB → SurfaceControl zero-copy)
 *    • sub-type: copy (AHB/pixels → VulkanRenderer compositor)
 *  - VulkanRenderer direct AHB content update (onUpdateWindowContent AHB non-scanout path)
 *  - VulkanRenderer direct scanout (nativeScanoutSetBuffer — bypasses X Present entirely)
 *  - VulkanRenderer pixel blit (onUpdateWindowContent pixel path)
 *  - GLRenderer onDrawFrame (EGL swap driven by GLSurfaceView RENDERMODE_WHEN_DIRTY)
 */
public final class FramePacingLogger {
    private static final String TAG = "FramePacing";
    private static final long REPORT_INTERVAL_MS = 10_000L; // 10 seconds

    // ── Per-path frame counters ──────────────────────────────────────────────
    private static final AtomicInteger cntXPresent        = new AtomicInteger(0);
    private static final AtomicInteger cntXPresentScanout = new AtomicInteger(0);
    private static final AtomicInteger cntXPresentCopy    = new AtomicInteger(0);
    private static final AtomicInteger cntVkAHB           = new AtomicInteger(0);
    private static final AtomicInteger cntVkScanout       = new AtomicInteger(0);
    private static final AtomicInteger cntVkPixels        = new AtomicInteger(0);
    private static final AtomicInteger cntGLDraw          = new AtomicInteger(0);
    private static final AtomicInteger cntVkSkipped       = new AtomicInteger(0);
    private static final AtomicInteger cntGLSkipped       = new AtomicInteger(0);
    private static final AtomicInteger cntVortek          = new AtomicInteger(0);
    private static final AtomicInteger cntVkUpdate        = new AtomicInteger(0);

    // ── Limit state (updated by each subsystem) ──────────────────────────────
    private static volatile int  presentExtLimit   = 0;   // 0 = unlimited
    private static volatile int  vkScanoutHint     = 0;   // SurfaceControl rate hint; 0 = cleared
    private static volatile boolean limiterEnabled = false;
    private static volatile int  limiterTarget     = 0;

    // ── Launch-time env-var state (set when the Wine process is spawned) ─────
    private static volatile String launchDxvkFrameRate   = "NOT SET";
    private static volatile String launchVkd3dFrameLimit = "NOT SET";
    private static volatile String launchMangoHudConfig  = "NOT SET";

    // ── Background reporter ──────────────────────────────────────────────────
    private static volatile Thread sReportThread = null;
    private static volatile boolean reporterStarted = false;

    // ── Enable flag — off by default; set via GN_FRAME_LOG=1 env var ────────
    private static volatile boolean enabled = false;

    private FramePacingLogger() {}

    /**
     * Enable or disable the frame pacing logger.
     * When disabled the reporter thread is stopped and all record/log calls
     * become no-ops. Called from XServerScreen when GN_FRAME_LOG=1 is set.
     */
    public static synchronized void setEnabled(boolean enable) {
        enabled = enable;
        if (!enable) {
            stopReporter();
            resetCounters();
        }
    }

    public static boolean isEnabled() {
        return enabled;
    }

    // Self-starting: any record* call spins up the 10 s reporter if not already
    // running, so we get data even without an explicit notifyLaunch() hook.
    private static void maybeStartReporter() {
        if (!enabled) return;
        if (reporterStarted) return;
        ensureReporterRunning();
    }

    /** Call once when the game process is about to be launched. */
    public static synchronized void notifyLaunch(
            String dxvkFrameRate, String vkd3dFrameLimit, String mangoHudConfig) {
        if (!enabled) return;
        launchDxvkFrameRate   = dxvkFrameRate  != null && !dxvkFrameRate.isEmpty()
                                ? dxvkFrameRate   : "NOT SET";
        launchVkd3dFrameLimit = vkd3dFrameLimit != null && !vkd3dFrameLimit.isEmpty()
                                ? vkd3dFrameLimit : "NOT SET";
        launchMangoHudConfig  = mangoHudConfig  != null && !mangoHudConfig.isEmpty()
                                ? mangoHudConfig  : "NOT SET";
        resetCounters();
        ensureReporterRunning();
        Log.i(TAG, "FramePacingLogger: game launching — DXVK_FRAME_RATE=" + launchDxvkFrameRate
                + "  VKD3D_FRAME_LIMIT=" + launchVkd3dFrameLimit
                + "  MANGOHUD_CONFIG=" + launchMangoHudConfig);
    }

    // ── Frame recording API ──────────────────────────────────────────────────

    /**
     * Called by PresentExtension for each PRESENT_PIXMAP request handled.
     * @param isScanout true when routed to the zero-copy scanout path
     * @param isCopy    true when content is blitted into the compositor
     */
    public static void recordXPresent(boolean isScanout, boolean isCopy) {
        maybeStartReporter();
        cntXPresent.incrementAndGet();
        if (isScanout) cntXPresentScanout.incrementAndGet();
        else if (isCopy) cntXPresentCopy.incrementAndGet();
    }

    /** Called by VortekRendererComponent each time a Vortek-rendered frame is
     *  pushed to the window (native Vulkan present that bypasses X Present). */
    public static void recordVortekPresent() {
        maybeStartReporter();
        cntVortek.incrementAndGet();
    }

    /**
     * Called by VulkanRenderer.onUpdateWindowContent — the COMMON compositor sink
     * for every source (X Present, Vortek forceUpdate, etc.). Comparing this rate
     * against recordXPresent tells us whether frames originate from X Present.
     */
    public static void recordVkUpdate() {
        maybeStartReporter();
        cntVkUpdate.incrementAndGet();
    }

    /**
     * Called by VulkanRenderer for each direct AHB content update.
     * @param isScanout true if the buffer is routed to the zero-copy scanout surface
     */
    public static void recordVkContentAHB(boolean isScanout) {
        maybeStartReporter();
        if (isScanout) cntVkScanout.incrementAndGet();
        else           cntVkAHB.incrementAndGet();
    }

    /** Called by VulkanRenderer for each pixel/software-blit content update. */
    public static void recordVkContentPixels() {
        maybeStartReporter();
        cntVkPixels.incrementAndGet();
    }

    /** Called by VulkanRenderer when a content update is skipped by the runtime FPS limiter. */
    public static void recordVkContentSkipped() {
        cntVkSkipped.incrementAndGet();
    }

    /** Called by GLRenderer.onDrawFrame() on every EGL swap. */
    public static void recordGLDrawFrame() {
        maybeStartReporter();
        cntGLDraw.incrementAndGet();
    }

    /** Called by GLRenderer when a render request is skipped by the runtime FPS limiter. */
    public static void recordGLDrawSkipped() {
        cntGLSkipped.incrementAndGet();
    }

    // ── Limit-state update API ───────────────────────────────────────────────

    /** Updated by PresentExtension whenever setFrameRateLimit is called. */
    public static void updatePresentExtLimit(int limit) {
        presentExtLimit = limit;
        if (!enabled) return;
        Log.d(TAG, "PresentExtension frame limit → " + (limit > 0 ? limit + " fps" : "UNLIMITED"));
    }

    /**
     * Updated by VulkanRenderer whenever a SurfaceControl rate hint is applied.
     * @param hint 0 = hint cleared (no pinning), >0 = hint fps value
     */
    public static void updateVkScanoutHint(int hint) {
        vkScanoutHint = hint;
        if (!enabled) return;
        Log.d(TAG, "VulkanRenderer SurfaceControl hint → "
                + (hint > 0 ? hint + " fps" : "cleared (no pinning)"));
    }

    /**
     * Updated by XServerScreen whenever the quick-menu fps limiter state changes.
     */
    public static void updateFpsLimiterState(boolean enabled2, int target) {
        limiterEnabled = enabled2;
        limiterTarget  = target;
        if (!enabled) return;
        Log.d(TAG, "QuickMenu FPS limiter → " + (enabled2 ? "ON @ " + target + " fps" : "OFF (unlimited)"));
    }

    // ── Private helpers ──────────────────────────────────────────────────────

    private static void resetCounters() {
        cntXPresent.set(0); cntXPresentScanout.set(0); cntXPresentCopy.set(0);
        cntVkAHB.set(0);    cntVkScanout.set(0);        cntVkPixels.set(0);
        cntGLDraw.set(0);
        cntVkSkipped.set(0); cntGLSkipped.set(0);
        cntVortek.set(0);    cntVkUpdate.set(0);
    }

    private static synchronized void stopReporter() {
        if (sReportThread != null) {
            sReportThread.interrupt();
            sReportThread = null;
        }
        reporterStarted = false;
    }

    private static synchronized void ensureReporterRunning() {
        if (sReportThread != null && sReportThread.isAlive()) return;
        reporterStarted = true;
        sReportThread = new Thread(() -> {
            while (!Thread.interrupted()) {
                try { Thread.sleep(REPORT_INTERVAL_MS); }
                catch (InterruptedException e) { break; }
                report();
            }
        }, "FramePacingLogger");
        sReportThread.setDaemon(true);
        sReportThread.setPriority(Thread.MIN_PRIORITY);
        sReportThread.start();
    }

    private static void report() {
        if (!enabled) return;
        // Snap and reset counters atomically-per-counter
        int xPresent   = cntXPresent.getAndSet(0);
        int xScanout   = cntXPresentScanout.getAndSet(0);
        int xCopy      = cntXPresentCopy.getAndSet(0);
        int vkAHB      = cntVkAHB.getAndSet(0);
        int vkScanout  = cntVkScanout.getAndSet(0);
        int vkPixels   = cntVkPixels.getAndSet(0);
        int glDraw     = cntGLDraw.getAndSet(0);
        int vkSkipped  = cntVkSkipped.getAndSet(0);
        int glSkipped  = cntGLSkipped.getAndSet(0);
        int vortek     = cntVortek.getAndSet(0);
        int vkUpdate   = cntVkUpdate.getAndSet(0);

        float sec = REPORT_INTERVAL_MS / 1000f;

        Log.i(TAG, "══════════ FRAME PACING REPORT (last 10 s) ══════════");
        Log.i(TAG, String.format(
            "  [X Present protocol]  %.1f fps  | total=%d  scanout=%d  copy=%d",
            xPresent / sec, xPresent, xScanout, xCopy));
        Log.i(TAG, String.format(
            "    └─ back-pressure paced by PresentExtension (idle-notify delay = %s)",
            presentExtLimit > 0 ? presentExtLimit + " fps target" : "UNLIMITED – no delay"));
        Log.i(TAG, String.format(
            "  [Vortek native]       %.1f fps  | %d frames  (native Vulkan present, bypasses X Present)",
            vortek / sec, vortek));
        Log.i(TAG, String.format(
            "  [Vk compositor sink]  %.1f fps  | %d onUpdateWindowContent calls (ALL sources)",
            vkUpdate / sec, vkUpdate));
        Log.i(TAG, String.format(
            "  [VkRenderer AHB]      %.1f fps  | %d frames  (non-scanout compositor path)",
            vkAHB / sec, vkAHB));
        Log.i(TAG, String.format(
            "  [VkRenderer Scanout]  %.1f fps  | %d frames  (zero-copy SurfaceControl path)",
            vkScanout / sec, vkScanout));
        Log.i(TAG, String.format(
            "  [VkRenderer Pixels]   %.1f fps  | %d frames  (software pixel blit)",
            vkPixels / sec, vkPixels));
        Log.i(TAG, String.format(
            "  [GL Renderer draws]   %.1f fps  | %d frames  (EGL onDrawFrame)",
            glDraw / sec, glDraw));
        Log.i(TAG, String.format(
            "  [Limiter skipped]     Vk=%d  GL=%d  (updates intentionally not submitted)",
            vkSkipped, glSkipped));
        Log.i(TAG, "  ─────────────── Source diagnosis ───────────────");
        // If the compositor sink is busy but X Present is idle, frames are NOT
        // coming through PresentExtension → X-Present back-pressure cannot cap them.
        String source;
        if (xPresent > 5 && xPresent >= vortek) source = "X Present (PresentExtension CAN cap)";
        else if (vortek > 5)                    source = "Vortek native (PresentExtension CANNOT cap)";
        else if (vkUpdate > 5)                  source = "compositor-only (X Present NOT the source)";
        else                                    source = "idle / no frames";
        Log.i(TAG, "  Frame source        : " + source);
        Log.i(TAG, "  ───────────────────────── Active Limiters ─────────────────────────");
        Log.i(TAG, String.format(
            "  QuickMenu limiter   : %s  target=%d fps",
            limiterEnabled ? "ON" : "OFF", limiterTarget));
        Log.i(TAG, String.format(
            "  PresentExt limit    : %s  [X Present back-pressure; 0=unlimited=fires immediately]",
            presentExtLimit > 0 ? presentExtLimit + " fps" : "UNLIMITED (0)"));
        Log.i(TAG, String.format(
            "  VkSurfaceCtrl hint  : %s  [SurfaceControl setFrameRate – display HINT only, not a hard cap]",
            vkScanoutHint > 0 ? vkScanoutHint + " fps" : "none / cleared"));
        Log.i(TAG, "  ───────────────────────── Launch-time Env Vars ────────────────────");
        Log.i(TAG, "  DXVK_FRAME_RATE     : " + launchDxvkFrameRate
                + "  [DXVK internal limiter – set at Wine process launch, cannot change at runtime]");
        Log.i(TAG, "  VKD3D_FRAME_LIMIT   : " + launchVkd3dFrameLimit
                + "  [VKD3D-Proton DX12 limiter – set at Wine process launch, cannot change at runtime]");
        Log.i(TAG, "  MANGOHUD_CONFIG     : " + launchMangoHudConfig
                + "  [MangoHUD – check fps_limit inside config if set]");
        Log.i(TAG, "  ───────────────────────── Path Notes ──────────────────────────────");
        Log.i(TAG, "  DXVK/VKD3D games route D3D Present() → winevulkan → winex11 → X Present → PresentExtension");
        Log.i(TAG, "  GL / virgl games  route via GLX/EGL → X Present → PresentExtension");
        Log.i(TAG, "  Native Vulkan scanout path bypasses X Present; only SurfaceControl hint applies");
        Log.i(TAG, "  GLRenderer has NO independent frame cap; relies solely on PresentExtension pacing");
        Log.i(TAG, "  If PresentExtension limit=0 AND no DXVK/VKD3D env-var cap → game runs UNCAPPED");
        Log.i(TAG, "══════════════════════════════════════════════════════════════════════");
    }
}

