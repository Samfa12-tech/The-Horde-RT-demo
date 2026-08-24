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

enum class RtLabScrollAction
{
    Top,
    Bottom,
    LineUp,
    LineDown,
    PageUp,
    PageDown,
    Thumb,
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

inline int StepRtLabScroll(
    const int current,
    const int maximum,
    const int lineDistance,
    const int pageDistance,
    const RtLabScrollAction action,
    const int thumbPosition = 0)
{
    int next = current;
    switch (action)
    {
    case RtLabScrollAction::Top: next = 0; break;
    case RtLabScrollAction::Bottom: next = maximum; break;
    case RtLabScrollAction::LineUp: next -= lineDistance; break;
    case RtLabScrollAction::LineDown: next += lineDistance; break;
    case RtLabScrollAction::PageUp: next -= pageDistance; break;
    case RtLabScrollAction::PageDown: next += pageDistance; break;
    case RtLabScrollAction::Thumb: next = thumbPosition; break;
    }
    return std::clamp(next, 0, std::max(0, maximum));
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
