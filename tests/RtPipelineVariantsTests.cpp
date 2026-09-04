#include "vulkan/raytracing/RtPipelineVariants.h"

#include <array>
#include <iostream>
#include <string_view>

namespace {

using horde::vulkan::raytracing::DielectricQuality;
using horde::vulkan::raytracing::RtInstrumentation;
using horde::vulkan::raytracing::RtMaterialStrategy;
using horde::vulkan::raytracing::RtPipelineVariantKey;

bool Require(bool condition, std::string_view message)
{
    if (!condition) {
        std::cerr << message << '\n';
    }
    return condition;
}

} // namespace

int main()
{
    bool ok = true;
    constexpr std::array<std::string_view, 8> expectedKeys{
        "shipping_mobile_opaque_fast", "shipping_mobile_generic_dielectric",
        "shipping_high_opaque_fast", "shipping_high_generic_dielectric",
        "diagnostic_mobile_opaque_fast", "diagnostic_mobile_generic_dielectric",
        "diagnostic_high_opaque_fast", "diagnostic_high_generic_dielectric"};
    constexpr std::array<RtInstrumentation, 2> instrumentations{
        RtInstrumentation::Shipping, RtInstrumentation::Diagnostic};
    constexpr std::array<DielectricQuality, 2> qualities{
        DielectricQuality::Mobile, DielectricQuality::High};
    constexpr std::array<RtMaterialStrategy, 2> materials{
        RtMaterialStrategy::OpaqueFast, RtMaterialStrategy::GenericDielectric};

    std::size_t index = 0;
    for (const auto instrumentation : instrumentations) {
        for (const auto quality : qualities) {
            for (const auto material : materials) {
                const RtPipelineVariantKey key{instrumentation, quality, material};
                ok &= Require(horde::vulkan::raytracing::FormatRtPipelineVariantKey(key) == expectedKeys[index],
                              "canonical variant key changed");
                ++index;
            }
        }
    }

    ok &= Require(!horde::vulkan::raytracing::TryFormatRtPipelineVariantKey(
                       {static_cast<RtInstrumentation>(99), DielectricQuality::Mobile, RtMaterialStrategy::OpaqueFast})
                       .has_value(),
                  "invalid instrumentation must not resolve to an adjacent key");
    ok &= Require(!horde::vulkan::raytracing::TryFormatRtPipelineVariantKey(
                       {RtInstrumentation::Shipping, static_cast<DielectricQuality>(99), RtMaterialStrategy::OpaqueFast})
                       .has_value(),
                  "invalid quality must not resolve to an adjacent key");
    ok &= Require(!horde::vulkan::raytracing::TryFormatRtPipelineVariantKey(
                       {RtInstrumentation::Shipping, DielectricQuality::Mobile, static_cast<RtMaterialStrategy>(99)})
                       .has_value(),
                  "invalid material must not resolve to an adjacent key");
    ok &= Require(!horde::vulkan::raytracing::TryMakeRtPipelineBundleRequest(
                       static_cast<RtInstrumentation>(99), DielectricQuality::Mobile)
                       .has_value(),
                  "invalid bundle instrumentation must fail closed");
    ok &= Require(!horde::vulkan::raytracing::TryMakeRtPipelineBundleRequest(
                       RtInstrumentation::Shipping, static_cast<DielectricQuality>(99))
                       .has_value(),
                  "invalid bundle quality must fail closed");
    return ok ? 0 : 1;
}
