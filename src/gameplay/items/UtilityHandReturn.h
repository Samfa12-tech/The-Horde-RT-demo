#pragma once

#include <algorithm>
#include <cmath>
#include <cstdint>
#include <limits>

namespace horde::gameplay::items
{

enum class UtilityHandPhase : std::uint8_t
{
    Idle,
    Using,
    ReverseLift,
    ForwardReturn,
    Released,
};

struct UtilityHandReturnSnapshot
{
    UtilityHandPhase phase = UtilityHandPhase::Idle;
    double profileSeconds = 0.0;
    double usingElapsedSeconds = 0.0;
    bool cancelled = false;
    std::uint64_t generation = 0u;

    bool operator==(const UtilityHandReturnSnapshot&) const = default;
};

// The healing owner supplies the Using sample. Only cancelled hand return has
// an advancing profile here; it consumes the same simulation delta, never a
// renderer timer. This class grants no healing, equipment or combat readiness.
class UtilityHandReturn
{
public:
    static constexpr double kUseDurationSeconds = 1.25;
    static constexpr double kGripAcquireSeconds = 0.30;
    static constexpr double kLiftEndSeconds = 0.55;
    static constexpr double kHoldEndSeconds = 0.70;
    static constexpr double kEquipmentReleaseSeconds = 53.0 / 60.0;
    static constexpr double kGripBlendSeconds = 0.05;
    static constexpr double kMaximumStepSeconds = 0.05;

    // Seed a retained owner counter without importing an in-progress action.
    explicit UtilityHandReturn(const std::uint64_t previousGeneration = 0u)
    {
        snapshot_.generation = previousGeneration;
    }

    const UtilityHandReturnSnapshot& Snapshot() const { return snapshot_; }

    bool Begin()
    {
        if (snapshot_.phase != UtilityHandPhase::Idle ||
            snapshot_.generation == std::numeric_limits<std::uint64_t>::max())
            return false;
        ++snapshot_.generation;
        snapshot_.phase = UtilityHandPhase::Using;
        return true;
    }

    bool SynchronizeUsing(const double authoritativeElapsedSeconds)
    {
        if (snapshot_.phase != UtilityHandPhase::Using ||
            !std::isfinite(authoritativeElapsedSeconds) ||
            authoritativeElapsedSeconds < snapshot_.usingElapsedSeconds ||
            authoritativeElapsedSeconds > kUseDurationSeconds)
            return false;
        snapshot_.usingElapsedSeconds = authoritativeElapsedSeconds;
        snapshot_.profileSeconds = authoritativeElapsedSeconds;
        return true;
    }

    bool Cancel()
    {
        if (snapshot_.phase == UtilityHandPhase::Idle)
            return false;
        if (snapshot_.cancelled)
            return true;
        snapshot_.cancelled = true;
        const double sample = snapshot_.profileSeconds;
        if (sample <= kGripAcquireSeconds || sample >= kEquipmentReleaseSeconds)
            snapshot_.phase = UtilityHandPhase::Released;
        else if (sample < kLiftEndSeconds)
            snapshot_.phase = UtilityHandPhase::ReverseLift;
        else
        {
            // Every source pose in the mouth hold is the same. Skip the
            // remaining hold, retaining the real use time in its own field.
            snapshot_.profileSeconds = std::max(sample, kHoldEndSeconds);
            snapshot_.phase = UtilityHandPhase::ForwardReturn;
        }
        return true;
    }

    bool Step(const double gameplaySeconds)
    {
        if (!std::isfinite(gameplaySeconds) || gameplaySeconds < 0.0 ||
            gameplaySeconds > kMaximumStepSeconds)
            return false;
        if (gameplaySeconds == 0.0)
            return true;

        // A few arithmetic ULPs prevent a repeated subtraction from leaving a
        // profile infinitesimally above its endpoint for an extra owner tick.
        constexpr double roundoff = 16.0 * std::numeric_limits<double>::epsilon();
        if (snapshot_.phase == UtilityHandPhase::ReverseLift)
        {
            const double remaining = snapshot_.profileSeconds - kGripAcquireSeconds;
            if (remaining <= gameplaySeconds + roundoff)
            {
                snapshot_.profileSeconds = kGripAcquireSeconds;
                snapshot_.phase = UtilityHandPhase::Released;
            }
            else
                snapshot_.profileSeconds -= gameplaySeconds;
        }
        else if (snapshot_.phase == UtilityHandPhase::ForwardReturn)
        {
            const double remaining = kEquipmentReleaseSeconds - snapshot_.profileSeconds;
            if (remaining <= gameplaySeconds + roundoff)
            {
                snapshot_.profileSeconds = kEquipmentReleaseSeconds;
                snapshot_.phase = UtilityHandPhase::Released;
            }
            else
                snapshot_.profileSeconds += gameplaySeconds;
        }
        // Idle, Using and Released hold until their authoritative caller acts.
        return true;
    }

    bool FlaskHeld() const { return FlaskHeld(snapshot_); }

    static bool FlaskHeld(const UtilityHandReturnSnapshot& state)
    {
        return (state.phase == UtilityHandPhase::Using ||
                state.phase == UtilityHandPhase::ReverseLift ||
                state.phase == UtilityHandPhase::ForwardReturn) &&
            state.profileSeconds >= kGripAcquireSeconds &&
            state.profileSeconds < kEquipmentReleaseSeconds;
    }

    float GripWeight() const { return GripWeight(snapshot_); }

    static float GripWeight(const UtilityHandReturnSnapshot& state)
    {
        if (!FlaskHeld(state))
            return 0.0f;
        const auto smooth = [](const double value) {
            const double t = std::clamp(value, 0.0, 1.0);
            return t * t * (3.0 - 2.0 * t);
        };
        const double rise = smooth((state.profileSeconds - kGripAcquireSeconds) / kGripBlendSeconds);
        const double fall = smooth((kEquipmentReleaseSeconds - state.profileSeconds) / kGripBlendSeconds);
        return static_cast<float>(std::min(rise, fall));
    }

    bool EquipmentReleaseReady() const
    {
        return (snapshot_.phase == UtilityHandPhase::Using &&
                snapshot_.usingElapsedSeconds >= kEquipmentReleaseSeconds) ||
            (snapshot_.phase == UtilityHandPhase::Released && snapshot_.cancelled);
    }

    void Reset()
    {
        const auto generation = snapshot_.generation;
        snapshot_ = {};
        snapshot_.generation = generation;
    }

private:
    UtilityHandReturnSnapshot snapshot_{};
};

} // namespace horde::gameplay::items
