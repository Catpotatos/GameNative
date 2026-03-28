# ANGLE Integration Status — Full Investigation (2026-03-22)

## Executive Summary

ANGLE is **correctly built, correctly deployed, and correctly initializes at runtime**.
The remaining rendering issue — wined3d falling back to `adapter_no3d` — was caused by
`BOX64_X11GLX=0` in the first test run. The code now forces `BOX64_X11GLX=1`, and a
second test run at 01:08 confirmed the corrected environment reaches the guest process.
Full wined3d trace from the `BOX64_X11GLX=1` run is still needed to confirm GL context
creation succeeds end-to-end.

---

## 1. Build Artifacts — Verified ✅

### ANGLE (`angle-7736.tzst` in APK)

| Property | Value | Status |
|----------|-------|--------|
| Source | Google ANGLE, `chromium/7736` branch (commit `4554772`) | ✅ |
| Target | `target_os="android"` `target_cpu="arm64"` | ✅ |
| Toolchain | Chromium clang 23.0.0git (x86_64 cross-compiling for aarch64) | ✅ |
| GN flags | `angle_enable_vulkan=true`, `angle_enable_gl=false`, `is_debug=false` | ✅ |
| libEGL.so | 296 KB (pre-strip), SONAME patched to `libEGL.so` | ✅ |
| libGLESv2.so | 5.3 MB (pre-strip), SONAME patched to `libGLESv2.so` | ✅ |
| Dependencies | `libdl.so`, `libc.so`, `libm.so`, `libnativewindow.so`, `liblog.so` — **all Bionic** | ✅ |
| Vulkan WSI | `VK_KHR_android_surface` (correct for Android) | ✅ |
| **NOT** glibc | No `libc.so.6`, `libstdc++.so.6`, `libxcb.so.1`, `ld-linux-aarch64.so.1` | ✅ |

### GL4ES (`gl4es-bionic-1.1.7.tzst` in APK)

| Property | Value | Status |
|----------|-------|--------|
| Source | ptitSeb/gl4es `master` (commit `9e8037b`) | ✅ |
| Target | aarch64 Android/Bionic (API 28) | ✅ |
| libGL.so.1 | 1.6 MB, SONAME `libGL.so.1` | ✅ |
| Dependencies | `libm.so`, `libdl.so`, `liblog.so`, `libc.so` — **all Bionic** | ✅ |
| GLX symbols | 20+ glX* functions exported (glXChooseFBConfig, glXSwapBuffers, etc.) | ✅ |
| X11 stubs | Statically linked (XOpenDisplay, XGetVisualInfo, XCreateColormap) | ✅ |
| EGL backend | Uses `eglGetProcAddress` internally (picks up ANGLE via `LIBGL_EGL`) | ✅ |

---

## 2. WSL Build Directory — Stale Package Warning ⚠️

The WSL checkout at `/home/catarina/angle_build/` has **two** build output directories:

| Directory | target_os | Linker | SONAME | Status |
|-----------|-----------|--------|--------|--------|
| `out/Release_arm64/` | `"linux"` | **glibc** (`libc.so.6`, `libxcb.so.1`) | `libEGL.so` / `libGLESv2.so` | ❌ WRONG |
| `out/Android_arm64/` | `"android"` | **Bionic** (`libc.so`, `libnativewindow.so`) | `libEGL_angle.so` / `libGLESv2_angle.so` | ✅ CORRECT |

The `package/` directory and the `angle-7736.tzst` (1.9 MB) in WSL contain the **glibc** binaries.
The `angle-7736-android.tzst` (2.3 MB) in WSL contains the **correct Bionic** binaries.

**The APK asset `angle-7736.tzst` has the correct Bionic binaries** (verified by extraction).
This means the asset was copied from `angle-7736-android.tzst` at some point, which is correct.

### Build Script Issue

The `build_angle.sh` in WSL still has `target_os = "linux"` (the glibc build).
The Android build was done separately with manual `args.gn` in `out/Android_arm64/`.
The script in `_angle_build/build_angle.sh` (project repo) has different content from WSL.

---

## 3. Runtime Analysis — From Logcat

### Test Run 1 (01:05) — `BOX64_X11GLX=0` ❌

```
ANGLE initializes OK:
  I ANGLE: Version (2.1.1 git hash: 455477231ca6), Renderer (Vulkan 1.1.128 (Adreno (TM) 650))

But wined3d fails:
  01a4:trace:wgl:glxdrv_wglDescribePixelFormat (0x12010066,0,0,0x0)
  01a4:err:d3d:wined3d_caps_gl_ctx_create Failed to find a suitable pixel format.
  01a4:err:d3d:wined3d_adapter_gl_init Failed to get a GL context
  01a4:warn:d3d:wined3d_init Failed to create adapter.

Falls back to adapter_no3d → software rendering → "the current DirectDrawRenderer does not support this"
```

**Root cause:** `BOX64_X11GLX=0` means Box64 does not provide GLX-over-EGL wrapping.
Wine's x11drv calls `glXDescribePixelFormat` which returns 0 formats. No GL context → no GPU.

### Test Run 2 (01:08) — `BOX64_X11GLX=1` ✅ (env verified, trace pending)

```
BOX64_X11GLX=1 confirmed in final env:
  I BionicProgramLauncherComponent: ANGLE ENV FINAL: ... BOX64_X11GLX=1 ...
  D ProcessHelper: Executing: [box64, wine, ...], BOX64_X11GLX=1
```

No wined3d trace was captured for this run. The full D3D/WGL trace is needed to confirm
whether `BOX64_X11GLX=1` resolves the pixel format selection issue.

---

## 4. ANGLE's Role in the GameNative Stack

```
┌─────────────────────────────────────────────────────────┐
│                    Game (Windows)                        │
├─────────────────┬───────────────────────────────────────┤
│ DirectX 7/8/9   │     DirectX 10/11      │  DirectX 12 │
│   (wined3d)     │      (DXVK)            │   (VKD3D)   │
├─────────────────┤                        │             │
│   OpenGL 2.1    │      Vulkan            │   Vulkan    │
│   (via Box64    │      (direct)          │   (direct)  │
│    GLX→EGL)     │                        │             │
├─────────────────┤                        │             │
│   GL4ES         │                        │             │
│ (GL→GLES 2.0)  │                        │             │
├─────────────────┤                        │             │
│   ANGLE         │                        │             │
│ (GLES→Vulkan)  │                        │             │
├─────────────────┴───────────────────────────────────────┤
│              Device Vulkan Driver (Adreno/Mali)         │
└─────────────────────────────────────────────────────────┘
```

ANGLE handles the **left column only** (DX7/8/9 via wined3d → GL → GL4ES → ANGLE → Vulkan).
DXVK and VKD3D bypass ANGLE entirely and go straight to the Vulkan driver.

### When ANGLE is valuable

1. **DX7/8/9 games** that need wined3d's OpenGL renderer
2. **Games that use OpenGL directly** (not D3D)
3. **Mali/PowerVR devices** where Mesa OpenGL drivers are unavailable
4. **Launcher UI / middleware** that relies on GLES

### When ANGLE is NOT needed

1. **DX10/11 games** → Use DXVK (D3D→Vulkan direct, faster)
2. **DX12 games** → Use VKD3D
3. **Vulkan-native games** → Direct Vulkan

---

## 5. Critical Environment Variables

| Variable | Value | Purpose |
|----------|-------|---------|
| `BOX64_X11GLX` | `1` | **REQUIRED.** Box64 synthesises GLX over EGL. Without this, Wine gets 0 pixel formats. |
| `LIBGL_FB` | `3` | GL4ES framebuffer mode 3: EGL without X11/DRM display |
| `LIBGL_ES` | `2` | GL4ES targets OpenGL ES 2.0 |
| `LIBGL_GL` | `21` | GL4ES reports OpenGL 2.1 to Wine |
| `LIBGL_EGL` | `…/opt/angle/lib/libEGL.so` | GL4ES loads ANGLE's EGL |
| `LIBGL_GLES` | `…/opt/angle/lib/libGLESv2.so` | GL4ES loads ANGLE's GLES |
| `LIBGL_NOERROR` | `1` | Suppress GL error checking (performance) |
| `LIBGL_SILENTSTUB` | `1` | Don't log stub GL calls |
| `ANGLE_DEFAULT_PLATFORM` | `vulkan` | ANGLE selects Vulkan backend |
| `WINE_D3D_CONFIG` | `renderer=gl` | Wine uses OpenGL renderer (not Vulkan) |
| `WINE_X11FORCEGLX` | `1` | Force Wine's x11drv to use GLX |
| `LD_LIBRARY_PATH` | `…/gl4es/lib:…/angle/lib:/system/lib64:…/usr/lib` | Library search order |

---

## 6. Remaining Work

### Immediate (verify rendering)
- [ ] Get full logcat from `BOX64_X11GLX=1` run including wined3d trace
- [ ] Confirm Wine successfully creates a GL context (look for `wined3d_adapter_gl_init` success)
- [ ] Test with a simple DX9 game (e.g., Peggle Extreme)

### Build pipeline cleanup
- [ ] Update `_angle_build/build_angle.sh` to target `target_os = "android"`
- [ ] Create unified repackage script (strip → rename → patchelf SONAME → tar+zstd)
- [ ] Clean stale `Release_arm64` output and `package/` directory in WSL

### Future optimization
- [ ] Test `BOX64_X11GLX=1` vs GL4ES GLX path to determine if Box64's built-in GLX is sufficient
- [ ] Profile ANGLE Vulkan overhead vs native turnip
- [ ] Consider `angle_enable_gl=true` for hybrid backends on specific GPUs

