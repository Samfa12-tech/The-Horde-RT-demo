#pragma once

#include <algorithm>
#include <array>
#include <cmath>
#include <cstddef>
#include <cstdint>

#include "gameplay/CorridorCollision.h"

namespace horde::gameplay
{

enum class EnemyAnimation
{
    Idle,
    Walking,
    Attack,
    Dead
};

enum class PlayerCombatAction : std::uint8_t
{
    Idle,
    SwingWindup,
    SwingActive,
    SwingRecovery,
    UpwardSliceWindup,
    UpwardSliceActive,
    UpwardSliceRecovery,
    ParryStartup,
    ParryActive,
    ParryRecovery,
};

enum class PlayerAttackCut : std::uint8_t
{
    None,
    DownwardCut,
    UpwardSlice,
};

enum class EnemyCombatAction : std::uint8_t
{
    Locomotion,
    AttackWindup,
    AttackActive,
    AttackRecovery,
    Staggered,
    Dead,
};

enum class CombatReaction : std::uint8_t
{
    None,
    Hit,
    Parried,
};

struct PlayerCombatSnapshot
{
    PlayerCombatAction action = PlayerCombatAction::Idle;
    CombatReaction reaction = CombatReaction::None;
    float actionTime = 0.0f;
    float reactionTime = 0.0f;
    bool comboQueued = false;
};

inline constexpr std::size_t kSkeletonCombatantCapacity = 2u;

struct SkeletonCombatantSnapshot
{
    float x = 0.0f;
    float z = -4.65f;
    float facingRadians = 0.0f;
    float animationTime = 0.0f;
    float damageFlash = 0.0f;
    std::int32_t health = 1;
    EnemyAnimation animation = EnemyAnimation::Walking;
    EnemyCombatAction action = EnemyCombatAction::Locomotion;
    CombatReaction reaction = CombatReaction::None;
    float actionTime = 0.0f;
    float reactionTime = 0.0f;
    bool playerHitPulse = false;
    bool parrySuccessPulse = false;
};

struct CombatSnapshot
{
    float swordSwingRadians = 0.0f;
    PlayerCombatSnapshot player{};
    bool playerAttackPulse = false;
    PlayerAttackCut playerAttackCut = PlayerAttackCut::None;
    std::int32_t parriedAttackerIndex = -1;

    // Compatibility view of Skeleton A. New consumers should use combatants.
    float enemyX = -0.75f;
    float enemyZ = -4.65f;
    float enemyFacingRadians = 0.0f;
    float enemyAnimationTime = 0.0f;
    float damageFlash = 0.0f;
    bool playerHitPulse = false;
    EnemyAnimation enemyAnimation = EnemyAnimation::Walking;

    std::array<SkeletonCombatantSnapshot, kSkeletonCombatantCapacity> combatants{};
    std::size_t combatantCount = kSkeletonCombatantCapacity;
    std::size_t aliveCount = kSkeletonCombatantCapacity;
    std::int32_t attackerIndex = -1;
    bool encounterComplete = false;
};

// A bounded two-enemy encounter. Rendering consumes only this immutable
// snapshot; hit tests, deterministic attack ownership, collision, and encounter
// persistence remain gameplay concerns.
class SwordCombat
{
public:
    SwordCombat()
    {
        Reset();
    }

    void Reset(std::size_t combatantCount = kSkeletonCombatantCapacity)
    {
        combatantCount_ = std::clamp<std::size_t>(combatantCount, 1u, kSkeletonCombatantCapacity);
        combatants_ = {};
        for (Combatant& combatant : combatants_)
        {
            combatant.health = 0;
            combatant.action = EnemyCombatAction::Dead;
            combatant.animation = EnemyAnimation::Dead;
            combatant.walkAnimationHold = 0.0f;
        }
        combatants_[0].x = combatantCount_ == 1u ? 0.0f : -0.75f;
        combatants_[0].z = -4.65f;
        combatants_[1].x = 0.75f;
        combatants_[1].z = -4.65f;
        for (std::size_t index = 0u; index < combatantCount_; ++index)
        {
            Combatant& combatant = combatants_[index];
            combatant.action = EnemyCombatAction::Locomotion;
            combatant.health = 1;
            combatant.walkAnimationHold = kWalkAnimationHold;
            combatant.animation = EnemyAnimation::Walking;
        }
        snapshot_ = {};
        attackQueued_ = false;
        parryQueued_ = false;
        player_ = {};
        successfulParryEndsNextTick_ = false;
        attackerIndex_ = -1;
        PublishSnapshot();
    }

    PlayerAttackCut RequestAttack()
    {
        if (CanAcceptUpwardSlice())
        {
            player_.comboQueued = true;
            return PlayerAttackCut::UpwardSlice;
        }
        if (player_.action == PlayerCombatAction::Idle || successfulParryEndsNextTick_)
        {
            attackQueued_ = true;
            return PlayerAttackCut::DownwardCut;
        }
        return PlayerAttackCut::None;
    }

    void RequestParry()
    {
        parryQueued_ = true;
    }

    bool CanAcceptPlayerAction() const
    {
        return player_.action == PlayerCombatAction::Idle || successfulParryEndsNextTick_;
    }

    bool CanAcceptAttack() const
    {
        return CanAcceptPlayerAction() || CanAcceptUpwardSlice();
    }

    bool CanAcceptParry() const
    {
        return CanAcceptPlayerAction();
    }

    static bool IsPlayerTargetInRangeCone(float playerX,
                                          float playerZ,
                                          float playerYaw,
                                          float targetX,
                                          float targetZ)
    {
        const float dx = targetX - playerX;
        const float dz = targetZ - playerZ;
        const float distance = std::hypot(dx, dz);
        if (distance > kPlayerHitRange)
        {
            return false;
        }
        if (distance <= 0.0001f)
        {
            return true;
        }
        const float forwardX = std::sin(playerYaw);
        const float forwardZ = -std::cos(playerYaw);
        return (dx / distance) * forwardX + (dz / distance) * forwardZ >= kPlayerHitConeDot;
    }

    std::int32_t PlayerSwingTargetIndex(float playerX, float playerZ, float playerYaw) const
    {
        std::int32_t targetIndex = -1;
        float targetDistance = 0.0f;
        for (std::size_t index = 0; index < combatantCount_; ++index)
        {
            const Combatant& combatant = combatants_[index];
            const float distance = std::hypot(combatant.x - playerX, combatant.z - playerZ);
            if (combatant.health <= 0 ||
                !IsPlayerTargetInRangeCone(playerX, playerZ, playerYaw, combatant.x, combatant.z))
            {
                continue;
            }
            if (targetIndex < 0 || distance < targetDistance)
            {
                targetIndex = static_cast<std::int32_t>(index);
                targetDistance = distance;
            }
        }
        return targetIndex;
    }

    const CombatSnapshot& Update(float deltaSeconds, float playerX, float playerZ, float playerYaw)
    {
        deltaSeconds = std::clamp(deltaSeconds, 0.0f, 0.05f);
        snapshot_.playerAttackPulse = false;
        snapshot_.playerAttackCut = PlayerAttackCut::None;
        snapshot_.parriedAttackerIndex = -1;
        if (successfulParryEndsNextTick_)
        {
            player_ = {};
            successfulParryEndsNextTick_ = false;
        }
        for (Combatant& combatant : combatants_)
        {
            combatant.playerHitPulse = false;
            combatant.parrySuccessPulse = false;
            combatant.damageFlash = std::max(0.0f, combatant.damageFlash - deltaSeconds * 2.8f);
            combatant.walkAnimationHold = std::max(0.0f, combatant.walkAnimationHold - deltaSeconds);
        }

        if (player_.action == PlayerCombatAction::Idle)
        {
            if (attackQueued_)
            {
                player_.action = PlayerCombatAction::SwingWindup;
                player_.actionTime = 0.0f;
            }
            else if (parryQueued_)
            {
                player_.action = PlayerCombatAction::ParryStartup;
                player_.actionTime = 0.0f;
            }
        }
        attackQueued_ = false;
        parryQueued_ = false;
        UpdatePlayerAction(deltaSeconds, playerX, playerZ, playerYaw);

        const ShowcaseZone playerZone = QueryShowcaseZone(playerX, playerZ);
        const bool playerInsideEnemyArena = playerZone == ShowcaseZone::Opening ||
                                            playerZone == ShowcaseZone::SkeletonRoom;

        std::array<float, kSkeletonCombatantCapacity> distances{};
        const std::array<RoutePosition, kSkeletonCombatantCapacity> previousPositions{{
            {combatants_[0].x, combatants_[0].z},
            {combatants_[1].x, combatants_[1].z},
        }};
        for (std::size_t index = 0; index < combatantCount_; ++index)
        {
            Combatant& combatant = combatants_[index];
            const float toPlayerX = playerX - combatant.x;
            const float toPlayerZ = playerZ - combatant.z;
            distances[index] = std::hypot(toPlayerX, toPlayerZ);
            if (combatant.health > 0 && playerInsideEnemyArena && distances[index] > 0.0001f)
            {
                // The staged skeleton's authored forward direction is +Z.
                combatant.facing = std::atan2(toPlayerX, toPlayerZ);
            }
        }

        SelectAttacker(distances, playerInsideEnemyArena);
        for (std::size_t index = 0; index < combatantCount_; ++index)
        {
            UpdateCombatant(index,
                            deltaSeconds,
                            playerX,
                            playerZ,
                            distances[index],
                            playerInsideEnemyArena,
                            playerYaw);
        }
        ResolveCombatantSeparation(previousPositions);
        PublishSnapshot();
        return snapshot_;
    }

    const CombatSnapshot& Snapshot() const { return snapshot_; }

private:
    struct Combatant
    {
        float x = 0.0f;
        float z = -4.65f;
        float facing = 0.0f;
        float phaseTime = 0.0f;
        float animationTime = 0.0f;
        float damageFlash = 0.0f;
        float walkAnimationHold = kWalkAnimationHold;
        std::int32_t health = 1;
        EnemyCombatAction action = EnemyCombatAction::Locomotion;
        CombatReaction reaction = CombatReaction::None;
        float reactionTime = 0.0f;
        EnemyAnimation animation = EnemyAnimation::Walking;
        bool playerHitPulse = false;
        bool parrySuccessPulse = false;
    };

    void UpdatePlayerAction(float deltaSeconds, float playerX, float playerZ, float playerYaw)
    {
        player_.reactionTime = std::max(0.0f, player_.reactionTime - deltaSeconds);
        if (player_.reactionTime <= 0.0f)
        {
            player_.reaction = CombatReaction::None;
        }
        player_.actionTime += deltaSeconds;
        switch (player_.action)
        {
        case PlayerCombatAction::SwingWindup:
            if (player_.actionTime >= kSwingWindupDuration)
            {
                player_.action = PlayerCombatAction::SwingActive;
                player_.actionTime -= kSwingWindupDuration;
                snapshot_.playerAttackPulse = true;
                snapshot_.playerAttackCut = PlayerAttackCut::DownwardCut;
                ResolveSwordHit(playerX, playerZ, playerYaw);
            }
            break;
        case PlayerCombatAction::SwingActive:
            if (player_.comboQueued &&
                player_.actionTime >= kDownwardCutTravelDuration)
            {
                // Once the blade reaches the bottom, an early buffered edge
                // or a later human edge starts from the same exact contact
                // pose. Do not carry time spent in the input hold into the
                // upward wind-up or the continuation would visibly skip.
                player_.comboQueued = false;
                player_.action = PlayerCombatAction::UpwardSliceWindup;
                player_.actionTime = 0.0f;
            }
            else if (player_.actionTime >= kSwingActiveDuration)
            {
                player_.actionTime -= kSwingActiveDuration;
                player_.action = PlayerCombatAction::SwingRecovery;
            }
            break;
        case PlayerCombatAction::SwingRecovery:
            if (player_.actionTime >= kSwingRecoveryDuration)
            {
                player_ = {};
            }
            break;
        case PlayerCombatAction::UpwardSliceWindup:
            if (player_.actionTime >= kUpwardSliceWindupDuration)
            {
                player_.action = PlayerCombatAction::UpwardSliceActive;
                player_.actionTime -= kUpwardSliceWindupDuration;
                snapshot_.playerAttackPulse = true;
                snapshot_.playerAttackCut = PlayerAttackCut::UpwardSlice;
                ResolveSwordHit(playerX, playerZ, playerYaw);
            }
            break;
        case PlayerCombatAction::UpwardSliceActive:
            if (player_.actionTime >= kUpwardSliceActiveDuration)
            {
                player_.action = PlayerCombatAction::UpwardSliceRecovery;
                player_.actionTime -= kUpwardSliceActiveDuration;
            }
            break;
        case PlayerCombatAction::UpwardSliceRecovery:
            if (player_.actionTime >= kUpwardSliceRecoveryDuration)
            {
                player_ = {};
            }
            break;
        case PlayerCombatAction::ParryStartup:
            if (player_.actionTime >= kParryStartupDuration)
            {
                player_.action = PlayerCombatAction::ParryActive;
                player_.actionTime -= kParryStartupDuration;
            }
            break;
        case PlayerCombatAction::ParryActive:
            if (player_.actionTime >= kParryActiveDuration)
            {
                player_.action = PlayerCombatAction::ParryRecovery;
                player_.actionTime -= kParryActiveDuration;
            }
            break;
        case PlayerCombatAction::ParryRecovery:
            if (player_.actionTime >= kParryRecoveryDuration)
            {
                player_ = {};
            }
            break;
        case PlayerCombatAction::Idle:
            player_.actionTime = 0.0f;
            break;
        }

        float swingElapsed = 0.0f;
        if (player_.action == PlayerCombatAction::SwingWindup)
        {
            swingElapsed = player_.actionTime;
        }
        else if (player_.action == PlayerCombatAction::SwingActive)
        {
            swingElapsed = kSwingWindupDuration + player_.actionTime;
        }
        else if (player_.action == PlayerCombatAction::SwingRecovery)
        {
            swingElapsed = kSwingWindupDuration + kSwingActiveDuration + player_.actionTime;
        }
        if (player_.action == PlayerCombatAction::UpwardSliceWindup)
        {
            const float amount = SmoothStep(player_.actionTime / kUpwardSliceWindupDuration);
            snapshot_.swordSwingRadians =
                kUpwardSliceTransitionRadians +
                (kUpwardSliceStartRadians - kUpwardSliceTransitionRadians) * amount;
        }
        else if (player_.action == PlayerCombatAction::UpwardSliceActive)
        {
            const float amount = SmoothStep(player_.actionTime / kUpwardSliceActiveDuration);
            snapshot_.swordSwingRadians = kUpwardSliceStartRadians +
                (kUpwardSliceEndRadians - kUpwardSliceStartRadians) * amount;
        }
        else if (player_.action == PlayerCombatAction::UpwardSliceRecovery)
        {
            const float amount = SmoothStep(player_.actionTime / kUpwardSliceRecoveryDuration);
            snapshot_.swordSwingRadians = kUpwardSliceEndRadians * (1.0f - amount);
        }
        else
        {
            snapshot_.swordSwingRadians = swingElapsed > 0.0f
                ? -kDownwardSwingAmplitude *
                    std::sin((swingElapsed / kSwordDuration) * 3.14159265f)
                : 0.0f;
        }
    }

    bool CanAcceptUpwardSlice() const
    {
        // The second press is a buffered continuation of the visible
        // downward action, not a 160 ms active-frame quick-time event. Keep
        // the actual upward transition at the bottom of the cut, then retain
        // a bounded contact hold so an ordinary 400 ms human click/tap is not
        // acknowledged and silently discarded.
        const bool downwardAction =
            player_.action == PlayerCombatAction::SwingWindup ||
            player_.action == PlayerCombatAction::SwingActive;
        return downwardAction && !player_.comboQueued;
    }

    static float SmoothStep(const float value)
    {
        const float amount = std::clamp(value, 0.0f, 1.0f);
        return amount * amount * (3.0f - 2.0f * amount);
    }

    void SelectAttacker(const std::array<float, kSkeletonCombatantCapacity>& distances,
                        bool playerInsideEnemyArena)
    {
        if (!playerInsideEnemyArena)
        {
            attackerIndex_ = -1;
            return;
        }

        if (attackerIndex_ >= 0)
        {
            const Combatant& holder = combatants_[static_cast<std::size_t>(attackerIndex_)];
            if (holder.health > 0 && holder.action != EnemyCombatAction::Locomotion &&
                holder.action != EnemyCombatAction::Dead)
            {
                return;
            }
        }

        attackerIndex_ = -1;
        float nearestDistance = 0.0f;
        for (std::size_t index = 0; index < combatantCount_; ++index)
        {
            if (combatants_[index].health <= 0)
            {
                continue;
            }
            if (attackerIndex_ < 0 || distances[index] < nearestDistance)
            {
                attackerIndex_ = static_cast<std::int32_t>(index);
                nearestDistance = distances[index];
            }
            // Equal distances intentionally retain the lower stable entity ID.
        }
    }

    void ResolveSwordHit(float playerX, float playerZ, float playerYaw)
    {
        const std::int32_t targetIndex = PlayerSwingTargetIndex(playerX, playerZ, playerYaw);
        if (targetIndex < 0)
        {
            return;
        }
        Combatant& target = combatants_[static_cast<std::size_t>(targetIndex)];
        target.health = 0;
        target.action = EnemyCombatAction::Dead;
        target.phaseTime = 0.0f;
        target.animationTime = 0.0f;
        target.animation = EnemyAnimation::Dead;
        target.reaction = CombatReaction::Hit;
        target.reactionTime = 1.0f;
        if (attackerIndex_ == targetIndex)
        {
            attackerIndex_ = -1;
        }
    }

    void UpdateCombatant(std::size_t index,
                         float deltaSeconds,
                         float playerX,
                         float playerZ,
                         float distance,
                         bool playerInsideEnemyArena,
                         float playerYaw)
    {
        Combatant& combatant = combatants_[index];
        combatant.phaseTime += deltaSeconds;
        combatant.animationTime += deltaSeconds;
        combatant.reactionTime = std::max(0.0f, combatant.reactionTime - deltaSeconds);
        if (combatant.reactionTime <= 0.0f)
        {
            combatant.reaction = CombatReaction::None;
        }
        if (combatant.health <= 0 || combatant.action == EnemyCombatAction::Dead)
        {
            combatant.health = 0;
            combatant.action = EnemyCombatAction::Dead;
            combatant.animation = EnemyAnimation::Dead;
            return;
        }

        if (!playerInsideEnemyArena)
        {
            combatant.walkAnimationHold = 0.0f;
            combatant.action = EnemyCombatAction::Locomotion;
            combatant.phaseTime = 0.0f;
            combatant.animationTime = 0.0f;
            combatant.animation = EnemyAnimation::Idle;
            return;
        }

        const bool ownsAttackToken = attackerIndex_ == static_cast<std::int32_t>(index);
        if (!ownsAttackToken && combatant.action != EnemyCombatAction::Locomotion &&
            combatant.action != EnemyCombatAction::Dead)
        {
            combatant.action = EnemyCombatAction::Locomotion;
            combatant.phaseTime = 0.0f;
            combatant.animationTime = 0.0f;
        }

        if (combatant.action == EnemyCombatAction::Locomotion)
        {
            if (ownsAttackToken && distance <= kEnemyAttackEnterRange)
            {
                combatant.walkAnimationHold = 0.0f;
                combatant.action = EnemyCombatAction::AttackWindup;
                combatant.phaseTime = 0.0f;
                combatant.animationTime = 0.0f;
                combatant.animation = EnemyAnimation::Attack;
            }
            else if (distance > kEnemyAttackRange)
            {
                const float step = std::min(distance - kEnemyAttackRange, kEnemyWalkSpeed * deltaSeconds);
                float proposedX = combatant.x + (playerX - combatant.x) /
                                  std::max(distance, 0.0001f) * step;
                float proposedZ = combatant.z + (playerZ - combatant.z) /
                                  std::max(distance, 0.0001f) * step;
                ResolveSkeletonEnemyCollision(combatant.x, combatant.z, proposedX, proposedZ);
                const bool moved = std::abs(proposedX - combatant.x) > 0.00001f ||
                                   std::abs(proposedZ - combatant.z) > 0.00001f;
                combatant.x = proposedX;
                combatant.z = proposedZ;
                if (moved)
                {
                    combatant.walkAnimationHold = kWalkAnimationHold;
                }
                combatant.animation = combatant.walkAnimationHold > 0.0f
                    ? EnemyAnimation::Walking
                    : EnemyAnimation::Idle;
            }
            else
            {
                combatant.walkAnimationHold = 0.0f;
                combatant.animation = EnemyAnimation::Idle;
            }
            return;
        }

        combatant.animation = EnemyAnimation::Attack;
        if (combatant.action == EnemyCombatAction::AttackWindup &&
            combatant.phaseTime >= kEnemyAttackWindupDuration)
        {
            combatant.action = EnemyCombatAction::AttackActive;
            combatant.phaseTime -= kEnemyAttackWindupDuration;
            const bool inRange = distance <= kEnemyDamageRange;
            if (inRange && player_.action == PlayerCombatAction::ParryActive &&
                IsPlayerTargetInRangeCone(playerX, playerZ, playerYaw, combatant.x, combatant.z))
            {
                combatant.action = EnemyCombatAction::Staggered;
                combatant.phaseTime = 0.0f;
                combatant.reaction = CombatReaction::Parried;
                combatant.reactionTime = kEnemyStaggerDuration;
                combatant.parrySuccessPulse = true;
                player_.action = PlayerCombatAction::ParryRecovery;
                player_.actionTime = 0.0f;
                player_.reaction = CombatReaction::Parried;
                player_.reactionTime = kParryJoltDuration;
                successfulParryEndsNextTick_ = true;
                snapshot_.parriedAttackerIndex = static_cast<std::int32_t>(index);
                return;
            }
            if (inRange)
            {
                combatant.damageFlash = 1.0f;
                combatant.playerHitPulse = true;
            }
        }
        if (combatant.action == EnemyCombatAction::AttackActive &&
            combatant.phaseTime >= kEnemyAttackActiveDuration)
        {
            combatant.action = EnemyCombatAction::AttackRecovery;
            combatant.phaseTime -= kEnemyAttackActiveDuration;
        }
        if (combatant.action == EnemyCombatAction::AttackRecovery &&
            combatant.phaseTime >= kEnemyAttackRecoveryDuration)
        {
            FinishEnemyAction(combatant);
        }
        if (combatant.action == EnemyCombatAction::Staggered &&
            combatant.phaseTime >= kEnemyStaggerDuration)
        {
            FinishEnemyAction(combatant);
        }
    }

    void FinishEnemyAction(Combatant& combatant)
    {
        combatant.action = EnemyCombatAction::Locomotion;
        combatant.phaseTime = 0.0f;
        combatant.animationTime = 0.0f;
        attackerIndex_ = -1;
    }

    void ResolveCombatantSeparation(
        const std::array<RoutePosition, kSkeletonCombatantCapacity>& previousPositions)
    {
        Combatant& first = combatants_[0];
        Combatant& second = combatants_[1];
        if (combatantCount_ < 2u || (first.health <= 0 && second.health <= 0))
        {
            return;
        }
        float dx = second.x - first.x;
        float dz = second.z - first.z;
        float distance = std::hypot(dx, dz);
        if (distance >= kMinimumSeparation)
        {
            return;
        }
        const bool coincident = distance <= 0.00001f;
        const float unitX = coincident ? 1.0f : dx / distance;
        const float unitZ = coincident ? 0.0f : dz / distance;
        const float correction = (kMinimumSeparation - distance) * 0.5f;
        const bool firstFixed = first.health <= 0 || first.action == EnemyCombatAction::Staggered;
        const bool secondFixed = second.health <= 0 || second.action == EnemyCombatAction::Staggered;
        if (firstFixed)
        {
            const float targetX = first.x + unitX * kMinimumSeparation;
            const float targetZ = first.z + unitZ * kMinimumSeparation;
            if (IsSkeletonEnemyWalkableSweep({second.x, second.z}, {targetX, targetZ}))
            {
                second.x = targetX;
                second.z = targetZ;
            }
            else
            {
                second.x = previousPositions[1].x;
                second.z = previousPositions[1].z;
            }
            return;
        }
        if (secondFixed)
        {
            const float targetX = second.x - unitX * kMinimumSeparation;
            const float targetZ = second.z - unitZ * kMinimumSeparation;
            if (IsSkeletonEnemyWalkableSweep({first.x, first.z}, {targetX, targetZ}))
            {
                first.x = targetX;
                first.z = targetZ;
            }
            else
            {
                first.x = previousPositions[0].x;
                first.z = previousPositions[0].z;
            }
            return;
        }
        const float firstX = first.x - unitX * correction;
        const float firstZ = first.z - unitZ * correction;
        const float secondX = second.x + unitX * correction;
        const float secondZ = second.z + unitZ * correction;
        if (IsSkeletonEnemyWalkableSweep({first.x, first.z}, {firstX, firstZ}) &&
            IsSkeletonEnemyWalkableSweep({second.x, second.z}, {secondX, secondZ}))
        {
            first.x = firstX;
            first.z = firstZ;
            second.x = secondX;
            second.z = secondZ;
            return;
        }

        // Stable-ID fallback: keep A fixed and move B. If that is blocked, keep
        // B fixed and move A. Both routes remain deterministic and world-valid.
        const float pushedSecondX = first.x + unitX * kMinimumSeparation;
        const float pushedSecondZ = first.z + unitZ * kMinimumSeparation;
        if (IsSkeletonEnemyWalkableSweep({second.x, second.z}, {pushedSecondX, pushedSecondZ}))
        {
            second.x = pushedSecondX;
            second.z = pushedSecondZ;
            return;
        }
        const float pushedFirstX = second.x - unitX * kMinimumSeparation;
        const float pushedFirstZ = second.z - unitZ * kMinimumSeparation;
        if (IsSkeletonEnemyWalkableSweep({first.x, first.z}, {pushedFirstX, pushedFirstZ}))
        {
            first.x = pushedFirstX;
            first.z = pushedFirstZ;
            return;
        }

        // Both candidate corrections were world-blocked. Revert the pair to
        // the prior valid tick rather than allowing overlap.
        first.x = previousPositions[0].x;
        first.z = previousPositions[0].z;
        second.x = previousPositions[1].x;
        second.z = previousPositions[1].z;
    }

    void PublishSnapshot()
    {
        snapshot_.aliveCount = 0u;
        snapshot_.encounterComplete = true;
        snapshot_.combatantCount = combatantCount_;
        snapshot_.attackerIndex = attackerIndex_;
        snapshot_.player = player_;
        snapshot_.playerHitPulse = false;
        for (std::size_t index = 0; index < snapshot_.combatants.size(); ++index)
        {
            if (index >= combatantCount_)
            {
                snapshot_.combatants[index] = {};
                snapshot_.combatants[index].health = 0;
                snapshot_.combatants[index].animation = EnemyAnimation::Dead;
                snapshot_.combatants[index].action = EnemyCombatAction::Dead;
                continue;
            }
            const Combatant& source = combatants_[index];
            SkeletonCombatantSnapshot& target = snapshot_.combatants[index];
            target.x = source.x;
            target.z = source.z;
            target.facingRadians = source.facing;
            target.animationTime = source.animationTime;
            target.damageFlash = source.damageFlash;
            target.health = source.health;
            target.animation = source.animation;
            target.action = source.action;
            target.reaction = source.reaction;
            target.actionTime = source.phaseTime;
            target.reactionTime = source.reactionTime;
            target.playerHitPulse = source.playerHitPulse;
            target.parrySuccessPulse = source.parrySuccessPulse;
            if (source.health > 0)
            {
                ++snapshot_.aliveCount;
                snapshot_.encounterComplete = false;
            }
            snapshot_.playerHitPulse = snapshot_.playerHitPulse || source.playerHitPulse;
        }

        const SkeletonCombatantSnapshot& primary = snapshot_.combatants[0];
        snapshot_.enemyX = primary.x;
        snapshot_.enemyZ = primary.z;
        snapshot_.enemyFacingRadians = primary.facingRadians;
        snapshot_.enemyAnimationTime = primary.animationTime;
        snapshot_.damageFlash = primary.damageFlash;
        snapshot_.enemyAnimation = primary.animation;
    }

public:
    static constexpr float kSwingWindupDuration = 0.18f;
    static constexpr float kDownwardCutTravelDuration = 0.16f;
    static constexpr float kSwingActiveDuration = 0.42f;
    static constexpr float kSwingRecoveryDuration = 0.22f;
    static constexpr float kSwordDuration = kSwingWindupDuration + kSwingActiveDuration + kSwingRecoveryDuration;
    // The cut arcs are authoritative gameplay-animation inputs. They are
    // bounded so the audited sword remains in the 75% portrait safe frame;
    // held-item rendering and hand IK both consume these same values.
    static constexpr float kDownwardSwingAmplitude = 0.58f;
    static constexpr float kUpwardSliceWindupDuration = 0.10f;
    static constexpr float kUpwardSliceActiveDuration = 0.18f;
    static constexpr float kUpwardSliceRecoveryDuration = 0.24f;
    static constexpr float kUpwardSliceEndRadians = 0.18f;
    static constexpr float kPlayerHitRange = 1.72f;
    static constexpr float kPlayerHitConeDot = 0.52f;
    static constexpr float kParryStartupDuration = 0.04f;
    static constexpr float kParryActiveDuration = 0.22f;
    static constexpr float kParryRecoveryDuration = 0.24f;
    static constexpr float kEnemyAttackWindupDuration = 1.12f;
    static constexpr float kEnemyAttackActiveDuration = 0.18f;
    static constexpr float kEnemyAttackRecoveryDuration = 1.50f;
    static constexpr float kEnemyStaggerDuration = 0.80f;

private:
    static constexpr float kUpwardSliceTransitionRadians = -0.547f;
    static constexpr float kUpwardSliceStartRadians = -kDownwardSwingAmplitude;
    static constexpr float kEnemyWalkSpeed = 0.62f;
    static constexpr float kEnemyAttackRange = 1.28f;
    static constexpr float kEnemyAttackEnterRange = 1.34f;
    static constexpr float kMinimumSeparation = 0.70f;
    static constexpr float kWalkAnimationHold = 0.24f;
    static constexpr float kEnemyDamageRange = 1.55f;
    static constexpr float kParryJoltDuration = 0.12f;

    CombatSnapshot snapshot_{};
    std::array<Combatant, kSkeletonCombatantCapacity> combatants_{};
    std::size_t combatantCount_ = kSkeletonCombatantCapacity;
    bool attackQueued_ = false;
    bool parryQueued_ = false;
    bool successfulParryEndsNextTick_ = false;
    PlayerCombatSnapshot player_{};
    std::int32_t attackerIndex_ = -1;
};

} // namespace horde::gameplay
