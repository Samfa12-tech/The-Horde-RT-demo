#pragma once

#include <string>

#include <vulkan/vulkan.h>

#include "vulkan/DeviceCapabilities.h"

namespace horde::vulkan::raytracing
{

bool AreAccelerationStructurePreconditionsMet(const VkPhysicalDevice physicalDevice,
                                            const DeviceCapabilities& capabilities,
                                            std::string& diagnostic);

} // namespace horde::vulkan::raytracing
