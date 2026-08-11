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
    bool playerHitPulse = false;
};

struct CombatSnapshot
{
    float swordSwingRadians = 0.0f;

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
            combatant.phase = EnemyPhase::Dead;
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
            combatant.phase = EnemyPhase::Approach;
            combatant.health = 1;
            combatant.walkAnimationHold = kWalkAnimationHold;
            combatant.animation = EnemyAnimation::Walking;
        }
        snapshot_ = {};
        swordTime_ = kSwordDuration;
        attackQueued_ = false;
        swordHitConsumed_ = false;
        attackerIndex_ = -1;
        PublishSnapshot();
    }

    void RequestAttack()
    {
        attackQueued_ = true;
    }

    std::int32_t PlayerSwingTargetIndex(float playerX, float playerZ, float playerYaw) const
    {
        std::int32_t targetIndex = -1;
        float targetDistance = 0.0f;
        const float forwardX = std::sin(playerYaw);
        const float forwardZ = -std::cos(playerYaw);
        for (std::size_t index = 0; index < combatantCount_; ++index)
        {
            const Combatant& combatant = combatants_[index];
            const float distance = std::hypot(combatant.x - playerX, combatant.z - playerZ);
            if (combatant.health <= 0 || distance > kPlayerHitRange)
            {
                continue;
            }
            const float inverseDistance = distance > 0.0001f ? 1.0f / distance : 0.0f;
            const float targetX = (combatant.x - playerX) * inverseDistance;
            const float targetZ = (combatant.z - playerZ) * inverseDistance;
            if (targetX * forwardX + targetZ * forwardZ < kPlayerHitConeDot)
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
        for (Combatant& combatant : combatants_)
        {
            combatant.playerHitPulse = false;
            combatant.damageFlash = std::max(0.0f, combatant.damageFlash - deltaSeconds * 2.8f);
            combatant.walkAnimationHold = std::max(0.0f, combatant.walkAnimationHold - deltaSeconds);
        }

        if (attackQueued_ && swordTime_ >= kSwordDuration)
        {
            swordTime_ = 0.0f;
            swordHitConsumed_ = false;
        }
        attackQueued_ = false;

        if (swordTime_ < kSwordDuration)
        {
            swordTime_ = std::min(kSwordDuration, swordTime_ + deltaSeconds);
            const float phase = swordTime_ / kSwordDuration;
            snapshot_.swordSwingRadians = -1.12f * std::sin(phase * 3.14159265f);
        }
        else
        {
            snapshot_.swordSwingRadians = 0.0f;
        }

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
        ResolveSwordHit(playerX, playerZ, playerYaw);

        for (std::size_t index = 0; index < combatantCount_; ++index)
        {
            UpdateCombatant(index,
                            deltaSeconds,
                            playerX,
                            playerZ,
                            distances[index],
                            playerInsideEnemyArena);
        }
        ResolveCombatantSeparation(previousPositions);
        PublishSnapshot();
        return snapshot_;
    }

    const CombatSnapshot& Snapshot() const { return snapshot_; }

private:
    enum class EnemyPhase
    {
        Approach,
        Attack,
        Dead
    };

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
        EnemyPhase phase = EnemyPhase::Approach;
        EnemyAnimation animation = EnemyAnimation::Walking;
        bool playerHitApplied = false;
        bool playerHitPulse = false;
    };

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
            if (holder.health > 0 && holder.phase == EnemyPhase::Attack)
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
        if (swordHitConsumed_ || swordTime_ < 0.18f || swordTime_ > 0.34f)
        {
            return;
        }

        const std::int32_t targetIndex = PlayerSwingTargetIndex(playerX, playerZ, playerYaw);
        if (targetIndex < 0)
        {
            return;
        }
        Combatant& target = combatants_[static_cast<std::size_t>(targetIndex)];
        target.health = 0;
        target.phase = EnemyPhase::Dead;
        target.phaseTime = 0.0f;
        target.animationTime = 0.0f;
        target.animation = EnemyAnimation::Dead;
        target.playerHitApplied = false;
        swordHitConsumed_ = true;
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
                         bool playerInsideEnemyArena)
    {
        Combatant& combatant = combatants_[index];
        combatant.phaseTime += deltaSeconds;
        combatant.animationTime += deltaSeconds;
        if (combatant.health <= 0 || combatant.phase == EnemyPhase::Dead)
        {
            combatant.health = 0;
            combatant.phase = EnemyPhase::Dead;
            combatant.animation = EnemyAnimation::Dead;
            return;
        }

        if (!playerInsideEnemyArena)
        {
            combatant.walkAnimationHold = 0.0f;
            combatant.phase = EnemyPhase::Approach;
            combatant.phaseTime = 0.0f;
            combatant.animationTime = 0.0f;
            combatant.playerHitApplied = false;
            combatant.animation = EnemyAnimation::Idle;
            return;
        }

        const bool ownsAttackToken = attackerIndex_ == static_cast<std::int32_t>(index);
        if (!ownsAttackToken && combatant.phase == EnemyPhase::Attack)
        {
            combatant.phase = EnemyPhase::Approach;
            combatant.phaseTime = 0.0f;
            combatant.animationTime = 0.0f;
            combatant.playerHitApplied = false;
        }

        if (combatant.phase == EnemyPhase::Approach)
        {
            if (ownsAttackToken && distance <= kEnemyAttackEnterRange)
            {
                combatant.walkAnimationHold = 0.0f;
                combatant.phase = EnemyPhase::Attack;
                combatant.phaseTime = 0.0f;
                combatant.animationTime = 0.0f;
                combatant.playerHitApplied = false;
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
        if (!combatant.playerHitApplied && combatant.phaseTime >= 1.12f && distance <= 1.55f)
        {
            combatant.playerHitApplied = true;
            combatant.damageFlash = 1.0f;
            combatant.playerHitPulse = true;
        }
        if (combatant.phaseTime >= kEnemyAttackDuration)
        {
            combatant.phase = EnemyPhase::Approach;
            combatant.phaseTime = 0.0f;
            combatant.animationTime = 0.0f;
            combatant.playerHitApplied = false;
            attackerIndex_ = -1;
        }
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
        if (first.health <= 0)
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
        if (second.health <= 0)
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
        snapshot_.playerHitPulse = false;
        for (std::size_t index = 0; index < snapshot_.combatants.size(); ++index)
        {
            if (index >= combatantCount_)
            {
                snapshot_.combatants[index] = {};
                snapshot_.combatants[index].health = 0;
                snapshot_.combatants[index].animation = EnemyAnimation::Dead;
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
            target.playerHitPulse = source.playerHitPulse;
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

    static constexpr float kSwordDuration = 0.56f;
    static constexpr float kPlayerHitRange = 1.72f;
    static constexpr float kPlayerHitConeDot = 0.52f;
    static constexpr float kEnemyWalkSpeed = 0.62f;
    static constexpr float kEnemyAttackRange = 1.28f;
    static constexpr float kEnemyAttackEnterRange = 1.34f;
    static constexpr float kMinimumSeparation = 0.70f;
    static constexpr float kWalkAnimationHold = 0.24f;
    static constexpr float kEnemyAttackDuration = 2.80f;

    CombatSnapshot snapshot_{};
    std::array<Combatant, kSkeletonCombatantCapacity> combatants_{};
    std::size_t combatantCount_ = kSkeletonCombatantCapacity;
    float swordTime_ = kSwordDuration;
    bool attackQueued_ = false;
    bool swordHitConsumed_ = false;
    std::int32_t attackerIndex_ = -1;
};

} // namespace horde::gameplay
