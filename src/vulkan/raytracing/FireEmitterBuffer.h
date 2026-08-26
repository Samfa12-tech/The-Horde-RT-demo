#pragma once

#include "gameplay/effects/FireEmitterState.h"
#include "vulkan/raytracing/RtSceneAbi.generated.h"

#include <array>
#include <span>
#include <string>

namespace horde::vulkan::raytracing
{

enum class FireEmitterQuality : std::uint32_t
{
    Mobile = 1u,
    High = 2u,
};

struct FireEmitterQualityBudget
{
    std::uint32_t volumeSteps = 6u;
    std::uint32_t reflectionSamples = 1u;
};

struct FireEmitterTuning
{
    float strengthScale = 1.0f;
    float turbulenceScale = 1.0f;
    float smokeScale = 1.0f;
};

struct FireEmitterSelectionContext
{
    std::array<float, 3u> cameraWorld{};
    horde::gameplay::ShowcaseZone zone = horde::gameplay::ShowcaseZone::Opening;
    float maximumDistance = 24.0f;
};

struct FireEmitterUpload
{
    std::array<RtFireEmitterGpu, kRtFireEmitterCapacity> emitters{};
    std::array<std::uint32_t, kRtActiveFireEmitterCapacity> selectedStableIds{};
    std::uint32_t activeCount = 0u;
};

FireEmitterQualityBudget ResolveFireEmitterQualityBudget(FireEmitterQuality quality);
RtFireEmitterGpu PackFireEmitterGpu(
    const horde::gameplay::effects::FireEmitterState& emitter,
    const FireEmitterTuning& tuning,
    const FireEmitterQualityBudget& budget);
bool BuildFireEmitterUpload(
    std::span<const horde::gameplay::effects::FireEmitterState> configuredEmitters,
    const FireEmitterSelectionContext& selection,
    const FireEmitterTuning& tuning,
    FireEmitterQuality quality,
    FireEmitterUpload& upload,
    std::string& diagnostic);

} // namespace horde::vulkan::raytracing
