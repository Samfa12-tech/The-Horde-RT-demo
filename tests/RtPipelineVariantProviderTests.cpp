#include "vulkan/raytracing/RtPipelineVariantProvider.h"
#include "vulkan/raytracing/RtPipelineVariantCatalog.generated.h"
#include "vulkan/raytracing/RtRayQueryVariantCatalog.generated.h"

#include <iostream>
#include <string>
#include <string_view>

namespace {

using namespace horde::vulkan::raytracing;

bool Require(bool condition, std::string_view message)
{
    if (!condition) { std::cerr << message << '\n'; }
    return condition;
}

} // namespace

int main()
{
    bool ok = true;
    const auto& provider = RtPipelineVariantProvider::Compiled();
    ok &= Require(provider.request() == RtPipelineBundleRequest{RtInstrumentation::Shipping, DielectricQuality::Mobile},
                  "fixture policy must be Shipping/Mobile");
    std::string error;
    const auto opaque = provider.ResolveExact(
        {RtInstrumentation::Shipping, DielectricQuality::Mobile, RtMaterialStrategy::OpaqueFast}, &error);
    const auto generic = provider.ResolveExact(
        {RtInstrumentation::Shipping, DielectricQuality::Mobile, RtMaterialStrategy::GenericDielectric}, &error);
    ok &= Require(opaque.has_value() && generic.has_value(), "both exact selected strategies must resolve");
    if (opaque && generic) {
        const auto& opaqueCatalog = detail::kSelectedRtPipelineCatalog[0];
        const auto& genericCatalog = detail::kSelectedRtPipelineCatalog[1];
        ok &= Require(opaqueCatalog.canonicalKey == "shipping_mobile_opaque_fast" &&
                          genericCatalog.canonicalKey == "shipping_mobile_generic_dielectric" &&
                          opaque->words.size() == opaqueCatalog.words &&
                          generic->words.size() == genericCatalog.words &&
                          opaque->expectedWordCount == opaqueCatalog.words &&
                          generic->expectedWordCount == genericCatalog.words &&
                          opaque->spirvSha256 == opaqueCatalog.spirvSha256 &&
                          generic->spirvSha256 == genericCatalog.spirvSha256 &&
                          opaque->includeSha256 == opaqueCatalog.includeSha256 &&
                          generic->includeSha256 == genericCatalog.includeSha256 &&
                          opaque->artifactPath == opaqueCatalog.artifactPath &&
                          generic->artifactPath == genericCatalog.artifactPath &&
                          opaque->atomicInstructions == opaqueCatalog.atomicInstructions &&
                          generic->atomicInstructions == genericCatalog.atomicInstructions &&
                          opaque->hasDiagnosticsBinding == opaqueCatalog.hasDiagnosticsBinding &&
                          generic->hasDiagnosticsBinding == genericCatalog.hasDiagnosticsBinding,
                      "selected frozen artifact metadata changed");
        ok &= Require(opaque->canonicalKey == "shipping_mobile_opaque_fast" &&
                          generic->canonicalKey == "shipping_mobile_generic_dielectric",
                      "provider must keep complete independent semantic keys");
    }
    const auto mismatch = provider.ResolveExact(
        {RtInstrumentation::Diagnostic, DielectricQuality::Mobile, RtMaterialStrategy::OpaqueFast}, &error);
    ok &= Require(!mismatch && error == "diagnostic_mobile_opaque_fast",
                  "wrong instrumentation must fail with the canonical requested key");
    const auto wrongQuality = provider.ResolveExact(
        {RtInstrumentation::Shipping, DielectricQuality::High, RtMaterialStrategy::OpaqueFast}, &error);
    ok &= Require(!wrongQuality && error == "shipping_high_opaque_fast",
                  "wrong quality must not choose a nearby selected record");
    const auto invalid = provider.ResolveExact(
        {RtInstrumentation::Shipping, DielectricQuality::Mobile, static_cast<RtMaterialStrategy>(99)}, &error);
    ok &= Require(!invalid && error == "invalid_rt_pipeline_variant_key",
                  "invalid material must not fall back to a selected strategy");
    const auto& compute = RtPipelineVariantProvider::Compiled(
        horde::vulkan::RtExecutionBackend::RayQueryCompute);
    const RtPipelineVariantKey computeOpaqueKey{
        RtInstrumentation::Shipping, DielectricQuality::Mobile, RtMaterialStrategy::OpaqueFast,
        horde::vulkan::RtExecutionBackend::RayQueryCompute};
    const RtPipelineVariantKey computeGenericKey{
        RtInstrumentation::Shipping, DielectricQuality::Mobile, RtMaterialStrategy::GenericDielectric,
        horde::vulkan::RtExecutionBackend::RayQueryCompute};
    const auto computeOpaque = compute.ResolveExact(computeOpaqueKey, &error);
    const auto computeGeneric = compute.ResolveExact(computeGenericKey, &error);
    ok &= Require(compute.request().executionBackend ==
                      horde::vulkan::RtExecutionBackend::RayQueryCompute &&
                      computeOpaque && computeGeneric,
                  "compiled compute provider must resolve its exact two hardware-query artifacts");
    if (computeOpaque && computeGeneric && opaque && generic) {
        const auto& computeOpaqueCatalog = detail::kSelectedRtRayQueryCatalog[0];
        const auto& computeGenericCatalog = detail::kSelectedRtRayQueryCatalog[1];
        ok &= Require(computeOpaqueCatalog.canonicalKey == "rayquery_compute_shipping_mobile_opaque_fast" &&
                          computeGenericCatalog.canonicalKey == "rayquery_compute_shipping_mobile_generic_dielectric" &&
                          computeOpaque->words.size() == computeOpaqueCatalog.words &&
                          computeGeneric->words.size() == computeGenericCatalog.words &&
                          computeOpaque->expectedWordCount == computeOpaqueCatalog.words &&
                          computeGeneric->expectedWordCount == computeGenericCatalog.words &&
                          computeOpaque->spirvSha256 == computeOpaqueCatalog.spirvSha256 &&
                          computeGeneric->spirvSha256 == computeGenericCatalog.spirvSha256 &&
                          computeOpaque->includeSha256 == computeOpaqueCatalog.includeSha256 &&
                          computeGeneric->includeSha256 == computeGenericCatalog.includeSha256 &&
                          computeOpaque->artifactPath == computeOpaqueCatalog.artifactPath &&
                          computeGeneric->artifactPath == computeGenericCatalog.artifactPath &&
                          computeOpaque->atomicInstructions == computeOpaqueCatalog.atomicInstructions &&
                          computeGeneric->atomicInstructions == computeGenericCatalog.atomicInstructions &&
                          computeOpaque->hasDiagnosticsBinding == computeOpaqueCatalog.hasDiagnosticsBinding &&
                          computeGeneric->hasDiagnosticsBinding == computeGenericCatalog.hasDiagnosticsBinding &&
                          computeOpaque->canonicalKey == "rayquery_compute_shipping_mobile_opaque_fast" &&
                          computeGeneric->canonicalKey == "rayquery_compute_shipping_mobile_generic_dielectric" &&
                          computeOpaque->words.data() != opaque->words.data() &&
                          computeGeneric->words.data() != generic->words.data() &&
                          computeOpaque->atomicInstructions == 0u &&
                          computeGeneric->atomicInstructions == 0u &&
                          !computeOpaque->hasDiagnosticsBinding && !computeGeneric->hasDiagnosticsBinding,
                      "compute provider must own distinct stage bytes with Shipping diagnostics absent");
    }
    ok &= Require(!provider.ResolveExact(computeOpaqueKey) &&
                      !compute.ResolveExact({RtInstrumentation::Shipping, DielectricQuality::Mobile,
                                             RtMaterialStrategy::OpaqueFast}),
                  "a fixed stage provider must reject a cross-backend request");
    const auto& unsupported = RtPipelineVariantProvider::Compiled(
        horde::vulkan::RtExecutionBackend::Unsupported);
    ok &= Require(!unsupported.ResolveExact(computeOpaqueKey) &&
                      !unsupported.ResolveExact({RtInstrumentation::Shipping, DielectricQuality::Mobile,
                                                 RtMaterialStrategy::OpaqueFast}),
                  "unsupported execution provider must not silently return a pipeline artifact");
    return ok ? 0 : 1;
}
