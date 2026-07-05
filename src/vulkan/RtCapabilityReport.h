#pragma once

#include <string>

#include "vulkan/DeviceCapabilities.h"

namespace horde::vulkan
{

std::string BuildCapabilityTextReport(const DeviceCapabilities& capabilities);
std::string BuildCapabilityJsonReport(const DeviceCapabilities& capabilities);

} // namespace horde::vulkan
