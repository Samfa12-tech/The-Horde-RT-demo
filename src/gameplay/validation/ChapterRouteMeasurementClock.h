#pragma once
#include <optional>
#include <cstdint>
#include <cmath>
#include <algorithm>
namespace horde::gameplay::validation {
// A validation clock, not a gameplay owner. Produces only complete60Hz ticks
// from actual elapsed time. The caller consumes fresh probe input per tick.
// Catch-up is bounded per render without silently dropping simulation debt.
class ChapterRouteMeasurementClock {
public:
    std::optional<unsigned> Consume(double elapsedSeconds) {
        if(!std::isfinite(elapsedSeconds) || elapsedSeconds<lastSeconds_ ||
           elapsedSeconds<0 || elapsedSeconds>150) return std::nullopt;
        const auto due=static_cast<std::uint64_t>(std::floor(elapsedSeconds*60));
        const auto ticks=static_cast<unsigned>(std::min<std::uint64_t>(4,due-consumed_));
        consumed_+=ticks;pending_=due-consumed_;lastSeconds_=elapsedSeconds;
        return ticks;
    }
    std::uint64_t ConsumedTicks() const { return consumed_; }
    std::uint64_t PendingTicks() const { return pending_; }
private:
    double lastSeconds_=0;
    std::uint64_t consumed_=0,pending_=0;
};
}
