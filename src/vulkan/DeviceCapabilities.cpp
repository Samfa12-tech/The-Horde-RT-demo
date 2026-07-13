#include "vulkan/DeviceCapabilities.h"

namespace horde::vulkan
{

std::string ToString(const RtMode mode)
{
    switch (mode)
    {
    case RtMode::RayTracingPipeline:
        return "RayTracingPipeline";
    case RtMode::RayQuery:
        return "RayQuery";
    case RtMode::Unsupported:
    default:
        return "Unsupported";
    }
}

} // namespace horde::vulkan
