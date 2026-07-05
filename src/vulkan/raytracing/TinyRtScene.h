#pragma once

#include <string>

#include <vulkan/vulkan.h>

#include "vulkan/DeviceCapabilities.h"

namespace horde::vulkan::raytracing
{

TinyRtSceneSnapshot ExecuteTinyRtScene(VkPhysicalDevice physicalDevice, const DeviceCapabilities& capabilities);

} // namespace horde::vulkan::raytracing
