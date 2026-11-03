#pragma once

#include <algorithm>
#include <cmath>
#include <cstdint>
#include "gameplay/CombatTimeline.h"
#include "gameplay/ShowcaseGameplay.h"
#include "gameplay/SwordCombat.h"
#include "gameplay/effects/KeeperCastPresentation.h"
#include "gameplay/simulation/GameplayEvent.h"

namespace horde::gameplay::simulation
{
// Gameplay seconds, independent of animation and wall-frame count.
// Half-open [2/60,11/60): one extra landing-side tick, with the first
// accepted motor tick and the final landing still exposed at60Hz.
struct DodgeProtectionWindow
{
    static constexpr float kBeginSeconds = 2.0f / 60.0f;
    static constexpr float kEndSeconds = 11.0f / 60.0f;
    static bool Contains(float elapsedSeconds, bool acceptedAndMoving)
    {
        constexpr float epsilon = 0.000001f;
        return acceptedAndMoving && std::isfinite(elapsedSeconds) &&
            elapsedSeconds + epsilon >= kBeginSeconds &&
            elapsedSeconds + epsilon < kEndSeconds;
    }
};

// Preserve the published stage values consumed by platform snapshots.
enum class TutorialStage : std::uint8_t { Parry=0, Dodge=1, Complete=2, Skipped=3, Block=4, AttackFromGuard=5, Ready=6 };
// Attack category, not a widened defense window or lesson state. The presentation
// owner maps these semantic values to neutral/yellow/red and readable text.
enum class AttackReadability : std::uint8_t { Neutral = 0, Parryable = 1, Unblockable = 2 };

inline AttackReadability EvaluateAttackReadability(const CombatSnapshot& combat,
    const LichSnapshot& keeper, EnemyKind selectedEnemy, bool eligible)
{
    if (!eligible) return AttackReadability::Neutral;
    if (selectedEnemy == EnemyKind::Skeleton)
    {
        if (combat.aliveCount == 0 || combat.attackerIndex < 0)
            return AttackReadability::Neutral;
        const auto index = static_cast<std::size_t>(combat.attackerIndex);
        if (index >= combat.combatantCount || index >= combat.combatants.size())
            return AttackReadability::Neutral;
        const auto& attacker = combat.combatants[index];
        return attacker.health > 0 &&
            (attacker.action == EnemyCombatAction::AttackWindup ||
             attacker.action == EnemyCombatAction::AttackActive)
            ? (attacker.attackKind == EnemyAttackKind::GuardBreak ? AttackReadability::Parryable :
               attacker.attackKind == EnemyAttackKind::Unblockable ? AttackReadability::Unblockable :
               AttackReadability::Neutral) : AttackReadability::Neutral;
    }
    if (selectedEnemy != EnemyKind::Lich || keeper.health <= 0 ||
        !keeper.revealComplete || IsKeeperRevealing(keeper.revealPhase))
        return AttackReadability::Neutral;
    if (keeper.phase == LichPhase::Charging)
        return AttackReadability::Unblockable;
    // Reuse the actual discharge presentation discriminator instead of owning
    // another burst timer. The remainder of the existing recovery is neutral.
    return keeper.phase == LichPhase::Recovering && keeper.readableCastPresentation &&
        effects::KeeperShaderPresentationKind(true, keeper) == 2.0f
        ? AttackReadability::Unblockable : AttackReadability::Neutral;
}

enum class CombatTeachingCue : std::uint8_t
{
    None, ParryWindup, ParryNow, ParryRecovery, DodgeWindup, DodgeNow, Recovery, Learned,
    ParryActive, Block, AttackFromGuard, Ready, Replay, ExitPending, Accepted, Retry
};
struct CombatTeachingSnapshot
{
    bool naturalContinuation = false;
    bool liveFightNotice = false; // Short shared transition notice, not permanent encounter status.
    std::uint8_t continuationPhase = 0;
    bool enabled = false;
    bool parryLearned = false;
    bool dodgeLearned = false;
    bool blockLearned = false;
    bool attackFromGuardLearned = false;
    bool exitPending = false;
    bool safePractice = false;
    // Presentation only: practice protection/progress do not depend on a HUD.
    bool lessonSuppressed = false;
    bool slowdownActive = false;
    TutorialStage stage = TutorialStage::Block;
    CombatTeachingCue cue = CombatTeachingCue::None;
    AttackReadability attackReadability = AttackReadability::Neutral;
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
    void CompleteNaturalPractice() {
        if(snapshot_.stage==TutorialStage::Ready && !snapshot_.exitPending) {
            snapshot_.stage=TutorialStage::Complete; CancelTransient();
        }
    }
    void SetNaturalContinuation(bool enabled) { natural_=enabled; }
    void SetSequentialPractice(bool enabled) { sequential_=enabled; }
    void Reset(bool enabled)
    {
        const auto generation = snapshot_.generation + 1u;
        snapshot_ = {}; snapshot_.enabled = enabled; snapshot_.generation = generation;
        ClearGuardAttack(); ClearDodge(); observedDodgeSequence_=0; successSeconds_=retrySeconds_=0.f; dodgeOnly_=false; requestedEnabled_=enabled; exitProtection_=false; openingExitSeconds_=0; openingExitRequested_=false;
    }
    void ResetForKeeperPractice(bool enabled) { Reset(enabled); dodgeOnly_=true; snapshot_.stage=TutorialStage::Dodge; }
    // The existing explicit Skip/Finish command requests an exit. Damage may
    // resume only at a safe attack boundary, never in a protected windup/contact.
    void Skip()
    {
        ClearGuardAttack();
        if (!LearningStage()) { snapshot_.stage=TutorialStage::Skipped; snapshot_.exitPending=false; exitProtection_=false; return; }
        snapshot_.exitPending = true;
        exitProtection_ = exitProtection_ || snapshot_.enabled || requestedEnabled_;
    }
    // Validated earned records are already past this encounter. Do not invent
    // learned flags or show practice protection while their first frame is paused.
    // Leaving the opening encounter ends both its protection and its prompt.
    // Explicit Keeper practice retains its separate dodge-only lesson.
    void LeaveOpeningEncounter() {
        if(!dodgeOnly_ && LearningStage() && !openingExitRequested_) {
            openingExitRequested_=true; openingExitSeconds_=3.f; Skip();
        }
    }
    void AdvanceOpeningExit(float seconds,bool paused) {
        if(!paused && std::isfinite(seconds) && seconds>0)
            {
            openingExitSeconds_=std::max(0.f,openingExitSeconds_-std::min(seconds,.05f));
            successSeconds_=std::max(0.f,successSeconds_-std::min(seconds,.05f));
            retrySeconds_=std::max(0.f,retrySeconds_-std::min(seconds,.05f));
        }
    }
    void RestoreEnded() { snapshot_.stage=TutorialStage::Skipped; snapshot_.exitPending=false; exitProtection_=false; CancelTransient(); }
    void BlockSucceeded() { if (Learning() && (!sequential_ || snapshot_.stage==TutorialStage::Block)) { snapshot_.blockLearned=true; successSeconds_=1.2f; SelectStage(); } }
    void ParrySucceeded() { if (Learning() && (!sequential_ || snapshot_.stage==TutorialStage::Parry)) { snapshot_.parryLearned=true; successSeconds_=1.2f; SelectStage(); } }
    void DodgeSucceeded() { if (Learning() && (!sequential_ || snapshot_.stage==TutorialStage::Dodge)) { snapshot_.dodgeLearned=true; successSeconds_=1.2f; SelectStage(); } }
    // A real dodge away from a nearby committed red attack also demonstrates
    // evasion. This does not widen damage invulnerability or accept remote rolls.
    void ObserveDodge(const CombatSnapshot& combat, float x, float z,
                      std::uint64_t sequence, bool movingDodge)
    {
        if (!Learning() || snapshot_.stage != TutorialStage::Dodge || snapshot_.exitPending)
        { ClearDodge(); return; }
        if (sequence != observedDodgeSequence_) {
            ClearDodge(); observedDodgeSequence_=sequence;
            if (!sequence || !movingDodge || combat.attackerIndex<0 ||
                static_cast<std::size_t>(combat.attackerIndex)>=combat.combatants.size()) return;
            const auto& actor=combat.combatants[combat.attackerIndex];
            const float remaining=CombatTimeline::kSkeletonAttackWindupSeconds-actor.actionTime;
            const bool threat=actor.action==EnemyCombatAction::AttackActive ||
                (actor.action==EnemyCombatAction::AttackWindup && remaining<=.55f);
            if (!threat || actor.health<=0 || actor.attackKind!=EnemyAttackKind::Unblockable ||
                std::hypot(x-actor.x,z-actor.z)>1.7f ||
                IsRouteAudioObstructed(x,z,actor.x,actor.z)) return;
            dodgeActor_=combat.attackerIndex; dodgeStartX_=x; dodgeStartZ_=z;
            dodgeThreatX_=actor.x; dodgeThreatZ_=actor.z;
        }
        if (dodgeActor_<0) return;
        const auto& actor=combat.combatants[dodgeActor_];
        if (actor.health<=0 || (actor.action!=EnemyCombatAction::AttackWindup &&
            actor.action!=EnemyCombatAction::AttackActive && actor.action!=EnemyCombatAction::AttackRecovery))
        { ClearDodge(); return; }
        if (movingDodge && std::hypot(x-dodgeStartX_,z-dodgeStartZ_)>=.25f &&
            std::hypot(x-dodgeThreatX_,z-dodgeThreatZ_)>1.55f)
            dodgeEscaped_=true;
        if (actor.action!=EnemyCombatAction::AttackWindup) {
            if (dodgeEscaped_ && std::hypot(x-actor.x,z-actor.z)>1.55f) DodgeSucceeded();
            ClearDodge();
        }
    }
    void BeginGuardAttack()
    {
        ClearGuardAttack();
        guardAttackPending_ = Learning() && !dodgeOnly_ && (!sequential_ || snapshot_.stage==TutorialStage::AttackFromGuard);
    }
    void ObserveGuardAttack(const CombatSnapshot& combat, bool held, bool interrupted, bool actualHit)
    {
        if (!guardAttackPending_) return;
        if (!held || interrupted) { ClearGuardAttack(); return; }
        if (guardAttackId_ == 0u && combat.activePlayerAttackId != 0u)
            guardAttackId_ = combat.activePlayerAttackId;
        if (combat.activePlayerAttackId != 0u && combat.activePlayerAttackId != guardAttackId_)
        { ClearGuardAttack(); return; }
        if (guardAttackId_ != 0u && actualHit && combat.activePlayerAttackId == guardAttackId_)
            guardAttackHit_ = true;
        if (guardAttackId_ != 0u && combat.activePlayerAttackId == 0u)
        {
            if (combat.player.action == PlayerCombatAction::Guard && guardAttackHit_ && Learning())
            { snapshot_.attackFromGuardLearned = true; successSeconds_=1.2f; SelectStage(); }
            ClearGuardAttack();
        }
    }
    void PracticeMiss() { if (snapshot_.practiceMisses != UINT32_MAX) ++snapshot_.practiceMisses; retrySeconds_=1.2f; }
    void CancelTransient()
    {
        snapshot_.cue = CombatTeachingCue::None; snapshot_.source = EntityId::Invalid;
        snapshot_.attackReadability = AttackReadability::Neutral;
        snapshot_.slowdownActive = false; snapshot_.simulationTimeScale = 1.0f;
        snapshot_.safePractice = false; snapshot_.promptOpacity = 0.0f;
        ClearGuardAttack(); ClearDodge(); successSeconds_=retrySeconds_=0.f;
    }
    void Update(bool enabled, bool optionalSlowdown, bool suspended,
                const CombatSnapshot& combat, const LichSnapshot& keeper,
                EnemyKind selectedEnemy, bool rulesActive = true)
    {
        // A legacy/frozen rule profile is eligibility, not a user exit. Keep
        // learned progress, requested preference and explicit pending Skip.
        if (!rulesActive) { snapshot_.enabled=false; CancelTransient(); return; }
        // Turning teaching off is also a deliberate exit, committed at the same
        // safe boundary. Keep the protection label until that acknowledgement.
        if (!enabled && requestedEnabled_ && LearningStage())
        { snapshot_.exitPending = true; exitProtection_=true; }
        requestedEnabled_=enabled;
        snapshot_.enabled = enabled || (snapshot_.exitPending && exitProtection_);
        if (snapshot_.exitPending && openingExitSeconds_<=0.f && !suspended && SafeExit(combat, keeper, selectedEnemy))
        {
            snapshot_.stage = snapshot_.stage == TutorialStage::Ready && !natural_ ? TutorialStage::Complete : TutorialStage::Skipped;
            snapshot_.exitPending = false; exitProtection_=false;
            snapshot_.enabled = enabled;
        }
        snapshot_.attackReadability = EvaluateAttackReadability(combat, keeper, selectedEnemy, !suspended);
        snapshot_.cue = CombatTeachingCue::None; snapshot_.source = EntityId::Invalid;
        snapshot_.cueProgress = 0.0f;
        snapshot_.simulationTimeScale = 1.0f;
        const bool practicing = snapshot_.enabled && snapshot_.stage != TutorialStage::Complete &&
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
            else if (enemy.action == EnemyCombatAction::Staggered &&
                     enemy.reaction == CombatReaction::Parried)
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
        if (practicing)
        {
            if (snapshot_.exitPending) snapshot_.cue = CombatTeachingCue::ExitPending;
            else if (snapshot_.stage == TutorialStage::Ready) snapshot_.cue = CombatTeachingCue::Ready;
            else if (!dodgeOnly_ && combat.aliveCount == 0)
                snapshot_.cue = CombatTeachingCue::Replay;
            else if (snapshot_.cue == CombatTeachingCue::Learned) { /* Actual parried stagger owns its success cue. */ }
            else if (snapshot_.stage == TutorialStage::Block) snapshot_.cue = CombatTeachingCue::Block;
            else if (snapshot_.stage == TutorialStage::AttackFromGuard) snapshot_.cue = CombatTeachingCue::AttackFromGuard;
            else if (snapshot_.stage == TutorialStage::Dodge && selectedEnemy == EnemyKind::Skeleton)
            {
                snapshot_.cue = CombatTeachingCue::DodgeWindup;
                if (combat.attackerIndex >= 0 && static_cast<std::size_t>(combat.attackerIndex) < combat.combatants.size())
                {
                    const auto& attacker = combat.combatants[static_cast<std::size_t>(combat.attackerIndex)];
                    const float remaining = CombatTimeline::kSkeletonAttackWindupSeconds-attacker.actionTime;
                    if (attacker.action == EnemyCombatAction::AttackWindup && remaining >= DodgeProtectionWindow::kBeginSeconds &&
                        remaining <= DodgeProtectionWindow::kEndSeconds) snapshot_.cue=CombatTeachingCue::DodgeNow;
                }
            }
        }
        if (practicing && sequential_ && retrySeconds_>0.f && !snapshot_.exitPending &&
            snapshot_.stage!=TutorialStage::Ready && snapshot_.cue!=CombatTeachingCue::Learned &&
            snapshot_.cue!=CombatTeachingCue::ParryNow && snapshot_.cue!=CombatTeachingCue::DodgeNow) snapshot_.cue=CombatTeachingCue::Retry;
        if (practicing && sequential_ && successSeconds_>0.f && !snapshot_.exitPending &&
            snapshot_.stage!=TutorialStage::Ready && snapshot_.cue!=CombatTeachingCue::Learned &&
            snapshot_.cue!=CombatTeachingCue::ParryNow && snapshot_.cue!=CombatTeachingCue::DodgeNow) snapshot_.cue=CombatTeachingCue::Accepted;
        snapshot_.slowdownActive = practicing && optionalSlowdown &&
            ((snapshot_.stage == TutorialStage::Parry && snapshot_.cue == CombatTeachingCue::ParryNow && !snapshot_.parryLearned) ||
             (snapshot_.stage == TutorialStage::Dodge && snapshot_.cue == CombatTeachingCue::DodgeNow && !snapshot_.dodgeLearned));
        if (snapshot_.slowdownActive) snapshot_.simulationTimeScale = 0.25f;
        // Keep safe-practice status and the current skill visible even during
        // attack warnings. Presenters compose these; they must not early-return.
        const bool parrySuccessCue = snapshot_.enabled && snapshot_.stage != TutorialStage::Skipped &&
            snapshot_.cue == CombatTeachingCue::Learned;
        snapshot_.promptOpacity = practicing || parrySuccessCue ? 1.0f : 0.0f;
    }
    const CombatTeachingSnapshot& Snapshot() const { return snapshot_; }
private:
    bool LearningStage() const
    { return snapshot_.stage != TutorialStage::Complete && snapshot_.stage != TutorialStage::Skipped; }
    bool Learning() const { return snapshot_.enabled && LearningStage(); }
    void SelectStage()
    {
        if (dodgeOnly_) { snapshot_.stage=snapshot_.dodgeLearned ? TutorialStage::Ready : TutorialStage::Dodge; return; }
        snapshot_.stage = !snapshot_.blockLearned ? TutorialStage::Block :
            !snapshot_.parryLearned ? TutorialStage::Parry :
            !snapshot_.dodgeLearned ? TutorialStage::Dodge :
            !snapshot_.attackFromGuardLearned ? TutorialStage::AttackFromGuard : TutorialStage::Ready;
    }
    static bool SafeExit(const CombatSnapshot& combat, const LichSnapshot& keeper, EnemyKind selected)
    {
        if (combat.player.action != PlayerCombatAction::Idle && combat.player.action != PlayerCombatAction::Guard) return false;
        for (const auto& actor : combat.combatants)
            if (actor.health > 0 && (actor.action == EnemyCombatAction::AttackWindup || actor.action == EnemyCombatAction::AttackActive)) return false;
        return selected != EnemyKind::Lich || (keeper.phase != LichPhase::Charging &&
            !(keeper.phase == LichPhase::Recovering && keeper.readableCastPresentation &&
              effects::KeeperShaderPresentationKind(true, keeper) == 2.0f));
    }
    void ClearDodge() { dodgeActor_=-1; dodgeEscaped_=false; }
    int dodgeActor_=-1;
    std::uint64_t observedDodgeSequence_=0;
    bool dodgeEscaped_=false;
    float dodgeStartX_=0,dodgeStartZ_=0,dodgeThreatX_=0,dodgeThreatZ_=0,successSeconds_=0,retrySeconds_=0;
    void ClearGuardAttack() { guardAttackPending_=guardAttackHit_=false; guardAttackId_=0; }
    // User intent and a protected pending exit survive inactive rule profiles;
    // neither is the published active-teaching flag used by frozen captures.
    float openingExitSeconds_=0.f;
    bool openingExitRequested_=false;
    bool sequential_=false, natural_=false;
    bool requestedEnabled_=false, exitProtection_=false;
    bool dodgeOnly_=false; // Existing explicit Keeper practice, not story progress.
    bool guardAttackPending_=false, guardAttackHit_=false;
    std::uint64_t guardAttackId_=0;
    CombatTeachingSnapshot snapshot_{};
};
} // namespace horde::gameplay::simulation
