#include "vulkan/raytracing/RayTracingRequirements.h"

namespace horde::vulkan::raytracing
{

namespace
{

bool SupportsPresentableRayTracingPath(const ExtensionSupport& extensions, const FeatureSupport& features)
{
    return extensions.accelerationStructure &&
           extensions.rayTracingPipeline &&
           extensions.rayQuery &&
           extensions.bufferDeviceAddress &&
           extensions.deferredHostOperations &&
           features.accelerationStructure &&
           features.rayTracingPipeline &&
           features.rayQuery &&
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

} // namespace

RtMode EvaluateRtMode(const ExtensionSupport& extensions, const FeatureSupport& features)
{
    if (SupportsPresentableRayTracingPath(extensions, features))
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
