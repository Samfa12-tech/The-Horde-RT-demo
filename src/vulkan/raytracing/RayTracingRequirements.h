#pragma once

#include "vulkan/DeviceCapabilities.h"

namespace horde::vulkan::raytracing
{

RtMode EvaluateRtMode(const ExtensionSupport& extensions, const FeatureSupport& features,
                      std::uint32_t apiVersion = 0u);

} // namespace horde::vulkan::raytracing
