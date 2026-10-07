#pragma once

#include <algorithm>
#include <array>
#include <cstddef>
#include <cstdint>
#include <optional>

#include "graphics/GraphicsSettings.h"
#include "vulkan/raytracing/RtSceneAbi.generated.h"

namespace horde::vulkan::raytracing
{

enum class RtWorkloadPreset : std::uint32_t
{
    Lean = 0u,
    Authored = 1u,
    Max = 2u,
};

// Mirrors the shared shader policy. Secondary transport retains its bounded
// one-sample budget; this count describes contributing primary receivers.
constexpr std::uint32_t ResolvePrimaryAreaShadowSamples(
    const RtWorkloadPreset preset, const bool compiledHighQuality)
{
    return preset == RtWorkloadPreset::Max ? (compiledHighQuality ? 4u : 2u) : 1u;
}

// Mode3 explicitly retains historical whole-workload diagnostics for callers
// that do not supply a Graphics choice. Production Graphics always supplies one.
inline constexpr std::uint32_t kRtShadowLegacyWorkload = 3u;
inline std::optional<RtQualityControlsGpu> ResolveRtQualityControls(
    const std::optional<horde::graphics::ShadowQuality> shadow,
    const RtWorkloadPreset workload, const bool compiledHighQuality,
    const bool mistEnabled = true,
    const horde::graphics::DustQuality dust = horde::graphics::DustQuality::Off) noexcept
{
    if (!horde::graphics::ValidDustQuality(dust)) return std::nullopt;
    if (shadow && static_cast<unsigned>(*shadow) > 2u) return std::nullopt;
    const auto mode = shadow ? static_cast<std::uint32_t>(*shadow) : kRtShadowLegacyWorkload;
    const bool higher = shadow ? *shadow == horde::graphics::ShadowQuality::Higher :
        workload == RtWorkloadPreset::Max;
    return RtQualityControlsGpu{{mode, higher ? (compiledHighQuality ? 4u : 2u) : 1u,
        higher ? 2u : 1u, (mistEnabled ? 0u : 1u) | (static_cast<unsigned>(dust) << 1u)}};
}

// Quality-word bit0 disables the existing scene ground mist; bits1-2 select dust. Unknown flags
// are invalid; successful uploads remain distinguishable from desired settings.
inline std::optional<bool> ResolveUploadedMistEnabled(const RtQualityControlsGpu& quality) noexcept
{
    if (quality.controls[3] > 5u) return std::nullopt;
    return (quality.controls[3] & 1u) == 0u;
}

inline std::optional<horde::graphics::DustQuality> ResolveUploadedDustQuality(const RtQualityControlsGpu& quality) noexcept
{
    if (quality.controls[3] > 5u) return std::nullopt;
    return static_cast<horde::graphics::DustQuality>(quality.controls[3] >> 1u);
}

enum class RtLightGroup : std::uint32_t
{
    Torch = 0u,
    Skylight = 1u,
    Passage = 2u,
    Staff = 3u,
};

constexpr std::size_t kRtLightGroupCount = 4u;

struct RtLightTuning
{
    float hueDegrees = 0.0f;
    float intensityScale = 1.0f;
};

struct RtSceneTuning
{
    float waterfallWidthScale = 1.0f;
    std::optional<float> finaleRoofOpenOverride;
    std::optional<float> finaleDawnRevealOverride;
    float fogDensityScale = 1.0f;
    std::array<RtLightTuning, kRtLightGroupCount> lights{};
    RtWorkloadPreset workloadPreset = RtWorkloadPreset::Authored;
    float fireStrengthScale = 1.0f;
    float fireTurbulenceScale = 1.0f;
    float fireSmokeScale = 1.0f;
    float glassTransmission = 0.94f;
    float glassIor = 1.52f;
    float glassRoughness = 0.12f;
    bool glassFixtureVisible = false;
    std::array<float, 3u> glassAttenuationColor{{0.72f, 0.90f, 1.0f}};
    float glassAttenuationDistance = 2.4f;
    float glassDepthScale = 1.0f;
    bool productionRewardPropsVisible = false;
    bool productionLanternGlassOnly = false;
};

inline RtSceneTuning ClampRtSceneTuning(RtSceneTuning tuning)
{
    tuning.waterfallWidthScale = std::clamp(tuning.waterfallWidthScale, 0.25f, 2.0f);
    if (tuning.finaleRoofOpenOverride.has_value())
    {
        *tuning.finaleRoofOpenOverride = std::clamp(*tuning.finaleRoofOpenOverride, 0.0f, 1.0f);
    }
    if (tuning.finaleDawnRevealOverride.has_value())
    {
        *tuning.finaleDawnRevealOverride = std::clamp(*tuning.finaleDawnRevealOverride, 0.0f, 1.0f);
    }
    tuning.fogDensityScale = std::clamp(tuning.fogDensityScale, 0.0f, 2.0f);
    for (RtLightTuning& light : tuning.lights)
    {
        light.hueDegrees = std::clamp(light.hueDegrees, -180.0f, 180.0f);
        light.intensityScale = std::clamp(light.intensityScale, 0.0f, 2.0f);
    }
    tuning.fireStrengthScale = std::clamp(tuning.fireStrengthScale, 0.0f, 2.0f);
    tuning.fireTurbulenceScale = std::clamp(tuning.fireTurbulenceScale, 0.0f, 2.0f);
    tuning.fireSmokeScale = std::clamp(tuning.fireSmokeScale, 0.0f, 2.0f);
    tuning.glassTransmission = std::clamp(tuning.glassTransmission, 0.0f, 1.0f);
    tuning.glassIor = std::clamp(tuning.glassIor, 1.0f, 2.5f);
    tuning.glassRoughness = std::clamp(tuning.glassRoughness, 0.0f, 1.0f);
    for (float& channel : tuning.glassAttenuationColor)
        channel = std::clamp(channel, 0.0f, 1.0f);
    tuning.glassAttenuationDistance =
        std::clamp(tuning.glassAttenuationDistance, 0.0f, 100.0f);
    tuning.glassDepthScale = std::clamp(tuning.glassDepthScale, 0.005f, 1.0f);
    return tuning;
}

struct WaterfallCurtainScale
{
    float depth = 1.0f;
    float vertical = 1.0f;
    float crossLane = 1.0f;
};

inline WaterfallCurtainScale ResolveWaterfallCurtainScale(const RtSceneTuning& tuning)
{
    // The player approaches the curtain along world X, so world Z is its
    // visible width across the terminal lane. Keep the millimetre-thin depth
    // unchanged and apply the lab control only to that cross-lane span.
    return {1.0f, 1.0f, ClampRtSceneTuning(tuning).waterfallWidthScale};
}

inline float ResolveRtFinaleRoofOpen(const float authoredValue, const RtSceneTuning& tuning)
{
    return tuning.finaleRoofOpenOverride.has_value()
               ? std::clamp(*tuning.finaleRoofOpenOverride, 0.0f, 1.0f)
               : authoredValue;
}

inline float ResolveRtFinaleDawnReveal(const float authoredValue, const RtSceneTuning& tuning)
{
    return tuning.finaleDawnRevealOverride.has_value()
               ? std::clamp(*tuning.finaleDawnRevealOverride, 0.0f, 1.0f)
               : authoredValue;
}

} // namespace horde::vulkan::raytracing
