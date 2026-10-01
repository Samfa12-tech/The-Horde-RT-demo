#pragma once

#include "vulkan/raytracing/RtPipelineVariants.h"
#include "vulkan/raytracing/RtSceneAbi.generated.h"

#include <array>
#include <vulkan/vulkan.h>

namespace horde::vulkan::raytracing
{

// Investigation only. Call AFTER assembling/cloning all instances: primary
// region rejection belongs to WorldBody, not its viewmodel/legacy limb clones.
// Explicit Opaque secondary rays override this flag; filtered water/shadow rays
// still explicitly request NoOpaque. High instance policy is unchanged.
inline void ApplyMobilePrimaryOpacityAdmission(
    std::array<VkAccelerationStructureInstanceKHR, kRtInstanceMetadataCapacity>& instances,
    const DielectricQuality quality) noexcept
{
    if (quality == DielectricQuality::Mobile)
    {
        instances[kPlayerWorldBodyInstanceIndex].flags |=
            VK_GEOMETRY_INSTANCE_FORCE_NO_OPAQUE_BIT_KHR;
    }
}

} // namespace horde::vulkan::raytracing
