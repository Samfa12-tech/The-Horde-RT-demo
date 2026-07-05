#pragma once

#include <string>

#include "vulkan/DeviceCapabilities.h"

namespace horde::ui
{

std::string BuildDiagnosticOverlayText(const vulkan::DeviceCapabilities& capabilities);
std::string BuildUnsupportedDeviceText(const vulkan::DeviceCapabilities& capabilities);

} // namespace horde::ui
