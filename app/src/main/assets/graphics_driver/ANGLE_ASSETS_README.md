# ANGLE + GL4ES Asset Files — Bionic Build

> **✅ ANGLE works in the Bionic container variant only.**
>
> ANGLE is built with `target_os="android"` (Bionic), which selects
> `VK_KHR_android_surface` WSI — the native Android Vulkan windowing
> integration. This works on any Android GPU driver (Adreno, Mali, etc.).
>
> **GLIBC containers cannot use ANGLE directly**, but the **VirGL-ANGLE hybrid**
> uses these same ANGLE libraries on the **host side** as the EGL/GLES backend
> for VirGL rendering — see `VIRGL_ANGLE_IMPLEMENTATION.md`.

---

## Used By

| Driver mode | Container | How ANGLE is used |
|---|---|---|
| **ANGLE** (Bionic) | Bionic | GL4ES → ANGLE EGL/GLES → Vulkan (guest-side) |
| **VirGL-ANGLE** (GLIBC) | GLIBC | virglrenderer 1.3.0 → ANGLE EGL/GLES → Vulkan (host-side) |

---

## Required Assets

### `angle-7748.tzst`
- **Source:** Google ANGLE, chromium/7748 branch (commit c7ac96e)
- **License:** BSD 3-Clause (see `THIRD_PARTY_NOTICES`)
- **Build target:** `target_os="android" target_cpu="arm64"` (ARM64 Bionic)
- **Build date:** 2026-03-23
- **Build flags:** ThinLTO enabled, non-conformant extensions exposed, Vulkan-only backend
- **Contents:** Extracts to `opt/angle/lib/` relative to rootDir:
  - `libEGL.so` (325 KB) — ANGLE's EGL implementation (SONAME patched to match filename)
  - `libGLESv2.so` (5.5 MB) — ANGLE's GLES→Vulkan translator (SONAME patched to match filename)
- **Runtime dependencies:** Only Android Bionic system libs:
  - `libdl.so`, `libc.so` (Bionic)
  - `libm.so`, `liblog.so`, `libnativewindow.so` (Android system)
  - C++ stdlib is bundled (libc++)
- **SONAME note:** The upstream ANGLE build produces `libEGL_angle.so` and `libGLESv2_angle.so`
  as SONAMEs. These have been patched to `libEGL.so` and `libGLESv2.so` using `patchelf` so
  that GL4ES can find them via standard `dlopen("libEGL.so")` / `dlopen("libGLESv2.so")`.
- **IMPORTANT — symlinks required:** Even though SONAMEs were patched, `libEGL.so` still has
  an internal `dlopen("libGLESv2_angle.so")` call (hardcoded in ANGLE's EGL loader). Without
  a `libGLESv2_angle.so` → `libGLESv2.so` symlink, loading fails with:
  `"Error loading EGL entry points: dlopen(...libGLESv2_angle.so) not found"`
  which cascades into `wined3d_create` failure (hr 0x80004005) and a black screen.
  The extraction code in `XServerScreen.kt` creates these symlinks automatically.

### `gl4es-bionic-1.1.7.tzst` (current) / `gl4es-bionic-1.1.6.tzst` (previous)
- **Source:** https://github.com/ptitSeb/gl4es (`master`, commit `9e8037b`)
- **License:** MIT (see `THIRD_PARTY_NOTICES`)
- **Build target:** aarch64 Android/Bionic (built with Chromium clang 23, API level 28)
- **Contents:** Extracts to `opt/gl4es/lib/` relative to rootDir:
  - `libGL.so.1` (1.6 MB) — GL4ES desktop GL→GLES translation library
- **Runtime dependencies:** `libc.so` (Bionic), `libm.so`, `libdl.so`, `liblog.so`
- **GLX support:** 20+ glX* symbols exported (glXChooseFBConfig, glXSwapBuffers, etc.)
  with X11 stubs statically linked (XOpenDisplay, XGetVisualInfo, XCreateColormap).
- **IMPORTANT:** This is the **Bionic build**. The old `gl4es-1.1.6.tzst` was glibc-linked
  (`libc.so.6`, `libm.so.6`) and **cannot be loaded** in a Bionic container because Android's
  linker cannot resolve glibc dependencies. This bionic build fixes that.

## Graphics Pipeline

```
Game (DirectX 7/8/9)
  → wined3d (DirectX → OpenGL translation, Wine's built-in DLLs)
    → Wine x11drv (GLX context creation, pixel format selection)
      → Box64 GLX-over-EGL (BOX64_X11GLX=1 — synthesises GLX from EGL/GLES)
        → GL4ES libGL.so.1 (OpenGL 2.1 → OpenGL ES 2.0 translation)
          → ANGLE libEGL.so + libGLESv2.so (GLES → Vulkan translation)
            → Device Vulkan driver (hardware rendering)
```

**Critical:** `BOX64_X11GLX=1` is REQUIRED. Without it, Box64 provides no GLX
implementation and Wine's x11drv gets 0 pixel formats, falling back to the
software-only `adapter_no3d` path. See `ANGLE_INTEGRATION_STATUS.md` for details.

**Key limitation:** GL4ES provides approximately OpenGL 2.1 with some 3.x extensions.
DX10/11 games that need GL 3.0+ will likely not work. ANGLE is best suited for
DX7/8/9 games (older titles) and games that use OpenGL directly.

**When is ANGLE useful?**
- Games using OpenGL directly (not D3D)
- Mali devices where Mesa OpenGL drivers are unavailable
- As a fallback when other graphics drivers fail
- The DXVK path (D3D→Vulkan direct) is always faster for DX10/11 games

## DX Wrapper Compatibility

| DX Wrapper | Compatible | Notes |
|------------|-----------|-------|
| WineD3D    | ✅        | Uses GL renderer, works through GL4ES→ANGLE→Vulkan |
| CNC DDraw  | ✅        | DDraw wrapper, compatible with GL pipeline |
| DXVK       | ❌        | Needs native Vulkan, bypasses GL pipeline entirely |
| VKD3D      | ❌        | Needs native Vulkan for D3D12 translation |

When ANGLE is selected as the graphics driver, the UI automatically hides
DXVK and VKD3D from the DX Wrapper options.

## Build Notes

See `_angle_build/ANGLE_BIONIC_BUILD_NOTES.md` for the full build process,
GN args, and root cause analysis of why the GLIBC build failed.
