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
           extensions.deferredHostOperations &&
           features.accelerationStructure &&
           features.rayQuery &&
           features.bufferDeviceAddress;
}

} // namespace

RtMode EvaluateRtMode(const ExtensionSupport& extensions, const FeatureSupport& features,
                      const std::uint32_t apiVersion)
{
    constexpr std::uint32_t vulkan12 = (1u << 22u) | (2u << 12u);
    ExtensionSupport effective = extensions;
    effective.bufferDeviceAddress = extensions.bufferDeviceAddress || apiVersion >= vulkan12;
    if (SupportsPresentableRayTracingPath(effective, features))
    {
        return RtMode::RayTracingPipeline;
    }

    if (SupportsRayQuery(effective, features))
    {
        return RtMode::RayQuery;
    }

    return RtMode::Unsupported;
}

} // namespace horde::vulkan::raytracing
