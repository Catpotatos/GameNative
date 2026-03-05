#include <jni.h>
#include <vulkan/vulkan.h>
#include <stdlib.h>
#include <dlfcn.h>
#include <string.h>
#include <android/api-level.h>

static jobjectArray make_empty_array(JNIEnv *env)
{
    jclass stringCls = (*env)->FindClass(env, "java/lang/String");
    if (!stringCls) return NULL;
    return (*env)->NewObjectArray(env, 0, stringCls, NULL);
}

static jobjectArray vkGetDeviceExtensions_dynamic(JNIEnv *env)
{
    void *libvulkan = dlopen("libvulkan.so", RTLD_NOW | RTLD_LOCAL);
    if (!libvulkan) return make_empty_array(env);

    PFN_vkCreateInstance           pfn_vkCreateInstance           = (PFN_vkCreateInstance)           dlsym(libvulkan, "vkCreateInstance");
    PFN_vkEnumeratePhysicalDevices pfn_vkEnumeratePhysicalDevices = (PFN_vkEnumeratePhysicalDevices) dlsym(libvulkan, "vkEnumeratePhysicalDevices");
    PFN_vkEnumerateDeviceExtensionProperties pfn_vkEnumerateDeviceExtensionProperties =
            (PFN_vkEnumerateDeviceExtensionProperties) dlsym(libvulkan, "vkEnumerateDeviceExtensionProperties");
    PFN_vkDestroyInstance          pfn_vkDestroyInstance          = (PFN_vkDestroyInstance)          dlsym(libvulkan, "vkDestroyInstance");

    if (!pfn_vkCreateInstance || !pfn_vkEnumeratePhysicalDevices ||
        !pfn_vkEnumerateDeviceExtensionProperties || !pfn_vkDestroyInstance) {
        dlclose(libvulkan);
        return make_empty_array(env);
    }

    VkApplicationInfo appInfo = {
        .sType              = VK_STRUCTURE_TYPE_APPLICATION_INFO,
        .pApplicationName   = "GPUHelper",
        .applicationVersion = VK_MAKE_VERSION(1, 0, 0),
        .pEngineName        = "No Engine",
        .engineVersion      = VK_MAKE_VERSION(1, 0, 0),
        .apiVersion         = VK_API_VERSION_1_0,
    };

    VkInstanceCreateInfo ci = {
        .sType            = VK_STRUCTURE_TYPE_INSTANCE_CREATE_INFO,
        .pApplicationInfo = &appInfo,
    };

    VkInstance instance;
    VkResult   res = pfn_vkCreateInstance(&ci, NULL, &instance);
    if (res != VK_SUCCESS) {
        dlclose(libvulkan);
        return make_empty_array(env);
    }

    uint32_t pdCount = 0;
    res = pfn_vkEnumeratePhysicalDevices(instance, &pdCount, NULL);
    if (res != VK_SUCCESS || pdCount == 0) {
        pfn_vkDestroyInstance(instance, NULL);
        dlclose(libvulkan);
        return make_empty_array(env);
    }

    pdCount = 1;
    VkPhysicalDevice pd;
    res = pfn_vkEnumeratePhysicalDevices(instance, &pdCount, &pd);
    if (!(res == VK_SUCCESS || res == VK_INCOMPLETE)) {
        pfn_vkDestroyInstance(instance, NULL);
        dlclose(libvulkan);
        return make_empty_array(env);
    }

    uint32_t extCount = 0;
    res = pfn_vkEnumerateDeviceExtensionProperties(pd, NULL, &extCount, NULL);
    if (res != VK_SUCCESS || extCount == 0) {
        pfn_vkDestroyInstance(instance, NULL);
        dlclose(libvulkan);
        return make_empty_array(env);
    }

    VkExtensionProperties *ext = calloc(extCount, sizeof(VkExtensionProperties));
    if (!ext) {
        pfn_vkDestroyInstance(instance, NULL);
        dlclose(libvulkan);
        return make_empty_array(env);
    }

    res = pfn_vkEnumerateDeviceExtensionProperties(pd, NULL, &extCount, ext);
    if (res != VK_SUCCESS) {
        free(ext);
        pfn_vkDestroyInstance(instance, NULL);
        dlclose(libvulkan);
        return make_empty_array(env);
    }

    jclass stringCls = (*env)->FindClass(env, "java/lang/String");
    if (!stringCls) {
        free(ext);
        pfn_vkDestroyInstance(instance, NULL);
        dlclose(libvulkan);
        return NULL;
    }
    jobjectArray arr = (*env)->NewObjectArray(env, (jsize)extCount, stringCls, NULL);
    if (!arr) {
        free(ext);
        pfn_vkDestroyInstance(instance, NULL);
        dlclose(libvulkan);
        return NULL;
    }

    for (jsize i = 0; i < (jsize)extCount; ++i)
    {
        jstring js = (*env)->NewStringUTF(env, ext[i].extensionName);
        if (js) {
            (*env)->SetObjectArrayElement(env, arr, i, js);
            (*env)->DeleteLocalRef(env, js);
        }
    }
    free(ext);
    pfn_vkDestroyInstance(instance, NULL);
    dlclose(libvulkan);
    return arr;
}

static jobjectArray vkGetDeviceExtensions_static(JNIEnv *env)
{
    VkInstance instance;
    VkResult   res;

    VkInstanceCreateInfo ci = { .sType = VK_STRUCTURE_TYPE_INSTANCE_CREATE_INFO };
    res = vkCreateInstance(&ci, NULL, &instance);
    if (res != VK_SUCCESS) goto make_empty_array;

    uint32_t pdCount = 0;
    res = vkEnumeratePhysicalDevices(instance, &pdCount, NULL);
    if (res != VK_SUCCESS || pdCount == 0) goto make_empty_array;

    pdCount = 1;                        /* original code asks for only one */
    VkPhysicalDevice pd;
    res = vkEnumeratePhysicalDevices(instance, &pdCount, &pd);
    if (!(res == VK_SUCCESS || res == VK_INCOMPLETE)) goto make_empty_array;

    uint32_t extCount = 0;
    res = vkEnumerateDeviceExtensionProperties(pd, NULL, &extCount, NULL);
    if (res != VK_SUCCESS || extCount == 0) goto make_empty_array;

    VkExtensionProperties *ext =
            calloc(extCount, sizeof(VkExtensionProperties));
    res = vkEnumerateDeviceExtensionProperties(pd, NULL, &extCount, ext);
    if (res != VK_SUCCESS) { free(ext); goto make_empty_array; }

    jclass stringCls = (*env)->FindClass(env, "java/lang/String");
    jobjectArray arr =
            (*env)->NewObjectArray(env, (jsize)extCount, stringCls, NULL);

    for (jsize i = 0; i < (jsize)extCount; ++i)
    {
        jstring js = (*env)->NewStringUTF(env, ext[i].extensionName);
        (*env)->SetObjectArrayElement(env, arr, i, js);
    }
    free(ext);
    return arr;

    make_empty_array:
    {
        jclass stringCls = (*env)->FindClass(env, "java/lang/String");
        jobjectArray empty = (*env)->NewObjectArray(env, 0, stringCls, NULL);
        return empty;
    }
}

JNIEXPORT jobjectArray JNICALL
Java_com_winlator_core_GPUHelper_vkGetDeviceExtensions(JNIEnv *env, jclass clazz)
{
    (void)clazz;
    if (android_get_device_api_level() <= 29) {
        return vkGetDeviceExtensions_dynamic(env);
    }
    return vkGetDeviceExtensions_static(env);
}
