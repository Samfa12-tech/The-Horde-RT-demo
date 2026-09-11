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
    std::array<RtDescriptorBindingContract, 23u> bindings{};
    std::uint32_t bindingCount = 0u;
    std::uint32_t storageBufferDescriptorCount = 0u;
    std::uint32_t descriptorWriteCount = 0u;
    RtDiagnosticAvailability diagnosticAvailability = RtDiagnosticAvailability::Unavailable;
    RtDiagnosticIoContract diagnosticIo{};
};

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
    std::string& failureKey);

} // namespace horde::vulkan::raytracing
