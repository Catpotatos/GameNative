# GL4ES GLX Build Fix — ANGLE Rendering Pipeline

## Problem

When using the ANGLE graphics driver, Wine fails to initialize OpenGL with:

```
err:wgl:X11DRV_WineGL_InitOpenglInfo  couldn't initialize OpenGL, expect problems
err:d3d:wined3d_caps_gl_ctx_create Failed to find a suitable pixel format.
warn:d3d:wined3d_create Failed to initialize wined3d object, hr 0x80004005.
```

This results in a black screen — no game renders.

## Root Cause

The gl4es library (`libGL.so.1`) was built with **GLX stubs** instead of a real GLX
implementation:

```bash
# OLD (broken) build flags:
-DNOX11=ON      # ← disables X11/GLX support entirely
-DGLX_STUBS=ON  # ← provides empty GLX functions that return 0/NULL
```

The rendering pipeline requires working GLX:

```
Game → DirectX → wined3d → OpenGL/GLX → gl4es (GLX→EGL) → ANGLE (GLES→Vulkan) → GPU
                                          ↑
                            Wine calls glXGetFBConfigs here.
                            With GLX stubs, it returns 0 formats.
                            wined3d then fails to create an adapter.
```

Wine uses `WINE_X11FORCEGLX=1` to force GLX (handled by gl4es). gl4es's **real** GLX
implementation wraps EGL calls internally — it creates EGL contexts through ANGLE when
Wine calls GLX functions. But with stubs, the GLX functions just return failure.

## Fix

### 1. Build the X11 stub library

gl4es's GLX code references Xlib functions (`XFree`, `XOpenDisplay`, etc.) and X11 type
definitions. On Android/Bionic there's no native X11, so we provide minimal stubs:

```bash
cd _angle_build/x11_stubs
bash build_x11_stubs.sh
```

This creates `x11_stubs/lib/libX11.so` — a tiny (~8KB) bionic library with no-op
implementations of the handful of Xlib functions gl4es calls. With `LIBGL_FB=3`
(pbuffer/FBO mode), gl4es never actually renders to an X11 window, so these stubs
are sufficient.

### 2. Build gl4es with GLX support

```bash
cd _angle_build
bash build_gl4es_bionic.sh
```

Key changes from the old build:
- **Removed** `-DNOX11=ON` → enables gl4es's real GLX→EGL wrapping code
- **Removed** `-DGLX_STUBS=ON` → uses real GLX implementation, not empty stubs
- **Added** X11 stub headers via `-I` (for compile-time type definitions)
- **Added** X11 stub library via `-L` (for link-time symbol resolution)

### 3. Package and deploy

The build script outputs both files to `build_bionic/output/opt/gl4es/lib/`:
- `libGL.so.1` — gl4es with working GLX
- `libX11.so` — runtime stub for Xlib function references

Package them:
```bash
cd /home/catarina/gl4es_build/build_bionic/output
tar cf - opt/gl4es | zstd -19 -o gl4es-bionic-1.1.7.tzst
```

Copy the `.tzst` to `app/src/main/assets/graphics_driver/`.

### 4. Version bump

`DefaultVersion.GL4ES` has been updated from `"1.1.6"` to `"1.1.7"`. The ANGLE
cache ID now includes the GL4ES version, so existing installations will automatically
re-extract the new build.

## How It Works

After the fix, the initialization flow becomes:

1. Wine starts, loads `libGL.so.1` (gl4es with real GLX)
2. Wine calls `glXGetFBConfigs` → gl4es's GLX code runs
3. gl4es calls `eglGetDisplay(EGL_DEFAULT_DISPLAY)` → ANGLE's EGL initializes
4. ANGLE creates a Vulkan instance using the system Vulkan driver (`/system/lib64`)
5. gl4es enumerates EGL configs from ANGLE, wraps them as GLX FBConfigs
6. Wine gets N>0 pixel formats → `wined3d_caps_gl_ctx_create` succeeds
7. Wine creates an OpenGL context via GLX → gl4es creates an EGL context via ANGLE
8. wined3d initializes successfully → Direct3D works → game renders

## Caveats

- **`-DANDROID=ON` CONFIRMED ISSUE**: gl4es's CMakeLists.txt treats `ANDROID=ON`
  as implying `NOX11=ON`, silently overriding any `-DNOX11=OFF` you pass.
  **This has been removed from `build_gl4es_bionic.sh`.** The toolchain file
  already targets `aarch64-linux-android28`, and `-DUSE_ANDROID_LOG=ON` provides
  Android logging without the `ANDROID` flag. The build script now also includes
  a post-build GLX symbol verification step to catch this regression.

- **Box64 always intercepts libGL**: Box64's built-in `libGL.so.1` wrapper ALWAYS
  intercepts Wine's OpenGL/GLX calls, regardless of LD_LIBRARY_PATH contents.
  GL4ES in LD_LIBRARY_PATH is never loaded by Box64. The `BOX64_X11GLX` flag
  controls whether Box64's wrapper synthesises GLX over EGL:
    - `BOX64_X11GLX=0`: NO GLX → Wine gets 0 pixel formats → adapter_no3d → black screen.
    - `BOX64_X11GLX=1`: Box64 synthesises GLX over EGL → calls `dlopen("libEGL.so")`
      → finds ANGLE's EGL via LD_LIBRARY_PATH → Wine gets valid pixel formats → works.
  The ANGLE env setup sets `BOX64_X11GLX=1` (matching BionicProgramLauncherComponent).
  GL4ES is only relevant for FEX or direct ARM64 processes, not Box64.
  **Important**: Do NOT add `libGL.so.1:libGL.so` to `BOX64_EMULATED_LIBS` — gl4es is
  compiled as ARM64 (bionic), and listing it as emulated tells Box64 to load it as
  x86_64, which silently fails.

- **Additional X11 headers**: If gl4es's GLX code includes headers beyond what's in
  `x11_stubs/include/`, the build will fail with missing header errors. Check the
  error message and add the needed types to the appropriate stub header.

- **Xrandr**: gl4es may use Xrandr for screen resolution queries. A stub `Xrandr.h`
  is provided. If gl4es links against `libXrandr.so`, add it to the stub library or
  compile the Xrandr stubs into `libX11.so`.

## Files Changed

| File | Change |
|------|--------|
| `_angle_build/build_gl4es_bionic.sh` | Removed `-DNOX11=ON -DGLX_STUBS=ON`, added X11 stub paths |
| `_angle_build/x11_stubs/` | New: minimal X11 headers and stub library for cross-compilation |
| `app/.../DefaultVersion.java` | Bumped `GL4ES` from `"1.1.6"` to `"1.1.7"` |
| `app/.../XServerScreen.kt` | ANGLE cache ID now includes GL4ES version |
| `app/.../FileUtils.kt` | Fixed resource leak in `readFileAsString` (unrelated) |

