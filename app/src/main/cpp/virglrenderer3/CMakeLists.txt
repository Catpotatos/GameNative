cmake_minimum_required(VERSION 3.22.1)

project(VirGLRenderer3)

set(CMAKE_C_FLAGS "${CMAKE_C_FLAGS} -O2 -Wno-unused-function -Wno-implicit-function-declaration -Wno-missing-prototypes -Wno-sign-compare -Wno-unused-variable -Wno-incompatible-pointer-types -Wno-int-to-pointer-cast -Wno-pointer-to-int-cast -Wno-format -Wno-pedantic -Wno-return-mismatch -Wno-unused-but-set-variable -DHAVE_CONFIG_H -DUTIL_ARCH_LITTLE_ENDIAN=1 -DUTIL_ARCH_BIG_ENDIAN=0 -DHAVE_PTHREAD=1 -D_GNU_SOURCE=1")

include_directories(
    src
    src/vrend
    src/mesa
    src/mesa/util
    src/mesa/util/format
    src/mesa/compat
    src/mesa/pipe
    src/mesa/compiler
    src/gallium/include
    src/gallium/include/pipe
    src/gallium/auxiliary
    src/gallium/auxiliary/util
    src/epoxy
    server
)

add_library(virglrenderer3 SHARED
            # virglrenderer 1.3.0 public API
            src/virglrenderer.c
            src/virgl_context.c
            src/virgl_resource.c
            src/virgl_fence.c
            src/virgl_util.c

            # vrend core (renderer)
            src/vrend/iov.c
            src/vrend/vrend_blitter.c
            src/vrend/vrend_debug.c
            src/vrend/vrend_decode.c
            src/vrend/vrend_formats.c
            src/vrend/vrend_object.c
            src/vrend/vrend_renderer.c
            src/vrend/vrend_shader.c
            src/vrend/vrend_tweaks.c
            src/vrend/vrend_winsys.c
            src/vrend/vrend_winsys_egl.c

            # Server JNI bridge
            server/virgl_server3.c
            server/virgl_server3_renderer.c
            server/virgl_server_shm.c

            # Gallium auxiliary utilities (Mesa 24.3.4)
            src/gallium/auxiliary/util/u_format.c
            src/gallium/auxiliary/util/u_format_table.c
            src/gallium/auxiliary/util/u_texture.c
            src/gallium/auxiliary/util/u_hash_table.c
            src/gallium/auxiliary/util/u_debug_describe.c
            src/gallium/auxiliary/cso_cache/cso_cache.c
            src/gallium/auxiliary/cso_cache/cso_hash.c
            src/gallium/auxiliary/tgsi/tgsi_dump.c
            src/gallium/auxiliary/tgsi/tgsi_build.c
            src/gallium/auxiliary/tgsi/tgsi_scan.c
            src/gallium/auxiliary/tgsi/tgsi_info.c
            src/gallium/auxiliary/tgsi/tgsi_parse.c
            src/gallium/auxiliary/tgsi/tgsi_text.c
            src/gallium/auxiliary/tgsi/tgsi_strings.c
            src/gallium/auxiliary/tgsi/tgsi_sanity.c
            src/gallium/auxiliary/tgsi/tgsi_iterate.c
            src/gallium/auxiliary/tgsi/tgsi_util.c

            # Mesa utilities
            src/mesa/util/hash_table.c
            src/mesa/util/ralloc.c
            src/mesa/util/u_debug.c
            src/mesa/util/u_math.c
            src/mesa/util/u_cpu_detect.c
            src/mesa/util/os_misc.c
            src/mesa/util/bitscan.c
            src/mesa/util/os_file.c
            src/mesa/util/anon_file.c

            # GL stubs for desktop functions not in GLES
            src/gl_stubs.c
)

target_link_libraries(virglrenderer3
                      log
                      android
                      EGL
                      GLESv2
                      GLESv3)
