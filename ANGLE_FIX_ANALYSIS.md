# ANGLE Implementation Fix Analysis

## Date: March 19, 2026

## Problem Summary

Wine falls back to `adapter_no3d` (software, no GPU) with these symptoms:
1. **"max texture = 0!"** — GL_MAX_TEXTURE_SIZE reports 0
2. **"Failed to find a suitable pixel format"** — 0 GLX pixel formats available
3. **"DirectDrawRenderer does not support this"** — D3D device creation fails
4. **"X server not providing GLX pixel formats"** — Wine gets 0 configs from glXGetFBConfigs

## Root Cause: `EGL_PIXMAP_BIT` Mismatch (CRITICAL — FIXED)

### The Problem

GL4ES's `glXGetFBConfigs()` calls ANGLE's `eglChooseConfig()` with:
```c
// GL4ES src/glx/glx.c line 1947 (BEFORE fix)
EGL_SURFACE_TYPE = EGL_PBUFFER_BIT | EGL_PIXMAP_BIT  // = 0x03
```

But ANGLE's Vulkan backend generates EGL configs with:
```c
// ANGLE src/libANGLE/renderer/vulkan/vk_caps_utils.cpp line 1635
config.surfaceType = EGL_WINDOW_BIT | EGL_PBUFFER_BIT | EGL_SWAP_BEHAVIOR_PRESERVED_BIT
// NO EGL_PIXMAP_BIT — ANGLE does NOT support pixmap surfaces on Android
```

Per EGL spec, `EGL_SURFACE_TYPE` matching requires ALL requested bits to be present
in the config. The `EGL_PIXMAP_BIT` (0x02) is missing from ANGLE's configs, so:
- `0x03 & config.surfaceType` ≠ `0x03` → **no configs match**
- `eglChooseConfig()` returns 0 configs
- GL4ES returns NULL from `glXGetFBConfigs()`
- Wine gets 0 pixel formats → `adapter_no3d` fallback

### The Fix (Applied)

Three locations in GL4ES `src/glx/glx.c` were patched:

1. **Line 1947** (`glXGetFBConfigs`): Changed from `EGL_PBUFFER_BIT|EGL_PIXMAP_BIT`
   to `EGL_PBUFFER_BIT`
2. **Line 1844** (`glXChooseFBConfig`): Changed from `EGL_PIXMAP_BIT` to `EGL_PBUFFER_BIT`
3. **Line 1778** (`glXChooseVisual`): Fixed `GLX_PBUFFER_BIT → EGL_PIXMAP_BIT` mapping
   to correctly map to `EGL_PBUFFER_BIT`

GL4ES was rebuilt and deployed to:
`app/src/main/assets/graphics_driver/gl4es-bionic-1.1.7.tzst`

## Secondary Fix: GLXExtension.java buildVisualConfig (FIXED)

The X server's GLX extension stub reported configs with indices 18-27 all zero.
While GL4ES handles configs client-side (via EGL), Wine can also query the X server
for GLX visual configs. Zero values for `GLX_DRAWABLE_TYPE` and `GLX_RENDER_TYPE`
cause Wine to reject configs.

**Fixed** by populating all 28 GLX properties including:
- `GLX_FBCONFIG_ID` = visualId
- `GLX_DRAWABLE_TYPE` = `GLX_WINDOW_BIT | GLX_PBUFFER_BIT`
- `GLX_RENDER_TYPE` = `GLX_RGBA_BIT`
- `GLX_X_RENDERABLE` = True
- `GLX_MAX_PBUFFER_WIDTH/HEIGHT` = 4096

## Verified: ANGLE Binary is Correct

The `angle-7736.tzst` asset contains the **Android (bionic)** build:
- SONAME: `libEGL.so`, `libGLESv2.so` (patched from `_angle` suffix via patchelf)
- Dependencies: `libc.so`, `libdl.so`, `libm.so`, `libnativewindow.so`, `liblog.so` (all bionic)
- Build: `target_os="android"`, `angle_enable_vulkan=true`, `angle_enable_gl=false`
- WSI: `VK_KHR_android_surface` (correct for Android)

Note: There are TWO builds in WSL:
- `angle-7736.tzst` (1.9 MB) — Linux/glibc build from `out/Release_arm64/` — **NOT used**
- `angle-7736-android.tzst` (2.3 MB) — Android/bionic build from `out/Android_arm64/` — **NOT used directly**
- The app asset (2.3 MB) was created from the Android build with patchelf applied ✅

## Verified: Internal Library Loading Works

ANGLE's `libEGL.so` internally dlopens `libGLESv2_angle.so` (compiled-in
`ANGLE_DISPATCH_LIBRARY` constant). The app creates symlinks:
- `libGLESv2_angle.so → libGLESv2.so`
- `libEGL_angle.so → libEGL.so`

`SearchType::ModuleDir` uses `dladdr()` to find the module directory, then
constructs the full path. With the symlinks in place, this resolves correctly.

## Verified: Environment Variable Pipeline

| Variable | Value | Purpose |
|---|---|---|
| `LIBGL_EGL` | `/opt/angle/lib/libEGL.so` | GL4ES → ANGLE EGL |
| `LIBGL_GLES` | `/opt/angle/lib/libGLESv2.so` | GL4ES → ANGLE GLES |
| `LIBGL_FB` | `3` | GL4ES pbuffer mode |
| `LIBGL_ES` | `2` | GL4ES targets GLES2 |
| `ANGLE_DEFAULT_PLATFORM` | `vulkan` | ANGLE Vulkan backend |
| `BOX64_X11GLX` | `0` | Disable Box64 GLX interception |
| `WINE_X11FORCEGLX` | `1` | Wine uses GLX (handled by GL4ES) |
| `LD_LIBRARY_PATH` | gl4es:angle:/system/lib64:/usr/lib | Correct order |

The `putAll(this.envVars)` at line 314 correctly overrides `BOX64_X11GLX=1`
(from `addBox64EnvVars`) with `BOX64_X11GLX=0` (from ANGLE setup).

## Rendering Pipeline (Expected After Fix)

```
Wine (x86_64) → wined3d → OpenGL/GLX calls
    ↓ Box64 x86_64→ARM64 translation
GL4ES (ARM64, libGL.so.1) → GLX→EGL translation
    ↓ dlopen via LIBGL_EGL/LIBGL_GLES env vars
ANGLE (ARM64, libEGL.so + libGLESv2.so) → GLES→Vulkan translation
    ↓ dlopen libvulkan.so from /system/lib64
Android Vulkan driver → GPU
```

## Remaining Limitation: Frame Transport

Even with GL rendering working, rendered pixels stay in ANGLE's GPU pbuffer
and don't automatically appear on screen. The X server's `Drawable` needs to
receive the pixel data. Options:
1. GL4ES's `glXSwapBuffers` → `glReadPixels` → MIT-SHM → X server Drawable
2. Custom ANGLE surface that writes to shared memory
3. Accept no3d for display (CPU blit) while using GPU for rendering

This is a separate issue from the pixel format/adapter initialization failure.

