#include "vulkan/raytracing/RtPipelineVariants.h"

#include <stdexcept>

namespace horde::vulkan::raytracing {

std::optional<std::string_view> TryFormatRtPipelineVariantKey(RtPipelineVariantKey key) noexcept
{
    switch (key.instrumentation) {
    case RtInstrumentation::Shipping:
        switch (key.quality) {
        case DielectricQuality::Mobile:
            switch (key.material) {
            case RtMaterialStrategy::OpaqueFast: return "shipping_mobile_opaque_fast";
            case RtMaterialStrategy::GenericDielectric: return "shipping_mobile_generic_dielectric";
            }
            break;
        case DielectricQuality::High:
            switch (key.material) {
            case RtMaterialStrategy::OpaqueFast: return "shipping_high_opaque_fast";
            case RtMaterialStrategy::GenericDielectric: return "shipping_high_generic_dielectric";
            }
            break;
        }
        break;
    case RtInstrumentation::Diagnostic:
        switch (key.quality) {
        case DielectricQuality::Mobile:
            switch (key.material) {
            case RtMaterialStrategy::OpaqueFast: return "diagnostic_mobile_opaque_fast";
            case RtMaterialStrategy::GenericDielectric: return "diagnostic_mobile_generic_dielectric";
            }
            break;
        case DielectricQuality::High:
            switch (key.material) {
            case RtMaterialStrategy::OpaqueFast: return "diagnostic_high_opaque_fast";
            case RtMaterialStrategy::GenericDielectric: return "diagnostic_high_generic_dielectric";
            }
            break;
        }
        break;
    }
    return std::nullopt;
}

std::string_view FormatRtPipelineVariantKey(RtPipelineVariantKey key)
{
    if (const auto formatted = TryFormatRtPipelineVariantKey(key)) {
        return *formatted;
    }
    throw std::invalid_argument("invalid RT pipeline variant key");
}

std::optional<RtPipelineBundleRequest> TryMakeRtPipelineBundleRequest(
    RtInstrumentation instrumentation, DielectricQuality quality) noexcept
{
    const RtPipelineVariantKey opaque{instrumentation, quality, RtMaterialStrategy::OpaqueFast};
    const RtPipelineVariantKey generic{instrumentation, quality, RtMaterialStrategy::GenericDielectric};
    if (!TryFormatRtPipelineVariantKey(opaque) || !TryFormatRtPipelineVariantKey(generic)) {
        return std::nullopt;
    }
    return RtPipelineBundleRequest{instrumentation, quality};
}

} // namespace horde::vulkan::raytracing
