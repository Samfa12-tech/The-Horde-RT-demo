#include "vulkan/raytracing/RayTracingPipeline.h"

namespace horde::vulkan::raytracing
{

bool AreRayTracingPipelinePreconditionsMet(const VkPhysicalDevice physicalDevice, const DeviceCapabilities& capabilities,
                                          std::string& diagnostic)
{
    if (physicalDevice == VK_NULL_HANDLE)
    {
        diagnostic = "No physical device selected for RT pipeline creation.";
        return false;
    }

    if (!capabilities.extensions.rayTracingPipeline || !capabilities.features.rayTracingPipeline)
    {
        diagnostic = "VK_KHR_ray_tracing_pipeline extension/feature is not enabled on selected device.";
        return false;
    }

    if (!capabilities.extensions.deferredHostOperations)
    {
        diagnostic = "VK_KHR_deferred_host_operations extension is required for this phase.";
        return false;
    }

    diagnostic.clear();
    return true;
}

} // namespace horde::vulkan::raytracing
