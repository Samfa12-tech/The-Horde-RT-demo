#include "vulkan/raytracing/FireEmitterBuffer.h"

#include <algorithm>
#include <cmath>
#include <vector>

namespace horde::vulkan::raytracing
{
namespace
{

float FiniteClamped(const float value, const float minimum, const float maximum,
                    const float fallback)
{
    return std::clamp(std::isfinite(value) ? value : fallback, minimum, maximum);
}

std::array<float, 3u> FireColour(const horde::gameplay::effects::FireEmitterState& emitter)
{
    const float temperature = FiniteClamped(
        emitter.colourTemperatureKelvin, 1000.0f, 4000.0f, 1850.0f);
    const float warm = std::clamp((temperature - 1000.0f) / 3000.0f, 0.0f, 1.0f);
    const std::array<float, 3u> blackBody{{
        1.0f,
        0.28f + 0.72f * warm,
        0.02f + 0.58f * std::pow(warm, 1.4f)}};
    return {{
        blackBody[0] * FiniteClamped(emitter.artTint[0], 0.0f, 2.0f, 1.0f),
        blackBody[1] * FiniteClamped(emitter.artTint[1], 0.0f, 2.0f, 1.0f),
        blackBody[2] * FiniteClamped(emitter.artTint[2], 0.0f, 2.0f, 1.0f)}};
}

float DistanceSquared(const horde::gameplay::effects::FireEmitterState& emitter,
                      const std::array<float, 3u>& camera)
{
    const float x = emitter.worldFromFlame[12] - camera[0];
    const float y = emitter.worldFromFlame[13] - camera[1];
    const float z = emitter.worldFromFlame[14] - camera[2];
    return x * x + y * y + z * z;
}

} // namespace

FireEmitterQualityBudget ResolveFireEmitterQualityBudget(const FireEmitterQuality quality)
{
    return quality == FireEmitterQuality::High
        ? FireEmitterQualityBudget{10u, 2u}
        : FireEmitterQualityBudget{6u, 1u};
}

RtFireEmitterGpu PackFireEmitterGpu(
    const horde::gameplay::effects::FireEmitterState& emitter,
    const FireEmitterTuning& tuning,
    const FireEmitterQualityBudget& budget)
{
    const float strengthScale = FiniteClamped(tuning.strengthScale, 0.0f, 4.0f, 1.0f);
    const float turbulenceScale = FiniteClamped(tuning.turbulenceScale, 0.0f, 2.0f, 1.0f);
    const float smokeScale = FiniteClamped(tuning.smokeScale, 0.0f, 2.0f, 1.0f);
    const float baseStrength = FiniteClamped(emitter.strength, 0.0f, 1.0f, 0.0f) *
                               FiniteClamped(emitter.fuel, 0.0f, 1.0f, 0.0f) * strengthScale;
    const float coherentFlicker = std::clamp(
        0.88f + 0.12f * FiniteClamped(emitter.lowPassNoise, -1.0f, 1.0f, 0.0f),
        0.74f, 1.0f);
    const float radiance = baseStrength * coherentFlicker;
    const auto colour = FireColour(emitter);

    RtFireEmitterGpu gpu{};
    gpu.worldFromLocal0 = {{emitter.worldFromFlame[0], emitter.worldFromFlame[1],
                            emitter.worldFromFlame[2], emitter.worldFromFlame[3]}};
    gpu.worldFromLocal1 = {{emitter.worldFromFlame[4], emitter.worldFromFlame[5],
                            emitter.worldFromFlame[6], emitter.worldFromFlame[7]}};
    gpu.worldFromLocal2 = {{emitter.worldFromFlame[8], emitter.worldFromFlame[9],
                            emitter.worldFromFlame[10], emitter.worldFromFlame[11]}};
    gpu.worldFromLocal3 = {{emitter.worldFromFlame[12], emitter.worldFromFlame[13],
                            emitter.worldFromFlame[14], emitter.worldFromFlame[15]}};
    gpu.lightPositionStrength = {{emitter.worldFromLight[12], emitter.worldFromLight[13],
                                  emitter.worldFromLight[14], radiance}};
    gpu.colourIntensity = {{colour[0], colour[1], colour[2], radiance}};
    gpu.shape = {{emitter.radius, emitter.height, emitter.coreRadius, emitter.absorption}};
    gpu.animation = {{emitter.phase, emitter.lowPassNoise, emitter.leanX, emitter.leanZ}};
    gpu.smokeEmbers = {{emitter.smokeDensity * smokeScale, emitter.emberRate,
                        std::clamp((emitter.turbulence + emitter.motionTurbulence) *
                                       turbulenceScale, 0.0f, 2.0f),
                        emitter.fuel}};
    gpu.identity = {{emitter.stableId, emitter.seed,
                     std::clamp(budget.volumeSteps, 1u, 16u),
                     std::clamp(budget.reflectionSamples, 0u, 2u)}};
    return gpu;
}

bool BuildFireEmitterUpload(
    const std::span<const horde::gameplay::effects::FireEmitterState> configuredEmitters,
    const FireEmitterSelectionContext& selection,
    const FireEmitterTuning& tuning,
    const FireEmitterQuality quality,
    FireEmitterUpload& upload,
    std::string& diagnostic)
{
    upload = {};
    if (configuredEmitters.size() > kRtFireEmitterCapacity)
    {
        diagnostic = "FireEmitterBuffer capacity exceeded: " +
                     std::to_string(configuredEmitters.size()) + " configured, maximum " +
                     std::to_string(kRtFireEmitterCapacity) + ".";
        return false;
    }

    struct Candidate
    {
        const horde::gameplay::effects::FireEmitterState* emitter = nullptr;
        float distanceSquared = 0.0f;
    };
    std::vector<Candidate> candidates;
    candidates.reserve(configuredEmitters.size());
    const float maximumDistance = FiniteClamped(
        selection.maximumDistance, 0.0f, 1000.0f, 24.0f);
    const float maximumDistanceSquared = maximumDistance * maximumDistance;
    for (const auto& emitter : configuredEmitters)
    {
        if (emitter.stableId == 0u)
        {
            diagnostic = "FireEmitterBuffer requires non-zero stable IDs.";
            return false;
        }
        const bool duplicate = std::any_of(
            configuredEmitters.begin(), configuredEmitters.end(),
            [&emitter](const auto& other) {
                return &other != &emitter && other.stableId == emitter.stableId;
            });
        if (duplicate)
        {
            diagnostic = "FireEmitterBuffer requires unique stable IDs.";
            return false;
        }
        const float distanceSquared = DistanceSquared(emitter, selection.cameraWorld);
        if (emitter.zone == selection.zone && emitter.strength * emitter.fuel > 0.0001f &&
            distanceSquared <= maximumDistanceSquared)
            candidates.push_back({&emitter, distanceSquared});
    }
    std::sort(candidates.begin(), candidates.end(), [](const Candidate& left,
                                                        const Candidate& right) {
        if (left.distanceSquared != right.distanceSquared)
            return left.distanceSquared < right.distanceSquared;
        return left.emitter->stableId < right.emitter->stableId;
    });
    if (candidates.size() > kRtActiveFireEmitterCapacity)
        candidates.resize(kRtActiveFireEmitterCapacity);
    std::sort(candidates.begin(), candidates.end(), [](const Candidate& left,
                                                        const Candidate& right) {
        return left.emitter->stableId < right.emitter->stableId;
    });

    const FireEmitterQualityBudget budget = ResolveFireEmitterQualityBudget(quality);
    upload.activeCount = static_cast<std::uint32_t>(candidates.size());
    for (std::size_t index = 0u; index < candidates.size(); ++index)
    {
        upload.selectedStableIds[index] = candidates[index].emitter->stableId;
        upload.emitters[index] = PackFireEmitterGpu(*candidates[index].emitter, tuning, budget);
    }
    diagnostic.clear();
    return true;
}

} // namespace horde::vulkan::raytracing
