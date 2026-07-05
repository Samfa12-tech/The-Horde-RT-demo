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

bool HasEssentialAccelerationStructureSupport(const DeviceCapabilities& capabilities)
{
    return capabilities.extensions.accelerationStructure &&
           capabilities.extensions.bufferDeviceAddress &&
           capabilities.features.accelerationStructure &&
           capabilities.features.bufferDeviceAddress;
}

bool HasPreferredPipelineSupport(const DeviceCapabilities& capabilities)
{
    return HasEssentialAccelerationStructureSupport(capabilities) &&
           capabilities.extensions.rayTracingPipeline &&
           capabilities.extensions.deferredHostOperations &&
           capabilities.features.rayTracingPipeline;
}

} // namespace horde::vulkan
