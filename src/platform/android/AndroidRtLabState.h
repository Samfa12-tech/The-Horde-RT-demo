#pragma once

#include "vulkan/raytracing/RtSceneTuning.h"

#include <algorithm>
#include <cstdint>
#include <mutex>

namespace horde::platform::android
{

struct RtLabUnlockDecisionInputs
{
    bool finaleComplete = false;
    bool debugAutomation = false;
    bool captureActive = false;
    bool routeReplayActive = false;
    bool benchmarkActive = false;
};

inline bool ShouldPersistRtLabUnlock(const RtLabUnlockDecisionInputs& inputs)
{
    return inputs.finaleComplete &&
        !inputs.debugAutomation &&
        !inputs.captureActive &&
        !inputs.routeReplayActive &&
        !inputs.benchmarkActive;
}

class AndroidRtLabState
{
public:
    [[nodiscard]] vulkan::raytracing::RtSceneTuning Snapshot() const
    {
        std::lock_guard<std::mutex> lock(mutex_);
        return tuning_;
    }

    void Replace(const vulkan::raytracing::RtSceneTuning& tuning)
    {
        std::lock_guard<std::mutex> lock(mutex_);
        tuning_ = vulkan::raytracing::ClampRtSceneTuning(tuning);
    }

    void SetScene(const float waterfallWidthScale,
                  const bool roofOverrideEnabled,
                  const float roofOpen,
                  const bool dawnOverrideEnabled,
                  const float dawnReveal,
                  const float fogDensityScale)
    {
        std::lock_guard<std::mutex> lock(mutex_);
        tuning_.waterfallWidthScale = waterfallWidthScale;
        tuning_.finaleRoofOpenOverride = roofOverrideEnabled
            ? std::optional<float>{roofOpen}
            : std::nullopt;
        tuning_.finaleDawnRevealOverride = dawnOverrideEnabled
            ? std::optional<float>{dawnReveal}
            : std::nullopt;
        tuning_.fogDensityScale = fogDensityScale;
        tuning_ = vulkan::raytracing::ClampRtSceneTuning(tuning_);
    }

    void SetLight(const std::uint32_t group, const float hueDegrees, const float intensityScale)
    {
        if (group >= vulkan::raytracing::kRtLightGroupCount) return;
        std::lock_guard<std::mutex> lock(mutex_);
        tuning_.lights[group] = {hueDegrees, intensityScale};
        tuning_ = vulkan::raytracing::ClampRtSceneTuning(tuning_);
    }

    void SetWorkload(const std::int32_t preset)
    {
        std::lock_guard<std::mutex> lock(mutex_);
        tuning_.workloadPreset = static_cast<vulkan::raytracing::RtWorkloadPreset>(
            std::clamp(preset, 0, static_cast<std::int32_t>(vulkan::raytracing::RtWorkloadPreset::Max)));
    }

    void Reset()
    {
        std::lock_guard<std::mutex> lock(mutex_);
        tuning_ = {};
    }

private:
    mutable std::mutex mutex_;
    vulkan::raytracing::RtSceneTuning tuning_{};
};

} // namespace horde::platform::android
