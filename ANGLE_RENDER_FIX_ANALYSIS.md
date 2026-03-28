# ANGLE Render Fix — Complete Architecture Analysis

## Overview

ANGLE (OpenGL ES → Vulkan translation) support for GameNative targets devices without
Turnip/Mesa drivers. The intended rendering pipeline is:

```
Wine DX7 game → WineD3D (renderer=gl) → GL calls
  → GL4ES (libGL.so.1, GLX→EGL bridge)
  → ANGLE (libEGL.so + libGLESv2.so, GLES→Vulkan)
  → Vulkan → GPU
```

## Current Status

| Component | Status | Notes |
|-----------|--------|-------|
| ANGLE libraries extracted & loaded | ✅ | `libEGL.so`, `libGLESv2.so` in `/opt/angle/lib/` |
| GL4ES linked as `libGL.so.1` | ✅ | In `/opt/gl4es/lib/`, needs rebuild with GLX support |
| Environment variables | ✅ | `LIBGL_FB=3`, `LIBGL_ES=2`, `LIBGL_GL=21`, `ANGLE_DEFAULT_PLATFORM=vulkan` |
| `RENDERMODE_CONTINUOUSLY` | ✅ | Ensures GL thread redraws every vsync |
| GLX extension in X server | ✅ | Full protocol stub with 35 sub-opcodes |
| X11 stub shadowing fix | ✅ | Stubs statically linked into libGL.so.1 |
| `BOX64_EMULATED_LIBS` | ✅ | **Fixed**: libGL removed — gl4es is ARM64, must be natively wrapped by Box64 |
| `WINEDEBUG=+d3d,+wgl` | ✅ | Always enabled for ANGLE |
| GL4ES rebuilt with GLX | ⏳ | **Needs WSL rebuild** with updated `build_gl4es_bionic.sh` |
| Wine creates GL adapter | ❓ | **Needs on-device test** (was adapter_no3d before GLX fix) |
| Frame transport ANGLE→screen | ⚠️ | **Architectural gap** (see below) |

## Implemented Fixes

### Fix #1: RENDERMODE_CONTINUOUSLY

**File:** `XServerView.java`

`RENDERMODE_WHEN_DIRTY` only redraws on `requestRender()`. For ANGLE/adapter_no3d,
the notification chain (`forceUpdate → onDrawListener → requestRender`) may not fire
reliably. `RENDERMODE_CONTINUOUSLY` redraws every vsync. `Texture.updateFromDrawable()`
only uploads when `needsUpdate=true`, so idle overhead is minimal (~0.5ms/frame).

### Fix #2: GLX Extension Stub (Comprehensive)

**File:** `GLXExtension.java` (registered in `XServer.java`)

Wine's `winex11.drv` calls `XQueryExtension("GLX")` via X11 protocol BEFORE loading
libGL. Without GLX, Wine skips GL initialization → falls to `adapter_no3d` (CPU only).

The GLXExtension now handles **all 35 GLX sub-opcodes**:

**Reply-generating (prevent client hangs):**
- `QueryVersion` (7) → GLX 1.4
- `GetVisualConfigs` (14) → 1 RGBA32+D24S8 config matching X server visual
- `GetFBConfigs` (21) → same config as FBConfig
- `QueryExtensionsString` (18) → ARB_multisample, EXT_visual_info, ARB_create_context
- `QueryServerString` (19) → vendor, version, extensions
- `IsDirect` (6) → True (direct rendering via EGL/ANGLE)
- `MakeCurrent` (5) → returns context tag
- `MakeContextCurrent` (26) → returns context tag
- `QueryContext` (25) → returns visual/fbconfig/screen/render_type
- `GetDrawableAttributes` (29) → empty attribute list
- `VendorPrivateWithReply` (17) → empty reply

**No-reply (consume and log):**
- `CreateContext` (3), `CreateNewContext` (24), `CreateContextAttribsARB` (34)
- `DestroyContext` (4), `SwapBuffers` (11)
- Pixmap/Pbuffer/Window create/destroy, ClientInfo, WaitGL/WaitX, etc.

**Key design:** GL4ES handles all actual GLX→EGL translation client-side. These
handlers are safety nets. The critical function is that `XQueryExtension("GLX")`
returns "present" (opcode=-105), which unblocks Wine's GL initialization.

### Fix #3: X11 Stub Static Linking

**File:** `build_gl4es_bionic.sh`

X11 stubs are now compiled as `libX11.a` (static archive) and linked into `libGL.so.1`
via `-Wl,--whole-archive`. No separate `libX11.so` is deployed, so Wine's real
`/usr/lib/libX11.so` is not shadowed. Runtime cleanup in `XServerScreen.kt` removes
stale `libX11.so` files from the GL4ES directory.

### Fix #4: WINEDEBUG Always Active for ANGLE

**File:** `XServerScreen.kt`

Always sets `WINEDEBUG=+d3d,+wgl` for ANGLE builds. The `+wgl` channel traces
Wine's GLX initialization: pixel format selection, `glXCreateContext`, `glXMakeCurrent`,
and whether Wine uses GL adapter or falls to adapter_no3d. Full trace
(`+wgl,+opengl,+d3d,+x11drv`) when `angleLogs=1`.

## The Frame Transport Problem

Even with a working GL adapter, there's an architectural gap in getting rendered
pixels from ANGLE's GPU-side pbuffer to the Android screen:

```
Wine WineD3D → GL → GL4ES FBO (LIBGL_FB=3)
  → ANGLE GLES → Vulkan → GPU pbuffer
  → eglSwapBuffers (no-op on pbuffer)
  → ??? (pixels stay in GPU memory, never reach X server Drawable)

Needed: GPU pbuffer → X server Drawable → Texture → GLRenderer → screen
```

### Why It's Broken

1. GL4ES with `LIBGL_FB=3` renders to an EGL pbuffer (via ANGLE)
2. `glXSwapBuffers()` on a pbuffer is a no-op — there's no visible surface
3. GL4ES's X11 stubs are baked into `libGL.so.1` — calls like `XPutImage()` are no-ops
4. Wine's `winex11.drv` may try `XCopyArea` to present, but the X Pixmap and the
   GL pbuffer are disconnected memory regions

### What Actually Happens (Current Behavior)

Wine likely falls to `adapter_no3d` (CPU rendering) because either:
1. GL4ES can't create an EGL context (needs on-device verification)
2. Wine's pixel format matching between GLX and X server visuals fails

In `adapter_no3d` mode, frames arrive via:
```
Wine CPU render → GDI BitBlt → winex11.drv → X11 PutImage
  → Wine's real libX11 → Unix socket → GameNative X server
  → Drawable.drawImage() → forceUpdate() → Texture → screen
```

This path WORKS for frame transport. The frozen screen issue was due to
`RENDERMODE_WHEN_DIRTY` (now fixed with RENDERMODE_CONTINUOUSLY).

### Future Options for Full GPU Acceleration

1. **VirGL-like Bridge**: Create `ANGLERendererComponent` (like `VirGLRendererComponent`)
   that receives GL commands via Unix socket and renders using ANGLE's EGL context
   shared with `XServerView`. Uses `texture.copyFromFramebuffer()` for frame readback.

2. **AHardwareBuffer Sharing**: Render to an `AHardwareBuffer` (shared GPU memory)
   that both ANGLE and `XServerView` can access. Zero-copy frame transport.

3. **Modified GL4ES with readback**: Patch GL4ES's `glXSwapBuffers` to
   `glReadPixels` → shared memory → X server Drawable.

## Diagnostic Logging Guide

```bash
# Watch the complete ANGLE rendering pipeline:
adb logcat -s Drawable:* GLRenderer:* XServerView:* GLXExtension:* ExtensionRequests:*

# Key markers to look for:

# 1. GLX extension discovery (Wine finds GLX in X server):
#    ExtensionRequests: QueryExtension("GLX") → FOUND (opcode=-105)

# 2. GLX protocol traffic (shows which GLX operations Wine/GL4ES sends):
#    GLXExtension: GLX request: subOpcode=7 (QueryVersion)
#    GLXExtension: QueryVersion: client=1.4, server=1.4

# 3. Wine adapter selection (CRITICAL):
#    Look for "adapter_no3d" (CPU fallback) vs "wined3d_adapter_init" (GL adapter)
adb logcat | grep -iE "adapter_no3d|wined3d_adapter|wgl.*pixel|glx.*create"

# 4. Frame transport (are frames arriving at the X server?):
#    Drawable: forceUpdate stats: 30 calls/sec, 0 had NULL onDrawListener

# 5. Render pipeline health:
#    GLRenderer: Render stats: 60 frames drawn, 30 content updates, continuous=true

# 6. Wine WGL/GLX tracing (always enabled for ANGLE):
#    Look for: wgl:trace, wgl:warn, wgl:err in logcat
adb logcat | grep -i "wgl\|opengl\|pixel.format"
```

## Files Modified

| File | Change |
|------|--------|
| `XServerView.java` | `enableContinuousRenderMode()` + `isContinuousRenderMode()` |
| `Drawable.java` | Throttled diagnostic logging in `forceUpdate()` |
| `GLRenderer.java` | Render stats logging (frames/sec, content updates, render mode) |
| `XServerScreen.kt` | RENDERMODE_CONTINUOUSLY + WINEDEBUG=+d3d,+wgl + X11 stub cleanup |
| `GLXExtension.java` | **NEW** — Full GLX extension stub (35 sub-opcodes, dynamic visual ID) |
| `XServer.java` | Register GLX extension with XServer reference |
| `ExtensionRequests.java` | Diagnostic logging for `QueryExtension` requests |
| `build_gl4es_bionic.sh` | Static X11 stub linking into libGL.so.1 |

## Next Steps

1. **Rebuild GL4ES in WSL** using `build_gl4es_bionic.sh` (without `-DANDROID=ON` for GLX)
2. **Package** into `gl4es-bionic-1.1.7.tzst` → `app/src/main/assets/graphics_driver/`
3. **Deploy & test** — capture full logcat with:
   ```bash
   adb logcat -c && adb logcat | tee angle_test.log
   ```
4. **Analyze logs** for: GLX found, adapter type, frame transport activity
5. **If GL adapter is created**: investigate frame transport gap
6. **If still adapter_no3d**: debug GL4ES EGL initialization (set `LIBGL_DEBUG=1`)
