#pragma once
#include <algorithm>
#include <cmath>
#include <cstdint>

namespace horde::audio
{
inline constexpr float kMenuFlameGain = .16f;
inline constexpr float kMenuRoomGain = .09f;
inline constexpr float kMenuChainGain = .13f;

// Presentation-only authored cue. Observe the actual fixed-step pendulum's
// turning points; no wall-clock scheduler, gameplay event or catch-up burst.
class MenuChainCreaks
{
public:
    void Reset() noexcept { *this = {}; }
    void Step(std::uint64_t tick, float angle, float velocity, bool stationary) noexcept
    {
        if (!std::isfinite(angle) || !std::isfinite(velocity)) return;
        const int direction = velocity > .00001f ? 1 : velocity < -.00001f ? -1 : 0;
        if (!stationary && direction != 0 && previousDirection_ != 0 &&
            direction != previousDirection_ && std::abs(angle) >= .012f && tick >= nextTick_)
        {
            ++serial_;
            nextTick_ = tick + 720u; // At least twelve seconds, then the next real turn.
        }
        previousDirection_ = stationary ? 0 : direction == 0 ? previousDirection_ : direction;
    }
    std::uint64_t Serial() const noexcept { return serial_; }
private:
    std::uint64_t serial_ = 0u, nextTick_ = 480u;
    int previousDirection_ = 0;
};

// Platform voice owner consumes only the newest serial. Establish a baseline
// after silence/reset rather than replaying creaks accumulated while inaudible.
class MenuCreakDelivery
{
public:
    bool Observe(std::uint64_t tick, std::uint64_t serial, bool audible) noexcept
    {
        const bool restarted = initialized_ && (tick < tick_ || serial < serial_);
        const bool deliver = audible && audible_ && initialized_ && !restarted && serial > serial_;
        initialized_ = true; tick_ = tick; serial_ = serial; audible_ = audible;
        return deliver;
    }
    void Suspend() noexcept { audible_ = false; }
private:
    std::uint64_t tick_ = 0u, serial_ = 0u;
    bool initialized_ = false, audible_ = false;
};
} // namespace horde::audio
