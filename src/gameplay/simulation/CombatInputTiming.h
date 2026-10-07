#pragma once

#include <algorithm>
#include <array>
#include <cmath>
#include <cstddef>
#include <cstdint>

#include "gameplay/simulation/FixedStepRunner.h"
#include "gameplay/simulation/InputSnapshot.h"

namespace horde::gameplay::simulation
{

inline constexpr std::size_t kCombatInputTimingTraceCapacity = 32u;

enum class CombatInputTimingDisposition : std::uint8_t
{
    Timestamped,
    FirstTimestampFallback,
    MissingMetadataFallback,
    LateEdgeNextTick,
    HitchTailNewestTick,
    QueueOverflowFallback,
};

enum class CombatInputTimingStatus : std::uint8_t
{
    Scheduled,
    Consumed,
    Discarded,
};

struct CombatInputTimingTrace
{
    CombatInputEdgeKind kind = CombatInputEdgeKind::Attack;
    CombatInputTimingDisposition disposition = CombatInputTimingDisposition::Timestamped;
    CombatInputTimingStatus status = CombatInputTimingStatus::Scheduled;
    std::uint64_t commandSequence = 0u;
    std::uint64_t commandCount = 1u;
    std::uint64_t consumedCount = 0u;
    std::uint64_t edgeSteadyTimeNanoseconds = 0u;
    std::uint64_t publicationSequence = 0u;
    std::uint64_t targetTick = 0u;
    std::uint64_t actualTick = 0u;
    std::uint64_t semanticEventSequence = 0u;
    std::uint64_t semanticEventTick = 0u;
};

struct CombatInputTimingSnapshot
{
    std::array<CombatInputTimingTrace, kCombatInputTimingTraceCapacity> traces{};
    std::uint64_t traceOverwriteCount = 0u;
    std::uint64_t inputHistoryOverwriteCount = 0u;
    std::uint32_t traceCount = 0u;
    std::uint32_t nextTraceIndex = 0u;
    std::uint32_t scheduledEdgeCount = 0u;
    std::uint64_t timestampFallbackCount = 0u;
};

struct CombatInputScheduleResult
{
    std::uint64_t targetTick = 0u;
    CombatInputTimingDisposition disposition = CombatInputTimingDisposition::Timestamped;
};

inline std::uint64_t SaturatingAdd(std::uint64_t left, std::uint64_t right)
{
    return right > UINT64_MAX - left ? UINT64_MAX : left + right;
}

// Map an event in the raw monotonic owner interval onto the fixed-step phase
// represented by this frame. The runner's bounded contribution and its old
// accumulator determine the phase; timestamps are only an input-placement
// signal and never change simulation time.
inline CombatInputScheduleResult ScheduleCombatInputEdge(
    const std::uint64_t edgeNs,
    const std::uint64_t previousOwnerNs,
    const std::uint64_t ownerNowNs,
    const double frameDeltaSeconds,
    const double oldAccumulatorSeconds,
    const std::uint64_t currentTick,
    const std::uint32_t ticksProduced)
{
    CombatInputScheduleResult result;
    result.targetTick = SaturatingAdd(currentTick, 1u);

    if (ownerNowNs <= previousOwnerNs || edgeNs < previousOwnerNs || edgeNs > ownerNowNs)
    {
        result.disposition = CombatInputTimingDisposition::LateEdgeNextTick;
        return result;
    }

    const std::uint64_t rawSpanNs = ownerNowNs - previousOwnerNs;
    const double rawSpan = static_cast<double>(rawSpanNs) * 1.0e-9;
    const double accepted = frameDeltaSeconds >= 0.0 && frameDeltaSeconds <=
        FixedStepRunner::kMaximumFrameContributionSeconds
        ? frameDeltaSeconds
        : (frameDeltaSeconds > FixedStepRunner::kMaximumFrameContributionSeconds
            ? FixedStepRunner::kMaximumFrameContributionSeconds
            : 0.0);
    const double acceptedRawSpan = rawSpan > FixedStepRunner::kMaximumFrameContributionSeconds
        ? FixedStepRunner::kMaximumFrameContributionSeconds
        : rawSpan;

    if (rawSpan > FixedStepRunner::kMaximumFrameContributionSeconds &&
        static_cast<double>(edgeNs - previousOwnerNs) * 1.0e-9 >
            FixedStepRunner::kMaximumFrameContributionSeconds)
    {
        result.targetTick = ticksProduced > 0u
            ? SaturatingAdd(currentTick, ticksProduced)
            : SaturatingAdd(currentTick, 1u);
        result.disposition = CombatInputTimingDisposition::HitchTailNewestTick;
        return result;
    }

    double fraction = static_cast<double>(edgeNs - previousOwnerNs) /
                      static_cast<double>(rawSpanNs);
    if (acceptedRawSpan > 0.0)
        fraction = std::min(1.0, fraction * rawSpan / acceptedRawSpan);
    const double phase = oldAccumulatorSeconds + accepted * fraction;
    const double steps = phase <= 0.0
        ? 1.0
        : std::ceil((phase - 1.0e-12) / FixedStepRunner::kFixedDeltaSeconds);
    const auto boundedSteps = static_cast<std::uint64_t>(steps < 1.0 ? 1.0 : steps);
    result.targetTick = SaturatingAdd(currentTick, boundedSteps);
    return result;
}

} // namespace horde::gameplay::simulation
