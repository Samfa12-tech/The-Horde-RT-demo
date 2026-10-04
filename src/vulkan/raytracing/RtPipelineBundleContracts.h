#pragma once

#include "vulkan/raytracing/RtPipelineVariantProvider.h"

#include <array>
#include <cstddef>
#include <cstdint>
#include <optional>
#include <span>
#include <string>
#include <string_view>

namespace horde::vulkan::raytracing {

inline constexpr std::size_t kRtDielectricDiagnosticsSchema1FieldCount = 41u;

enum class RtDescriptorResourceKind {
    AccelerationStructure,
    StorageImage,
    StorageBuffer,
    CombinedImageSampler,
};

enum class RtDiagnosticAvailability {
    Unavailable,
    CompiledOut,
    Available,
};

struct RtDescriptorBindingContract {
    std::uint32_t binding = 0u;
    RtDescriptorResourceKind kind = RtDescriptorResourceKind::StorageBuffer;
};

struct RtDiagnosticIoContract {
    bool allocateBuffer = false;
    bool descriptorInfo = false;
    bool descriptorWrite = false;
    bool priorFrameRead = false;
    bool zeroReset = false;
    bool shaderWriteBarrier = false;
};

struct RtDescriptorIoContract {
    RtInstrumentation instrumentation = RtInstrumentation::Shipping;
    // Entries are the actual descriptor roster in binding order. Shipping is
    // deliberately non-contiguous because binding 22 is Diagnostic-only.
    std::array<RtDescriptorBindingContract, 27u> bindings{};
    std::uint32_t bindingCount = 0u;
    std::uint32_t storageBufferDescriptorCount = 0u;
    std::uint32_t combinedImageSamplerDescriptorCount = 0u;
    std::uint32_t descriptorWriteCount = 0u;
    RtDiagnosticAvailability diagnosticAvailability = RtDiagnosticAvailability::Unavailable;
    RtDiagnosticIoContract diagnosticIo{};
};

struct RtSampledDescriptorLimits {
    std::uint32_t perStageSamplers = 0u;
    std::uint32_t setSamplers = 0u;
    std::uint32_t perStageSampledImages = 0u;
    std::uint32_t setSampledImages = 0u;
};

// Each current combined-image-sampler binding has descriptorCount1 and counts
// against both sampler and sampled-image limits, including the environment.
[[nodiscard]] inline bool ValidateRtSampledDescriptorLimits(
    const RtDescriptorIoContract& contract, const RtSampledDescriptorLimits& limits) noexcept
{
    const auto count = contract.combinedImageSamplerDescriptorCount;
    return count <= limits.perStageSamplers && count <= limits.setSamplers &&
           count <= limits.perStageSampledImages && count <= limits.setSampledImages;
}

struct RtPipelineBundlePreflight {
    RtPipelineBundleRequest request{};
    RtDescriptorIoContract descriptorIo{};
    std::array<RtPipelineVariantArtifact, 2u> strategies{};
};

[[nodiscard]] std::string_view ToString(RtDiagnosticAvailability availability) noexcept;
[[nodiscard]] std::optional<RtDescriptorIoContract> TryMakeRtDescriptorIoContract(
    RtInstrumentation instrumentation) noexcept;
[[nodiscard]] bool ValidateRtPipelineBundlePreflight(
    RtPipelineBundleRequest request,
    std::span<const RtPipelineVariantArtifact> candidateRecords,
    RtPipelineBundlePreflight& preflight,
    std::string& failureKey);
[[nodiscard]] bool ResolveCompiledRtPipelineBundlePreflight(
    RtPipelineBundlePreflight& preflight,
    std::string& failureKey,
    horde::vulkan::RtExecutionBackend executionBackend =
        horde::vulkan::RtExecutionBackend::RayTracingPipeline);

} // namespace horde::vulkan::raytracing
