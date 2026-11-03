#pragma once

#include "gameplay/simulation/GameplayEvent.h"

namespace horde::gameplay
{
// Type-scoped IDs fit the existing Android semantic-cue byte. These are audio
// choices for real outcomes, not another combat/contact authority.
enum class CombatAudioCue : std::uint8_t
{
    None = 0, GuardBlock = 1, SkeletonHit = 2, KeeperHit = 3
};
static_assert(static_cast<unsigned>(simulation::GameplayEventType::EnemyHit) == 6u);
static_assert(static_cast<unsigned>(simulation::GameplayEventType::PlayerGuardBlocked) == 37u);

constexpr CombatAudioCue CombatCueForEvent(const simulation::GameplayEvent& event,
                                         bool gameplayActive = true) noexcept
{
    using namespace simulation;
    if (!gameplayActive || event.source != EntityId::Player) return CombatAudioCue::None;
    const bool skeleton = event.target == EntityId::SkeletonA || event.target == EntityId::SkeletonB;
    if (event.type == GameplayEventType::PlayerGuardBlocked)
        return skeleton ? CombatAudioCue::GuardBlock : CombatAudioCue::None;
    if (event.type != GameplayEventType::EnemyHit) return CombatAudioCue::None;
    if (skeleton) return CombatAudioCue::SkeletonHit;
    return event.target == EntityId::Lich ? CombatAudioCue::KeeperHit : CombatAudioCue::None;
}

constexpr const char* CombatAudioClip(CombatAudioCue cue) noexcept
{
    switch (cue)
    {
    case CombatAudioCue::GuardBlock:
    case CombatAudioCue::SkeletonHit: return "sword_hit_1.wav";
    case CombatAudioCue::KeeperHit: return "lich_hurt.wav";
    default: return nullptr;
    }
}
} // namespace horde::gameplay
