#pragma once

#include "gameplay/simulation/CombatTeaching.h"

#include <cmath>
#include <cstdint>

namespace horde::gameplay::simulation
{

enum class AttackWarningPhase : std::uint8_t { None, Windup, Contact };

// Immutable description of the existing attack clock. Contact names the phase
// after the authoritative contact boundary, not a hit result or a one-tick pulse.
struct AttackWarningPresentation
{
    EntityId source = EntityId::Invalid;
    EnemyAttackKind kind = EnemyAttackKind::Normal;
    AttackWarningPhase phase = AttackWarningPhase::None;
    float phaseElapsedSeconds = 0.0f;
    float phaseDurationSeconds = 0.0f;
    // Attack-relative windup/contact boundary; elapsed above is phase-relative.
    float contactBoundarySeconds = 0.0f;
    bool operator==(const AttackWarningPresentation&) const = default;
};

inline AttackWarningPresentation BuildAttackWarningPresentation(
    const CombatSnapshot& combat, const LichSnapshot& keeper,
    EnemyKind selectedEnemy, EntityId skeletonSource, bool eligible)
{
    if (EvaluateAttackReadability(combat, keeper, selectedEnemy, eligible) ==
        AttackReadability::Neutral) return {};

    AttackWarningPresentation result;
    if (selectedEnemy == EnemyKind::Skeleton)
    {
        // Readability has validated the singular token, index, health and phase.
        const auto index = static_cast<std::size_t>(combat.attackerIndex);
        const auto expected = index == 0u ? EntityId::SkeletonA : EntityId::SkeletonB;
        if (skeletonSource != expected) return {};
        const auto& attacker = combat.combatants[index];
        result.source = expected;
        result.kind = attacker.attackKind;
        result.phase = attacker.action == EnemyCombatAction::AttackWindup
            ? AttackWarningPhase::Windup : AttackWarningPhase::Contact;
        result.phaseElapsedSeconds = attacker.actionTime;
        result.phaseDurationSeconds = result.phase == AttackWarningPhase::Windup
            ? CombatTimeline::kSkeletonAttackWindupSeconds
            : CombatTimeline::kSkeletonAttackActiveSeconds;
        result.contactBoundarySeconds = CombatTimeline::kSkeletonAttackWindupSeconds;
    }
    else if (selectedEnemy == EnemyKind::Lich)
    {
        result.source = EntityId::Lich;
        result.kind = EnemyAttackKind::Unblockable;
        result.phase = keeper.phase == LichPhase::Charging
            ? AttackWarningPhase::Windup : AttackWarningPhase::Contact;
        result.phaseElapsedSeconds = keeper.phaseTime;
        result.phaseDurationSeconds = result.phase == AttackWarningPhase::Windup
            ? LichEncounter::kChargeDuration : LichEncounter::kDischargeVisibleBurstDuration;
        result.contactBoundarySeconds = LichEncounter::kChargeDuration;
    }
    else return {};

    // Frozen source snapshots may retain the exact endpoint in their named
    // phase. Time never promotes Windup to Contact; only the source action does.
    // Do not clamp a malformed clock into a plausible warning or invent contact.
    if (!std::isfinite(result.phaseElapsedSeconds) || result.phaseElapsedSeconds < 0.0f ||
        result.phaseElapsedSeconds > result.phaseDurationSeconds) return {};
    return result;
}

} // namespace horde::gameplay::simulation
