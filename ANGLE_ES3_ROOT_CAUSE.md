# ANGLE ES3 Context Failure — Root Cause Analysis

## The Problem
```
VirGL-ANGLE: ES3 context failed (error 0x3009), trying ES2...
VirGL-ANGLE: ES2 context created (reduced features)
VirGL-ANGLE: GL Version: OpenGL ES 2.0 (ANGLE 2.1.58 git hash: c7ac96e8c670)
VirGL-ANGLE: Features: VAB=0 sync=0 UBO=0 TF=0 samplers=0 texArray=0 texStorage=0 drawInst=0 multisample=0
```

Error `0x3009` = `EGL_BAD_MATCH`. **ALL** VirGL features are disabled (zero).

## Architecture Overview

```
┌─────────────────────────────────────────────────────────────────────────┐
│ GUEST (GLIBC container via Box64)                                       │
│   Game → WineD3D → OpenGL → Mesa virgl driver → unix socket            │
├─────────────────────────────────────────────────────────────────────────┤
│ HOST (Android / Bionic)                                                 │
│   virgl_server (libvirglrenderer_angle.so)                              │
│     → angle_dispatch.c (redirects all egl/gl calls to ANGLE)           │
│       → dlopen ANGLE libEGL.so + libGLESv2.so from imagefs/opt/angle/  │
│         → ANGLE Vulkan backend → Vulkan ICD → GPU                      │
├─────────────────────────────────────────────────────────────────────────┤
│ Java side:                                                              │
│   GPUHelper.java → JNI → gpu_helper.c (queries Vulkan extensions)       │
│   VirGLAngleRendererComponent.java → loads libvirglrenderer_angle.so    │
│   XServerScreen.kt → extracts angle-7748.tzst + gl4es assets           │
│   GLRenderer.java → system EGL rendering (separate from ANGLE)         │
└─────────────────────────────────────────────────────────────────────────┘
```

## How ANGLE Decides ES Version

**File:** `angle/src/libANGLE/renderer/vulkan/vk_renderer.cpp:5036`
**Function:** `Renderer::getMaxSupportedESVersion()`

ANGLE starts at ES 3.2 and downgrades based on missing Vulkan features:

| Requirement | Missing → Limits To | Likely on Your Phone? |
|---|---|---|
| `VK_EXT_provoking_vertex` | ES 2.0 | **❌ Missing (Mali)** |
| `standardSampleLocations` | ES 2.0 | ✅ Usually yes |
| `independentBlend` | ES 2.0 | ✅ Usually yes |
| `VK_EXT_transform_feedback` OR `vertexPipelineStoresAndAtomics` | ES 2.0 | ✅ Usually yes |
| Sufficient uniform buffers | ES 2.0 | ✅ Usually yes |
| Sufficient vertex output components | ES 2.0 | ✅ Usually yes |

### The Blocker: `VK_EXT_provoking_vertex`

**What it does:** Controls which vertex provides the "flat shading" color for a triangle primitive. OpenGL uses "last vertex" (provoking vertex = last), D3D uses "first vertex".

**Why ANGLE requires it:** For strict OpenGL ES conformance, flat shading must use the last vertex. Without `VK_EXT_provoking_vertex`, ANGLE can't guarantee correct flat shading behavior, so it refuses to expose ES3.

**Why it's irrelevant for our use case:** WineD3D translates D3D calls to OpenGL. D3D uses "first vertex" convention by default. The provoking vertex mismatch only affects flat-shaded primitives (rare in modern games), and even when used, the visual difference is typically unnoticeable.

### How Extensions Flow

```
GPU Hardware
  → Vulkan ICD driver (vendor-specific)
    → Reports VK_EXT_provoking_vertex (or not)
      → ANGLE queries via vkGetPhysicalDeviceFeatures2()
        → Sets mProvokingVertexFeatures.provokingVertexLast
          → getMaxSupportedESVersion() checks this
            → Limits to ES 2.0 if missing
              → eglChooseConfig returns only ES2-capable configs
                → eglCreateContext with ES3 → EGL_BAD_MATCH
```

### How GPUHelper.java Fits In

`GPUHelper.java` uses JNI to call `gpu_helper.c` which queries:
- `vkGetDeviceExtensions()` — lists all Vulkan device extensions
- `vkGetApiVersion()` — reports Vulkan API version

This info is used by ContainerConfigDialog to show available extensions to the user, and by VortekRendererComponent to set Vulkan version limits. **But it is NOT passed to ANGLE.** ANGLE independently queries Vulkan when it initializes its display via `eglGetPlatformDisplayEXT()` → creates its own Vulkan instance internally.

## The Fix

**Patch ANGLE source** to skip the `provokingVertex` requirement when `exposeNonConformantExtensionsAndVersions` is enabled (which it is in our build — `angle_expose_non_conformant_extensions_and_versions = true`):

```diff
// In angle/src/libANGLE/renderer/vulkan/vk_renderer.cpp, getMaxSupportedESVersion():

    if (!mFeatures.provokingVertex.enabled)
    {
-       maxVersion = LimitVersionTo(maxVersion, {2, 0});
+       if (!mFeatures.exposeNonConformantExtensionsAndVersions.enabled)
+           maxVersion = LimitVersionTo(maxVersion, {2, 0});
    }
```

This is safe because:
1. The build already sets `angle_expose_non_conformant_extensions_and_versions = true`
2. The `exposeNonConformantExtensionsAndVersions` feature is specifically designed for relaxing conformance requirements
3. The provoking vertex issue only affects flat shading convention, which is rarely triggered and irrelevant for D3D games

## Files Modified

### Native Code (already applied):
- `virgl_server_renderer.c` — Added Vulkan diagnostic logging (`virgl_angle_dump_vulkan_caps()`)
  - Queries GPU name, Vulkan version
  - Checks `VK_EXT_provoking_vertex`, `VK_EXT_transform_feedback`, `VK_EXT_line_rasterization`
  - Checks `standardSampleLocations`, `independentBlend`, `vertexPipelineStoresAndAtomics`
  - Logs all GL/EGL extensions on context creation

### ANGLE Source (requires rebuild in WSL):
- `_angle_build/rebuild_angle_es3_fix.sh` — Build script that patches and rebuilds
- `_angle_build/patches/force_es3_provoking_vertex.patch` — The ES3 fix patch

## Expected Result After Fix

```
VirGL-ANGLE-Diag: GPU: Mali-G710 (Vulkan 1.1.xxx)
VirGL-ANGLE-Diag: VK_EXT_provoking_vertex: NO (this blocks ANGLE ES3!)
VirGL-ANGLE-Diag: VK_EXT_transform_feedback: YES
VirGL-ANGLE-Diag: standardSampleLocations: YES
VirGL-ANGLE-Diag: independentBlend: YES
VirGL-ANGLE: ES3 context created successfully ← after fix
VirGL-ANGLE: GL Version: OpenGL ES 3.0 (ANGLE ...)
VirGL-ANGLE: Features: VAB=0 sync=1 UBO=1 TF=1 samplers=1 texArray=1 texStorage=1 drawInst=1 multisample=1
```

## Rebuild Instructions

1. Open WSL terminal
2. Run: `bash /mnt/c/Users/Catarina/StudioProjects/GameNative-mine/_angle_build/rebuild_angle_es3_fix.sh`
3. Wait for ninja build (~2-5 min incremental)
4. The script deploys `angle-7748.tzst` to assets automatically
5. Rebuild APK in Android Studio
6. Install and test — check logcat for `VirGL-ANGLE-Diag:` and `VirGL-ANGLE:` lines

