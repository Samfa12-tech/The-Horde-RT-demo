#pragma once

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

    constexpr bool operator==(const RtPipelineVariantKey&) const = default;
};

struct RtPipelineBundleRequest {
    RtInstrumentation instrumentation;
    DielectricQuality quality;

    constexpr bool operator==(const RtPipelineBundleRequest&) const = default;
};

[[nodiscard]] std::optional<std::string> TryFormatRtPipelineVariantKey(
    RtPipelineVariantKey key);
[[nodiscard]] std::string FormatRtPipelineVariantKey(RtPipelineVariantKey key);
[[nodiscard]] std::optional<RtPipelineBundleRequest> TryMakeRtPipelineBundleRequest(
    RtInstrumentation instrumentation, DielectricQuality quality);

} // namespace horde::vulkan::raytracing
