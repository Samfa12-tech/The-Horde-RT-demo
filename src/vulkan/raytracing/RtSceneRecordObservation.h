#pragma once

#include "telemetry/RtPerformanceEvidence.h"

#include <chrono>
#include <cstdint>

namespace horde::vulkan::raytracing
{

using RtSceneObservationClock = std::uint64_t (*)(void*) noexcept;

// Stack-bound, non-owning observation context for one renderer record attempt.
// The record caller owns Begin/Commit/Abort on the accumulator; renderer
// helpers only append to its active scratch values.
struct RtSceneRecordObservation
{
    horde::telemetry::RtStageAccumulator* stages = nullptr;
    void* clockUser = nullptr;
    RtSceneObservationClock readClock = nullptr;
    bool healthy = true;
};

inline std::uint64_t ReadRtSceneSteadyClock(void*) noexcept
{
    const auto elapsed = std::chrono::duration_cast<std::chrono::nanoseconds>(
        std::chrono::steady_clock::now().time_since_epoch()).count();
    return elapsed < 0 ? 0u : static_cast<std::uint64_t>(elapsed);
}

class RtSceneStageScope final
{
public:
    RtSceneStageScope(RtSceneRecordObservation* observation,
                      const horde::telemetry::RtStage stage) noexcept
        : observation_(observation), stage_(stage)
    {
        if (observation_ == nullptr || observation_->stages == nullptr ||
            !observation_->stages->Active())
        {
            return;
        }
        clock_ = observation_->readClock != nullptr
            ? observation_->readClock
            : ReadRtSceneSteadyClock;
        startNanoseconds_ = clock_(observation_->clockUser);
        active_ = true;
    }

    RtSceneStageScope(const RtSceneStageScope&) = delete;
    RtSceneStageScope& operator=(const RtSceneStageScope&) = delete;

    void Complete(const std::uint64_t workInvocationCount = 0u,
                  const std::uint64_t byteCount = 0u,
                  const std::uint64_t operationCount = 0u) noexcept
    {
        if (!active_)
        {
            return;
        }
        const std::uint64_t endNanoseconds = clock_(observation_->clockUser);
        active_ = false;
        if (endNanoseconds < startNanoseconds_ ||
            !observation_->stages->Accumulate(
                stage_, endNanoseconds - startNanoseconds_,
                workInvocationCount, byteCount, operationCount))
        {
            observation_->healthy = false;
        }
    }

    void Cancel() noexcept { active_ = false; }
    [[nodiscard]] bool Active() const noexcept { return active_; }

private:
    RtSceneRecordObservation* observation_ = nullptr;
    RtSceneObservationClock clock_ = nullptr;
    horde::telemetry::RtStage stage_ = horde::telemetry::RtStage::Count;
    std::uint64_t startNanoseconds_ = 0u;
    bool active_ = false;
};

} // namespace horde::vulkan::raytracing
