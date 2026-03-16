/*
 * Epoxy GL shim for Android GLES
 * Maps libepoxy API calls to direct Android GLES headers.
 * This allows virglrenderer 1.3.0 source to compile against Android NDK
 * without requiring the full libepoxy library.
 */
#ifndef EPOXY_GL_H_SHIM
#define EPOXY_GL_H_SHIM

#include <GLES2/gl2.h>
#include <GLES2/gl2ext.h>
#include <GLES3/gl3.h>
#include <GLES3/gl31.h>
#include <GLES3/gl32.h>

#include <stdbool.h>
#include <string.h>
#include <ctype.h>

/* Desktop GL constants not present in GLES */
#ifndef GL_TEXTURE_1D
#define GL_TEXTURE_1D 0x0DE0
#endif
#ifndef GL_TEXTURE_RECTANGLE
#define GL_TEXTURE_RECTANGLE 0x84F5
#endif
#ifndef GL_TEXTURE_1D_ARRAY
#define GL_TEXTURE_1D_ARRAY 0x8C18
#endif
#ifndef GL_QUADS
#define GL_QUADS 0x0007
#endif
#ifndef GL_QUAD_STRIP
#define GL_QUAD_STRIP 0x0008
#endif
#ifndef GL_POLYGON
#define GL_POLYGON 0x0009
#endif
#ifndef GL_FILL
#define GL_FILL 0x1B02
#endif
#ifndef GL_LINE
#define GL_LINE 0x1B01
#endif
#ifndef GL_POINT
#define GL_POINT 0x1B00
#endif
#ifndef GL_TEXTURE_BUFFER
#define GL_TEXTURE_BUFFER 0x8C2A
#endif
#ifndef GL_SAMPLER_1D
#define GL_SAMPLER_1D 0x8B5D
#endif
#ifndef GL_SAMPLER_1D_SHADOW
#define GL_SAMPLER_1D_SHADOW 0x8B61
#endif
#ifndef GL_SAMPLER_1D_ARRAY
#define GL_SAMPLER_1D_ARRAY 0x8DC0
#endif
#ifndef GL_SAMPLER_1D_ARRAY_SHADOW
#define GL_SAMPLER_1D_ARRAY_SHADOW 0x8DC3
#endif
#ifndef GL_SAMPLER_2D_RECT
#define GL_SAMPLER_2D_RECT 0x8B63
#endif
#ifndef GL_SAMPLER_2D_RECT_SHADOW
#define GL_SAMPLER_2D_RECT_SHADOW 0x8B64
#endif
#ifndef GL_SAMPLER_BUFFER
#define GL_SAMPLER_BUFFER 0x8DC2
#endif
#ifndef GL_INT_SAMPLER_1D
#define GL_INT_SAMPLER_1D 0x8DC9
#endif
#ifndef GL_INT_SAMPLER_1D_ARRAY
#define GL_INT_SAMPLER_1D_ARRAY 0x8DCE
#endif
#ifndef GL_INT_SAMPLER_2D_RECT
#define GL_INT_SAMPLER_2D_RECT 0x8DCD
#endif
#ifndef GL_INT_SAMPLER_BUFFER
#define GL_INT_SAMPLER_BUFFER 0x8DD0
#endif
#ifndef GL_UNSIGNED_INT_SAMPLER_1D
#define GL_UNSIGNED_INT_SAMPLER_1D 0x8DD1
#endif
#ifndef GL_UNSIGNED_INT_SAMPLER_1D_ARRAY
#define GL_UNSIGNED_INT_SAMPLER_1D_ARRAY 0x8DD6
#endif
#ifndef GL_UNSIGNED_INT_SAMPLER_2D_RECT
#define GL_UNSIGNED_INT_SAMPLER_2D_RECT 0x8DD5
#endif
#ifndef GL_UNSIGNED_INT_SAMPLER_BUFFER
#define GL_UNSIGNED_INT_SAMPLER_BUFFER 0x8DD8
#endif
#ifndef GL_TEXTURE_CUBE_MAP_SEAMLESS
#define GL_TEXTURE_CUBE_MAP_SEAMLESS 0x884F
#endif
#ifndef GL_CLAMP_TO_BORDER
#define GL_CLAMP_TO_BORDER 0x812D
#endif
#ifndef GL_MIRROR_CLAMP_TO_EDGE
#define GL_MIRROR_CLAMP_TO_EDGE 0x8743
#endif
#ifndef GL_CLIP_DISTANCE0
#define GL_CLIP_DISTANCE0 0x3000
#endif
#ifndef GL_PROGRAM_POINT_SIZE
#define GL_PROGRAM_POINT_SIZE 0x8642
#endif
#ifndef GL_POINT_SPRITE
#define GL_POINT_SPRITE 0x8861
#endif
#ifndef GL_TEXTURE_RECTANGLE_NV
#define GL_TEXTURE_RECTANGLE_NV GL_TEXTURE_RECTANGLE
#endif
#ifndef GL_RGBA8_SNORM
#define GL_RGBA8_SNORM 0x8F97
#endif
#ifndef GL_FRAMEBUFFER_SRGB
#define GL_FRAMEBUFFER_SRGB 0x8DB9
#endif
#ifndef GL_BGRA
#define GL_BGRA 0x80E1
#endif
#ifndef GL_ABGR_EXT
#define GL_ABGR_EXT 0x8000
#endif
#ifndef GL_UNSIGNED_INT_8_8_8_8
#define GL_UNSIGNED_INT_8_8_8_8 0x8035
#endif
#ifndef GL_UNSIGNED_BYTE_3_3_2
#define GL_UNSIGNED_BYTE_3_3_2 0x8032
#endif
#ifndef GL_UNSIGNED_SHORT_4_4_4_4_REV
#define GL_UNSIGNED_SHORT_4_4_4_4_REV 0x8365
#endif
#ifndef GL_UNSIGNED_SHORT_1_5_5_5_REV
#define GL_UNSIGNED_SHORT_1_5_5_5_REV 0x8366
#endif
#ifndef GL_ALPHA_TEST
#define GL_ALPHA_TEST 0x0BC0
#endif
#ifndef GL_CLAMP
#define GL_CLAMP 0x2900
#endif
#ifndef GL_COMPARE_R_TO_TEXTURE
#define GL_COMPARE_R_TO_TEXTURE 0x884E
#endif
#ifndef GL_DEPTH_COMPONENT32
#define GL_DEPTH_COMPONENT32 0x81A7
#endif
#ifndef GL_DEPTH_TEXTURE_MODE
#define GL_DEPTH_TEXTURE_MODE 0x884B
#endif
#ifndef GL_DEPTH24_STENCIL8_EXT
#define GL_DEPTH24_STENCIL8_EXT 0x88F0
#endif
#ifndef GL_DOUBLE
#define GL_DOUBLE 0x140A
#endif
#ifndef GL_INTERLEAVED_ATTRIBS_EXT
#define GL_INTERLEAVED_ATTRIBS_EXT 0x8C8C
#endif
#ifndef GL_MIRROR_CLAMP_EXT
#define GL_MIRROR_CLAMP_EXT 0x8742
#endif
#ifndef GL_MIRROR_CLAMP_TO_BORDER_EXT
#define GL_MIRROR_CLAMP_TO_BORDER_EXT 0x8912
#endif
#ifndef GL_R16
#define GL_R16 0x822A
#endif
#ifndef GL_R3_G3_B2
#define GL_R3_G3_B2 0x2A10
#endif
#ifndef GL_RG16
#define GL_RG16 0x822C
#endif
#ifndef GL_RGB16
#define GL_RGB16 0x8054
#endif
#ifndef GL_RGBA16
#define GL_RGBA16 0x805B
#endif
#ifndef GL_TEXTURE_SWIZZLE_RGBA
#define GL_TEXTURE_SWIZZLE_RGBA 0x8E46
#endif
#ifndef GL_TIME_ELAPSED
#define GL_TIME_ELAPSED 0x88BF
#endif
#ifndef GL_TIMESTAMP
#define GL_TIMESTAMP 0x8E28
#endif

/* GLclampd type not in GLES */
typedef double GLclampd;

/* Legacy / desktop GL format defines */
#ifndef GL_ALPHA8
#define GL_ALPHA8 0x803C
#endif
#ifndef GL_ALPHA16
#define GL_ALPHA16 0x803E
#endif
#ifndef GL_ALPHA_INTEGER
#define GL_ALPHA_INTEGER 0x8D97
#endif
#ifndef GL_ALPHA8UI_EXT
#define GL_ALPHA8UI_EXT 0x8D7E
#endif
#ifndef GL_ALPHA8I_EXT
#define GL_ALPHA8I_EXT 0x8D90
#endif
#ifndef GL_ALPHA16UI_EXT
#define GL_ALPHA16UI_EXT 0x8D78
#endif
#ifndef GL_LUMINANCE_INTEGER_EXT
#define GL_LUMINANCE_INTEGER_EXT 0x8D9C
#endif
#ifndef GL_LUMINANCE8UI_EXT
#define GL_LUMINANCE8UI_EXT 0x8D80
#endif
#ifndef GL_LUMINANCE8I_EXT
#define GL_LUMINANCE8I_EXT 0x8D92
#endif
#ifndef GL_LUMINANCE16UI_EXT
#define GL_LUMINANCE16UI_EXT 0x8D7A
#endif
#ifndef GL_LUMINANCE_ALPHA_INTEGER_EXT
#define GL_LUMINANCE_ALPHA_INTEGER_EXT 0x8D9D
#endif
#ifndef GL_LUMINANCE_ALPHA8UI_EXT
#define GL_LUMINANCE_ALPHA8UI_EXT 0x8D81
#endif
#ifndef GL_LUMINANCE_ALPHA8I_EXT
#define GL_LUMINANCE_ALPHA8I_EXT 0x8D93
#endif
#ifndef GL_LUMINANCE_ALPHA16UI_EXT
#define GL_LUMINANCE_ALPHA16UI_EXT 0x8D7B
#endif

/* Logic op defines */
#ifndef GL_CLEAR
#define GL_CLEAR 0x1500
#endif
#ifndef GL_AND_REVERSE
#define GL_AND_REVERSE 0x1502
#endif
#ifndef GL_AND_INVERTED
#define GL_AND_INVERTED 0x1504
#endif
#ifndef GL_NAND
#define GL_NAND 0x150E
#endif
#ifndef GL_NOR
#define GL_NOR 0x1508
#endif
#ifndef GL_XOR
#define GL_XOR 0x1506
#endif
#ifndef GL_COPY_INVERTED
#define GL_COPY_INVERTED 0x150C
#endif

/* Barrier bits */
#ifndef GL_CLIENT_MAPPED_BUFFER_BARRIER_BIT
#define GL_CLIENT_MAPPED_BUFFER_BARRIER_BIT 0x00004000
#endif
#ifndef GL_QUERY_BUFFER_BARRIER_BIT
#define GL_QUERY_BUFFER_BARRIER_BIT 0x00008000
#endif

/* Primitive restart */
#ifndef GL_PRIMITIVE_RESTART
#define GL_PRIMITIVE_RESTART 0x8F9D
#endif
#ifndef GL_PRIMITIVE_RESTART_NV
#define GL_PRIMITIVE_RESTART_NV 0x8558
#endif

/* Indirect / parameter buffer */
#ifndef GL_PARAMETER_BUFFER_ARB
#define GL_PARAMETER_BUFFER_ARB 0x80EE
#endif

/* Dual-source blending */
#ifndef GL_SRC1_COLOR
#define GL_SRC1_COLOR 0x88F9
#endif
#ifndef GL_SRC1_ALPHA
#define GL_SRC1_ALPHA 0x8589
#endif
#ifndef GL_ONE_MINUS_SRC1_COLOR
#define GL_ONE_MINUS_SRC1_COLOR 0x88FA
#endif
#ifndef GL_ONE_MINUS_SRC1_ALPHA
#define GL_ONE_MINUS_SRC1_ALPHA 0x88FB
#endif

/* 16/32-bit integer/float alpha/luminance EXT formats */
#ifndef GL_ALPHA16I_EXT
#define GL_ALPHA16I_EXT 0x8D8A
#endif
#ifndef GL_ALPHA32F_ARB
#define GL_ALPHA32F_ARB 0x8816
#endif
#ifndef GL_ALPHA32I_EXT
#define GL_ALPHA32I_EXT 0x8D84
#endif
#ifndef GL_ALPHA32UI_EXT
#define GL_ALPHA32UI_EXT 0x8D72
#endif
#ifndef GL_ALPHA16F_ARB
#define GL_ALPHA16F_ARB 0x881C
#endif
#ifndef GL_LUMINANCE16I_EXT
#define GL_LUMINANCE16I_EXT 0x8D8C
#endif
#ifndef GL_LUMINANCE16F_ARB
#define GL_LUMINANCE16F_ARB 0x881E
#endif
#ifndef GL_LUMINANCE32F_ARB
#define GL_LUMINANCE32F_ARB 0x8818
#endif
#ifndef GL_LUMINANCE32I_EXT
#define GL_LUMINANCE32I_EXT 0x8D86
#endif
#ifndef GL_LUMINANCE_ALPHA16I_EXT
#define GL_LUMINANCE_ALPHA16I_EXT 0x8D8D
#endif
#ifndef GL_LUMINANCE_ALPHA16F_ARB
#define GL_LUMINANCE_ALPHA16F_ARB 0x881F
#endif
#ifndef GL_LUMINANCE_ALPHA32F_ARB
#define GL_LUMINANCE_ALPHA32F_ARB 0x8819
#endif
#ifndef GL_LUMINANCE_ALPHA32I_EXT
#define GL_LUMINANCE_ALPHA32I_EXT 0x8D87
#endif
#ifndef GL_LUMINANCE_ALPHA32UI_EXT
#define GL_LUMINANCE_ALPHA32UI_EXT 0x8D75
#endif
#ifndef GL_HALF_FLOAT_ARB
#define GL_HALF_FLOAT_ARB 0x140B
#endif

/* SNORM texture formats */
#ifndef GL_R16_SNORM
#define GL_R16_SNORM 0x8F98
#endif
#ifndef GL_RG16_SNORM
#define GL_RG16_SNORM 0x8F99
#endif
#ifndef GL_RGBA16_SNORM
#define GL_RGBA16_SNORM 0x8F9B
#endif

/* Additional logic ops */
#ifndef GL_AND
#define GL_AND 0x1501
#endif
#ifndef GL_COPY
#define GL_COPY 0x1503
#endif
#ifndef GL_EQUIV
#define GL_EQUIV 0x1509
#endif
#ifndef GL_NOOP
#define GL_NOOP 0x1505
#endif
#ifndef GL_OR
#define GL_OR 0x1507
#endif
#ifndef GL_OR_INVERTED
#define GL_OR_INVERTED 0x150D
#endif
#ifndef GL_OR_REVERSE
#define GL_OR_REVERSE 0x150B
#endif
#ifndef GL_SET
#define GL_SET 0x150F
#endif
#ifndef GL_COLOR_LOGIC_OP
#define GL_COLOR_LOGIC_OP 0x0BF2
#endif

/* Shading model */
#ifndef GL_FLAT
#define GL_FLAT 0x1D00
#endif
#ifndef GL_SMOOTH
#define GL_SMOOTH 0x1D01
#endif

/* Clip control */
#ifndef GL_LOWER_LEFT
#define GL_LOWER_LEFT 0x8CA1
#endif
#ifndef GL_UPPER_LEFT
#define GL_UPPER_LEFT 0x8CA2
#endif
#ifndef GL_NEGATIVE_ONE_TO_ONE
#define GL_NEGATIVE_ONE_TO_ONE 0x935E
#endif
#ifndef GL_ZERO_TO_ONE
#define GL_ZERO_TO_ONE 0x935F
#endif

/* Point sprite */
#ifndef GL_POINT_SPRITE_COORD_ORIGIN
#define GL_POINT_SPRITE_COORD_ORIGIN 0x8CA0
#endif

/* Polygon stipple */
#ifndef GL_POLYGON_STIPPLE
#define GL_POLYGON_STIPPLE 0x0B42
#endif

/* SNORM alpha/luminance */
#ifndef GL_ALPHA8_SNORM
#define GL_ALPHA8_SNORM 0x9014
#endif
#ifndef GL_ALPHA16_SNORM
#define GL_ALPHA16_SNORM 0x9018
#endif
#ifndef GL_LUMINANCE8_ALPHA8_SNORM
#define GL_LUMINANCE8_ALPHA8_SNORM 0x9016
#endif
#ifndef GL_LUMINANCE16_ALPHA16_SNORM
#define GL_LUMINANCE16_ALPHA16_SNORM 0x901A
#endif

/* ARB buffer objects */
#ifndef GL_ARRAY_BUFFER_ARB
#define GL_ARRAY_BUFFER_ARB 0x8892
#endif
#ifndef GL_ELEMENT_ARRAY_BUFFER_ARB
#define GL_ELEMENT_ARRAY_BUFFER_ARB 0x8893
#endif

/* BGRA integer format */
#ifndef GL_BGRA_INTEGER
#define GL_BGRA_INTEGER 0x8D9B
#endif

/* Color clamping */
#ifndef GL_CLAMP_FRAGMENT_COLOR_ARB
#define GL_CLAMP_FRAGMENT_COLOR_ARB 0x891B
#endif
#ifndef GL_CLAMP_VERTEX_COLOR_ARB
#define GL_CLAMP_VERTEX_COLOR_ARB 0x891A
#endif

/* Clip planes */
#ifndef GL_CLIP_PLANE0
#define GL_CLIP_PLANE0 0x3000
#endif

/* RGTC / BPTC compression */
#ifndef GL_COMPRESSED_RED_RGTC1
#define GL_COMPRESSED_RED_RGTC1 0x8DBB
#endif
#ifndef GL_COMPRESSED_SIGNED_RED_RGTC1
#define GL_COMPRESSED_SIGNED_RED_RGTC1 0x8DBC
#endif
#ifndef GL_COMPRESSED_RG_RGTC2
#define GL_COMPRESSED_RG_RGTC2 0x8DBD
#endif
#ifndef GL_COMPRESSED_SIGNED_RG_RGTC2
#define GL_COMPRESSED_SIGNED_RG_RGTC2 0x8DBE
#endif
#ifndef GL_COMPRESSED_RGBA_BPTC_UNORM
#define GL_COMPRESSED_RGBA_BPTC_UNORM 0x8E8C
#endif
#ifndef GL_COMPRESSED_SRGB_ALPHA_BPTC_UNORM
#define GL_COMPRESSED_SRGB_ALPHA_BPTC_UNORM 0x8E8D
#endif
#ifndef GL_COMPRESSED_RGB_BPTC_SIGNED_FLOAT
#define GL_COMPRESSED_RGB_BPTC_SIGNED_FLOAT 0x8E8E
#endif
#ifndef GL_COMPRESSED_RGB_BPTC_UNSIGNED_FLOAT
#define GL_COMPRESSED_RGB_BPTC_UNSIGNED_FLOAT 0x8E8F
#endif

/* Line stipple */
#ifndef GL_LINE_STIPPLE
#define GL_LINE_STIPPLE 0x0B24
#endif

/* Persistent/coherent mapping */
#ifndef GL_MAP_PERSISTENT_BIT
#define GL_MAP_PERSISTENT_BIT 0x0040
#endif
#ifndef GL_MAP_COHERENT_BIT
#define GL_MAP_COHERENT_BIT 0x0080
#endif

/* Multisample */
#ifndef GL_MULTISAMPLE
#define GL_MULTISAMPLE 0x809D
#endif

/* Query buffer */
#ifndef GL_QUERY_BUFFER
#define GL_QUERY_BUFFER 0x9192
#endif

/* Texture max anisotropy */
#ifndef GL_TEXTURE_MAX_ANISOTROPY
#define GL_TEXTURE_MAX_ANISOTROPY 0x84FE
#endif

/* Vertex program two side */
#ifndef GL_VERTEX_PROGRAM_TWO_SIDE
#define GL_VERTEX_PROGRAM_TWO_SIDE 0x8643
#endif

/* Color read clamp */
#ifndef GL_CLAMP_READ_COLOR_ARB
#define GL_CLAMP_READ_COLOR_ARB 0x891C
#endif

/* Pipeline statistics queries */
#ifndef GL_VERTICES_SUBMITTED_ARB
#define GL_VERTICES_SUBMITTED_ARB 0x82EE
#endif
#ifndef GL_PRIMITIVES_SUBMITTED_ARB
#define GL_PRIMITIVES_SUBMITTED_ARB 0x82EF
#endif
#ifndef GL_VERTEX_SHADER_INVOCATIONS_ARB
#define GL_VERTEX_SHADER_INVOCATIONS_ARB 0x82F0
#endif
#ifndef GL_GEOMETRY_SHADER_PRIMITIVES_EMITTED_ARB
#define GL_GEOMETRY_SHADER_PRIMITIVES_EMITTED_ARB 0x82F3
#endif
#ifndef GL_CLIPPING_INPUT_PRIMITIVES_ARB
#define GL_CLIPPING_INPUT_PRIMITIVES_ARB 0x82F6
#endif

/* Depth scale */
#ifndef GL_DEPTH_SCALE
#define GL_DEPTH_SCALE 0x0D1E
#endif

/* Pack invert (Mesa extension) */
#ifndef GL_PACK_INVERT_MESA
#define GL_PACK_INVERT_MESA 0x8758
#endif

/* Tessellation defaults */
#ifndef GL_PATCH_DEFAULT_INNER_LEVEL
#define GL_PATCH_DEFAULT_INNER_LEVEL 0x8E73
#endif
#ifndef GL_PATCH_DEFAULT_OUTER_LEVEL
#define GL_PATCH_DEFAULT_OUTER_LEVEL 0x8E74
#endif

/* Pixel buffer object */
#ifndef GL_PIXEL_PACK_BUFFER_ARB
#define GL_PIXEL_PACK_BUFFER_ARB 0x88EB
#endif

/* Query object results */
#ifndef GL_QUERY_RESULT_ARB
#define GL_QUERY_RESULT_ARB 0x8866
#endif
#ifndef GL_QUERY_RESULT_AVAILABLE_ARB
#define GL_QUERY_RESULT_AVAILABLE_ARB 0x8867
#endif

/* Scaled resolve */
#ifndef GL_SCALED_RESOLVE_NICEST_EXT
#define GL_SCALED_RESOLVE_NICEST_EXT 0x90BB
#endif

/* Pipeline statistics queries (continued) */
#ifndef GL_CLIPPING_OUTPUT_PRIMITIVES_ARB
#define GL_CLIPPING_OUTPUT_PRIMITIVES_ARB 0x82F7
#endif
#ifndef GL_FRAGMENT_SHADER_INVOCATIONS_ARB
#define GL_FRAGMENT_SHADER_INVOCATIONS_ARB 0x82F4
#endif
#ifndef GL_TESS_CONTROL_SHADER_PATCHES_ARB
#define GL_TESS_CONTROL_SHADER_PATCHES_ARB 0x82F1
#endif
#ifndef GL_TESS_EVALUATION_SHADER_INVOCATIONS_ARB
#define GL_TESS_EVALUATION_SHADER_INVOCATIONS_ARB 0x82F2
#endif
#ifndef GL_COMPUTE_SHADER_INVOCATIONS_ARB
#define GL_COMPUTE_SHADER_INVOCATIONS_ARB 0x82F5
#endif

/* Occlusion query */
#ifndef GL_SAMPLES_PASSED_ARB
#define GL_SAMPLES_PASSED_ARB 0x8914
#endif

/* Transform feedback overflow */
#ifndef GL_TRANSFORM_FEEDBACK_OVERFLOW_ARB
#define GL_TRANSFORM_FEEDBACK_OVERFLOW_ARB 0x82EC
#endif
#ifndef GL_TRANSFORM_FEEDBACK_STREAM_OVERFLOW_ARB
#define GL_TRANSFORM_FEEDBACK_STREAM_OVERFLOW_ARB 0x82ED
#endif
#ifndef GL_MAX_TRANSFORM_FEEDBACK_BUFFERS
#define GL_MAX_TRANSFORM_FEEDBACK_BUFFERS 0x8E70
#endif

/* Conditional render / query modes */
#ifndef GL_QUERY_WAIT
#define GL_QUERY_WAIT 0x8E13
#endif
#ifndef GL_QUERY_WAIT_INVERTED
#define GL_QUERY_WAIT_INVERTED 0x8E17
#endif
#ifndef GL_QUERY_NO_WAIT
#define GL_QUERY_NO_WAIT 0x8E14
#endif
#ifndef GL_QUERY_NO_WAIT_INVERTED
#define GL_QUERY_NO_WAIT_INVERTED 0x8E18
#endif
#ifndef GL_QUERY_BY_REGION_WAIT
#define GL_QUERY_BY_REGION_WAIT 0x8E15
#endif
#ifndef GL_QUERY_BY_REGION_WAIT_INVERTED
#define GL_QUERY_BY_REGION_WAIT_INVERTED 0x8E19
#endif
#ifndef GL_QUERY_BY_REGION_NO_WAIT
#define GL_QUERY_BY_REGION_NO_WAIT 0x8E16
#endif
#ifndef GL_QUERY_BY_REGION_NO_WAIT_INVERTED
#define GL_QUERY_BY_REGION_NO_WAIT_INVERTED 0x8E1A
#endif

/* Query result no wait */
#ifndef GL_QUERY_RESULT_NO_WAIT
#define GL_QUERY_RESULT_NO_WAIT 0x9194
#endif

/* Dual source draw buffers */
#ifndef GL_MAX_DUAL_SOURCE_DRAW_BUFFERS
#define GL_MAX_DUAL_SOURCE_DRAW_BUFFERS 0x88FC
#endif

/* Texture gather components */
#ifndef GL_MAX_PROGRAM_TEXTURE_GATHER_COMPONENTS_ARB
#define GL_MAX_PROGRAM_TEXTURE_GATHER_COMPONENTS_ARB 0x8F9F
#endif

/* Viewports */
#ifndef GL_MAX_VIEWPORTS
#define GL_MAX_VIEWPORTS 0x825B
#endif

/* Smooth ranges */
#ifndef GL_SMOOTH_POINT_SIZE_RANGE
#define GL_SMOOTH_POINT_SIZE_RANGE 0x0B12
#endif
#ifndef GL_SMOOTH_LINE_WIDTH_RANGE
#define GL_SMOOTH_LINE_WIDTH_RANGE 0x0B22
#endif

/* Image samples */
#ifndef GL_MAX_IMAGE_SAMPLES
#define GL_MAX_IMAGE_SAMPLES 0x906D
#endif

/* Max texture anisotropy */
#ifndef GL_MAX_TEXTURE_MAX_ANISOTROPY
#define GL_MAX_TEXTURE_MAX_ANISOTROPY 0x84FF
#endif

/* NVX GPU memory info */
#ifndef GL_GPU_MEMORY_INFO_DEDICATED_VIDMEM_NVX
#define GL_GPU_MEMORY_INFO_DEDICATED_VIDMEM_NVX 0x9047
#endif
#ifndef GL_GPU_MEMORY_INFO_TOTAL_AVAILABLE_MEMORY_NVX
#define GL_GPU_MEMORY_INFO_TOTAL_AVAILABLE_MEMORY_NVX 0x9048
#endif
#ifndef GL_GPU_MEMORY_INFO_EVICTION_COUNT_NVX
#define GL_GPU_MEMORY_INFO_EVICTION_COUNT_NVX 0x904A
#endif
#ifndef GL_GPU_MEMORY_INFO_EVICTED_MEMORY_NVX
#define GL_GPU_MEMORY_INFO_EVICTED_MEMORY_NVX 0x904B
#endif

/* ATI memory info */
#ifndef GL_VBO_FREE_MEMORY_ATI
#define GL_VBO_FREE_MEMORY_ATI 0x87FB
#endif

/* Additional desktop GL defines used by virglrenderer */
#ifndef GL_TEXTURE_WRAP_R
#define GL_TEXTURE_WRAP_R 0x8072
#endif
#ifndef GL_TEXTURE_LOD_BIAS
#define GL_TEXTURE_LOD_BIAS 0x8501
#endif
#ifndef GL_DEPTH_CLAMP
#define GL_DEPTH_CLAMP 0x864F
#endif
#ifndef GL_SAMPLE_ALPHA_TO_ONE
#define GL_SAMPLE_ALPHA_TO_ONE 0x809F
#endif
#ifndef GL_POLYGON_OFFSET_LINE
#define GL_POLYGON_OFFSET_LINE 0x2A02
#endif
#ifndef GL_POLYGON_OFFSET_POINT
#define GL_POLYGON_OFFSET_POINT 0x2A01
#endif
#ifndef GL_POLYGON_SMOOTH
#define GL_POLYGON_SMOOTH 0x0B41
#endif
#ifndef GL_LINE_SMOOTH
#define GL_LINE_SMOOTH 0x0B20
#endif

/* Epoxy API function shims */
static inline int epoxy_gl_version(void)
{
    const char *version = (const char *)glGetString(GL_VERSION);
    int major = 0, minor = 0;
    if (!version) return 0;
    /* Skip "OpenGL ES " prefix */
    while (*version && !isdigit((unsigned char)*version))
        version++;
    sscanf(version, "%d.%d", &major, &minor);
    return major * 10 + minor;
}

static inline bool epoxy_is_desktop_gl(void)
{
    /* On Android, we're always GLES */
    return false;
}

static inline bool epoxy_has_gl_extension(const char *ext)
{
    int num_extensions = 0;
    glGetIntegerv(GL_NUM_EXTENSIONS, &num_extensions);
    for (int i = 0; i < num_extensions; i++) {
        const char *gl_ext = (const char *)glGetStringi(GL_EXTENSIONS, i);
        if (gl_ext && strcmp(ext, gl_ext) == 0)
            return true;
    }
    return false;
}

/* GLSL version helper */
static inline int epoxy_glsl_version(void)
{
    const char *version_str = (const char *)glGetString(GL_SHADING_LANGUAGE_VERSION);
    int major = 0, minor = 0;
    if (!version_str) return 0;
    while (*version_str && !isdigit((unsigned char)*version_str))
        version_str++;
    sscanf(version_str, "%d.%d", &major, &minor);
    return major * 100 + minor;
}

#endif /* EPOXY_GL_H_SHIM */

