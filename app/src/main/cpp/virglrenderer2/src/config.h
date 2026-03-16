/*
 * config.h - Android-specific build configuration for virglrenderer 1.3.0
 * Adapted from config.h.meson for Android NDK cross-compilation.
 */
#ifndef CONFIG_H
#define CONFIG_H

#define VERSION "1.3.0"
#define _GNU_SOURCE 1
#define VIRGL_RENDERER_UNSTABLE_APIS 1

/* Compiler builtins - GCC/Clang on Android supports all of these */
#define HAVE___BUILTIN_BSWAP32 1
#define HAVE___BUILTIN_BSWAP64 1
#define HAVE___BUILTIN_CLZ 1
#define HAVE___BUILTIN_CLZLL 1
#define HAVE___BUILTIN_EXPECT 1
#define HAVE___BUILTIN_FFS 1
#define HAVE___BUILTIN_FFSLL 1
#define HAVE___BUILTIN_POPCOUNT 1
#define HAVE___BUILTIN_POPCOUNTLL 1
#define HAVE___BUILTIN_TYPES_COMPATIBLE_P 1
#define HAVE___BUILTIN_UNREACHABLE 1

/* Function attributes */
#define HAVE_FUNC_ATTRIBUTE_CONST 1
#define HAVE_FUNC_ATTRIBUTE_FLATTEN 1
#define HAVE_FUNC_ATTRIBUTE_FORMAT 1
#define HAVE_FUNC_ATTRIBUTE_MALLOC 1
#define HAVE_FUNC_ATTRIBUTE_NORETURN 1
#define HAVE_FUNC_ATTRIBUTE_PACKED 1
#define HAVE_FUNC_ATTRIBUTE_PURE 1
#define HAVE_FUNC_ATTRIBUTE_RETURNS_NONNULL 1
#define HAVE_FUNC_ATTRIBUTE_UNUSED 1
#define HAVE_FUNC_ATTRIBUTE_WARN_UNUSED_RESULT 1
#define HAVE_FUNC_ATTRIBUTE_WEAK 1

/* Android system features */
#define HAVE_MEMFD_CREATE 1
#define HAVE_STRTOK_R 1
#define HAVE_TIMESPEC_GET 0
#define HAVE_SYS_UIO_H 1
#define HAVE_PTHREAD 1
#define HAVE_PTHREAD_SETAFFINITY 0
/* #undef HAVE_PTHREAD_NP_H */
#define HAVE_EVENTFD_H 1
#define HAVE_DLFCN_H 1

/* EGL support (yes on Android) */
#define HAVE_EPOXY_EGL_H 1
/* No GLX on Android */
/* #undef HAVE_EPOXY_GLX_H */

/* Disable features not available/needed on Android */
/* #undef CHECK_GL_ERRORS */
/* #undef ENABLE_GBM_ALLOCATION */
/* #undef ENABLE_VENUS */
/* #undef ENABLE_VULKAN_DLOAD */
/* #undef ENABLE_VULKAN_PRELOAD */
/* #undef ENABLE_GBM */
/* #undef ENABLE_DRM */
/* #undef ENABLE_DRM_MSM */
/* #undef ENABLE_DRM_AMDGPU */
/* #undef ENABLE_DRM_ASAHI */
/* #undef ENABLE_DRM_PANFROST */
/* #undef ENABLE_DRM_I915 */
/* #undef ENABLE_LIBDRM */
/* #undef ENABLE_RENDER_SERVER */
/* #undef ENABLE_RENDER_SERVER_WORKER_PROCESS */
/* #undef ENABLE_RENDER_SERVER_WORKER_THREAD */
/* #undef ENABLE_RENDER_SERVER_WORKER_MINIJAIL */
/* #undef HAVE_DMABUF_H */
/* #undef HAVE_LINUX_UDMABUF_H */
/* #undef ENABLE_VIDEO */
/* #undef ENABLE_TRACING */
/* #undef ENABLE_TESTS */

/* Endianness - ARM is little endian */
#define UTIL_ARCH_LITTLE_ENDIAN 1
#define UTIL_ARCH_BIG_ENDIAN 0

/* Architecture detection */
#if defined(__aarch64__)
#define PIPE_ARCH_AARCH64 1
#elif defined(__arm__)
#define PIPE_ARCH_ARM 1
#elif defined(__x86_64__)
#define PIPE_ARCH_X86_64 1
#elif defined(__i386__)
#define PIPE_ARCH_X86 1
#endif

#endif /* CONFIG_H */

