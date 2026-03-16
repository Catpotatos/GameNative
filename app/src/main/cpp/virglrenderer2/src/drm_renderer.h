/*
 * Stub drm_renderer.h for Android
 * DRM renderer is not used on Android.
 */
#ifndef DRM_RENDERER_H
#define DRM_RENDERER_H

#include <stdint.h>
#include <stddef.h>

#ifndef DRM_FORMAT_MOD_INVALID
#define DRM_FORMAT_MOD_INVALID ((1ULL << 56) - 1)
#endif

struct virgl_context;

static inline int drm_renderer_init(int fd)
{
    (void)fd;
    return -1;
}

static inline void drm_renderer_fini(void)
{
}

static inline void drm_renderer_reset(void)
{
}

static inline size_t drm_renderer_capset(void *caps)
{
    (void)caps;
    return 0;
}

static inline struct virgl_context *drm_renderer_create(uint32_t nlen, const char *name, int fd)
{
    (void)nlen; (void)name; (void)fd;
    return NULL;
}

#endif

