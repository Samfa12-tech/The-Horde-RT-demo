#pragma once

#include <algorithm>
#include <cmath>
#include <cstdint>
#include "gameplay/CombatTimeline.h"
#include "gameplay/ShowcaseGameplay.h"
#include "gameplay/SwordCombat.h"
#include "gameplay/simulation/GameplayEvent.h"

namespace horde::gameplay::simulation
{
// Provisional gameplay seconds, independent of animation and wall-frame count.
// Half-open [2/60,10/60) leaves both start and landing exposed.
struct DodgeProtectionWindow
{
    static constexpr float kBeginSeconds = 2.0f / 60.0f;
    static constexpr float kEndSeconds = 10.0f / 60.0f;
    static bool Contains(float elapsedSeconds, bool acceptedAndMoving)
    {
        constexpr float epsilon = 0.000001f;
        return acceptedAndMoving && std::isfinite(elapsedSeconds) &&
            elapsedSeconds + epsilon >= kBeginSeconds &&
            elapsedSeconds + epsilon < kEndSeconds;
    }
};

enum class TutorialStage : std::uint8_t { Parry, Dodge, Complete, Skipped };
enum class CombatTeachingCue : std::uint8_t
{
    None, ParryWindup, ParryNow, ParryRecovery, DodgeWindup, DodgeNow, Recovery, Learned,
    ParryActive
};
struct CombatTeachingSnapshot
{
    bool enabled = false;
    bool parryLearned = false;
    bool dodgeLearned = false;
    bool safePractice = false;
    bool slowdownActive = false;
    TutorialStage stage = TutorialStage::Parry;
    CombatTeachingCue cue = CombatTeachingCue::None;
    EntityId source = EntityId::Invalid;
    float cueProgress = 0.0f;
    float simulationTimeScale = 1.0f;
    float promptOpacity = 0.0f;
    std::uint32_t practiceMisses = 0u;
    std::uint64_t generation = 1u;
};

// Lesson state belongs to the shared owner. UI consumes this snapshot only.
// Readable warnings survive skip/disable; teaching and safe practice do not.
class CombatTeaching
{
public:
    void Reset(bool enabled)
    {
        const auto generation = snapshot_.generation + 1u;
        snapshot_ = {}; snapshot_.enabled = enabled; snapshot_.generation = generation;
    }
    void Skip()
    {
        snapshot_.stage = TutorialStage::Skipped; snapshot_.safePractice = false;
        snapshot_.slowdownActive = false; snapshot_.simulationTimeScale = 1.0f;
        snapshot_.promptOpacity = 0.0f;
    }
    void ParrySucceeded() { snapshot_.parryLearned = true; CompleteIfLearned(); }
    void DodgeSucceeded() { snapshot_.dodgeLearned = true; CompleteIfLearned(); }
    void PracticeMiss() { if (snapshot_.practiceMisses != UINT32_MAX) ++snapshot_.practiceMisses; }
    void CancelTransient()
    {
        snapshot_.cue = CombatTeachingCue::None; snapshot_.source = EntityId::Invalid;
        snapshot_.slowdownActive = false; snapshot_.simulationTimeScale = 1.0f;
        snapshot_.safePractice = false; snapshot_.promptOpacity = 0.0f;
    }
    void Update(bool enabled, bool optionalSlowdown, bool suspended,
                const CombatSnapshot& combat, const LichSnapshot& keeper,
                EnemyKind selectedEnemy)
    {
        snapshot_.enabled = enabled;
        snapshot_.cue = CombatTeachingCue::None; snapshot_.source = EntityId::Invalid;
        snapshot_.cueProgress = 0.0f;
        snapshot_.simulationTimeScale = 1.0f;
        const bool practicing = enabled && snapshot_.stage != TutorialStage::Complete &&
            snapshot_.stage != TutorialStage::Skipped;
        snapshot_.safePractice = practicing;
        snapshot_.promptOpacity = practicing ? 1.0f : 0.0f;
        if (suspended) { CancelTransient(); return; }
        if (selectedEnemy == EnemyKind::Skeleton && combat.attackerIndex >= 0 &&
            static_cast<std::size_t>(combat.attackerIndex) < combat.combatants.size())
        {
            const auto& enemy = combat.combatants[static_cast<std::size_t>(combat.attackerIndex)];
            snapshot_.source = combat.attackerIndex == 0 ? EntityId::SkeletonA : EntityId::SkeletonB;
            if (enemy.action == EnemyCombatAction::AttackWindup)
            {
                constexpr float contact = CombatTimeline::kSkeletonAttackWindupSeconds;
                // The existing 40ms startup + 220ms active interval is not widened.
                snapshot_.cue = enemy.actionTime + 0.000001f >= contact - 0.26f &&
                    enemy.actionTime < contact - 0.04f
                    ? CombatTeachingCue::ParryNow : CombatTeachingCue::ParryWindup;
                snapshot_.cueProgress = std::clamp(enemy.actionTime / contact,0.0f,1.0f);
            }
            else if (enemy.action == EnemyCombatAction::AttackActive ||
                     enemy.action == EnemyCombatAction::AttackRecovery)
                snapshot_.cue = CombatTeachingCue::ParryRecovery;
            else if (enemy.action == EnemyCombatAction::Staggered)
                snapshot_.cue = CombatTeachingCue::Learned;
            if (combat.player.action == PlayerCombatAction::ParryActive)
                snapshot_.cue = CombatTeachingCue::ParryActive;
        }
        else if (selectedEnemy == EnemyKind::Lich && keeper.phase == LichPhase::Charging)
        {
            snapshot_.source = EntityId::Lich;
            const float remaining = LichEncounter::kChargeDuration - keeper.phaseTime;
            snapshot_.cue = remaining <= DodgeProtectionWindow::kEndSeconds &&
                remaining >= DodgeProtectionWindow::kBeginSeconds
                ? CombatTeachingCue::DodgeNow : CombatTeachingCue::DodgeWindup;
            snapshot_.cueProgress = std::clamp(keeper.phaseTime / LichEncounter::kChargeDuration,0.0f,1.0f);
        }
        else if (selectedEnemy == EnemyKind::Lich && keeper.phase == LichPhase::Recovering)
            snapshot_.cue = CombatTeachingCue::Recovery;
        // Input still reaches each fixed tick. All gameplay clocks, movement,
        // contact, defense and pose use the SAME scaled step delta. No shader or
        // animation-only dilation and no timestamp remapping are introduced.
        snapshot_.slowdownActive = practicing && optionalSlowdown &&
            ((snapshot_.cue == CombatTeachingCue::ParryNow && !snapshot_.parryLearned) ||
             (snapshot_.cue == CombatTeachingCue::DodgeNow && !snapshot_.dodgeLearned));
        if (snapshot_.slowdownActive) snapshot_.simulationTimeScale = 0.25f;
        const bool relevantUnlearnedCue =
            (selectedEnemy == EnemyKind::Skeleton && !snapshot_.parryLearned) ||
            (selectedEnemy == EnemyKind::Lich && !snapshot_.dodgeLearned);
        snapshot_.promptOpacity = practicing && relevantUnlearnedCue &&
            snapshot_.cue != CombatTeachingCue::None ? 1.0f : 0.0f;
    }
    const CombatTeachingSnapshot& Snapshot() const { return snapshot_; }
private:
    void CompleteIfLearned()
    {
        if (snapshot_.parryLearned && snapshot_.dodgeLearned) snapshot_.stage = TutorialStage::Complete;
        else if (snapshot_.parryLearned) snapshot_.stage = TutorialStage::Dodge;
    }
    CombatTeachingSnapshot snapshot_{};
};
} // namespace horde::gameplay::simulation
