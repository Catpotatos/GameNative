# 🎯 MONITORING ACTIVE — What's Running

## Background Monitoring Streams (All Active)

✅ **Full System Logs**
- File: `device_logs_full.txt`
- All messages with timestamps and process info

✅ **System Metrics**
- File: `device_metrics.log`
- CPU, memory, swap every 2 seconds

✅ **Real-Time Filter**
- File: `real_time_monitor.log`
- Only TarCompressor/ANGLE/Bionic events

✅ **Live Terminal Output**
- Displayed in console as events happen

---

## What to Expect

### Installing Bionic Components:
```
TarCompressorUtils: Extracted 247 files, 14.5 MB in 0.82 sec (17.7 MB/s), type=ZSTD
```
- ✅ Should see 0.6-0.8 sec extraction (with our chmod fix!)
- ⚠️ If > 2 sec → storage issue or chmod still slow

### Launching Game:
```
XServerScreen: ANGLE: dxwrapper = 'winediid'
XServerScreen: ANGLE_DEFAULT_PLATFORM after user env merge: 'vulkan'
BionicProgramLauncherComponent: ANGLE ENV FINAL: ANGLE_DEFAULT_PLATFORM=vulkan, LD_LIBRARY_PATH=/opt/angle/lib:/opt/gl4es/lib:...
```
- ✅ Confirms all ANGLE env vars set correctly
- Shows final LD_LIBRARY_PATH with base paths included

### During Game Play:
- Monitor FPS in game UI
- Watch for rendering errors
- Memory should stay stable (120-150MB)

---

## Commands You Can Run While Playing

### Check current memory (from PC terminal):
```bash
adb shell dumpsys meminfo app.gamenative | grep TOTAL
```

### See live CPU usage:
```bash
adb shell "while true; do top -n 1 -b -H | grep app.gamenative | head -3; sleep 1; done"
```

### Extract latest logs to file:
```bash
adb logcat > latest_logcat.txt &
# Play for a bit
# Ctrl+C to stop
```

---

## What Should Happen

1. **Install Bionic** → See extraction logs (fast now!)
2. **Launch Game** → See ANGLE init logs
3. **Play** → See rendering happening
4. **Stop** → Logs continue to file for analysis

---

## Monitoring Files Created

After test completes:

```
device_logs_full.txt       ← Complete system log
device_metrics.log         ← Performance timeline
real_time_monitor.log      ← Filtered ANGLE events
MONITORING_STATUS.md       ← This guide
ANGLE_BIONIC_AUDIT.md      ← Architecture review
EXTRACTION_PERFORMANCE_ANALYSIS.md ← Performance deep-dive
PERFORMANCE_FIX_SUMMARY.md ← Changes made
ADB_PERFORMANCE_DEBUG_GUIDE.md ← Debug commands
```

---

## Ready to Test!

**Your device is connected and monitoring is active.**

Start installing Bionic components on the device now.
We'll capture everything happening in real-time.

🎮 **Good luck with the test!**

