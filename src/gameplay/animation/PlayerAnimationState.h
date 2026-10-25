#pragma once

#include <cstdint>

#include "gameplay/SwordCombat.h"
#include "gameplay/animation/PlayerIkTargets.h"
#include "gameplay/items/HeldItemKinematics.h"

namespace horde::gameplay::animation
{

inline constexpr float kPlayerSwingWindupSeconds = SwordCombat::kSwingWindupDuration;
inline constexpr float kPlayerSwingActiveSeconds = SwordCombat::kSwingActiveDuration;
inline constexpr float kPlayerSwingRecoverySeconds = SwordCombat::kSwingRecoveryDuration;
inline constexpr float kPlayerUpwardSliceWindupSeconds =
    SwordCombat::kUpwardSliceWindupDuration;
inline constexpr float kPlayerUpwardSliceActiveSeconds =
    SwordCombat::kUpwardSliceActiveDuration;
inline constexpr float kPlayerUpwardSliceRecoverySeconds =
    SwordCombat::kUpwardSliceRecoveryDuration;
inline constexpr float kPlayerParryStartupSeconds = SwordCombat::kParryStartupDuration;
inline constexpr float kPlayerParryActiveSeconds = SwordCombat::kParryActiveDuration;
inline constexpr float kPlayerParryRecoverySeconds = SwordCombat::kParryRecoveryDuration;
// Freeze the authored raised pose at active-parry completion, immediately
// before its recovery lowers the sword. Gameplay still owns guard eligibility.
inline constexpr float kPlayerGuardNormalizedParryTime =
    (kPlayerParryStartupSeconds + kPlayerParryActiveSeconds) /
    (kPlayerParryStartupSeconds + kPlayerParryActiveSeconds + kPlayerParryRecoverySeconds);
inline constexpr float kLanternPoseBlendRatePerSecond = 1.0f / 0.65f;

enum class PlayerLocomotionClip : std::uint8_t
{
    Idle,
    Walk,
};

enum class PlayerUpperBodyAction : std::uint8_t
{
    None,
    Sword,
    UpwardSlice,
    Parry,
};

struct PlayerCombatLayer
{
    PlayerUpperBodyAction action = PlayerUpperBodyAction::None;
    float normalizedActionTime = 0.0f;
    float weight = 0.0f;
    // Snapshot metadata for the same shared hand/item blend. A nonzero weight
    // during an attack is presentation only and does not grant mitigation.
    float guardPoseWeight = 0.0f;

    bool operator==(const PlayerCombatLayer&) const = default;
};

struct PlayerVisibilityFlags
{
    bool headPrimaryVisible = false;
    bool nearFacePrimaryVisible = false;
    bool shadowVisible = true;
    bool reflectionVisible = true;
    std::uint32_t firstPersonMaskedPrimitiveFlags = 0x3u;

    bool operator==(const PlayerVisibilityFlags&) const = default;
};

// Transient visual layer only. Direction is in the current +Z-forward,
// +X-anatomical-left model basis, derived from the accepted world-space dodge.
struct PlayerDodgePresentation
{
    float weight = 0.0f;
    float modelX = 0.0f;
    float modelZ = 0.0f;
    bool operator==(const PlayerDodgePresentation&) const = default;
};

PlayerDodgePresentation EvaluatePlayerDodgePresentation(bool active,
    float elapsedSeconds, float durationSeconds, float acceptedWorldX,
    float acceptedWorldZ, float yawRadians, float torsoTwistRadians);

struct PlayerAnimationSnapshot
{
    PlayerLocomotionClip locomotionClip = PlayerLocomotionClip::Idle;
    float locomotionBlend = 0.0f;
    float locomotionTime = 0.0f;
    PlayerCombatLayer combatLayer{};
    horde::gameplay::CombatReaction reaction = horde::gameplay::CombatReaction::None;
    float reactionTime = 0.0f;
    float lanternPoseBlend = 0.0f;
    float swordStowBlend = 0.0f;
    float swordHandGripBlend = 1.0f;
    // Stationary traversal may publish the bounded native planted-leg pose.
    // Ordinary animation keeps this zero; it is not a crouch movement control.
    float plantedPelvisDropMetres = 0.0f;
    // Grounded Idle/Walk crouch lowers the torso while retaining animated feet.
    // Published separately from the stationary rescue stance; [0, 0.6] metres.
    float crouchPelvisDropMetres = 0.0f;
    PlayerDodgePresentation dodge{};
    PlayerArmIkTarget leftIk{};
    PlayerArmIkTarget rightIk{};
    PlayerVisibilityFlags visibility{};
    // Continuous correction to the legacy forward clock. A reverse walk uses
    // the same owned clip; this does not represent an authored backward run.
    float locomotionPhaseOffsetSeconds = 0.0f;

    bool operator==(const PlayerAnimationSnapshot&) const = default;
};

struct PlayerAnimationInput
{
    float walkAmount = 0.0f;
    float walkTime = 0.0f;
    horde::gameplay::PlayerCombatSnapshot playerCombat{};
    horde::gameplay::items::HeldItemKinematicsState heldItemKinematics{};
    float lanternPoseTarget = 0.0f;
    bool carryingRewardLantern = false;
    float lanternForwardAngleRadians = 0.0f;
    float lanternStrafeAngleRadians = 0.0f;
    bool carryingOriginalTorch = false;
    float locomotionPhaseOffsetSeconds = 0.0f;
};

PlayerLocomotionClip MapPlayerLocomotionClip(float locomotionBlend);
float UpdateBackwardWalkPhaseOffset(float currentOffset, float legacyAdvanceSeconds,
    float acceptedDistanceMetres, float acceptedForwardDistanceMetres, float walkSpeed);
float PlayerLocomotionSampleTime(const PlayerAnimationSnapshot& animation,
                                float clipDurationSeconds);
PlayerCombatLayer EvaluatePlayerCombatLayer(
    const horde::gameplay::PlayerCombatSnapshot& combat);

class PlayerAnimationState
{
public:
    void StepFixed(const PlayerAnimationInput& input, float fixedDeltaSeconds);
    void Reset();
    void Import(const PlayerAnimationSnapshot& snapshot);
    const PlayerAnimationSnapshot& Snapshot() const { return snapshot_; }

private:
    PlayerAnimationSnapshot snapshot_{};
};

} // namespace horde::gameplay::animation
