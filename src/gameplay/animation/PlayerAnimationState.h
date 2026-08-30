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

struct PlayerAnimationSnapshot
{
    PlayerLocomotionClip locomotionClip = PlayerLocomotionClip::Idle;
    float locomotionBlend = 0.0f;
    float locomotionTime = 0.0f;
    PlayerCombatLayer combatLayer{};
    horde::gameplay::CombatReaction reaction = horde::gameplay::CombatReaction::None;
    float reactionTime = 0.0f;
    float lanternPoseBlend = 0.0f;
    PlayerArmIkTarget leftIk{};
    PlayerArmIkTarget rightIk{};
    PlayerVisibilityFlags visibility{};

    bool operator==(const PlayerAnimationSnapshot&) const = default;
};

struct PlayerAnimationInput
{
    float walkAmount = 0.0f;
    float walkTime = 0.0f;
    horde::gameplay::PlayerCombatSnapshot playerCombat{};
    horde::gameplay::items::HeldItemKinematicsState heldItemKinematics{};
    float lanternPoseTarget = 0.0f;
};

PlayerLocomotionClip MapPlayerLocomotionClip(float locomotionBlend);
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
