#include "vulkan/raytracing/RtPipelineVariantProvider.h"

#include "vulkan/raytracing/RtPipelineVariantCatalog.generated.h"
#include "vulkan/raytracing/RtRayQueryVariantCatalog.generated.h"

#include <array>

#if defined(HORDE_RT_PRIMARY_HIT_PROBE)
#include "PrimaryHitProbeShaders.generated.h"
static_assert(HORDE_RT_SELECTED_INSTRUMENTATION == 1 && HORDE_RT_SELECTED_DIELECTRIC_QUALITY == 0,
              "Primary-hit isolation is Diagnostic/Mobile only; Shipping must remain untouched.");
#ifdef NDEBUG
#error "Primary-hit isolation is restricted to Debug investigation builds."
#endif
#endif

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
constexpr std::uint32_t kComputeOpaqueWords[] = {
#include "vulkan/raytracing/variants/rayquery_compute_shipping_mobile_opaque_fast.inc"
};
constexpr std::uint32_t kComputeGenericWords[] = {
#include "vulkan/raytracing/variants/rayquery_compute_shipping_mobile_generic_dielectric.inc"
};
#elif HORDE_RT_SELECTED_INSTRUMENTATION == 0 && HORDE_RT_SELECTED_DIELECTRIC_QUALITY == 1
constexpr RtPipelineBundleRequest kCompiledRequest{RtInstrumentation::Shipping, DielectricQuality::High};
constexpr std::uint32_t kOpaqueWords[] = {
#include "vulkan/raytracing/variants/shipping_high_opaque_fast.inc"
};
constexpr std::uint32_t kGenericWords[] = {
#include "vulkan/raytracing/variants/shipping_high_generic_dielectric.inc"
};
constexpr std::uint32_t kComputeOpaqueWords[] = {
#include "vulkan/raytracing/variants/rayquery_compute_shipping_high_opaque_fast.inc"
};
constexpr std::uint32_t kComputeGenericWords[] = {
#include "vulkan/raytracing/variants/rayquery_compute_shipping_high_generic_dielectric.inc"
};
#elif HORDE_RT_SELECTED_INSTRUMENTATION == 1 && HORDE_RT_SELECTED_DIELECTRIC_QUALITY == 0
constexpr RtPipelineBundleRequest kCompiledRequest{RtInstrumentation::Diagnostic, DielectricQuality::Mobile};
constexpr std::uint32_t kOpaqueWords[] = {
#include "vulkan/raytracing/variants/diagnostic_mobile_opaque_fast.inc"
};
constexpr std::uint32_t kGenericWords[] = {
#include "vulkan/raytracing/variants/diagnostic_mobile_generic_dielectric.inc"
};
constexpr std::uint32_t kComputeOpaqueWords[] = {
#include "vulkan/raytracing/variants/rayquery_compute_diagnostic_mobile_opaque_fast.inc"
};
constexpr std::uint32_t kComputeGenericWords[] = {
#include "vulkan/raytracing/variants/rayquery_compute_diagnostic_mobile_generic_dielectric.inc"
};
#elif HORDE_RT_SELECTED_INSTRUMENTATION == 1 && HORDE_RT_SELECTED_DIELECTRIC_QUALITY == 1
constexpr RtPipelineBundleRequest kCompiledRequest{RtInstrumentation::Diagnostic, DielectricQuality::High};
constexpr std::uint32_t kOpaqueWords[] = {
#include "vulkan/raytracing/variants/diagnostic_high_opaque_fast.inc"
};
constexpr std::uint32_t kGenericWords[] = {
#include "vulkan/raytracing/variants/diagnostic_high_generic_dielectric.inc"
};
constexpr std::uint32_t kComputeOpaqueWords[] = {
#include "vulkan/raytracing/variants/rayquery_compute_diagnostic_high_opaque_fast.inc"
};
constexpr std::uint32_t kComputeGenericWords[] = {
#include "vulkan/raytracing/variants/rayquery_compute_diagnostic_high_generic_dielectric.inc"
};
#else
#error "Unsupported exact RT raygen provider policy."
#endif

static_assert(std::size(kOpaqueWords) == detail::kSelectedRtPipelineCatalog[0].words);
static_assert(std::size(kGenericWords) == detail::kSelectedRtPipelineCatalog[1].words);
static_assert(std::size(kComputeOpaqueWords) == detail::kSelectedRtRayQueryCatalog[0].words);
static_assert(std::size(kComputeGenericWords) == detail::kSelectedRtRayQueryCatalog[1].words);

} // namespace

const RtPipelineVariantProvider& RtPipelineVariantProvider::Compiled(
    const RtExecutionBackend executionBackend) noexcept
{
    static constexpr RtPipelineVariantProvider provider{kCompiledRequest};
    static constexpr RtPipelineVariantProvider computeProvider{{
        kCompiledRequest.instrumentation, kCompiledRequest.quality,
        RtExecutionBackend::RayQueryCompute}};
    static constexpr RtPipelineVariantProvider unsupportedProvider{{
        kCompiledRequest.instrumentation, kCompiledRequest.quality,
        RtExecutionBackend::Unsupported}};
    switch (executionBackend) {
    case RtExecutionBackend::RayTracingPipeline: return provider;
    case RtExecutionBackend::RayQueryCompute: return computeProvider;
    default: return unsupportedProvider;
    }
}

const RtPipelineBundleRequest& RtPipelineVariantProvider::request() const noexcept
{
    return request_;
}

std::optional<RtPipelineVariantArtifact> RtPipelineVariantProvider::ResolveExact(
    RtPipelineVariantKey requested, std::string* error) const
{
    const auto requestedKey = TryFormatRtPipelineVariantKey(requested);
    if (!requestedKey || requested.instrumentation != request_.instrumentation ||
        requested.quality != request_.quality || requested.executionBackend != request_.executionBackend) {
        if (error) { *error = requestedKey ? std::string(*requestedKey) : "invalid_rt_pipeline_variant_key"; }
        return std::nullopt;
    }

    const bool compute = requested.executionBackend == RtExecutionBackend::RayQueryCompute;
    const std::size_t index = requested.material == RtMaterialStrategy::OpaqueFast ? 0u : 1u;
    const auto& record = compute ? detail::kSelectedRtRayQueryCatalog[index]
                                 : detail::kSelectedRtPipelineCatalog[index];
    if (record.key != requested || record.canonicalKey != *requestedKey) {
        if (error) { *error = std::string(*requestedKey); }
        return std::nullopt;
    }
    std::span<const std::uint32_t> words = compute
        ? (index == 0u ? std::span<const std::uint32_t>{kComputeOpaqueWords}
                        : std::span<const std::uint32_t>{kComputeGenericWords})
        : (index == 0u ? std::span<const std::uint32_t>{kOpaqueWords}
                        : std::span<const std::uint32_t>{kGenericWords});
    if (error) { error->clear(); }
    RtPipelineVariantArtifact artifact{record.key, words, record.canonicalKey,
        record.artifactPath, record.spirvSha256, record.includeSha256, record.words,
        record.atomicInstructions, record.hasDiagnosticsBinding};
#if defined(HORDE_RT_PRIMARY_HIT_PROBE)
    if (compute) {
        if (error) *error = "Primary-hit isolation requires the native RT pipeline; compute is not substituted.";
        return std::nullopt;
    }
    // The canonical key identifies the policy slot, NOT the uninstrumented
    // module. Every actual binary fact is replaced and preflight-validated.
    using namespace experimental;
    artifact.words = index == 0u ? std::span<const std::uint32_t>{kProbeNormalOpaqueWords}
                                : std::span<const std::uint32_t>{kProbeNormalGenericWords};
    artifact.spirvSha256 = index == 0u ? kProbeNormalOpaqueSha256 : kProbeNormalGenericSha256;
    artifact.includeSha256 = index == 0u ? kProbeNormalOpaqueIncludeSha256 : kProbeNormalGenericIncludeSha256;
    artifact.expectedWordCount = artifact.words.size();
    artifact.atomicInstructions = index == 0u ? kProbeNormalOpaqueAtomicInstructions : kProbeNormalGenericAtomicInstructions;
#endif
    return artifact;
}

} // namespace horde::vulkan::raytracing
