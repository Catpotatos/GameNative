# REAL-TIME MONITORING STATUS

## Device State: March 17, 2026, 17:23:00 GMT

### Current Process Status
- **App**: app.gamenative running
- **Main thread**: DefaultDispatch @ 100% CPU (processing)
- **Render thread**: RenderThread @ 5.8% CPU
- **Memory**: 122MB (app) / 89MB (shared)
- **Total system**: 5.9GB / 7.9GB used (75%)

### System Resources
```
Threads: 2801 total (mostly sleeping)
Swap: 440MB / 8.3GB used
Free RAM: 1.9GB available
Storage: ~23GB free
```

---

## Monitoring Streams ACTIVE ✅

### 1. Full Logcat (Background)
**File**: `device_logs_full.txt`
- Captures ALL log messages with timestamps
- Format: threadtime (includes process ID, thread ID, time)
- Running continuously in background

### 2. System Metrics (Background)
**File**: `device_metrics.log`
- CPU usage per thread
- Memory statistics
- Swap usage
- Updated every 2 seconds

### 3. Live Extraction Logs
**Command**: Monitor these tags in real-time:
```
adb logcat TarCompressorUtils:I "ANGLE:*" "BionicProgram:*" -v threadtime
```

---

## What to Watch For

### During Bionic Component Installation:
- ✅ Extraction logs show progress (file count, size, speed)
- ✅ CPU spikes during decompression (normal)
- ✅ Memory stays < 300MB (app data)
- ⚠️ If stuck > 5 seconds per file → storage bottleneck
- ❌ Permission errors → chmod issue
- ❌ OOM kill → insufficient memory

### During Game Launch:
- ✅ ANGLE initialization logs (ANGLE_DEFAULT_PLATFORM set)
- ✅ LD_LIBRARY_PATH includes angle libs
- ✅ Vulkan driver loading
- ✅ Display surface creation
- ⚠️ Rendering performance (FPS)
- ❌ VK_ERROR or Display::initialize error → GPU issue

### Performance Metrics to Track:
```
Extraction speed: ≥ 10 MB/s (should be with fix)
Game startup: < 10 seconds
FPS during gameplay: ≥ 30 FPS recommended
Memory stable: < 500MB during play
```

---

## How to Read the Logs

### Extract Performance:
```
TarCompressorUtils: Extracted 247 files, 14.5 MB in 0.82 sec (17.7 MB/s), type=ZSTD
```
- **247 files** = total files extracted
- **14.5 MB** = total size
- **0.82 sec** = elapsed time (✅ FAST with our fix!)
- **17.7 MB/s** = throughput

### ANGLE Initialization:
```
XServerScreen: ANGLE_DEFAULT_PLATFORM after user env merge: 'vulkan'
XServerScreen: ANGLE final env check: ANGLE_DEFAULT_PLATFORM=vulkan, ...
```
- Confirms ANGLE vars are set correctly

### Bionic Launcher:
```
BionicProgramLauncherComponent: ANGLE ENV FINAL: ANGLE_DEFAULT_PLATFORM=vulkan, ...
```
- Confirms env vars made it to process
- Shows final LD_LIBRARY_PATH, WINE_D3D_CONFIG

---

## Commands to Run During Test

### Monitor Extraction (while installing):
```bash
adb logcat TarCompressorUtils -v threadtime | grep "Extracted"
```

### Monitor ANGLE Init (while launching game):
```bash
adb logcat XServerScreen -v threadtime | grep "ANGLE"
```

### Check Memory During Play:
```bash
adb shell dumpsys meminfo app.gamenative | grep TOTAL
```

### Monitor CPU in Real-Time:
```bash
adb shell top -b -H | grep app.gamenative
```

---

## Testing Checklist

- [ ] Install Bionic container
- [ ] Select ANGLE as graphics driver
- [ ] Watch extraction logs (measure time)
- [ ] Launch game
- [ ] Monitor ANGLE init logs
- [ ] Check for rendering errors
- [ ] Play for 30+ seconds
- [ ] Record FPS/performance
- [ ] Check final memory usage

---

## Files to Review After Test

1. **device_logs_full.txt** → Complete log trace
2. **device_metrics.log** → Performance timeline
3. **device_problems.log** (if any) → Error analysis

---

## Monitoring started: 17:23:00 GMT
## Awaiting your Bionic installation...

