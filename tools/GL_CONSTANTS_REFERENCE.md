# VirGL Update: GL Constants Reference

## Current GLES Compatibility Stubs (in vrend_util.h)

These constants exist in desktop OpenGL but NOT in OpenGL ES.
Bruno defined them as stubs so the code compiles against GLES headers.

```c
#define GL_TEXTURE_1D        0x0DE0
#define GL_TEXTURE_RECTANGLE 0x84F5
#define GL_TEXTURE_1D_ARRAY  0x8C18
#define GL_QUADS             0x0007
#define GL_QUAD_STRIP        0x0008
#define GL_POLYGON           0x0009
```

## Potentially Needed Additional Constants

When updating to a newer Mesa version, the new code may reference additional
desktop GL constants not available in GLES. Check for compilation errors like:
"undeclared identifier 'GL_XXX'"

Common desktop-only constants that may appear in newer virgl code:

```c
// Texture targets
#define GL_TEXTURE_BUFFER              0x8C2A  // May already be in GLES3.2
#define GL_TEXTURE_CUBE_MAP_ARRAY      0x9009

// Primitive types
#define GL_LINES_ADJACENCY             0x000A
#define GL_LINE_STRIP_ADJACENCY        0x000B
#define GL_TRIANGLES_ADJACENCY         0x000C
#define GL_TRIANGLE_STRIP_ADJACENCY    0x000D
#define GL_PATCHES                     0x000E

// Shader types
#define GL_GEOMETRY_SHADER             0x8DD9
#define GL_TESS_CONTROL_SHADER         0x8E88
#define GL_TESS_EVALUATION_SHADER      0x8E87

// Clip control
#define GL_LOWER_LEFT                  0x8CA1
#define GL_UPPER_LEFT                  0x8CA2
#define GL_NEGATIVE_ONE_TO_ONE         0x935E
#define GL_ZERO_TO_ONE                 0x935F

// Provoking vertex
#define GL_FIRST_VERTEX_CONVENTION     0x8E4D
#define GL_LAST_VERTEX_CONVENTION      0x8E4E
#define GL_PROVOKING_VERTEX            0x8E4F

// Polygon modes (not in GLES)
#define GL_POINT                       0x1B00
#define GL_LINE                        0x1B01
#define GL_FILL                        0x1B02

// Stencil export
#define GL_STENCIL_INDEX               0x1901

// Logic ops (may not be fully supported on GLES)
#define GL_LOGIC_OP_MODE               0x0BF0

// Misc
#define GL_SAMPLE_ALPHA_TO_ONE         0x809F
#define GL_PROGRAM_POINT_SIZE          0x8642
#define GL_POINT_SPRITE                0x8861
```

## How to Find Missing Constants

When you get compilation errors after updating, search for the constant value:
1. Look it up at https://registry.khronos.org/OpenGL/api/GL/glext.h
2. Add the `#define` to `vrend_util.h`
3. Note: The constant being defined doesn't mean the feature works on GLES!
   The renderer code should have fallback paths that skip unsupported features.

## GLES Extensions That Provide Desktop-Like Features

Some "desktop-only" features are available via GLES extensions:
- `GL_EXT_geometry_shader` → geometry shaders on GLES
- `GL_EXT_tessellation_shader` → tessellation on GLES
- `GL_EXT_texture_buffer` → texture buffers on GLES
- `GL_EXT_texture_cube_map_array` → cube map arrays on GLES
- `GL_OES_primitive_bounding_box` → primitive bounding box
- `GL_EXT_clip_cull_distance` → clip/cull distances on GLES

These are typically checked at runtime in `vrend_renderer.c`'s feature detection code.

