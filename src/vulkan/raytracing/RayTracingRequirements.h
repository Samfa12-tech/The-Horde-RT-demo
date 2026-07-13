#pragma once

#include "vulkan/DeviceCapabilities.h"

namespace horde::vulkan::raytracing
{

RtMode EvaluateRtMode(const ExtensionSupport& extensions, const FeatureSupport& features);

} // namespace horde::vulkan::raytracing
