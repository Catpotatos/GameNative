/*
 * Stub vkr_allocator.h for Android
 * VKR (Vulkan renderer) allocator is not used on Android.
 */
#ifndef VKR_ALLOCATOR_H
#define VKR_ALLOCATOR_H

#include <stdint.h>
#include <stddef.h>

struct virgl_resource;

static inline int vkr_allocator_resource_map(struct virgl_resource *res, void **map, size_t *map_size)
{
    (void)res; (void)map; (void)map_size;
    return -1;
}

static inline int vkr_allocator_resource_unmap(struct virgl_resource *res)
{
    (void)res;
    return -1;
}

static inline void vkr_allocator_fini(void)
{
}

#endif

