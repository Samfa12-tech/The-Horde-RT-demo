#pragma once

#include <algorithm>
#include <array>
#include <cmath>
#include <cstddef>
#include <cstdint>

#include "gameplay/ShowcaseRoute.h"

namespace horde::gameplay
{

enum class LanternPhase
{
    Held,
    Guttering,
    Falling,
    Settled,
};

struct LanternSnapshot
{
    LanternPhase phase = LanternPhase::Held;
    float phaseTime = 0.0f;
    float sequenceTime = 0.0f;
    float flameStrength = 1.0f;
    float fallProgress = 0.0f;
    float leftArmLowerBlend = 0.0f;
    float droppedX = 0.0f;
    float droppedY = -0.95f;
    float droppedZ = 0.0f;
    float droppedYawRadians = 0.0f;
    float droppedViewPitchRadians = 0.0f;
    float droppedPitchRadians = 0.0f;
    bool heldByPlayer = true;
    bool triggered = false;
};

struct LowerBodyPoseState
{
    float gaitPhase = 0.0f;
    float leftStride = 0.0f;
    float rightStride = 0.0f;
    float pelvisBob = 0.0f;
    float pelvisSway = 0.0f;
    float torsoTwistRadians = 0.0f;
    float leftFootLift = 0.0f;
    float rightFootLift = 0.0f;
    float leftKneeBend = 0.0f;
    float rightKneeBend = 0.0f;
    float leftToeRoll = 0.0f;
    float rightToeRoll = 0.0f;
};

enum class PlayerLifePhase
{
    Alive,
    Dying,
    Dead,
};

enum class PlayerDamageResult
{
    Ignored,
    Damaged,
    Killed,
};

struct PlayerVitalsSnapshot
{
    PlayerLifePhase phase = PlayerLifePhase::Alive;
    int vitality = 3;
    int maxVitality = 3;
    float invulnerabilityRemaining = 0.0f;
    float damageFlash = 0.0f;
    float deathHoldRemaining = 0.0f;
};

class PlayerVitals
{
public:
    const PlayerVitalsSnapshot& Update(float deltaSeconds)
    {
        deltaSeconds = std::clamp(deltaSeconds, 0.0f, 0.05f);
        snapshot_.invulnerabilityRemaining =
            std::max(0.0f, snapshot_.invulnerabilityRemaining - deltaSeconds);
        snapshot_.damageFlash = std::max(0.0f, snapshot_.damageFlash - deltaSeconds * kDamageFlashDecay);
        if (snapshot_.phase == PlayerLifePhase::Dying)
        {
            snapshot_.deathHoldRemaining = std::max(0.0f, snapshot_.deathHoldRemaining - deltaSeconds);
            if (snapshot_.deathHoldRemaining <= 0.00001f)
            {
                snapshot_.phase = PlayerLifePhase::Dead;
            }
        }
        return snapshot_;
    }

    PlayerDamageResult TryApplyDamage()
    {
        if (snapshot_.phase != PlayerLifePhase::Alive ||
            snapshot_.invulnerabilityRemaining > 0.00001f)
        {
            return PlayerDamageResult::Ignored;
        }

        snapshot_.vitality = std::max(0, snapshot_.vitality - 1);
        snapshot_.invulnerabilityRemaining = kInvulnerabilityDuration;
        snapshot_.damageFlash = 1.0f;
        if (snapshot_.vitality == 0)
        {
            snapshot_.phase = PlayerLifePhase::Dying;
            snapshot_.deathHoldRemaining = kDeathHoldDuration;
            return PlayerDamageResult::Killed;
        }
        return PlayerDamageResult::Damaged;
    }

    void ResetForEncounter()
    {
        snapshot_ = {};
    }

    const PlayerVitalsSnapshot& Snapshot() const { return snapshot_; }

    static constexpr int kMaxVitality = 3;
    static constexpr float kInvulnerabilityDuration = 1.0f;
    static constexpr float kDeathHoldDuration = 0.65f;

private:
    static constexpr float kDamageFlashDecay = 2.8f;
    PlayerVitalsSnapshot snapshot_{};
};

constexpr const char* PlayerLifePhaseName(PlayerLifePhase phase)
{
    switch (phase)
    {
    case PlayerLifePhase::Alive: return "alive";
    case PlayerLifePhase::Dying: return "dying";
    case PlayerLifePhase::Dead: return "dead";
    default: return "unknown";
    }
}

inline LowerBodyPoseState EvaluateLowerBodyPose(float walkTime, float walkAmount)
{
    const float rawAmount = std::clamp(walkAmount, 0.0f, 1.0f);
    const float amount = rawAmount * rawAmount * (3.0f - 2.0f * rawAmount);
    const float phase = walkTime * 6.2f;
    const float gaitSine = std::sin(phase);
    const float gaitCosine = std::cos(phase);
    const float leftLift = std::max(0.0f, gaitSine) * amount;
    const float rightLift = std::max(0.0f, -gaitSine) * amount;
    LowerBodyPoseState pose;
    pose.gaitPhase = phase;
    pose.leftStride = gaitSine * amount;
    pose.rightStride = -pose.leftStride;
    pose.pelvisBob = (0.5f - 0.5f * std::cos(phase * 2.0f)) * 0.024f * amount;
    pose.pelvisSway = -gaitSine * 0.018f * amount;
    pose.torsoTwistRadians = -gaitSine * 0.055f * amount;
    pose.leftFootLift = leftLift;
    pose.rightFootLift = rightLift;
    pose.leftKneeBend = std::clamp((0.16f + leftLift * 0.84f) * amount, 0.0f, 1.0f);
    pose.rightKneeBend = std::clamp((0.16f + rightLift * 0.84f) * amount, 0.0f, 1.0f);
    pose.leftToeRoll = std::max(0.0f, -gaitCosine) * amount;
    pose.rightToeRoll = std::max(0.0f, gaitCosine) * amount;
    return pose;
}

struct SceneLightingState
{
    std::array<float, 3u> position{{0.0f, 0.0f, 0.0f}};
    std::array<float, 3u> colour{{1.0f, 0.28f, 0.055f}};
    float intensity = 0.0f;
    float radius = 0.0f;
};

constexpr SceneLightingState RouteLightForZone(ShowcaseZone zone)
{
    switch (zone)
    {
    case ShowcaseZone::YellowTorchBay:
        return {{{-11.0f, 0.67f, -13.98f}}, {{1.0f, 0.42f, 0.06f}}, 1.0f, 5.0f};
    case ShowcaseZone::BlueTorchBay:
        return {{{-16.0f, 0.67f, -13.98f}}, {{0.055f, 0.22f, 1.0f}}, 1.0f, 5.0f};
    case ShowcaseZone::RedTorchBay:
        return {{{-21.0f, 0.67f, -13.98f}}, {{1.0f, 0.045f, 0.018f}}, 1.0f, 5.0f};
    case ShowcaseZone::GreenTorchBay:
        return {{{-26.0f, 0.67f, -13.98f}}, {{0.16f, 0.78f, 0.22f}}, 1.0f, 5.0f};
    default:
        return {};
    }
}

constexpr SceneLightingState RouteLightForPosition(float x, float z)
{
    return RouteLightForZone(QueryShowcaseZone(x, z));
}

// The failure begins only near the western exit of the final zig-zag leg. It
// therefore cannot fire while the player merely passes either earlier bend.
constexpr bool IsLanternFailureTriggerPosition(float x, float z)
{
    return x >= -2.50f && x <= -1.35f && z >= -16.40f && z <= -14.00f;
}

class LanternSequence
{
public:
    const LanternSnapshot& Update(float deltaSeconds, float playerX, float playerZ,
                                  float playerYawRadians = 0.0f,
                                  float playerViewPitchRadians = 0.0f)
    {
        deltaSeconds = std::clamp(deltaSeconds, 0.0f, 0.05f);
        if (!snapshot_.triggered && IsLanternFailureTriggerPosition(playerX, playerZ))
        {
            snapshot_.triggered = true;
            snapshot_.phase = LanternPhase::Guttering;
            snapshot_.phaseTime = 0.0f;
            snapshot_.sequenceTime = 0.0f;
            snapshot_.droppedX = playerX;
            snapshot_.droppedZ = playerZ;
            snapshot_.droppedYawRadians = playerYawRadians;
            snapshot_.droppedViewPitchRadians = playerViewPitchRadians;
        }

        if (!snapshot_.triggered)
        {
            return snapshot_;
        }

        const bool wasHeld = snapshot_.heldByPlayer;
        snapshot_.sequenceTime += deltaSeconds;
        snapshot_.phaseTime += deltaSeconds;

        if (snapshot_.sequenceTime < kReleaseTime)
        {
            snapshot_.phase = LanternPhase::Guttering;
            snapshot_.phaseTime = snapshot_.sequenceTime;
            snapshot_.heldByPlayer = true;
            snapshot_.droppedX = playerX;
            snapshot_.droppedZ = playerZ;
            snapshot_.droppedYawRadians = playerYawRadians;
            snapshot_.droppedViewPitchRadians = playerViewPitchRadians;
            snapshot_.fallProgress = 0.0f;

            // A deterministic two-frequency gutter avoids frame-random state
            // while still supplying an irregular-looking renderer input.
            const float gutter = 0.78f + 0.14f * std::sin(snapshot_.sequenceTime * 31.0f) +
                                 0.08f * std::sin(snapshot_.sequenceTime * 73.0f);
            const float fade = 1.0f - 0.38f * (snapshot_.sequenceTime / kReleaseTime);
            snapshot_.flameStrength = std::clamp(gutter * fade, 0.0f, 1.0f);
        }
        else if (snapshot_.sequenceTime < kSettleTime)
        {
            if (wasHeld)
            {
                // Capture the exact release frame, then keep the world-space
                // fall independent of later camera movement.
                snapshot_.droppedX = playerX;
                snapshot_.droppedZ = playerZ;
                snapshot_.droppedYawRadians = playerYawRadians;
                snapshot_.droppedViewPitchRadians = playerViewPitchRadians;
            }
            snapshot_.phase = LanternPhase::Falling;
            snapshot_.phaseTime = snapshot_.sequenceTime - kReleaseTime;
            snapshot_.heldByPlayer = false;
            snapshot_.flameStrength = 0.0f;
            snapshot_.fallProgress = SmoothStep((snapshot_.sequenceTime - kReleaseTime) / kFallDuration);
        }
        else
        {
            snapshot_.phase = LanternPhase::Settled;
            snapshot_.phaseTime = snapshot_.sequenceTime - kSettleTime;
            snapshot_.heldByPlayer = false;
            snapshot_.flameStrength = 0.0f;
            snapshot_.fallProgress = 1.0f;
        }

        snapshot_.leftArmLowerBlend = SmoothStep(
            (snapshot_.sequenceTime - kArmLowerStart) / (kArmLowerEnd - kArmLowerStart));
        snapshot_.droppedY = -0.95f + 0.72f * (1.0f - snapshot_.fallProgress);
        snapshot_.droppedPitchRadians = 1.36f * snapshot_.fallProgress;
        return snapshot_;
    }

    void Reset()
    {
        snapshot_ = {};
    }

    const LanternSnapshot& Snapshot() const { return snapshot_; }

    static constexpr float kReleaseTime = 0.70f;
    static constexpr float kFallDuration = 0.45f;
    static constexpr float kSettleTime = kReleaseTime + kFallDuration;
    static constexpr float kArmLowerStart = 0.70f;
    static constexpr float kArmLowerEnd = 1.20f;

private:
    static float SmoothStep(float value)
    {
        value = std::clamp(value, 0.0f, 1.0f);
        return value * value * (3.0f - 2.0f * value);
    }

    LanternSnapshot snapshot_{};
};

enum class EnemyKind
{
    None,
    Skeleton,
    Lich,
};

enum class EncounterStatus
{
    Inactive,
    Active,
    Dead,
};

struct EnemyEncounterSnapshot
{
    EnemyKind kind = EnemyKind::None;
    EncounterStatus status = EncounterStatus::Inactive;
    std::uint32_t resetGeneration = 0;
};

inline constexpr std::size_t kEnemyRosterCapacity = 2;
inline constexpr std::size_t kEnemyRenderSelectionCapacity = 8;

struct EnemyRosterSnapshot
{
    std::array<EnemyEncounterSnapshot, kEnemyRosterCapacity> encounters{{
        {EnemyKind::Skeleton, EncounterStatus::Inactive, 0},
        {EnemyKind::Lich, EncounterStatus::Inactive, 0},
    }};
    std::array<EnemyKind, kEnemyRenderSelectionCapacity> renderedEnemies{};
    std::size_t renderedEnemyCount = 0;
    std::size_t renderedEnemyCapacity = 1;
    EnemyKind selectedEnemy = EnemyKind::None;
};

struct EnemyDirectorConfig
{
    // One is the validated showcase setting. Keeping capacity explicit avoids
    // baking that measured limit into the future multi-enemy architecture.
    std::size_t maxRenderedEnemies = 1;
};

constexpr EnemyKind EnemyKindForShowcaseZone(ShowcaseZone zone)
{
    switch (zone)
    {
    case ShowcaseZone::Opening:
    case ShowcaseZone::SkeletonRoom:
    case ShowcaseZone::ShadowCorridor:
        return EnemyKind::Skeleton;
    case ShowcaseZone::SkylightChamber:
    case ShowcaseZone::YellowTorchBay:
    case ShowcaseZone::BlueTorchBay:
    case ShowcaseZone::RedTorchBay:
    case ShowcaseZone::GreenTorchBay:
    case ShowcaseZone::TransmissionThreshold:
    case ShowcaseZone::Finale:
        return EnemyKind::Lich;
    default:
        return EnemyKind::None;
    }
}

class EnemyDirector
{
public:
    EnemyDirector(EnemyDirectorConfig config = {})
    {
        snapshot_.renderedEnemyCapacity = std::clamp<std::size_t>(
            config.maxRenderedEnemies, 1, kEnemyRenderSelectionCapacity);
        Select(EnemyKind::Skeleton);
    }

    const EnemyRosterSnapshot& Update(float playerX, float playerZ)
    {
        const EnemyKind routeEnemy = EnemyKindForShowcaseZone(QueryShowcaseZone(playerX, playerZ));
        // Outside is not a gate. Retaining selection prevents accidental resets
        // caused by tiny collision/precision excursions at rectangle seams.
        if (routeEnemy != EnemyKind::None && routeEnemy != snapshot_.selectedEnemy)
        {
            Select(routeEnemy);
        }
        return snapshot_;
    }

    void MarkSelectedDead()
    {
        if (auto* encounter = Find(snapshot_.selectedEnemy))
        {
            encounter->status = EncounterStatus::Dead;
        }
    }

    // Windows debug harness only: lets clip/BLAS inspection swap the active
    // skinned character without changing the authored route gate.
    void ForceSelectForDebug(EnemyKind kind)
    {
        if (kind != EnemyKind::None && kind != snapshot_.selectedEnemy)
        {
            Select(kind);
        }
    }

    void Reset()
    {
        snapshot_.selectedEnemy = EnemyKind::None;
        snapshot_.renderedEnemyCount = 0;
        for (auto& encounter : snapshot_.encounters)
        {
            encounter.status = EncounterStatus::Inactive;
            encounter.resetGeneration = 0;
        }
        Select(EnemyKind::Skeleton);
    }

    const EnemyRosterSnapshot& Snapshot() const { return snapshot_; }

private:
    EnemyEncounterSnapshot* Find(EnemyKind kind)
    {
        for (auto& encounter : snapshot_.encounters)
        {
            if (encounter.kind == kind)
            {
                return &encounter;
            }
        }
        return nullptr;
    }

    void Select(EnemyKind kind)
    {
        for (auto& encounter : snapshot_.encounters)
        {
            encounter.status = EncounterStatus::Inactive;
        }
        EnemyEncounterSnapshot* selected = Find(kind);
        if (selected == nullptr)
        {
            return;
        }
        selected->status = EncounterStatus::Active;
        ++selected->resetGeneration;
        snapshot_.selectedEnemy = kind;
        snapshot_.renderedEnemyCount = 1;
        snapshot_.renderedEnemies.fill(EnemyKind::None);
        snapshot_.renderedEnemies[0] = kind;
    }

    EnemyRosterSnapshot snapshot_{};
};

enum class LichPhase
{
    Dormant,
    MaintainingRange,
    Charging,
    Recovering,
    Dead,
};

enum class FinaleEndingPhase
{
    Inactive,
    LichFalling,
    SkylightOpening,
    DawnRevealed,
    Complete,
};

struct LichSnapshot
{
    LichPhase phase = LichPhase::Dormant;
    float x = -32.2f;
    float y = -0.77f;
    float z = -13.1f;
    float facingRadians = 0.0f;
    float animationTime = 0.0f;
    float phaseTime = 0.0f;
    float staffLightStrength = 0.0f;
    float hitCooldownRemaining = 0.0f;
    float hitRecoil = 0.0f;
    float finaleSkylightOpenProgress = 0.0f;
    float finaleDawnRevealProgress = 0.0f;
    int health = 3;
    bool damagePulse = false;
    bool hitPulse = false;
    bool deathAnimationComplete = false;
    FinaleEndingPhase finaleEndingPhase = FinaleEndingPhase::Inactive;
};

class LichEncounter
{
public:
    const LichSnapshot& Update(float deltaSeconds, float playerX, float playerZ,
                               bool staffHasLineOfSight, bool finaleActive)
    {
        deltaSeconds = std::clamp(deltaSeconds, 0.0f, 0.05f);
        snapshot_.damagePulse = false;
        snapshot_.hitCooldownRemaining = std::max(0.0f, snapshot_.hitCooldownRemaining - deltaSeconds);
        hitPulseTime_ = std::max(0.0f, hitPulseTime_ - deltaSeconds);
        snapshot_.hitPulse = hitPulseTime_ > 0.0f;
        hitRecoilTime_ = std::max(0.0f, hitRecoilTime_ - deltaSeconds);
        const float recoilRemaining = hitRecoilTime_ / kHitRecoilDuration;
        snapshot_.hitRecoil = recoilRemaining * recoilRemaining * (3.0f - 2.0f * recoilRemaining);

        // Death is a visible, non-looping encounter phase. Reset its animation
        // clock on the third accepted hit, advance independently of finale activation, and
        // only tell platform orchestration that removal is safe after the
        // authored Dead clip has reached its clamped final pose.
        if (snapshot_.phase == LichPhase::Dead)
        {
            float remainingDelta = deltaSeconds;
            const float deathTimeRemaining = std::max(0.0f, kDeathAnimationDuration - snapshot_.phaseTime);
            const float deathDelta = std::min(remainingDelta, deathTimeRemaining);
            snapshot_.phaseTime = std::min(kDeathAnimationDuration, snapshot_.phaseTime + deathDelta);
            snapshot_.animationTime = snapshot_.phaseTime;
            remainingDelta -= deathDelta;
            snapshot_.deathAnimationComplete =
                snapshot_.phaseTime + 0.00001f >= kDeathAnimationDuration;
            if (snapshot_.deathAnimationComplete)
            {
                snapshot_.finaleEndingPhase = FinaleEndingPhase::SkylightOpening;
                const float skylightTimeRemaining =
                    std::max(0.0f, kFinaleSkylightOpenDuration - finaleSkylightOpenTime_);
                const float skylightDelta = std::min(remainingDelta, skylightTimeRemaining);
                finaleSkylightOpenTime_ = std::min(
                    kFinaleSkylightOpenDuration,
                    finaleSkylightOpenTime_ + skylightDelta);
                remainingDelta -= skylightDelta;
                snapshot_.finaleSkylightOpenProgress =
                    finaleSkylightOpenTime_ / kFinaleSkylightOpenDuration;
                if (snapshot_.finaleSkylightOpenProgress + 0.00001f >= 1.0f)
                {
                    snapshot_.finaleEndingPhase = FinaleEndingPhase::DawnRevealed;
                    finaleDawnRevealTime_ = std::min(
                        kFinaleDawnRevealDuration,
                        finaleDawnRevealTime_ + remainingDelta);
                    snapshot_.finaleDawnRevealProgress =
                        finaleDawnRevealTime_ / kFinaleDawnRevealDuration;
                    if (snapshot_.finaleDawnRevealProgress + 0.00001f >= 1.0f)
                    {
                        snapshot_.finaleEndingPhase = FinaleEndingPhase::Complete;
                    }
                }
            }
            snapshot_.staffLightStrength = 0.0f;
            return snapshot_;
        }

        if (!finaleActive)
        {
            snapshot_.phase = LichPhase::Dormant;
            snapshot_.phaseTime = 0.0f;
            snapshot_.staffLightStrength = 0.0f;
            return snapshot_;
        }
        if (snapshot_.phase == LichPhase::Dormant)
        {
            snapshot_.phase = LichPhase::MaintainingRange;
            snapshot_.phaseTime = 0.0f;
        }
        snapshot_.animationTime += deltaSeconds;
        snapshot_.phaseTime += deltaSeconds;
        snapshot_.y = kBaseY + kHoverAmplitude * std::sin(snapshot_.animationTime * kHoverRadiansPerSecond);

        const float toPlayerX = playerX - snapshot_.x;
        const float toPlayerZ = playerZ - snapshot_.z;
        const float distance = std::sqrt(toPlayerX * toPlayerX + toPlayerZ * toPlayerZ);
        if (distance > 0.0001f)
        {
            snapshot_.facingRadians = std::atan2(toPlayerX, toPlayerZ);
            MaintainRange(deltaSeconds, toPlayerX, toPlayerZ, distance,
                          MovementScaleForPhase(snapshot_.phase));
        }

        switch (snapshot_.phase)
        {
        case LichPhase::MaintainingRange:
            snapshot_.staffLightStrength = 0.0f;
            if (snapshot_.phaseTime >= kMinimumRepositionDuration &&
                distance >= kPreferredMinRange && distance <= kPreferredMaxRange)
            {
                snapshot_.phase = LichPhase::Charging;
                snapshot_.phaseTime = 0.0f;
                snapshot_.staffLightStrength = kStaffLightStart;
            }
            break;
        case LichPhase::Charging:
        {
            const float charge = std::clamp(snapshot_.phaseTime / kChargeDuration, 0.0f, 1.0f);
            snapshot_.staffLightStrength = kStaffLightStart +
                                           (kStaffLightPeak - kStaffLightStart) * charge;
            if (snapshot_.phaseTime + 0.00001f >= kChargeDuration)
            {
                snapshot_.damagePulse = staffHasLineOfSight && distance <= kAttackRange;
                snapshot_.staffLightStrength = kStaffLightPeak;
                snapshot_.phase = LichPhase::Recovering;
                snapshot_.phaseTime = 0.0f;
            }
            break;
        }
        case LichPhase::Recovering:
            snapshot_.staffLightStrength = kStaffLightPeak *
                (1.0f - std::clamp(snapshot_.phaseTime / kRecoveryDuration, 0.0f, 1.0f));
            if (snapshot_.phaseTime + 0.00001f >= kRecoveryDuration)
            {
                snapshot_.phase = LichPhase::MaintainingRange;
                snapshot_.phaseTime = 0.0f;
                snapshot_.staffLightStrength = 0.0f;
            }
            break;
        default:
            break;
        }
        return snapshot_;
    }

    bool TryAcceptPlayerHit(float playerX, float playerZ)
    {
        if (snapshot_.phase == LichPhase::Dormant || snapshot_.phase == LichPhase::Dead ||
            snapshot_.health <= 0 || snapshot_.hitCooldownRemaining > 0.00001f)
        {
            return false;
        }
        const float dx = playerX - snapshot_.x;
        const float dz = playerZ - snapshot_.z;
        if (dx * dx + dz * dz > kPlayerHitRange * kPlayerHitRange)
        {
            return false;
        }

        --snapshot_.health;
        snapshot_.hitCooldownRemaining = kPlayerHitLockout;
        hitPulseTime_ = kHitPulseDuration;
        hitRecoilTime_ = kHitRecoilDuration;
        snapshot_.hitPulse = true;
        snapshot_.hitRecoil = 1.0f;
        if (snapshot_.health == 0)
        {
            BeginDeath();
        }
        return true;
    }

    void Reset()
    {
        snapshot_ = {};
        hitPulseTime_ = 0.0f;
        hitRecoilTime_ = 0.0f;
        finaleSkylightOpenTime_ = 0.0f;
        finaleDawnRevealTime_ = 0.0f;
    }

    const LichSnapshot& Snapshot() const { return snapshot_; }

    static constexpr float kChargeDuration = 1.20f;
    static constexpr float kRecoveryDuration = 1.80f;
    static constexpr float kDeathAnimationDuration = 2.967f;
    static constexpr float kFinaleSkylightOpenDuration = 4.50f;
    static constexpr float kFinaleDawnRevealDuration = 1.75f;
    static constexpr float kMinimumRepositionDuration = 0.65f;
    static constexpr float kAttackRange = 7.0f;
    static constexpr float kPreferredMinRange = 3.0f;
    static constexpr float kPreferredMaxRange = 5.0f;
    static constexpr float kPlayerHitRange = 2.0f;
    static constexpr float kPlayerHitLockout = 2.0f;
    static constexpr float kHitRecoilDuration = 0.38f;
    static constexpr float kStaffLightStart = 0.55f;
    static constexpr float kStaffLightPeak = 2.20f;

private:
    void BeginDeath()
    {
        snapshot_.phase = LichPhase::Dead;
        snapshot_.phaseTime = 0.0f;
        snapshot_.animationTime = 0.0f;
        snapshot_.staffLightStrength = 0.0f;
        snapshot_.damagePulse = false;
        snapshot_.deathAnimationComplete = false;
        snapshot_.finaleSkylightOpenProgress = 0.0f;
        snapshot_.finaleDawnRevealProgress = 0.0f;
        snapshot_.finaleEndingPhase = FinaleEndingPhase::LichFalling;
        finaleSkylightOpenTime_ = 0.0f;
        finaleDawnRevealTime_ = 0.0f;
    }
    static constexpr float MovementScaleForPhase(LichPhase phase)
    {
        switch (phase)
        {
        case LichPhase::Charging: return 0.48f;
        case LichPhase::Recovering: return 0.72f;
        case LichPhase::MaintainingRange: return 1.0f;
        default: return 0.0f;
        }
    }

    void MaintainRange(float deltaSeconds, float toPlayerX, float toPlayerZ, float distance,
                       float movementScale)
    {
        if (distance <= 0.0001f || movementScale <= 0.0f)
        {
            return;
        }
        const float inverseDistance = 1.0f / distance;
        const float towardX = toPlayerX * inverseDistance;
        const float towardZ = toPlayerZ * inverseDistance;
        constexpr float preferredRadius = (kPreferredMinRange + kPreferredMaxRange) * 0.5f;
        const float radialSpeed = std::clamp((distance - preferredRadius) * 0.62f,
                                             -kMoveSpeed, kMoveSpeed);
        // Keep an orbiting baseline instead of starting strafe at zero. Phase
        // scaling lets the caster glide while charging and recover more freely,
        // without making the staff wind-up look like full-speed locomotion.
        const float orbitSpeed = 0.34f + 0.10f * std::sin(snapshot_.animationTime * 0.91f + 0.65f);
        const float velocityX = (towardX * radialSpeed - towardZ * orbitSpeed) * movementScale;
        const float velocityZ = (towardZ * radialSpeed + towardX * orbitSpeed) * movementScale;
        snapshot_.x = std::clamp(snapshot_.x + velocityX * deltaSeconds, kMinX, kMaxX);
        snapshot_.z = std::clamp(snapshot_.z + velocityZ * deltaSeconds, kMinZ, kMaxZ);
    }

    static constexpr float kBaseY = -0.77f;
    static constexpr float kHoverAmplitude = 0.06f;
    static constexpr float kHoverRadiansPerSecond = 1.65f;
    static constexpr float kMoveSpeed = 0.55f;
    static constexpr float kHitPulseDuration = 0.14f;
    static constexpr float kMinX = -36.40f;
    static constexpr float kMaxX = -30.90f;
    static constexpr float kMinZ = -17.90f;
    static constexpr float kMaxZ = -12.50f;

    LichSnapshot snapshot_{};
    float hitPulseTime_ = 0.0f;
    float hitRecoilTime_ = 0.0f;
    float finaleSkylightOpenTime_ = 0.0f;
    float finaleDawnRevealTime_ = 0.0f;
};

} // namespace horde::gameplay
