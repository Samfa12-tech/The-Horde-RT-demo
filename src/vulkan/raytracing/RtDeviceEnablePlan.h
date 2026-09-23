#pragma once

#include "vulkan/DeviceCapabilities.h"
#include "vulkan/RtExecutionBackend.h"

#include <array>
#include <optional>

namespace horde::vulkan::raytracing
{

// Both frozen shader launchers target Vulkan 1.2 (SPIR-V 1.5).
inline constexpr std::uint32_t kRtShaderMinimumApiVersion = (1u << 22u) | (2u << 12u);

struct RtDeviceEnablePlan
{
    FeatureSupport features{};
    std::array<const char*, 4u> extensions{};
    std::uint32_t extensionCount = 0u;
};

[[nodiscard]] inline bool SameRtDeviceIdentity(
    const DeviceIdentity& probed,
    const DeviceIdentity& candidate) noexcept
{
    return probed.vendorId == candidate.vendorId && probed.deviceId == candidate.deviceId &&
           probed.gpuName == candidate.gpuName && probed.driverVersion == candidate.driverVersion &&
           probed.vulkanApiVersion == candidate.vulkanApiVersion;
}

[[nodiscard]] inline RtExecutionBackend SelectRtExecutionBackend(
    const DeviceCapabilities& capabilities,
    const bool computeQueueSupported,
    const bool requireRayQueryCompute = false) noexcept
{
    const auto& extensions = capabilities.extensions;
    const auto& features = capabilities.features;
    if (!computeQueueSupported || capabilities.identity.vulkanApiVersion < kRtShaderMinimumApiVersion ||
        !extensions.accelerationStructure || !extensions.rayQuery || !extensions.deferredHostOperations ||
        !features.accelerationStructure || !features.rayQuery || !features.bufferDeviceAddress)
    {
        return RtExecutionBackend::Unsupported;
    }
    if (!requireRayQueryCompute && extensions.rayTracingPipeline && features.rayTracingPipeline)
    {
        return RtExecutionBackend::RayTracingPipeline;
    }
    return RtExecutionBackend::RayQueryCompute;
}

[[nodiscard]] inline std::optional<RtDeviceEnablePlan> MakeRtDeviceEnablePlan(
    const RtExecutionBackend backend) noexcept
{
    if (backend != RtExecutionBackend::RayTracingPipeline && backend != RtExecutionBackend::RayQueryCompute)
    {
        return std::nullopt;
    }
    // BDA, SPIR-V 1.4 and shader float controls are core in the required 1.2
    // API. Keep raw extension reporting honest and request only non-core names.
    RtDeviceEnablePlan plan{};
    plan.features = {true, backend == RtExecutionBackend::RayTracingPipeline, true, true};
    plan.extensions = {"VK_KHR_acceleration_structure", "VK_KHR_ray_query",
                       "VK_KHR_deferred_host_operations", nullptr};
    plan.extensionCount = 3u;
    if (plan.features.rayTracingPipeline)
    {
        plan.extensions[3] = "VK_KHR_ray_tracing_pipeline";
        plan.extensionCount = 4u;
    }
    return plan;
}

} // namespace horde::vulkan::raytracing
