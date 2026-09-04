#include "vulkan/raytracing/RtPipelineVariants.h"

#include <stdexcept>

namespace horde::vulkan::raytracing {

std::optional<std::string> TryFormatRtPipelineVariantKey(RtPipelineVariantKey key)
{
    const char* instrumentation = nullptr;
    const char* quality = nullptr;
    const char* material = nullptr;
    switch (key.instrumentation) {
    case RtInstrumentation::Shipping: instrumentation = "shipping"; break;
    case RtInstrumentation::Diagnostic: instrumentation = "diagnostic"; break;
    default: return std::nullopt;
    }
    switch (key.quality) {
    case DielectricQuality::Mobile: quality = "mobile"; break;
    case DielectricQuality::High: quality = "high"; break;
    default: return std::nullopt;
    }
    switch (key.material) {
    case RtMaterialStrategy::OpaqueFast: material = "opaque_fast"; break;
    case RtMaterialStrategy::GenericDielectric: material = "generic_dielectric"; break;
    default: return std::nullopt;
    }
    std::string formatted(instrumentation);
    formatted += '_';
    formatted += quality;
    formatted += '_';
    formatted += material;
    return formatted;
}

std::string FormatRtPipelineVariantKey(RtPipelineVariantKey key)
{
    if (const auto formatted = TryFormatRtPipelineVariantKey(key)) {
        return *formatted;
    }
    throw std::invalid_argument("invalid RT pipeline variant key");
}

std::optional<RtPipelineBundleRequest> TryMakeRtPipelineBundleRequest(
    RtInstrumentation instrumentation, DielectricQuality quality)
{
    const RtPipelineVariantKey opaque{instrumentation, quality, RtMaterialStrategy::OpaqueFast};
    const RtPipelineVariantKey generic{instrumentation, quality, RtMaterialStrategy::GenericDielectric};
    if (!TryFormatRtPipelineVariantKey(opaque) || !TryFormatRtPipelineVariantKey(generic)) {
        return std::nullopt;
    }
    return RtPipelineBundleRequest{instrumentation, quality};
}

} // namespace horde::vulkan::raytracing
