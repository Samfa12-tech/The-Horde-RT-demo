#pragma once

#include <algorithm>
#include <cmath>
#include <cstdint>

#include "gameplay/CombatTimeline.h"
#include "gameplay/SwordCombat.h"
#include "gameplay/simulation/GameplayEvent.h"

namespace horde::gameplay::simulation
{

// Immutable per-snapshot provenance for the most recent successful parry
// presentation. It is independent of the combat action, so a riposte can
// begin on the next fixed tick without erasing the visual response.
struct CombatPresentationSnapshot
{
    bool parrySuccessActive = false;
    float parrySuccessRemainingSeconds = 0.0f;
    std::uint64_t parrySuccessEventSequence = 0u;
    std::uint64_t parrySuccessTickIndex = 0u;
    EntityId parrySuccessEntity = EntityId::Invalid;
};

class CombatPresentationTimeline
{
public:
    void AdvanceFrame(double frameDeltaSeconds, bool paused)
    {
        if (paused)
        {
            Reset();
            return;
        }
        if (!std::isfinite(frameDeltaSeconds) || frameDeltaSeconds <= 0.0)
        {
            return;
        }

        // Age already-presented feedback by elapsed frame time, independently
        // of discarded simulation time during a long hitch. A new event later
        // in this frame starts its full envelope after this age operation.
        const double contribution = std::min(
            frameDeltaSeconds,
            static_cast<double>(horde::gameplay::CombatTimeline::kParryPresentationSeconds));
        snapshot_.parrySuccessRemainingSeconds = std::max(
            0.0f,
            snapshot_.parrySuccessRemainingSeconds - static_cast<float>(contribution));
        if (snapshot_.parrySuccessRemainingSeconds <= 0.0f)
        {
            Reset();
        }
    }

    void BeginParrySuccess(std::uint64_t eventSequence,
                           std::uint64_t tickIndex,
                           EntityId entity)
    {
        if (eventSequence == 0u || entity == EntityId::Invalid)
        {
            return;
        }
        snapshot_.parrySuccessActive = true;
        snapshot_.parrySuccessRemainingSeconds =
            horde::gameplay::CombatTimeline::kParryPresentationSeconds;
        snapshot_.parrySuccessEventSequence = eventSequence;
        snapshot_.parrySuccessTickIndex = tickIndex;
        snapshot_.parrySuccessEntity = entity;
    }

    PlayerCombatSnapshot PlayerCombatForPresentation(
        const PlayerCombatSnapshot& authoritative) const
    {
        PlayerCombatSnapshot result = authoritative;
        // The independent presentation record exclusively owns this effect;
        // the frozen one-tick combat reaction must not leak through pause/reset.
        if (result.reaction == CombatReaction::Parried)
        {
            result.reaction = CombatReaction::None;
            result.reactionTime = 0.0f;
        }
        if (snapshot_.parrySuccessActive)
        {
            result.reaction = CombatReaction::Parried;
            result.reactionTime = snapshot_.parrySuccessRemainingSeconds;
        }
        return result;
    }

    const CombatPresentationSnapshot& Snapshot() const { return snapshot_; }

    void Reset() { snapshot_ = {}; }

private:
    CombatPresentationSnapshot snapshot_{};
};

} // namespace horde::gameplay::simulation
