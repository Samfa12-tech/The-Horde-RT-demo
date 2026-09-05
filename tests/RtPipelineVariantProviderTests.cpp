#include "vulkan/raytracing/RtPipelineVariantProvider.h"

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
        ok &= Require(opaque->words.size() == 122292 && generic->words.size() == 54232,
                      "selected frozen word counts changed");
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
    return ok ? 0 : 1;
}
