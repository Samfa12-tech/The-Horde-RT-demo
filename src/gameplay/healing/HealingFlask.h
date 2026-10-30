#pragma once

#include <algorithm>
#include <cmath>
#include <cstdint>

namespace horde::gameplay::healing {

struct FlaskResources {
    bool upgraded = false;
    std::uint32_t charges = 2;
    constexpr std::uint32_t Capacity() const { return upgraded ? 3u : 2u; }
    constexpr bool Valid() const { return charges <= Capacity(); }
    bool operator==(const FlaskResources&) const = default;
};

enum class FlaskAction : std::uint8_t { Idle, Drinking, Resting };

// The simulation owner supplies current, post-damage health and explicitly
// resolved eligibility. The booleans are not requests or saved entitlement:
// grounded/stationary/readiness, earned anchor, combat/dialogue and ownership
// checks belong to that owner. Looking alone must not invalidate eligibility.
struct FlaskContext {
    int healthHalfUnits = 6;
    bool drinkEligible = false;
    bool restEligible = false;
    bool cancelRequested = false;
    bool acceptedDamage = false;
};

struct FlaskSnapshot {
    FlaskResources resources{};
    FlaskAction action = FlaskAction::Idle;
    double elapsedSeconds = 0;
    bool operator==(const FlaskSnapshot&) const = default;
};

struct FlaskCompletion {
    FlaskAction action = FlaskAction::Idle;
    int healHalfUnits = 0;
    bool completed = false;
};

// Pure fixed-step state. No platform input, inventory, timers outside gameplay,
// filesystem or presentation ownership. Begin methods consume only a fresh
// semantic request already admitted by the simulation; this is not an edge queue.
class HealingFlask {
public:
    static constexpr int kMaximumHealthHalfUnits = 6;
    static constexpr double kDrinkSeconds = 1.25;
    static constexpr double kRestSeconds = 2.0;
    static constexpr double kMaximumStepSeconds = .05;

    const FlaskSnapshot& Snapshot() const { return snapshot_; }

    void ResetNewGame() { snapshot_ = {}; }

    // Failed restore preserves all state. Successful exact restore always drops
    // transient action/timer state; legacy resource absence is resolved by the
    // future versioned campaign integration, never guessed here.
    bool Restore(FlaskResources resources) {
        if (!resources.Valid()) return false;
        snapshot_ = {};
        snapshot_.resources = resources;
        return true;
    }

    bool BeginDrink(const FlaskContext& context) {
        if (snapshot_.action != FlaskAction::Idle || !CanDrink(context)) return false;
        snapshot_.action = FlaskAction::Drinking;
        snapshot_.elapsedSeconds = 0;
        return true;
    }

    bool BeginRest(const FlaskContext& context) {
        if (snapshot_.action != FlaskAction::Idle || !CanRest(context)) return false;
        snapshot_.action = FlaskAction::Resting;
        snapshot_.elapsedSeconds = 0;
        return true;
    }

    void Cancel() {
        snapshot_.action = FlaskAction::Idle;
        snapshot_.elapsedSeconds = 0;
    }

    // Caller resolves this completion and its bounded heal in the same owner
    // tick before publishing a snapshot. No callbacks or other mutation may
    // intervene. Resources are committed exactly once; no timer is serialized.
    FlaskCompletion Step(double gameplaySeconds, const FlaskContext& context) {
        const auto action = snapshot_.action;
        if (action == FlaskAction::Idle) return {};
        // Cancellation, damage and prerequisite loss win the completion tie.
        if (!std::isfinite(gameplaySeconds) || gameplaySeconds < 0 ||
            (action == FlaskAction::Drinking ? !CanDrink(context) : !CanRest(context))) {
            Cancel();
            return {};
        }
        snapshot_.elapsedSeconds += std::min(gameplaySeconds, kMaximumStepSeconds);
        const double duration = action == FlaskAction::Drinking ? kDrinkSeconds : kRestSeconds;
        if (snapshot_.elapsedSeconds + 1e-9 < duration) return {};

        const int heal = action == FlaskAction::Drinking
            ? std::min(2, kMaximumHealthHalfUnits - context.healthHalfUnits)
            : kMaximumHealthHalfUnits - context.healthHalfUnits;
        if (action == FlaskAction::Drinking) --snapshot_.resources.charges;
        else snapshot_.resources.charges = snapshot_.resources.Capacity();
        Cancel();
        return {action, heal, true};
    }

    // The caller validates actual pickup contact and cancels other actions
    // before accepting an upgrade interaction. Capacity changes only once;
    // neither health nor existing charges are granted by the entitlement.
    bool TryUpgrade() {
        if (snapshot_.action != FlaskAction::Idle || snapshot_.resources.upgraded) return false;
        snapshot_.resources.upgraded = true;
        return true;
    }

private:
    static bool Ready(const FlaskContext& context) {
        return context.healthHalfUnits > 0 && context.healthHalfUnits <= kMaximumHealthHalfUnits &&
            !context.cancelRequested && !context.acceptedDamage;
    }
    bool CanDrink(const FlaskContext& context) const {
        return Ready(context) && context.drinkEligible &&
            context.healthHalfUnits < kMaximumHealthHalfUnits && snapshot_.resources.charges > 0;
    }
    static bool CanRest(const FlaskContext& context) {
        return Ready(context) && context.restEligible;
    }
    FlaskSnapshot snapshot_{};
};
} // namespace horde::gameplay::healing
