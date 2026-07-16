#pragma once

#include <algorithm>
#include <cmath>

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

struct CombatSnapshot
{
    float swordSwingRadians = 0.0f;
    float enemyX = 0.0f;
    float enemyZ = -4.65f;
    float enemyFacingRadians = 0.0f;
    float enemyAnimationTime = 0.0f;
    float damageFlash = 0.0f;
    EnemyAnimation enemyAnimation = EnemyAnimation::Idle;
};

// A deliberately small one-enemy loop. Rendering consumes only the immutable
// snapshot; hit tests, timing, and respawn decisions remain gameplay concerns.
class SwordCombat
{
public:
    void RequestAttack()
    {
        attackQueued_ = true;
    }

    const CombatSnapshot& Update(float deltaSeconds, float playerX, float playerZ, float playerYaw)
    {
        deltaSeconds = std::clamp(deltaSeconds, 0.0f, 0.05f);
        damageFlash_ = std::max(0.0f, damageFlash_ - deltaSeconds * 2.8f);
        walkAnimationHold_ = std::max(0.0f, walkAnimationHold_ - deltaSeconds);

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

        const float toPlayerX = playerX - enemyX_;
        const float toPlayerZ = playerZ - enemyZ_;
        const float distance = std::sqrt(toPlayerX * toPlayerX + toPlayerZ * toPlayerZ);
        const ShowcaseZone playerZone = QueryShowcaseZone(playerX, playerZ);
        const bool playerInsideEnemyArena = playerZone == ShowcaseZone::Opening ||
                                            playerZone == ShowcaseZone::SkeletonRoom;
        if (playerInsideEnemyArena && distance > 0.0001f)
        {
            // The staged skeleton's authored forward direction is +Z.
            enemyFacing_ = std::atan2(toPlayerX, toPlayerZ);
        }

        if (phase_ != EnemyPhase::Dead && !swordHitConsumed_ && swordTime_ >= 0.18f && swordTime_ <= 0.34f)
        {
            const float inverseDistance = distance > 0.0001f ? 1.0f / distance : 0.0f;
            const float targetX = (enemyX_ - playerX) * inverseDistance;
            const float targetZ = (enemyZ_ - playerZ) * inverseDistance;
            const float forwardX = std::sin(playerYaw);
            const float forwardZ = -std::cos(playerYaw);
            if (distance <= 1.72f && targetX * forwardX + targetZ * forwardZ >= 0.52f)
            {
                phase_ = EnemyPhase::Dead;
                phaseTime_ = 0.0f;
                animationTime_ = 0.0f;
                swordHitConsumed_ = true;
            }
        }

        phaseTime_ += deltaSeconds;
        animationTime_ += deltaSeconds;
        switch (phase_)
        {
        case EnemyPhase::Approach:
            if (!playerInsideEnemyArena)
            {
                walkAnimationHold_ = 0.0f;
                snapshot_.enemyAnimation = EnemyAnimation::Idle;
            }
            else if (distance > 1.28f)
            {
                const float step = std::min(distance - 1.28f, kEnemyWalkSpeed * deltaSeconds);
                float proposedEnemyX = enemyX_ + toPlayerX / std::max(distance, 0.0001f) * step;
                float proposedEnemyZ = enemyZ_ + toPlayerZ / std::max(distance, 0.0001f) * step;
                ResolveSkeletonEnemyCollision(enemyX_, enemyZ_, proposedEnemyX, proposedEnemyZ);
                const bool moved = std::abs(proposedEnemyX - enemyX_) > 0.00001f ||
                                   std::abs(proposedEnemyZ - enemyZ_) > 0.00001f;
                enemyX_ = proposedEnemyX;
                enemyZ_ = proposedEnemyZ;
                // Collision sliding can alternate between a tiny accepted step
                // and a rejected step at a wall/post. Preserve locomotion for a
                // short no-motion grace period so that geometric jitter cannot
                // thrash the skinned clip between Walking and Idle every frame.
                if (moved)
                {
                    walkAnimationHold_ = kWalkAnimationHold;
                }
                snapshot_.enemyAnimation = walkAnimationHold_ > 0.0f
                    ? EnemyAnimation::Walking
                    : EnemyAnimation::Idle;
            }
            else
            {
                walkAnimationHold_ = 0.0f;
                phase_ = EnemyPhase::Attack;
                phaseTime_ = 0.0f;
                animationTime_ = 0.0f;
                playerHitApplied_ = false;
                snapshot_.enemyAnimation = EnemyAnimation::Attack;
            }
            break;
        case EnemyPhase::Attack:
            if (!playerInsideEnemyArena)
            {
                walkAnimationHold_ = 0.0f;
                phase_ = EnemyPhase::Approach;
                phaseTime_ = 0.0f;
                animationTime_ = 0.0f;
                playerHitApplied_ = false;
                snapshot_.enemyAnimation = EnemyAnimation::Idle;
                break;
            }
            snapshot_.enemyAnimation = EnemyAnimation::Attack;
            if (!playerHitApplied_ && phaseTime_ >= 1.12f && distance <= 1.55f)
            {
                playerHitApplied_ = true;
                damageFlash_ = 1.0f;
            }
            if (phaseTime_ >= kEnemyAttackDuration)
            {
                phase_ = EnemyPhase::Approach;
                phaseTime_ = 0.0f;
                animationTime_ = 0.0f;
            }
            break;
        case EnemyPhase::Dead:
            snapshot_.enemyAnimation = EnemyAnimation::Dead;
            if (phaseTime_ >= kEnemyDeadDuration + kRespawnHold)
            {
                ResetEnemy();
            }
            break;
        }

        snapshot_.enemyX = enemyX_;
        snapshot_.enemyZ = enemyZ_;
        snapshot_.enemyFacingRadians = enemyFacing_;
        snapshot_.enemyAnimationTime = animationTime_;
        snapshot_.damageFlash = damageFlash_;
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

    void ResetEnemy()
    {
        enemyX_ = 0.0f;
        enemyZ_ = -4.65f;
        enemyFacing_ = 0.0f;
        phase_ = EnemyPhase::Approach;
        phaseTime_ = 0.0f;
        animationTime_ = 0.0f;
        walkAnimationHold_ = kWalkAnimationHold;
        playerHitApplied_ = false;
        snapshot_.enemyAnimation = EnemyAnimation::Walking;
    }

    static constexpr float kSwordDuration = 0.56f;
    static constexpr float kEnemyWalkSpeed = 0.62f;
    static constexpr float kWalkAnimationHold = 0.24f;
    static constexpr float kEnemyAttackDuration = 2.80f;
    static constexpr float kEnemyDeadDuration = 2.967f;
    static constexpr float kRespawnHold = 0.85f;

    CombatSnapshot snapshot_{};
    EnemyPhase phase_ = EnemyPhase::Approach;
    float enemyX_ = 0.0f;
    float enemyZ_ = -4.65f;
    float enemyFacing_ = 0.0f;
    float phaseTime_ = 0.0f;
    float animationTime_ = 0.0f;
    float swordTime_ = kSwordDuration;
    float damageFlash_ = 0.0f;
    float walkAnimationHold_ = kWalkAnimationHold;
    bool attackQueued_ = false;
    bool swordHitConsumed_ = false;
    bool playerHitApplied_ = false;
};

} // namespace horde::gameplay
