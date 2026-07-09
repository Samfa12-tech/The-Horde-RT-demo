#include "vulkan/raytracing/AccelerationStructures.h"

namespace horde::vulkan::raytracing
{

bool AreAccelerationStructurePreconditionsMet(const VkPhysicalDevice physicalDevice, const DeviceCapabilities& capabilities,
                                            std::string& diagnostic)
{
    if (physicalDevice == VK_NULL_HANDLE)
    {
        diagnostic = "No physical device selected for acceleration structure work.";
        return false;
    }

    if (!capabilities.extensions.accelerationStructure || !capabilities.features.accelerationStructure)
    {
        diagnostic = "Acceleration structure extension/feature is required but not supported on selected device.";
        return false;
    }

    if (!capabilities.extensions.bufferDeviceAddress || !capabilities.features.bufferDeviceAddress)
    {
        diagnostic = "bufferDeviceAddress extension/feature required for Vulkan RT primitives is not present.";
        return false;
    }

    diagnostic.clear();
    return true;
}

} // namespace horde::vulkan::raytracing
