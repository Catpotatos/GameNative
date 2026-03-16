# OpenGL ES Extension Analysis for Mali-G76 (GLES 3.2)

## Phone: Mali-G76 — OpenGL ES 3.2 v1.r18p0

---

## Part 1: Can virglrenderer3 (Mesa 24.3.4) render DX10/DX11 via WineD3D?

### Short Answer: **Partially — DX10 yes, DX11 mostly yes, but with caveats.**

### How the chain works:
```
Windows Game (DX10/11) → WineD3D → OpenGL 3.3–4.3 calls → virglrenderer3 (translates to GLES) → Mali-G76 GLES 3.2
```

### What virglrenderer3 advertises to Wine (on your Mali-G76):
Your GLES 3.2 + extensions allow virglrenderer3 to set **GLSL level 430** because:
- ✅ `feat_tessellation` → `GL_EXT_tessellation_shader` / `GL_OES_tessellation_shader` (GLES 3.2)
- ✅ `feat_geometry_shader` → `GL_EXT_geometry_shader` / `GL_OES_geometry_shader` (GLES 3.2)
- ✅ `feat_gpu_shader5` → `GL_EXT_gpu_shader5` / `GL_OES_gpu_shader5` (GLES 3.2)
- ✅ `feat_separate_shader_objects` (GLES 3.1) → bumps to 410
- ✅ `feat_compute_shader` (GLES 3.1) → bumps to **430**

**GLSL 430 ≈ OpenGL 4.3** which means:
- **DX10 (requires GL 3.3)**: ✅ **YES — fully supported**
- **DX11 (requires GL 4.3+)**: ✅ **Mostly — GLSL 430 advertised, most features available**

### DX11 Feature Level Checklist:
| DX11 Feature | Required GL | Status on your device |
|---|---|---|
| Geometry Shaders | GL 3.2 | ✅ `GL_EXT_geometry_shader` |
| Tessellation | GL 4.0 | ✅ `GL_EXT_tessellation_shader` |
| Compute Shaders | GL 4.3 | ✅ GLES 3.1 core |
| SSBOs | GL 4.3 | ✅ GLES 3.1 core |
| Image Load/Store | GL 4.2 | ✅ GLES 3.1 core |
| Indirect Draw | GL 4.0 | ✅ GLES 3.1 core |
| Texture Buffer Objects | GL 3.1 | ✅ `GL_EXT_texture_buffer` |
| Cube Map Arrays | GL 4.0 | ✅ `GL_EXT_texture_cube_map_array` |
| Sample Shading | GL 4.0 | ✅ `GL_OES_sample_shading` |
| Integer Textures | GL 3.0 | ✅ GLES 3.0 core |
| Stream Output / TF | GL 3.0 | ✅ GLES 3.0 core |
| Clip Control (half-Z) | GL 4.5 | ✅ `GL_EXT_clip_control` |
| fp64 (double precision) | GL 4.0 | ❌ Not on GLES (faked via VIRGL_CAP_HOST_IS_GLES) |
| Transform Feedback 3 | GL 4.0 | ❌ Not available on GLES |
| Conditional Render | GL 3.0 | ❌ No `GL_NV_conditional_render` on Mali |
| Multi Draw Indirect | GL 4.3 | ❌ No `GL_EXT_multi_draw_indirect` on Mali |

---

## Part 2: Your Phone's Extensions vs. Virgl Feature Mapping

### ✅ ALREADY MAPPED (feature_list detects these correctly for GLES 3.2):

These are detected either by GLES version check or by extension name match:

| Virgl Feature | Detection | Your Extension |
|---|---|---|
| `feat_geometry_shader` | GLES 3.2 | `GL_EXT_geometry_shader` + `GL_OES_geometry_shader` |
| `feat_tessellation` | GLES 3.2 | `GL_EXT_tessellation_shader` + `GL_OES_tessellation_shader` |
| `feat_gpu_shader5` | GLES 3.2 | `GL_EXT_gpu_shader5` + `GL_OES_gpu_shader5` |
| `feat_copy_image` | GLES 3.2 | `GL_EXT_copy_image` + `GL_OES_copy_image` |
| `feat_sample_shading` | GLES 3.2 | `GL_OES_sample_shading` |
| `feat_cube_map_array` | GLES 3.2 | `GL_EXT_texture_cube_map_array` |
| `feat_khr_debug` | GLES 3.2 | `GL_KHR_debug` |
| `feat_compute_shader` | GLES 3.1 | Core |
| `feat_ssbo` | GLES 3.1 | Core |
| `feat_images` | GLES 3.1 | Core |
| `feat_indirect_draw` | GLES 3.1 | Core |
| `feat_atomic_counters` | GLES 3.1 | Core |
| `feat_texture_gather` | GLES 3.1 | Core |
| `feat_fb_no_attach` | GLES 3.1 | Core |
| `feat_separate_shader_objects` | GLES 3.1 | Core |
| `feat_stencil_texturing` | GLES 3.1 | Core |
| `feat_storage_multisample` | GLES 3.1 | Core |
| `feat_texture_multisample` | GLES 3.1 | Core |
| `feat_sample_mask` | GLES 3.1 | Core |
| `feat_sampler_border_colors` | GLES 3.2 | `GL_EXT_texture_border_clamp` |
| `feat_indep_blend` | GLES 3.2 | `GL_EXT_draw_buffers_indexed` |
| `feat_indep_blend_func` | GLES 3.2 | `GL_OES_draw_buffers_indexed` |
| `feat_draw_instance` | GLES 3.0 | Core |
| `feat_transform_feedback` | GLES 3.0 | Core |
| `feat_transform_feedback2` | GLES 3.0 | Core |
| `feat_ubo` | GLES 3.0 | Core |
| `feat_texture_array` | GLES 3.0 | Core |
| `feat_samplers` | GLES 3.0 | Core |
| `feat_occlusion_query_boolean` | GLES 3.0 | `GL_EXT_occlusion_query_boolean` |
| `feat_texture_storage` | GLES 3.0 | Core |
| `feat_arb_or_gles_ext_texture_buffer` | ext match | `GL_EXT_texture_buffer` |
| `feat_blend_equation_advanced` | GLES 3.2 | `GL_KHR_blend_equation_advanced` |
| `feat_implicit_msaa` | ext match | `GL_EXT_multisampled_render_to_texture` |
| `feat_texture_srgb_decode` | ext match | `GL_EXT_texture_sRGB_decode` |
| `feat_srgb_write_control` | ext match | `GL_EXT_sRGB_write_control` |
| `feat_egl_image` | ext match | `GL_OES_EGL_image` |
| `feat_robust_buffer_access` | ext match | `GL_KHR_robust_buffer_access_behavior` |
| `feat_gles_khr_robustness` | ext match | `GL_KHR_robustness` |
| `feat_timer_query` | ext match | `GL_EXT_disjoint_timer_query` |
| `feat_anisotropic_filter` | ext match | `GL_EXT_texture_filter_anisotropic` |
| `feat_clip_control` | ext match | `GL_EXT_clip_control` |
| `feat_arb_buffer_storage` | ext match | `GL_EXT_buffer_storage` |
| `feat_base_instance` | ext match | `GL_EXT_base_instance` ... **WAIT** |

---

### 🔴 UNMAPPED EXTENSIONS — Available on your phone but NOT in the feature_list!

These extensions exist on your Mali-G76 but virglrenderer3's `feature_list[]` does NOT
check for them. Some can be **added** to improve DX10/DX11 rendering:

#### 1. `GL_EXT_clip_control` — **CRITICAL for DX11** ⚠️
- **Status**: The feature_list entry is:
  ```c
  FEAT(clip_control, 45, UNAVAIL, "GL_ARB_clip_control", "GL_EXT_clip_control")
  ```
- ✅ **Already mapped!** `GL_EXT_clip_control` IS in the list. Good.
- BUT: The `gl_stubs.c` has `glClipControl` as a **no-op stub!** This must be fixed
  to use the actual EGL extension function pointer.

#### 2. `GL_EXT_draw_elements_base_vertex` / `GL_OES_draw_elements_base_vertex`
- **Your phone has**: ✅ Both extensions
- **Virgl checks**: Only via `feat_base_instance` with `GL_EXT_base_instance`
  which your phone does NOT have.
- **Impact**: `glDrawElementsBaseVertex` is available on your phone and virgl
  DOES use it (via GLES 3.2 core). This should work since GLES 3.2 includes it.

#### 3. `GL_ARM_shader_framebuffer_fetch` / `GL_ARM_shader_framebuffer_fetch_depth_stencil`
- **Your phone has**: ✅ Both ARM-specific extensions
- **Virgl checks for**: `GL_EXT_shader_framebuffer_fetch` only
- **Impact**: These are ARM-specific variants. The `feat_framebuffer_fetch` looks for
  `GL_EXT_shader_framebuffer_fetch` which your phone does NOT have.
  **Could be added** as an additional extension string to map.
- **Benefit**: Enables `VIRGL_CAP_TGSI_FBFETCH` — allows deferred rendering optimizations.

#### 4. `GL_EXT_shader_pixel_local_storage`
- **Your phone has**: ✅
- **Virgl does NOT check**: Not in feature_list
- **Impact**: Could improve tile-based rendering efficiency. Not directly needed for
  DX10/11 but could improve performance on Mali.

#### 5. `GL_OES_shader_image_atomic`
- **Your phone has**: ✅
- **Virgl checks**: Via `feat_images` (GLES 3.1 core) — image atomics are part of
  image load/store. **Already covered.**

#### 6. `GL_EXT_color_buffer_half_float` / `GL_EXT_color_buffer_float`
- **Your phone has**: ✅ Both
- **Virgl checks**: These are used in format table detection (`vrend_formats.c`),
  not feature_list. **Already handled via format probing.**

#### 7. `GL_EXT_texture_sRGB_R8` / `GL_EXT_texture_sRGB_RG8`
- **Your phone has**: ✅ Both
- **Virgl does NOT check**: Not in feature_list, and not in format probing
- **Impact**: Enables sRGB single-channel and two-channel textures.
  Some DX10/11 games use SR8/SRG8 formats. **Could be added to format table.**

#### 8. `GL_EXT_external_buffer`
- **Your phone has**: ✅
- **Virgl does NOT check**: Not in feature_list
- **Impact**: Could improve buffer sharing between contexts.

#### 9. `GL_OVR_multiview` / `GL_OVR_multiview2`
- **Your phone has**: ✅ Both
- **Virgl does NOT check**: Not relevant for DX10/11 (VR extensions)

#### 10. `GL_EXT_protected_textures`
- **Your phone has**: ✅
- Not relevant for game rendering.

---

## Part 3: ACTIONABLE FIXES for DX10/DX11 improvement

### Fix 1: `glClipControl` stub → real implementation (CRITICAL)
The `gl_stubs.c` has `glClipControl` as a no-op, but your Mali-G76 supports
`GL_EXT_clip_control`. This is **critical for DX11** depth handling (half-Z).

**File**: `virglrenderer3/src/gl_stubs.c` line 97
**Current**: `void glClipControl(GLenum origin, GLenum depth) { (void)origin;(void)depth; }`
**Fix**: Remove the stub and use the real `glClipControlEXT` from the extension,
or use `eglGetProcAddress` to resolve it at runtime.

### Fix 2: Add `GL_ARM_shader_framebuffer_fetch` to feature_list
**File**: `vrend_renderer.c` line 275
**Current**: `FEAT(framebuffer_fetch, UNAVAIL, UNAVAIL, "GL_EXT_shader_framebuffer_fetch")`
**Add**: `"GL_ARM_shader_framebuffer_fetch"` as an additional ext string

### Fix 3: `glPolygonOffsetClampEXT` stub → real implementation
The `GL_EXT_polygon_offset_clamp` extension may or may not be on Mali-G76
(not listed in your dump), but if it were, the stub would prevent it working.

### Fix 4: Add sRGB R8/RG8 format support
Add `GL_EXT_texture_sRGB_R8` and `GL_EXT_texture_sRGB_RG8` detection to the
format table in `vrend_formats.c` for the corresponding VIRGL formats.

---

## Part 4: What WineD3D feature levels map to

| WineD3D/DX Level | Required GL | Your virglrenderer3 advertises |
|---|---|---|
| DX9 | GL 2.1 | ✅ |
| DX9Ex | GL 3.0 | ✅ |
| DX10 | GL 3.3 | ✅ |
| DX10.1 | GL 3.3 + extensions | ✅ |
| DX11 FL 10_0 | GL 3.3 | ✅ |
| DX11 FL 10_1 | GL 3.3 | ✅ |
| DX11 FL 11_0 | GL 4.2-4.3 | ✅ (GLSL 430 advertised) |
| DX11 FL 11_1 | GL 4.4+ | ⚠️ Partial (some 4.4+ features missing) |

### Summary: Your Mali-G76 + virglrenderer3 with Mesa 24.3.4 should support:
- **DX9/DX10**: Full support ✅
- **DX11 Feature Level 11_0**: Supported ✅ (with the clip_control fix)
- **DX11 Feature Level 11_1**: Partial ⚠️ (missing some GL 4.4+ features)

The biggest immediate win is **fixing the `glClipControl` stub** since your hardware
actually supports it via `GL_EXT_clip_control`.

