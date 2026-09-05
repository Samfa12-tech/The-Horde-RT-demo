#include "vulkan/raytracing/RtPipelineVariantProvider.h"

#include "vulkan/raytracing/RtPipelineVariantCatalog.generated.h"

#include <array>

#if !defined(HORDE_RT_SELECTED_INSTRUMENTATION) || !defined(HORDE_RT_SELECTED_DIELECTRIC_QUALITY)
#error "The RT raygen provider requires an exact compiled policy."
#endif

namespace horde::vulkan::raytracing {
namespace {

#if HORDE_RT_SELECTED_INSTRUMENTATION == 0 && HORDE_RT_SELECTED_DIELECTRIC_QUALITY == 0
constexpr RtPipelineBundleRequest kCompiledRequest{RtInstrumentation::Shipping, DielectricQuality::Mobile};
constexpr std::uint32_t kOpaqueWords[] = {
#include "vulkan/raytracing/variants/shipping_mobile_opaque_fast.inc"
};
constexpr std::uint32_t kGenericWords[] = {
#include "vulkan/raytracing/variants/shipping_mobile_generic_dielectric.inc"
};
#elif HORDE_RT_SELECTED_INSTRUMENTATION == 0 && HORDE_RT_SELECTED_DIELECTRIC_QUALITY == 1
constexpr RtPipelineBundleRequest kCompiledRequest{RtInstrumentation::Shipping, DielectricQuality::High};
constexpr std::uint32_t kOpaqueWords[] = {
#include "vulkan/raytracing/variants/shipping_high_opaque_fast.inc"
};
constexpr std::uint32_t kGenericWords[] = {
#include "vulkan/raytracing/variants/shipping_high_generic_dielectric.inc"
};
#elif HORDE_RT_SELECTED_INSTRUMENTATION == 1 && HORDE_RT_SELECTED_DIELECTRIC_QUALITY == 0
constexpr RtPipelineBundleRequest kCompiledRequest{RtInstrumentation::Diagnostic, DielectricQuality::Mobile};
constexpr std::uint32_t kOpaqueWords[] = {
#include "vulkan/raytracing/variants/diagnostic_mobile_opaque_fast.inc"
};
constexpr std::uint32_t kGenericWords[] = {
#include "vulkan/raytracing/variants/diagnostic_mobile_generic_dielectric.inc"
};
#elif HORDE_RT_SELECTED_INSTRUMENTATION == 1 && HORDE_RT_SELECTED_DIELECTRIC_QUALITY == 1
constexpr RtPipelineBundleRequest kCompiledRequest{RtInstrumentation::Diagnostic, DielectricQuality::High};
constexpr std::uint32_t kOpaqueWords[] = {
#include "vulkan/raytracing/variants/diagnostic_high_opaque_fast.inc"
};
constexpr std::uint32_t kGenericWords[] = {
#include "vulkan/raytracing/variants/diagnostic_high_generic_dielectric.inc"
};
#else
#error "Unsupported exact RT raygen provider policy."
#endif

static_assert(std::size(kOpaqueWords) == detail::kSelectedRtPipelineCatalog[0].words);
static_assert(std::size(kGenericWords) == detail::kSelectedRtPipelineCatalog[1].words);

} // namespace

const RtPipelineVariantProvider& RtPipelineVariantProvider::Compiled() noexcept
{
    static constexpr RtPipelineVariantProvider provider{kCompiledRequest};
    return provider;
}

const RtPipelineBundleRequest& RtPipelineVariantProvider::request() const noexcept
{
    return request_;
}

std::optional<RtPipelineVariantArtifact> RtPipelineVariantProvider::ResolveExact(
    RtPipelineVariantKey requested, std::string* error) const
{
    const auto requestedKey = TryFormatRtPipelineVariantKey(requested);
    if (!requestedKey || requested.instrumentation != request_.instrumentation || requested.quality != request_.quality) {
        if (error) { *error = requestedKey ? std::string(*requestedKey) : "invalid_rt_pipeline_variant_key"; }
        return std::nullopt;
    }

    if (requested.material == RtMaterialStrategy::OpaqueFast) {
        const auto& record = detail::kSelectedRtPipelineCatalog[0];
        if (record.key != requested || record.canonicalKey != *requestedKey) {
            if (error) { *error = std::string(*requestedKey); }
            return std::nullopt;
        }
        return RtPipelineVariantArtifact{record.key, kOpaqueWords, record.canonicalKey,
            record.artifactPath, record.spirvSha256, record.includeSha256, record.words,
            record.atomicInstructions, record.hasDiagnosticsBinding};
    }
    if (requested.material == RtMaterialStrategy::GenericDielectric) {
        const auto& record = detail::kSelectedRtPipelineCatalog[1];
        if (record.key != requested || record.canonicalKey != *requestedKey) {
            if (error) { *error = std::string(*requestedKey); }
            return std::nullopt;
        }
        return RtPipelineVariantArtifact{record.key, kGenericWords, record.canonicalKey,
            record.artifactPath, record.spirvSha256, record.includeSha256, record.words,
            record.atomicInstructions, record.hasDiagnosticsBinding};
    }
    if (error) { *error = std::string(*requestedKey); }
    return std::nullopt;
}

} // namespace horde::vulkan::raytracing
