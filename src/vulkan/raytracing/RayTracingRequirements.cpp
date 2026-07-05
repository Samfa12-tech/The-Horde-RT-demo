#include "vulkan/raytracing/RayTracingRequirements.h"

namespace horde::vulkan::raytracing
{

bool SupportsRayTracingPipeline(const ExtensionSupport& extensions, const FeatureSupport& features)
{
    return extensions.accelerationStructure &&
           extensions.rayTracingPipeline &&
           extensions.bufferDeviceAddress &&
           extensions.deferredHostOperations &&
           features.accelerationStructure &&
           features.rayTracingPipeline &&
           features.bufferDeviceAddress;
}

bool SupportsRayQuery(const ExtensionSupport& extensions, const FeatureSupport& features)
{
    return extensions.accelerationStructure &&
           extensions.rayQuery &&
           extensions.bufferDeviceAddress &&
           features.accelerationStructure &&
           features.rayQuery &&
           features.bufferDeviceAddress;
}

RtMode EvaluateRtMode(const ExtensionSupport& extensions, const FeatureSupport& features)
{
    if (SupportsRayTracingPipeline(extensions, features))
    {
        return RtMode::RayTracingPipeline;
    }

    if (SupportsRayQuery(extensions, features))
    {
        return RtMode::RayQuery;
    }

    return RtMode::Unsupported;
}

} // namespace horde::vulkan::raytracing
