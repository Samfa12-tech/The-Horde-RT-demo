#pragma once

#include <cstdint>

namespace horde::gameplay::interactions
{

enum class HeldLightKind : std::uint8_t
{
    None,
    Torch,
    RewardLantern,
};

enum class HeldLightPose : std::uint8_t
{
    High,
    TransitioningToLow,
    Low,
    TransitioningToHigh,
};

struct InteractionPosition
{
    float x = 0.0f;
    float z = 0.0f;
};

struct InteractionQuery
{
    float playerX = 0.0f;
    float playerZ = 0.0f;
    float playerYawRadians = 0.0f;
};

struct InteractionState
{
    HeldLightKind heldLightKind = HeldLightKind::Torch;
    HeldLightPose heldLightPose = HeldLightPose::High;
    float heldLightPoseProgress = 1.0f;

    bool operator==(const InteractionState&) const = default;
};

inline constexpr InteractionPosition kRewardChestInteractionPosition{-12.0f, -15.20f};
inline constexpr float kInteractionMaximumRangeMetres = 1.35f;
inline constexpr float kInteractionFacingMaximumDegrees = 55.0f;
inline constexpr float kHeldLightPoseTransitionSeconds = 0.65f;

bool IsInteractionAvailable(const InteractionQuery& query,
                            const InteractionPosition& target);
void ResetInteractionState(InteractionState& state,
                           HeldLightKind heldLightKind = HeldLightKind::Torch);
void EquipRewardLantern(InteractionState& state);
bool RequestHeldLightPoseToggle(InteractionState& state);
void AdvanceHeldLightPose(InteractionState& state, float fixedDeltaSeconds);

} // namespace horde::gameplay::interactions
