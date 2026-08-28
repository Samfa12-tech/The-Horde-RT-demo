#pragma once

#include "gameplay/items/HeldLightState.h"
#include "gameplay/items/HeldItemState.h"
#include "gameplay/ShowcaseGameplay.h"
#include "gameplay/SwordCombat.h"
#include "gameplay/interactions/InteractionState.h"
#include "scene/assets/StaticMeshAsset.h"

#include <span>
#include <string>
#include <string_view>

namespace horde::gameplay::items
{

inline constexpr float kSwordGripRollRadians = 1.3962634f;

struct HeldItemKinematicsInput
{
    float cameraX = 0.0f;
    float cameraZ = 1.85f;
    float cameraYawRadians = 0.0f;
    float walkTime = 0.0f;
    float walkAmount = 0.0f;
    TorchFailureSnapshot torchFailure{};
    PlayerCombatSnapshot playerCombat{};
    float swordSwingRadians = 0.0f;
    horde::gameplay::interactions::InteractionState interaction{};
};

struct HeldSwordPose
{
    std::array<float, 3u> rightHandLocal{};
    float swordRadians = 0.0f;
    float swordForwardRadians = 0.0f;
    float parryBlend = 0.0f;
    float successJolt = 0.0f;
};

struct HeldItemKinematicsState
{
    std::array<float, 3u> leftShoulderLocal{};
    std::array<float, 3u> rightShoulderLocal{};
    std::array<float, 3u> leftHandLocal{};
    std::array<float, 3u> rightHandLocal{};
    float heldPropDepth = 1.05f;
    float rewardLanternPresentationYawRadians = 0.0f;
    float swordRadians = 0.0f;
    float swordForwardRadians = 0.0f;
    float parryBlend = 0.0f;
    float successJolt = 0.0f;
};

struct HeldItemFixedStepInput
{
    float playerX = 0.0f;
    float playerZ = 1.85f;
    float playerYawRadians = 0.0f;
    float playerPitchRadians = -0.05f;
    float walkTime = 0.0f;
    float walkAmount = 0.0f;
    TorchFailureSnapshot torchFailure{};
    PlayerCombatSnapshot playerCombat{};
    float swordSwingRadians = 0.0f;
    horde::gameplay::interactions::InteractionState interaction{};
};

struct HeldItemFixedStepState
{
    HeldItemKinematicsState kinematics{};
    HeldLightState light{};
    HeldItemTransform worldFromLeftHand{};
};

struct SwordGripBasisInView
{
    // The authored sword is +Y blade-long, +X toward its sharpened edges,
    // and +Z normal to the broad flat. These axes are expressed in view
    // right/up/forward coordinates after the RightHand Grip roll.
    std::array<float, 3u> edgeDirection{};
    std::array<float, 3u> bladeAxis{};
    std::array<float, 3u> flatNormal{};
};

struct FirstPersonSafeFrame
{
    float minimumNdcX = 0.0f;
    float maximumNdcX = 0.0f;
    bool includesTorchGrip = false;
    bool includesFlame = false;
    bool includesLight = false;
    bool includesSwordGrip = false;
    bool includesBladeBounds = false;
};

HeldSwordPose EvaluateHeldSwordPose(const PlayerCombatSnapshot& playerCombat,
                                   float swordSwingRadians,
                                   float heldPropDepth);

std::array<float, 3u> EvaluateSwordBladeAxisInView(float inwardRadians,
                                                   float forwardRadians);

SwordGripBasisInView EvaluateSwordGripBasisInView(float inwardRadians,
                                                  float forwardRadians,
                                                  float gripRollRadians);

FirstPersonSafeFrame EvaluateOwnerFeedbackPortraitSafeFrame(
    const HeldItemKinematicsState& kinematics,
    float portraitAspect);

HeldItemKinematicsState EvaluateHeldItemKinematics(const HeldItemKinematicsInput& input);

const horde::scene::assets::StaticSocket* FindHeldItemSocket(
    std::span<const horde::scene::assets::StaticSocket> sockets,
    std::string_view name);

bool ValidateHeldItemSocketTransform(const HeldItemTransform& transform,
                                     std::string& diagnostic);

bool ComposeWorldFromItem(const HeldItemTransform& worldFromHandSocket,
                          const HeldItemTransform& itemFromGrip,
                          HeldItemTransform& worldFromItem,
                          std::string& diagnostic);

HeldItemTransform MultiplyHeldItemTransforms(const HeldItemTransform& left,
                                             const HeldItemTransform& right);

HeldItemTransform SelectHandSocketTransform(HeldHand hand,
                                            const HeldItemTransform& worldFromLeftHand,
                                            const HeldItemTransform& worldFromRightHand);

bool ResolveHeldItemsFixedStep(HeldItemStates& items,
                               const HeldItemFixedStepInput& input,
                               std::uint64_t tick,
                               HeldItemFixedStepState& state,
                               std::string& diagnostic);

} // namespace horde::gameplay::items
