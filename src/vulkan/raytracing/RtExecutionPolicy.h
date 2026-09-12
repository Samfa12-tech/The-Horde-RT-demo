#pragma once

#include "vulkan/RtExecutionBackend.h"

#include <array>
#include <optional>
#include <vulkan/vulkan.h>

namespace horde::vulkan::raytracing
{

struct RtExecutionPolicy
{
    VkPipelineStageFlags shaderPipelineStage = 0u;
    VkShaderStageFlags shaderStage = 0u;
    VkShaderStageFlags pushConstantStages = 0u;
    VkPipelineBindPoint bindPoint = VK_PIPELINE_BIND_POINT_MAX_ENUM;
    bool requiresShaderBindingTable = false;
};

[[nodiscard]] inline std::optional<RtExecutionPolicy> TryMakeRtExecutionPolicy(
    const RtExecutionBackend backend) noexcept
{
    switch (backend)
    {
    case RtExecutionBackend::RayTracingPipeline:
        return RtExecutionPolicy{
            VK_PIPELINE_STAGE_RAY_TRACING_SHADER_BIT_KHR,
            VK_SHADER_STAGE_RAYGEN_BIT_KHR,
            VK_SHADER_STAGE_RAYGEN_BIT_KHR | VK_SHADER_STAGE_CLOSEST_HIT_BIT_KHR,
            VK_PIPELINE_BIND_POINT_RAY_TRACING_KHR, true};
    case RtExecutionBackend::RayQueryCompute:
        return RtExecutionPolicy{
            VK_PIPELINE_STAGE_COMPUTE_SHADER_BIT, VK_SHADER_STAGE_COMPUTE_BIT,
            VK_SHADER_STAGE_COMPUTE_BIT, VK_PIPELINE_BIND_POINT_COMPUTE, false};
    default:
        return std::nullopt;
    }
}

[[nodiscard]] inline std::optional<std::array<std::uint32_t, 3u>> TryMakeRtComputeDispatch(
    const VkExtent2D extent,
    const VkPhysicalDeviceLimits& limits) noexcept
{
    if (extent.width == 0u || extent.height == 0u ||
        limits.maxComputeWorkGroupInvocations < 64u ||
        limits.maxComputeWorkGroupSize[0] < 8u ||
        limits.maxComputeWorkGroupSize[1] < 8u ||
        limits.maxComputeWorkGroupSize[2] < 1u)
    {
        return std::nullopt;
    }
    // Quotient plus remainder avoids overflow in (extent + 7) / 8.
    const std::array<std::uint32_t, 3u> groups{
        extent.width / 8u + (extent.width % 8u != 0u ? 1u : 0u),
        extent.height / 8u + (extent.height % 8u != 0u ? 1u : 0u), 1u};
    for (std::size_t index = 0u; index < groups.size(); ++index)
    {
        if (groups[index] > limits.maxComputeWorkGroupCount[index])
        {
            return std::nullopt;
        }
    }
    return groups;
}

} // namespace horde::vulkan::raytracing
