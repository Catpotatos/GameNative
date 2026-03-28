# VirGL-ANGLE Implementation: ANGLE as VirGL's Host-Side EGL/GLES Backend

## Overview

This document describes the VirGL-ANGLE feature — a second VirGL option in the
GLIBC container settings that uses Google ANGLE as the host-side EGL/GLES backend
for VirGL rendering, instead of the device's system GLES driver.

## Component Versions

| Component | Version | Source | Notes |
|---|---|---|---|
| **virglrenderer** (host-side renderer) | 1.3.0 | [freedesktop.org virglrenderer](https://gitlab.freedesktop.org/virgl/virglrenderer) | Custom `server/` JNI shim; vrend_renderer core from upstream 1.3.0 |
| **ANGLE** (host-side EGL/GLES backend) | chromium/7748 (commit `c7ac96e`) | [Google ANGLE](https://chromium.googlesource.com/angle/angle) | Built for Android arm64 with Vulkan-only backend, ThinLTO |
| **Mesa virpipe** (guest-side driver) | 23.1.9 | [Mesa](https://mesa3d.org) | Gallium virgl/virpipe guest driver, extracted from `virgl-23.1.9.tzst` |
| **NDK** (build toolchain) | r28.2.13676358 (Clang 19.0.1) | Android NDK | Used to compile `libvirglrenderer_angle.so` for arm64-v8a |

> **Build date:** 2026-03-27
>
> The `libvirglrenderer_angle.so` JNI library was built on this date using the
> versions listed above. If you update any component, rebuild the library using
> `_angle_build/build_virgl_angle_wsl.sh` (WSL) or the Windows-native instructions
> in the Building section below.

## Architecture

### Standard VirGL Chain (existing)

```
┌──────────────────────── GLIBC Container (chroot/proot) ────────────────┐
│                                                                         │
│  Game (D3D) → WineD3D → OpenGL → Mesa virgl guest driver → unix socket │
│                                                                         │
└────────────────────────────────┬────────────────────────────────────────┘
                                 │ virgl protocol over /tmp/.virgl/V0
                                 ▼
┌──────────────────────── Android Host (Java/JNI) ───────────────────────┐
│                                                                         │
│  VirGLRendererComponent → virgl_server.c → virgl_server_renderer.c      │
│                                  │                                      │
│                        eglGetDisplay(EGL_DEFAULT_DISPLAY)               │
│                        eglCreateContext(ES 3.0)                         │
│                        glDrawArrays / glTexImage2D etc.                 │
│                                  │                                      │
│                                  ▼                                      │
│  System EGL/GLES driver (vendor: Qualcomm/Adreno, ARM/Mali, etc.)      │
│                                  │                                      │
│                                  ▼                                      │
│                              GPU Hardware                               │
└─────────────────────────────────────────────────────────────────────────┘
```

### VirGL-ANGLE Chain (new)

```
┌──────────────────────── GLIBC Container (chroot/proot) ────────────────┐
│                                                                         │
│  Game (D3D) → WineD3D → OpenGL → Mesa virgl guest driver → unix socket │
│  (identical guest side — same Mesa virpipe driver)                      │
└────────────────────────────────┬────────────────────────────────────────┘
                                 │ virgl protocol over /tmp/.virgl/V0
                                 ▼
┌──────────────────────── Android Host (Java/JNI) ───────────────────────┐
│                                                                         │
│  VirGLAngleRendererComponent → virgl_server.c → virgl_server_renderer.c │
│                                       │                                 │
│                 eglGetPlatformDisplayEXT(EGL_PLATFORM_ANGLE_ANGLE, ...) │
│                 → ANGLE libEGL.so / libGLESv2.so                        │
│                 → ANGLE translates all GLES → Vulkan internally         │
│                                       │                                 │
│                                       ▼                                 │
│                 Vulkan ICD (system Vulkan or Turnip)  →  GPU            │
└─────────────────────────────────────────────────────────────────────────┘
```

## Key Differences from Standard VirGL

| Aspect                  | Standard VirGL              | VirGL-ANGLE                              |
|-------------------------|-----------------------------|------------------------------------------|
| Host-side GLES          | System vendor driver        | ANGLE (GLES→Vulkan)                      |
| GL version              | Whatever vendor provides    | ANGLE exposes GLES 3.2 + extensions      |
| Extension coverage      | Vendor-dependent            | ANGLE adds many missing GLES extensions  |
| Shader compilation      | Vendor's GLSL compiler      | ANGLE: GLSL → SPIR-V → Vulkan           |
| Vendor bugs             | Directly exposed            | ANGLE abstracts vendor GLES bugs         |
| Non-Adreno support      | Only as good as vendor GLES | Works on any Vulkan GPU (Mali, PowerVR)  |
| Guest-side driver       | Mesa virpipe (identical)    | Mesa virpipe (identical)                 |
| Guest env vars          | Same (GALLIUM_DRIVER=virpipe) | Same                                   |
| JNI library             | libvirglrenderer.so         | libvirglrenderer_angle.so                |
| Java component          | VirGLRendererComponent      | VirGLAngleRendererComponent              |

## Key Difference from Bionic ANGLE

The **Bionic ANGLE** implementation (paused) puts ANGLE in the container's graphics
driver slot, replacing the Vulkan ICD path. This is architecturally wrong because:
- ANGLE provides EGL/GLES, not a Vulkan ICD
- DXVK/VKD3D need a Vulkan ICD — ANGLE can't provide one
- Wine emits desktop OpenGL, not OpenGL ES

**VirGL-ANGLE** places ANGLE correctly: as the **host-side EGL/GLES backend** for
the VirGL renderer. The guest container is unchanged — it still runs Mesa's virpipe
Gallium driver. Only the host-side VirGL server uses ANGLE instead of the system
GLES driver for its rendering calls.

## Files Changed/Added

### New Files
- `app/src/main/java/com/winlator/xenvironment/components/VirGLAngleRendererComponent.java`
  - Java component that loads `libvirglrenderer_angle.so` and manages the ANGLE-backed
    VirGL renderer lifecycle
- `app/src/main/cpp/virglrenderer_angle/` (copied from `virglrenderer/`)
  - Modified `CMakeLists.txt` — builds as `virglrenderer_angle` with `VIRGL_ANGLE_MODE=1`
  - Modified `server/virgl_server.c` — JNI entry points reference `VirGLAngleRendererComponent`
  - Modified `server/virgl_server_renderer.c` — ANGLE EGL init via `eglGetPlatformDisplayEXT`
- `_angle_build/build_virglrenderer_angle.sh` — build script for the JNI library
- `VIRGL_ANGLE_IMPLEMENTATION.md` — this document

### Modified Files
- `app/src/main/res/values/arrays.xml`
  - Added "VirGL-ANGLE (Universal)" to `graphics_driver_entries`
  - Added `virgl_angle_version_entries` array
- `app/src/main/cpp/CMakeLists.txt`
  - Added `add_subdirectory(virglrenderer_angle)`
- `app/src/main/java/app/gamenative/ui/component/dialog/ContainerConfigState.kt`
  - Added `virglAngleVersions` field
- `app/src/main/java/app/gamenative/ui/component/dialog/ContainerConfigDialog.kt`
  - Loads `virgl_angle_version_entries` resource
  - Added `virgl-angle` case to `getVersionsForDriver()`
  - Passes `virglAngleVersions` to `ContainerConfigState`
- `app/src/main/java/app/gamenative/ui/screen/xserver/XServerScreen.kt`
  - Added import for `VirGLAngleRendererComponent`
  - Added `virgl-angle` driver version handling in `extractGraphicsDriverFiles()`
  - Added `virgl-angle` component creation in `setupXEnvironment()`

## Building

### Prerequisites
- Android NDK r25c or later
- CMake 3.22.1+
- The ANGLE asset file `angle-7748.tzst` must exist in `app/src/main/assets/graphics_driver/`

### Build the JNI library
```bash
export ANDROID_NDK_HOME=$HOME/Android/Sdk/ndk/25.2.9519653
bash _angle_build/build_virglrenderer_angle.sh
```

This produces `app/src/main/jniLibs/arm64-v8a/libvirglrenderer_angle.so`.

### How it works at runtime

1. User selects "VirGL-ANGLE (Universal)" in GLIBC container graphics settings
2. `extractGraphicsDriverFiles()` extracts:
   - `virgl-23.1.9.tzst` → guest-side Mesa virpipe driver (same as standard VirGL)
   - `angle-7748.tzst` → ANGLE's `libEGL.so` + `libGLESv2.so` to `opt/angle/lib/`
3. `setupXEnvironment()` creates `VirGLAngleRendererComponent` instead of `VirGLRendererComponent`
4. `VirGLAngleRendererComponent` loads `libvirglrenderer_angle.so` (JNI)
5. When a guest connects, `virgl_server_renderer.c` calls `virgl_server_egl_init()`:
   - Calls `eglGetPlatformDisplayEXT(EGL_PLATFORM_ANGLE_ANGLE, ...)` to request ANGLE
   - ANGLE's Vulkan backend initializes
   - All subsequent GLES calls go through ANGLE → Vulkan → GPU
6. The guest Mesa virpipe driver sends VirGL commands over the unix socket
7. The host VirGL renderer executes them using ANGLE's GLES/Vulkan backend

## Pros and Cons

### Pros
- ✅ **Better GLES extension coverage** — ANGLE exposes GLES 3.2 with many extensions
  that vendor drivers skip, improving VirGL's reported capabilities to the guest
- ✅ **Avoids vendor GLES bugs** — ANGLE abstracts away Mali/Adreno/PowerVR GLES bugs
- ✅ **Better shader compilation** — ANGLE compiles GLSL→SPIR-V→Vulkan, more reliable
  than vendor GLSL compilers
- ✅ **Universal GPU support** — Works on any Vulkan-capable GPU, not just Adreno
- ✅ **No guest-side changes** — Same Mesa virpipe driver, same env vars
- ✅ **Architecturally correct** — ANGLE sits where it belongs (host-side GLES backend)
- ✅ **Non-breaking** — Standard VirGL remains as option 1, VirGL-ANGLE is option 2

### Cons
- ❌ **Extra translation layer** — GLES→Vulkan adds overhead compared to direct GLES
- ❌ **Larger APK** — ANGLE's libGLESv2.so is ~5.5 MB (already bundled for Bionic ANGLE)
- ❌ **Shared EGL context compatibility** — The shared EGL context from GLRenderer uses
  the system EGL, while VirGL-ANGLE uses ANGLE's EGL. Context sharing between different
  EGL implementations may not work. The fallback (EGL_NO_CONTEXT) is safe.
- ❌ **Requires separate JNI library build** — `libvirglrenderer_angle.so` must be built
  and included in `jniLibs/`

## Future Work

- [x] Build `libvirglrenderer_angle.so` and add to jniLibs
  — Done 2026-03-27: virglrenderer 1.3.0 + ANGLE chromium/7748, NDK r28 (Clang 19.0.1), arm64-v8a
- [ ] Test on Adreno, Mali, and PowerVR devices
- [ ] Investigate shared EGL context compatibility between system EGL and ANGLE
- [ ] Consider runtime dlopen approach to avoid needing a separate JNI library
- [ ] Profile performance difference between system GLES and ANGLE-backed VirGL

