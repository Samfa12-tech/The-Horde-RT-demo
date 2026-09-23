#pragma once

#include "vulkan/RtExecutionBackend.h"

#include <optional>
#include <string>

namespace horde::vulkan::raytracing {

enum class RtInstrumentation { Shipping, Diagnostic };
enum class DielectricQuality { Mobile, High };
enum class RtMaterialStrategy { OpaqueFast, GenericDielectric };

struct RtPipelineVariantKey {
    RtInstrumentation instrumentation;
    DielectricQuality quality;
    RtMaterialStrategy material;
    RtExecutionBackend executionBackend = RtExecutionBackend::RayTracingPipeline;

    constexpr bool operator==(const RtPipelineVariantKey&) const = default;
};

struct RtPipelineBundleRequest {
    RtInstrumentation instrumentation;
    DielectricQuality quality;

    RtExecutionBackend executionBackend = RtExecutionBackend::RayTracingPipeline;

    constexpr bool operator==(const RtPipelineBundleRequest&) const = default;
};

[[nodiscard]] std::optional<std::string> TryFormatRtPipelineVariantKey(
    RtPipelineVariantKey key);
[[nodiscard]] std::string FormatRtPipelineVariantKey(RtPipelineVariantKey key);
[[nodiscard]] std::optional<RtPipelineBundleRequest> TryMakeRtPipelineBundleRequest(
    RtInstrumentation instrumentation, DielectricQuality quality,
    RtExecutionBackend executionBackend = RtExecutionBackend::RayTracingPipeline);

} // namespace horde::vulkan::raytracing
