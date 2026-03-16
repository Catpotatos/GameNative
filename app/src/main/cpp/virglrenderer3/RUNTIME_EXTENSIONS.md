# virglrenderer3 — Runtime Extension Resolution Guide

## Overview

virglrenderer3 (built from virglrenderer 1.3.0 + Mesa 24.3.4 utilities) runs on
Android GLES 3.2 devices. Since the GLES API lacks many desktop GL functions that
virglrenderer references, `gl_stubs.c` provides fallbacks. Originally these were
no-op stubs, but many Android GPUs actually support these functions through GLES
extensions.

**Strategy**: At first call, each stub uses `eglGetProcAddress()` to attempt
runtime resolution of the real function. If the driver provides it, we use it.
If not, we fall back to the original no-op or safe fallback behavior.

This ensures:
- **High-end GPUs** (Adreno 750, etc.) use their full capabilities
- **Mid-range GPUs** (Mali-G76, etc.) continue working without regressions
- **No compile-time branching** — everything is resolved at runtime

---

## Runtime-Resolved Functions (gl_stubs.c)

### Critical for DX11 Rendering

| Function | Extension | Resolves via | Impact |
|---|---|---|---|
| `glClipControl` | `GL_EXT_clip_control` | `glClipControlEXT` | **DX11 half-Z depth** — without this, depth buffer mapping is incorrect for all DX10/11 games |
| `glBufferStorage` | `GL_EXT_buffer_storage` | `glBufferStorageEXT` | Persistent/coherent buffer mapping — major performance improvement, fewer GPU stalls |
| `glTextureView` | `GL_OES_texture_view` / `GL_EXT_texture_view` | `glTextureViewOES` or `glTextureViewEXT` | Texture aliasing/reinterpretation — enables `VIRGL_CAP_TEXTURE_VIEW`, needed for DX10/11 texture format casting |
| `glBindFragDataLocationIndexed` | `GL_EXT_blend_func_extended` | `glBindFragDataLocationIndexedEXT` | Dual-source blending — used by DX10/11 for alpha-to-coverage and advanced blend modes |

### Performance / Quality

| Function | Extension | Resolves via | Impact |
|---|---|---|---|
| `glPolygonOffsetClampEXT` | `GL_EXT_polygon_offset_clamp` | `glPolygonOffsetClampEXT` | Clamped polygon offset for shadow mapping quality |

### Buffer/Resource Sharing

| Function | Extension | Resolves via | Impact |
|---|---|---|---|
| `glCreateMemoryObjectsEXT` | `GL_EXT_memory_object` | `glCreateMemoryObjectsEXT` | Cross-process buffer import |
| `glDeleteMemoryObjectsEXT` | `GL_EXT_memory_object` | `glDeleteMemoryObjectsEXT` | Memory object cleanup |
| `glMemoryObjectParameterivEXT` | `GL_EXT_memory_object` | `glMemoryObjectParameterivEXT` | Memory object configuration |
| `glImportMemoryFdEXT` | `GL_EXT_memory_object_fd` | `glImportMemoryFdEXT` | Import FD-backed memory |
| `glTexStorageMem2DEXT` | `GL_EXT_memory_object` | `glTexStorageMem2DEXT` | Create texture from imported memory |
| `glEGLImageTargetTexStorageEXT` | `GL_EXT_EGL_image_storage` | `glEGLImageTargetTexStorageEXT` | EGL image → texture import |

---

## Feature Detection (vrend_renderer.c feature_list)

### Standard features (detected by GLES version or extension string)

These are detected automatically by `init_features()` which checks `gles_ver` and
then falls through to `epoxy_has_gl_extension()` for each extension string:

| Feature | GLES ver | Extension alternatives | Adreno 750 | Mali-G76 |
|---|---|---|---|---|
| `feat_geometry_shader` | 3.2 | `GL_EXT_geometry_shader`, `GL_OES_geometry_shader` | ✅ | ✅ |
| `feat_tessellation` | 3.2 | `GL_EXT_tessellation_shader`, `GL_OES_tessellation_shader` | ✅ | ✅ |
| `feat_gpu_shader5` | 3.2 | `GL_EXT_gpu_shader5`, `GL_OES_gpu_shader5` | ✅ | ✅ |
| `feat_compute_shader` | 3.1 | — | ✅ | ✅ |
| `feat_clip_control` | — | `GL_EXT_clip_control` | ✅ | ✅ |
| `feat_texture_view` | — | `GL_OES_texture_view`, `GL_EXT_texture_view` | ✅ | ❌ |
| `feat_dual_src_blend` | — | `GL_EXT_blend_func_extended` | ✅ | ❌ |
| `feat_depth_clamp` | — | `GL_EXT_depth_clamp` | ✅ | ❌ |
| `feat_cull_distance` | — | `GL_EXT_clip_cull_distance` | ✅ | ❌ |
| `feat_polygon_offset_clamp` | — | `GL_EXT_polygon_offset_clamp` | ✅ | ❌ |
| `feat_texture_mirror_clamp_to_edge` | — | `GL_EXT_texture_mirror_clamp_to_edge` | ✅ | ❌ |
| `feat_shader_noperspective_interpolation` | — | `GL_NV_shader_noperspective_interpolation` | ✅ | ❌ |
| `feat_memory_object` | — | `GL_EXT_memory_object` | ✅ | ❌ |
| `feat_memory_object_fd` | — | `GL_EXT_memory_object_fd` | ✅ | ❌ |
| `feat_egl_image_storage` | — | `GL_EXT_EGL_image_storage` | ✅ | ❌ |
| `feat_framebuffer_fetch` | — | `GL_EXT_shader_framebuffer_fetch` | ✅ | ❌ |
| `feat_arb_buffer_storage` | — | `GL_EXT_buffer_storage` | ✅ | ✅ |
| `feat_anisotropic_filter` | — | `GL_EXT_texture_filter_anisotropic` | ✅ | ✅ |

### ARM-specific features

| Feature | Extension | Adreno 750 | Mali-G76 |
|---|---|---|---|
| `feat_arm_framebuffer_fetch` | `GL_ARM_shader_framebuffer_fetch` | ❌ | ✅ |
| `feat_arm_framebuffer_fetch_depth_stencil` | `GL_ARM_shader_framebuffer_fetch_depth_stencil` | ✅ | ✅ |

Note: `feat_framebuffer_fetch` (EXT) and `feat_arm_framebuffer_fetch` (ARM) are
separate because they require different GLSL code generation:
- **EXT**: Uses `inout` output qualifier, emits `#extension GL_EXT_shader_framebuffer_fetch`
- **ARM**: Uses `gl_LastFragColorARM` built-in, emits `#extension GL_ARM_shader_framebuffer_fetch`

---

## Compressed Texture Format Support

Detected at runtime by `vrend_build_format_list_common()` via extension checks:

| Format Family | Extension | Adreno 750 | Mali-G76 | DX Equivalent |
|---|---|---|---|---|
| S3TC/DXTn | `GL_EXT_texture_compression_s3tc` | ✅ | ❌ | BC1-BC3 (DX10/11 standard) |
| RGTC | `GL_EXT_texture_compression_rgtc` | ✅ | ❌ | BC4-BC5 |
| BPTC | `GL_EXT_texture_compression_bptc` | ✅ | ❌ | BC6H-BC7 (DX11) |
| ASTC | `GL_KHR_texture_compression_astc_ldr` | ✅ | ✅ | — |
| ETC2 | GLES 3.0 core | ✅ | ✅ | — |

**S3TC/RGTC/BPTC on Adreno is huge for DX10/11**: These are the native DirectX
compressed texture formats. Without them, Wine/WineD3D must decompress textures
to RGBA, using 4-8x more VRAM and bandwidth.

---

## Regression Safety

All runtime resolution follows the pattern:
```c
static FuncPtr _resolved = NULL;
static int _flag = 0;
void glFoo(...) {
    if (!_flag) {
        _resolved = eglGetProcAddress("glFooEXT");
        _flag = 1;
    }
    if (_resolved) _resolved(...);  // use real function
    // else: no-op or safe fallback
}
```

The `feature_list[]` + `has_feature()` system in virglrenderer ensures that
capabilities are only advertised to the guest when the host actually supports them.
If `eglGetProcAddress` returns NULL, `has_feature()` will also return false (because
the extension string won't be in `glGetStringi(GL_EXTENSIONS)`), so the renderer
never enters a code path that calls the unresolved function.

**Double safety**: Even if a code path were reached, the stub gracefully no-ops.

---

## Files Modified (virglrenderer3 only — virglrenderer v1 untouched)

| File | Changes |
|---|---|
| `src/gl_stubs.c` | Runtime resolution for: glClipControl, glBufferStorage, glPolygonOffsetClampEXT, glTextureView, glBindFragDataLocationIndexed/EXT, glCreateMemoryObjectsEXT, glDeleteMemoryObjectsEXT, glMemoryObjectParameterivEXT, glImportMemoryFdEXT, glTexStorageMem2DEXT, glEGLImageTargetTexStorageEXT |
| `src/vrend/vrend_renderer.c` | Added feat_arm_framebuffer_fetch + feat_arm_framebuffer_fetch_depth_stencil; separated ARM/EXT fbfetch detection |
| `src/vrend/vrend_shader.h` | Added has_arm_fbfetch to shader config |
| `src/vrend/vrend_shader.c` | ARM framebuffer fetch shader codegen (gl_LastFragColorARM) |

