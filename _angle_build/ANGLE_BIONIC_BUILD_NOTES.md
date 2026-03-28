# ANGLE Bionic Build — Analysis & Integration Notes

## Build Summary

**Date:** March 17, 2026
**ANGLE Version:** chromium/7736 (commit 4554772)
**Build Target:** `target_os = "android"`, `target_cpu = "arm64"`
**Source:** `/home/catarina/angle_build/angle` (WSL Ubuntu)
**Output:** `/home/catarina/angle_build/angle/out/Android_arm64/`
**Status:** ✅ Fully integrated into Bionic container variant

### Build Output Files

| File | Size (stripped) | Size (unstripped) |
|------|----------------|-------------------|
| `libEGL_angle.so` | 302 KB | 4.6 MB |
| `libGLESv2_angle.so` | 5.5 MB | 37.4 MB |

### Library Dependencies (Bionic Build)

**libEGL_angle.so:**
- `libdl.so` (Android Bionic)
- `libc.so` (Android Bionic)

**libGLESv2_angle.so:**
- `libdl.so` (Android Bionic)
- `libm.so` (Android Bionic)
- `libnativewindow.so` (Android system)
- `liblog.so` (Android system)
- `libc.so` (Android Bionic)

### Comparison: Old glibc Build vs New Bionic Build

| Aspect | Old (glibc, target_os=linux) | New (Bionic, target_os=android) |
|--------|------------------------------|--------------------------------|
| **C library** | `libc.so.6` (glibc) | `libc.so` (Bionic) ✅ |
| **Dynamic linker** | `ld-linux-aarch64.so.1` | Android linker ✅ |
| **Threading** | `libpthread.so.0` (separate) | Integrated in libc.so ✅ |
| **X11 dependency** | `libxcb.so.1` ❌ | None ✅ |
| **C++ stdlib** | `libstdc++.so.6` | Bundled (libc++) ✅ |
| **Vulkan WSI** | X11/XCB (VK_KHR_xcb_surface) ❌ | Android (VK_KHR_android_surface) ✅ |
| **Android surface** | `WindowSurfaceVkSimple.o` | `WindowSurfaceVkAndroid.o` ✅ |
| **Display backend** | Unknown Linux display | `DisplayVkAndroid.o` ✅ |
| **HW buffer** | None | `HardwareBufferImageSiblingVkAndroid.o` ✅ |
| **AHB functions** | None | `AHBFunctions.o` ✅ |

## GN Build Args Used

```gn
target_os = "android"
target_cpu = "arm64"
arm_control_flow_integrity = "none"
is_debug = false
is_component_build = false
angle_enable_vulkan = true
angle_enable_gl = false
angle_enable_null = false
angle_enable_metal = false
angle_enable_d3d9 = false
angle_enable_d3d11 = false
angle_enable_wgpu = false
angle_enable_swiftshader = false
angle_build_all = false
enable_rust = false
treat_warnings_as_errors = false
angle_expose_non_conformant_extensions_and_versions = true
```

## Why the glibc Build Failed (Root Cause Analysis)

The original glibc build (`target_os = "linux"`) failed with:
```
VK_ERROR_INITIALIZATION_FAILED (-3) in vk_renderer.cpp:initialize:2433
```

**Root cause:** When `target_os = "linux"`, ANGLE's Vulkan backend selects X11/XCB WSI
(`VK_KHR_xcb_surface` / `VK_KHR_xlib_surface`) as its windowing system integration.
Inside GameNative's Android process, the Vulkan driver (freedreno/turnip) cannot provide
these X11 extensions because:
1. There is no real X11 display server — XConnectorEpoll is a Java emulation
2. The freedreno ICD doesn't advertise VK_KHR_xcb_surface on Android
3. ANGLE's `vk_renderer.cpp:initialize()` fails when it can't create a VkSurface

**Why Turnip+Zink works:** Mesa's Zink uses `VK_KHR_display` or custom present mechanisms
(via Vortek socket), bypassing X11 WSI entirely.

**The fix:** Build with `target_os = "android"`, which selects `VK_KHR_android_surface` WSI.
This is the native Android Vulkan windowing integration that works with any Android GPU driver.

## Could the glibc Build Have Worked?

**Short answer: No, not without major ANGLE source modifications.**

The notes suggesting "it just wasn't implemented correctly" are **partially correct but mostly wrong**:

### What the notes got RIGHT:
1. ✅ Building for Bionic is the correct approach
2. ✅ ANGLE does support Vulkan backend on Android
3. ✅ The system's ANGLE/GPU driver works (it renders the app's UI)
4. ✅ LD_LIBRARY_PATH-based driver selection is the right integration pattern
5. ✅ ANGLE should be an optional component, not replacing DXVK/VKD3D

### What the notes got WRONG or INCOMPLETE:
1. ❌ "Missing libraries and paths" — The real issue was WSI, not missing libs
2. ❌ "Static linking eliminates libc dependency" — Not practical for ANGLE (huge binary, still needs Vulkan loader)
3. ❌ "Run glibc ANGLE in a separate chroot" — Wouldn't help; the Vulkan driver still can't provide X11 WSI
4. ❌ "ANGLE could work with WineD3D" — WineD3D uses full OpenGL, ANGLE is GLES-focused
5. ❌ "Wine3D rendering pipeline with ANGLE" — This would add translation layers, not remove them

### Could you make a glibc ANGLE work?
Only with these impractical changes:
1. Patch ANGLE to add a custom WSI backend (e.g., socket-based like Vortek)
2. Or patch ANGLE to use VK_KHR_display (headless) and blit to the X server
3. Both require deep ANGLE source modifications and ongoing maintenance

## Integration Architecture for Bionic Containers

```
┌─────────────────────────────────────────────┐
│  Game (DirectX/OpenGL)                       │
└────────────────────┬────────────────────────┘
                     │
        ┌────────────┴────────────┐
        │                         │
    ┌───▼──────┐          ┌──────▼──────────┐
    │  DXVK    │          │  WineD3D (GL)   │
    │(D3D→VK)  │          │                 │
    └───┬──────┘          └──────┬──────────┘
        │                        │
        │                 ┌──────▼──────────┐
        │                 │  GL4ES          │
        │                 │  (GL→GLES)      │
        │                 └──────┬──────────┘
        │                        │
        │                 ┌──────▼──────────┐
        │                 │  ANGLE          │  ← NEW (Bionic)
        │                 │  (GLES→VK)      │
        │                 └──────┬──────────┘
        │                        │
        └────────┬───────────────┘
                 │
           ┌─────▼──────┐
           │   Vulkan    │
           │   Driver    │  (Adreno/Mali/etc)
           └─────────────┘
```

**Note:** The DXVK path (D3D→Vulkan direct) is always faster than the
WineD3D→GL4ES→ANGLE→Vulkan path. ANGLE is primarily useful for:
- Games that use OpenGL directly (not D3D)
- Mali devices where Mesa OpenGL drivers are unavailable
- As a fallback when other drivers fail

## Next Steps for Full Bionic Integration

1. ~~**Add ANGLE as a Bionic graphics driver option** in `GraphicsTab.kt`~~ ✅ Done
2. ~~**Update BionicProgramLauncherComponent** to set ANGLE env vars when selected~~ ✅ Done (env vars set in `extractGraphicsDriverFiles`)
3. ~~**Load ANGLE through Android's native linker** (not glibc LD_LIBRARY_PATH)~~ ✅ Done (Bionic build links against Android Bionic libc)
4. **Test on actual device** — especially Mali GPUs where this provides the most value

## Implementation Status

### Completed
- `arrays.xml`: ANGLE added to `bionic_graphics_driver_entries`
- `GraphicsTab.kt`: ANGLE-specific UI with `AngleDxWrapperSection` (hides DXVK/VKD3D)
- `ContainerConfigDialog.kt`: ANGLE state initialization, cleanup on driver switch
- `XServerScreen.kt` → `extractGraphicsDriverFiles`: Full ANGLE extraction and env var setup (Bionic branch)
- `XServerScreen.kt` → `setupXEnvironment`: ANGLE env-var safety block after `container.envVars` merge
- `XServerScreen.kt` → `setupXEnvironment`: ANGLE needs no renderer component (runs in-process)
- Process cleanup: Leftover ANGLE/GL4ES files deleted when switching away from ANGLE
- GLIBC ANGLE code fully removed (was dead code — ANGLE cannot work in GLIBC containers)

### GLIBC Integration — Dead End
ANGLE **cannot** work in GLIBC containers, even through the Bionic build. The reasons:
1. The Bionic-built ANGLE links against Android Bionic `libc.so`, not GLIBC `libc.so.6`
2. The Bionic dynamic linker is incompatible with the GLIBC dynamic linker
3. Even if you could load it, the WSI mechanism (`VK_KHR_android_surface`) requires Android surfaces, not X11
4. Running the Bionic ANGLE through a compatibility layer (e.g., LD_PRELOAD shim) would require reimplementing the entire Android native surface stack

**Bottom line:** ANGLE is Bionic-only. GLIBC containers should use turnip/zink/virgl/vortek instead.

## Files Modified

- `_angle_build/build_android.sh` — Android build script
- `_angle_build/ANGLE_BIONIC_BUILD_NOTES.md` — This file
- `app/src/main/assets/graphics_driver/angle-7736.tzst` — Bionic ANGLE build
- `app/src/main/assets/graphics_driver/gl4es-1.1.6.tzst` — GL4ES translation shim
- `app/src/main/assets/graphics_driver/ANGLE_ASSETS_README.md` — Updated for Bionic
- `app/src/main/res/values/arrays.xml` — ANGLE in bionic driver entries
- `app/src/main/res/values/strings.xml` — ANGLE UI strings
- `app/src/main/java/app/gamenative/ui/component/dialog/GraphicsTab.kt` — ANGLE-aware UI
- `app/src/main/java/app/gamenative/ui/component/dialog/ContainerConfigDialog.kt` — ANGLE state management
- `app/src/main/java/app/gamenative/ui/screen/xserver/XServerScreen.kt` — ANGLE extraction, env vars, cleanup
