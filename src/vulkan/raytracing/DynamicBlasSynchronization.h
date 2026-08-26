#pragma once

#include <vulkan/vulkan.h>

namespace horde::vulkan::raytracing
{

struct DynamicBlasUpdateSet
{
    bool player = false;
    bool skeletonPose0 = false;
    bool skeletonPose1 = false;
    bool lich = false;
};

struct DynamicBlasToTlasDependency
{
    bool required = false;
    VkPipelineStageFlags sourceStageMask = 0u;
    VkPipelineStageFlags destinationStageMask = 0u;
    VkAccessFlags sourceAccessMask = 0u;
    VkAccessFlags destinationAccessMask = 0u;
};

inline DynamicBlasToTlasDependency BuildDynamicBlasToTlasDependency(
    const DynamicBlasUpdateSet& updates)
{
    DynamicBlasToTlasDependency result;
    // This predicate is the renderer's command-recording seam. The focused
    // contract covers every producer that can record a BLAS update before the
    // frame's TLAS update.
    result.required = updates.player || updates.skeletonPose0 ||
                      updates.skeletonPose1 || updates.lich;
    result.sourceStageMask = VK_PIPELINE_STAGE_ACCELERATION_STRUCTURE_BUILD_BIT_KHR;
    result.destinationStageMask = VK_PIPELINE_STAGE_ACCELERATION_STRUCTURE_BUILD_BIT_KHR;
    result.sourceAccessMask = VK_ACCESS_ACCELERATION_STRUCTURE_WRITE_BIT_KHR;
    result.destinationAccessMask = VK_ACCESS_ACCELERATION_STRUCTURE_READ_BIT_KHR;
    return result;
}

} // namespace horde::vulkan::raytracing
