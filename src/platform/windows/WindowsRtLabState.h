#pragma once

#include <algorithm>
#include <cstddef>

namespace horde::platform::windows
{

struct RtLabUnlockContext
{
    bool finaleComplete = false;
    bool capture = false;
    bool checkpoint = false;
    bool replay = false;
    bool benchmark = false;
    bool debugInjection = false;
};

inline bool CanPersistRtLabUnlock(const RtLabUnlockContext& context)
{
    return context.finaleComplete && !context.capture && !context.checkpoint &&
           !context.replay && !context.benchmark && !context.debugInjection;
}

enum class RtLabControlRange
{
    WaterfallPercent,
    UnitPercent,
    DoublePercent,
    HueDegrees,
};

inline int StepRtLabControl(const int value, const bool increase, const RtLabControlRange range)
{
    int minimum = 0;
    int maximum = 200;
    switch (range)
    {
    case RtLabControlRange::WaterfallPercent: minimum = 25; break;
    case RtLabControlRange::UnitPercent: maximum = 100; break;
    case RtLabControlRange::HueDegrees: minimum = -180; maximum = 180; break;
    case RtLabControlRange::DoublePercent: break;
    }
    return std::clamp(value + (increase ? 5 : -5), minimum, maximum);
}

inline std::size_t WrapRtLabFocus(
    const std::size_t current,
    const int direction,
    const std::size_t count)
{
    if (count == 0u) return 0u;
    return direction < 0 ? (current + count - 1u) % count : (current + 1u) % count;
}

inline bool ShouldPlayControllerMenuSound(const bool rtLabVisible)
{
    return !rtLabVisible;
}

} // namespace horde::platform::windows
