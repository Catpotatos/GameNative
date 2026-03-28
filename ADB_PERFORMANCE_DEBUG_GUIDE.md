# ADB Performance Debugging Commands

## Quick Check: Is Extraction Still Running?

```bash
# Check app CPU/thread usage
adb shell top -n 1 -b -H | grep "app.gamenative"

# Watch in real-time (requires Ctrl+C to stop):
adb shell top -b -H | grep -E "app.gamenative|Threads|Mem|Swap"
```

Expected output while extracting:
```
DefaultDispatch thread: 80-95% CPU
app.gamenative: 40% CPU (helper thread)
```

---

## Monitor Extraction Logs

### Start logging BEFORE triggering extraction:
```bash
# Terminal 1: Start continuous logcat
adb logcat TarCompressorUtils -v threadtime -b main

# Terminal 2 (different window):
# Launch app and create container
adb shell am start app.gamenative/.MainActivity
```

### Expected output:
```
TarCompressorUtils: Extracted 247 files, 14.5 MB in 0.82 sec (17.7 MB/s), type=ZSTD
TarCompressorUtils: Extracted 89 files, 11.2 MB in 0.64 sec (17.5 MB/s), type=ZSTD
```

---

## Measure Extraction Time Precisely

### Option 1: Time from logcat
```bash
adb logcat TarCompressorUtils -v threadtime > /tmp/extract_log.txt &
# ... trigger extraction ...
# Ctrl+C after done
# Search log for elapsed times
```

### Option 2: Timestamp before/after
```bash
# Terminal 1: Watch for start
adb logcat | grep "ANGLE: extracting"

# Terminal 2: Time the operation
date; adb shell sleep 10; date  # Replace 10 with actual wait time
```

---

## Storage Performance Check

```bash
# Check available space
adb shell "df /data"

# Check write speed (fast, ~1 second):
adb shell "dd if=/dev/zero of=/data/test.bin bs=1M count=10 && rm /data/test.bin"

# Check read speed:
adb shell "time cat /data/data/app.gamenative/files/imagefs/opt/angle/lib/*.so > /dev/null"
```

---

## Thread Analysis

### How many threads is the app using?
```bash
# While extraction is happening:
adb shell "cat /proc/$(adb shell pidof app.gamenative)/status | grep Threads"

# Expected: ~2700-3000 threads total (mostly sleeping)
# Active: DefaultDispatch + maybe 1-2 decompression threads
```

### Is it using thread pool?
```bash
# Check for thread pool activity
adb shell "ps -eLo tid,name | grep app.gamenative | wc -l"

# If thread count jumps > 100 during extraction → parallel processing
# If stays constant → single-threaded
```

---

## Memory During Extraction

```bash
# Get memory snapshot during extraction:
adb shell "dumpsys meminfo app.gamenative"

# Key metrics:
# - TOTAL: ~170-200 MB (app + libraries)
# - Graphics: ~20-50 MB (rendering)
# - Native heap: varies based on decompression

# Should NOT increase > 300 MB or crash
```

---

## File Permission Check

After extraction, verify permissions:

```bash
# Check if ANGLE libs are executable
adb shell "ls -la /data/data/app.gamenative/files/imagefs/opt/angle/lib/"

# Should show x (execute) bits on .so files:
# -rwxrwx--- libEGL_angle.so
# -rwxrwx--- libGLESv2_angle.so

# Non-executable file (data):
# -rw-rw---- some_config.txt
```

---

## Performance Regression Check

Track performance over time:

```bash
# Create script: check_extraction_perf.sh
#!/bin/bash
DATE=$(date +"%Y-%m-%d %H:%M:%S")
adb logcat TarCompressorUtils -v raw 2>&1 | \
  grep "Extracted" | \
  awk -v d="$DATE" '{print d, $0}' >> extraction_perf.log
```

Then compare logs:
```bash
# Show trend
cat extraction_perf.log | tail -20
```

---

## Debug Flags (If Needed)

These can be added to `TarCompressorUtils.java` for deeper debugging:

```java
// Add at top of extract() method:
private static final boolean DEBUG_CHMOD = true;

// Inside loop, before chmod:
if (DEBUG_CHMOD && isExecutable) {
    Log.d("TarCompressorUtils", "chmod: " + entryName);
}

// Count chmod calls:
int chmodCount = 0;
if (isExecutable) {
    chmodCount++;
    FileUtils.chmod(file, 0771);
}

// At end:
Log.i("TarCompressorUtils", "Total chmod calls: " + chmodCount);
```

---

## Expected Timing (Retroid Pocket 5)

### Before Optimization
- ANGLE (5.5MB) + GL4ES (2MB) + extras: **2-3 seconds**

### After Optimization
- Same archives: **0.6-0.8 seconds** (3-5x faster)

### Variance Factors
- **Lower**: App just started, cache warm, storage fast
- **Higher**: Many other apps running, storage busy, first time

---

## Troubleshooting

| Issue | Cause | Check |
|-------|-------|-------|
| Extraction stuck | I/O blocked | `adb shell "ps aux \| grep dd"` |
| Permissions failed | chmod didn't run | `ls -la /opt/angle/lib/` |
| Still slow after fix | Other bottleneck | Log analysis, systrace |
| Process killed | OOM | `dmesg \| grep -i killed` |


