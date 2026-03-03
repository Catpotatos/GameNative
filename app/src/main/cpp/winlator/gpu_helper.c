#include <jni.h>
#include <vulkan/vulkan.h>
#include <stdlib.h>
#include <dlfcn.h>
#include <string.h>

static jobjectArray make_empty_array(JNIEnv *env) {
    jclass stringCls = (*env)->FindClass(env, "java/lang/String");
    if (!stringCls) return NULL;
    return (*env)->NewObjectArray(env, 0, stringCls, NULL);
}

JNIEXPORT jobjectArray JNICALL
Java_com_winlator_core_GPUHelper_vkGetDeviceExtensions(JNIEnv *env, jclass clazz)
{
    (void)clazz;

    void *vulkan_handle = NULL;
    PFN_vkGetInstanceProcAddr gip = NULL;
    PFN_vkCreateInstance createInstance = NULL;
    PFN_vkEnumeratePhysicalDevices enumeratePhysicalDevices = NULL;
    PFN_vkEnumerateDeviceExtensionProperties enumerateDeviceExtensionProperties = NULL;
    PFN_vkDestroyInstance destroyInstance = NULL;
    VkInstance instance = VK_NULL_HANDLE;
    VkResult res;

    // Dynamic load for better compatibility on older Android stacks
    vulkan_handle = dlopen("libvulkan.so", RTLD_LOCAL | RTLD_NOW);
    if (!vulkan_handle) goto fail;

    gip = (PFN_vkGetInstanceProcAddr)dlsym(vulkan_handle, "vkGetInstanceProcAddr");
    createInstance = (PFN_vkCreateInstance)dlsym(vulkan_handle, "vkCreateInstance");
    if (!gip || !createInstance) goto fail;

    VkApplicationInfo app_info;
    memset(&app_info, 0, sizeof(app_info));
    app_info.sType = VK_STRUCTURE_TYPE_APPLICATION_INFO;
    app_info.pApplicationName = "GameNative";
    app_info.applicationVersion = VK_MAKE_VERSION(1, 0, 0);
    app_info.pEngineName = "GameNative";
    app_info.engineVersion = VK_MAKE_VERSION(1, 0, 0);
    app_info.apiVersion = VK_API_VERSION_1_0;

    VkInstanceCreateInfo ci;
    memset(&ci, 0, sizeof(ci));
    ci.sType = VK_STRUCTURE_TYPE_INSTANCE_CREATE_INFO;
    ci.pApplicationInfo = &app_info;

    res = createInstance(&ci, NULL, &instance);
    if (res != VK_SUCCESS || instance == VK_NULL_HANDLE) goto fail;

    enumeratePhysicalDevices =
            (PFN_vkEnumeratePhysicalDevices)gip(instance, "vkEnumeratePhysicalDevices");
    enumerateDeviceExtensionProperties =
            (PFN_vkEnumerateDeviceExtensionProperties)gip(instance, "vkEnumerateDeviceExtensionProperties");
    destroyInstance =
            (PFN_vkDestroyInstance)gip(instance, "vkDestroyInstance");
    if (!enumeratePhysicalDevices || !enumerateDeviceExtensionProperties || !destroyInstance) goto fail;

    uint32_t pdCount = 0;
    res = enumeratePhysicalDevices(instance, &pdCount, NULL);
    if (res != VK_SUCCESS || pdCount == 0) goto fail;

    VkPhysicalDevice *pds = (VkPhysicalDevice *)calloc(pdCount, sizeof(VkPhysicalDevice));
    if (!pds) goto fail;

    res = enumeratePhysicalDevices(instance, &pdCount, pds);
    if (!(res == VK_SUCCESS || res == VK_INCOMPLETE)) {
        free(pds);
        goto fail;
    }

    // Use first physical device (typical Android case)
    VkPhysicalDevice pd = pds[0];
    free(pds);

    uint32_t extCount = 0;
    res = enumerateDeviceExtensionProperties(pd, NULL, &extCount, NULL);
    if (res != VK_SUCCESS || extCount == 0) goto fail;

    VkExtensionProperties *ext = (VkExtensionProperties *)calloc(extCount, sizeof(VkExtensionProperties));
    if (!ext) goto fail;

    res = enumerateDeviceExtensionProperties(pd, NULL, &extCount, ext);
    if (res != VK_SUCCESS) {
        free(ext);
        goto fail;
    }

    jclass stringCls = (*env)->FindClass(env, "java/lang/String");
    if (!stringCls) {
        free(ext);
        goto fail;
    }

    jobjectArray arr = (*env)->NewObjectArray(env, (jsize)extCount, stringCls, NULL);
    if (!arr) {
        free(ext);
        goto fail;
    }

    for (jsize i = 0; i < (jsize)extCount; ++i) {
        jstring js = (*env)->NewStringUTF(env, ext[i].extensionName);
        if (!js) continue;
        (*env)->SetObjectArrayElement(env, arr, i, js);
        (*env)->DeleteLocalRef(env, js);
    }

    free(ext);
    destroyInstance(instance, NULL);
    dlclose(vulkan_handle);
    return arr;

    fail:
    if (destroyInstance && instance != VK_NULL_HANDLE) {
        destroyInstance(instance, NULL);
    }
    if (vulkan_handle) {
        dlclose(vulkan_handle);
    }
    return make_empty_array(env);
}
