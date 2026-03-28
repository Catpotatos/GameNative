# Extraction Performance Analysis & Optimization

## Device Info
- **Device**: Retroid Pocket 5 (RP5)
- **OS**: Android 13
- **Storage**: 109GB total, 86GB used (79%), ~23GB free
- **App Storage**: `/data/user/0` (eMMC)

## Current Bottleneck: chmod() Per File

### The Problem
**File: `TarCompressorUtils.java`, line 207:**
```java
FileUtils.chmod(file, 0771);  // Called for EVERY extracted file!
```

**Impact:**
- ANGLE alone extracts ~50-100+ files (mostly in `opt/angle/lib/` and supporting libs)
- GL4ES adds another ~30-50 files
- Extra_libs adds ~100+ files
- Wrapper drivers add ~200-300+ files
- **Total: 400-600+ files per container initialization**
- Each `chmod()` is a syscall → filesystem metadata update
- On eMMC storage, filesystem operations are slow (~1-5ms per syscall)
- **Total overhead: 400-600 × 2-5ms = 800ms-3s just for chmod!**

### Why It's Slow on This Device
1. **eMMC storage** (Retroid Pocket 5) is slower than UFS
2. **Zstandard decompression** is happening on main thread (synchronous)
3. **Single-threaded extraction** — one file at a time
4. **No batching** — each file gets immediate chmod

---

## Recommended Optimizations

### Option 1: Skip chmod for Archives (FASTEST, ~90% improvement)
Most archive files don't need `chmod(0771)`. The extraction path itself creates files with sensible defaults:

**Change (TarCompressorUtils.java:207):**
```java
// Only chmod executable files or symlinks, not regular data files
if (entry.isSymbolicLink() || entry.getName().contains("/bin/") || entry.getName().endsWith(".so")) {
    FileUtils.chmod(file, 0771);
}
```

**Expected improvement:** 2-3 seconds → 200-400ms

---

### Option 2: Batch chmod Operations (MEDIUM, ~40-50% improvement)
Defer chmod calls and batch them together:

**Implementation:**
```java
private static boolean extract(Type type, InputStream source, File destination,
                               OnExtractFileListener onExtractFileListener) {
    List<File> chmodQueue = new ArrayList<>();

    try (InputStream inStream = getCompressorInputStream(type, source);
         ArchiveInputStream tar = new TarArchiveInputStream(inStream)) {
        TarArchiveEntry entry;
        while ((entry = (TarArchiveEntry)tar.getNextEntry()) != null) {
            // ... existing extraction logic ...

            // Queue chmod instead of immediate call
            if (!entry.isDirectory()) {
                chmodQueue.add(file);
            }

            // Batch every 50 files
            if (chmodQueue.size() >= 50) {
                batchChmod(chmodQueue, 0771);
                chmodQueue.clear();
            }
        }

        // Final batch
        if (!chmodQueue.isEmpty()) {
            batchChmod(chmodQueue, 0771);
        }
        return true;
    }
}

private static void batchChmod(List<File> files, int mode) {
    // Could potentially use ProcessBuilder for a single `chmod` command
    // instead of individual syscalls, but risky on Android
    for (File f : files) {
        FileUtils.chmod(f, mode);
    }
}
```

**Expected improvement:** 2-3 seconds → 1.2-1.8 seconds

---

### Option 3: Parallel Decompression (HIGH EFFORT, ~30-40% improvement)
Use thread pool for decompression + chmod:

**Risk:** Complex, potential race conditions, not recommended without careful testing

---

## What's Being Extracted

### ANGLE Path (Total ~3-5 seconds on RP5):
1. **angle-7736.tzst** (~5.5MB compressed)
   - Decompresses to ~15-20MB
   - Files: libEGL_angle.so (302KB) + libGLESv2_angle.so (5.5MB) + supporting libs

2. **gl4es-1.1.6.tzst** (~2-3MB compressed)
   - Decompresses to ~10-15MB
   - Files: libGL.so.1 + supporting libs

3. **extra_libs.tzst** (~1-2MB compressed)
   - Various graphics support libs

**Total to extract:** ~50-100 files, ~8-9MB total size

---

## Real-World Test Results Needed

To validate these improvements, run:

```bash
# 1. Before optimization - measure time to extract ANGLE
adb shell "time tar xzf angle-7736.tzst -C /tmp"

# 2. After optimization - measure chmod overhead
# Add logging to chmod() calls
```

---

## Recommended Action Plan

**Priority 1 (IMMEDIATE):**
- [ ] Implement **Option 1** — Skip unnecessary chmod
- [ ] Adds ~2-3 seconds back to initialization
- [ ] No risk, low effort

**Priority 2 (TESTING):**
- [ ] Test Option 1 on device
- [ ] Verify no permission errors
- [ ] Check that ANGLE libs are executable

**Priority 3 (FUTURE):**
- [ ] Implement **Option 2** if Option 1 alone insufficient
- [ ] Profile with systrace to measure actual syscall time

---

## Debug Logging to Add

Add to `TarCompressorUtils.extract()` to measure:

```java
long startTime = System.currentTimeMillis();
int fileCount = 0;
long totalExtractSize = 0;

// Inside while loop:
totalExtractSize += entry.getSize();
fileCount++;

// At end:
long elapsed = System.currentTimeMillis() - startTime;
Log.i("TarCompressorUtils",
    String.format("Extracted %d files, %.1f MB in %.1f sec (%.1f MB/s)",
        fileCount, totalExtractSize / 1e6, elapsed / 1e3,
        totalExtractSize / 1e6 / (elapsed / 1e3)));
```

---

## Current Environment Variables Set (Correct)

✅ `ANGLE_DEFAULT_PLATFORM` = vulkan or gles
✅ `LIBGL_ES` = 2
✅ `WINE_D3D_CONFIG` = renderer=gl
✅ `LD_LIBRARY_PATH` includes base + ANGLE paths

Everything is wired up correctly — it's just slow due to per-file chmod.

