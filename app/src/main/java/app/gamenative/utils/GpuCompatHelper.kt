package app.gamenative.utils

import android.content.Context
import android.os.Build
import com.winlator.core.GPUHelper
import com.winlator.core.GPUInformation

object GpuCompatHelper {
    data class VulkanExtensionsResult(
        val extensions: List<String>,
        val usedFallback: Boolean,
    )
    //API 29 Compatibility Fix
    /**
     * Resolves the list of Vulkan device extensions to use on this device.
     *
     * Applies a compatibility workaround for some Mali GPUs on Android 10 (API 29), where
     * probing Vulkan extensions in native code can crash. In such cases, a safe Vulkan 1.0
     * fallback extension list is returned instead.
     *
     * @param context Android [Context] used to query GPU / renderer information.
     * @param inspectionMode If `true`, skips native probing and returns a small, predictable
     * extension set intended for inspection tools, without applying compatibility probing.
     * @return [VulkanExtensionsResult] containing the resolved extension list and a flag
     * indicating whether a fallback list had to be used.
     */
    fun resolveVulkanExtensions(context: Context, inspectionMode: Boolean): VulkanExtensionsResult {
        if (inspectionMode) {
            return VulkanExtensionsResult(
                extensions = listOf(
                    "VK_KHR_swapchain",
                    "VK_KHR_maintenance1",
                    "VK_KHR_timeline_semaphore",
                ),
                usedFallback = false,
            )
        }

        val isApi29Mali = Build.VERSION.SDK_INT == 29 &&
            GPUInformation.getRenderer(context).contains("mali", ignoreCase = true)
        if (isApi29Mali) {
            return try {
                val probed = GPUHelper.vkGetDeviceExtensions().toList()
                if (probed.isEmpty()) {
                    // API 29 compatibility fix: empty results act like a probe failure on Mali.
                    VulkanExtensionsResult(
                        extensions = defaultVulkan10Extensions,
                        usedFallback = true,
                    )
                } else {
                    VulkanExtensionsResult(
                        extensions = probed,
                        usedFallback = false,
                    )
                }
            } catch (_: Exception) {
                VulkanExtensionsResult(
                    extensions = defaultVulkan10Extensions,
                    usedFallback = true,
                )
            }
        }

        return VulkanExtensionsResult(
            extensions = GPUHelper.vkGetDeviceExtensions().toList(),
            usedFallback = false,
        )
    }

    private val defaultVulkan10Extensions = listOf(
        "VK_KHR_swapchain",
        "VK_KHR_maintenance1",
        "VK_KHR_maintenance2",
        "VK_KHR_maintenance3",
        "VK_KHR_sampler_mirror_clamp_to_edge",
        "VK_KHR_surface",
        "VK_KHR_android_surface",
        "VK_KHR_get_physical_device_properties2",
    )
}
