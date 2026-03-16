/*
 * Stub proxy_renderer.h for Android
 * Proxy renderer (for Venus/Vulkan) is not used on Android.
 */
#ifndef PROXY_RENDERER_H
#define PROXY_RENDERER_H

#include <stdint.h>
#include <stddef.h>

struct virgl_context;

struct proxy_renderer_cbs {
    int (*get_server_fd)(uint32_t version);
};

static inline int proxy_renderer_init(const struct proxy_renderer_cbs *cbs, uint32_t flags)
{
    (void)cbs; (void)flags;
    return -1;
}

static inline void proxy_renderer_fini(void)
{
}

static inline void proxy_renderer_reset(void)
{
}

static inline size_t proxy_get_capset(uint32_t set, void *caps)
{
    (void)set; (void)caps;
    return 0;
}

static inline struct virgl_context *proxy_context_create(uint32_t ctx_id, uint32_t ctx_flags, uint32_t nlen, const char *name)
{
    (void)ctx_id; (void)ctx_flags; (void)nlen; (void)name;
    return NULL;
}

#endif

