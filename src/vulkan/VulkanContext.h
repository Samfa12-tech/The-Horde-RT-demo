#pragma once

#include "vulkan/DeviceCapabilities.h"

namespace horde::vulkan
{

class VulkanContext
{
public:
    VulkanContext() = default;

    // Scaffold only. The next task must replace this with real Vulkan startup:
    // vkCreateInstance, vkEnumeratePhysicalDevices, extension enumeration, and
    // VkPhysicalDevice*FeaturesKHR queries.
    bool InitialiseForCapabilityProbe();

    DeviceCapabilities QueryDeviceCapabilities() const;
};

} // namespace horde::vulkan
