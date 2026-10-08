#pragma once

#include "gameplay/items/HeldLightState.h"
#include "gameplay/items/HeldItemState.h"
#include "gameplay/ShowcaseGameplay.h"
#include "gameplay/SwordCombat.h"
#include "gameplay/interactions/InteractionState.h"
#include "scene/assets/StaticMeshAsset.h"

#include <span>
#include <cstdint>
#include <string>
#include <string_view>

namespace horde::gameplay::items
{

// Owner-requested 12% reduction from the .50 presentation. Ring, cage,
// dielectric geometry and authored Flame/Light sockets share this scale;
// the gameplay-owned hand Grip remains fixed.
inline constexpr float kClaimedRewardLanternScale = 0.44f;

inline constexpr float kSwordGripRollRadians = 1.3962634f;

// Original production torch: Grip->Flame +.525 m, visible fire .34 m. Keep
// this legacy admission value for its direct asset contract.
inline constexpr float kHeldTorchEnvelopeTopFromGrip = 0.925f;
// Player Rag torch: authored Flame is .565 m above Grip. The unchanged engine
// fire extends .34 m above it, plus .06 m for its animated tip.
inline constexpr float kPlayerRagTorchEnvelopeTopFromGrip = 0.965f;
inline constexpr float kHeldTorchEnvelopeRadius = 0.15f;
inline constexpr float kHeldTorchOverheadGap = 0.03f;
// Imported player rig palm socket: desired wrist target relative to its Grip,
// expressed in the canonical left-Grip view frame. The actual-rig clearance
// regression pins this profile against the loaded LeftHand/LeftGrip nodes.
inline constexpr std::array<float, 3u> kPlayerPalmHandOffsetFromGripInView{{
    -0.0336753f, 0.000359222f, -0.0596841f}};
// Runtime player rig bind reach from the scaled LeftArm/LeftForeArm/LeftHand
// hierarchy, bounded by the imported solver's 1.75x maximum stretch and a
// 5 mm reserve against its straight-chain singularity.
inline constexpr float kPlayerAnatomicalHandReachLimitMetres =
    (0.276333f + 0.216431f) * 1.75f - 0.005f;

// Shared pose authority, not a renderer-only offset. The anatomical profile is
// opt-in until the modelled body/viewmodel route passes live owner acceptance.
enum class PlayerMountProfile : std::uint8_t { LegacyViewRelative, AnatomicalBody };

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
    PlayerMountProfile playerMountProfile = PlayerMountProfile::LegacyViewRelative;
    float cameraPitchRadians = 0.0f;
    // Logical presentation aspect, used only to spread held hand targets in
    // wide views. Portrait and square views retain the authored targets.
    float logicalViewAspect = 1.0f;
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
    // The left grip is deliberately rolled around its vertical handle axis so
    // first-person primary rays see the knuckles/back of the anatomical left
    // hand rather than looking end-on into a palm capsule. These axes remain
    // shared fixed-step state: rig IK and rigid held-item composition consume
    // the same frame.
    std::array<float, 3u> leftGripXInView{{1.0f, 0.0f, 0.0f}};
    std::array<float, 3u> leftGripYInView{{0.0f, 1.0f, 0.0f}};
    std::array<float, 3u> leftGripZInView{{0.0f, 0.0f, -1.0f}};
    float heldPropDepth = 1.05f;
    float rewardLanternPresentationYawRadians = 0.0f;
    float swordRadians = 0.0f;
    float swordForwardRadians = 0.0f;
    float parryBlend = 0.0f;
    float successJolt = 0.0f;
    // World-down translation of the shared hand target, never a light offset.
    float torchOverheadLowering = 0.0f;
    // Camera-side retreat of that same torch target when overhead lowering
    // would exceed the anatomical arm's reachable carry envelope.
    float torchOverheadRetraction = 0.0f;
    // World-down translation of the shared sword hand target. IK, rigid sword
    // composition, shadows, and reflections consume the same lowered frame.
    float swordOverheadLowering = 0.0f;
    // Camera-side retreat of that same target, applied only when ceiling
    // lowering would exceed the production arm's reachable carry envelope.
    float swordOverheadRetraction = 0.0f;
    // Renderer consumes this snapshot-safe blend to compose the sword between
    // the animated Hips mount and the solved hand Grip.
    float swordStowBlend = 0.0f;
    // The same fixed-step transition releases/reacquires the right hand Grip;
    // zero returns to the shared empty-hand carry, one follows the item Grip.
    float swordHandGripBlend = 1.0f;
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
    PlayerMountProfile playerMountProfile = PlayerMountProfile::LegacyViewRelative;
    const HeldItemState* swordItemState = nullptr;
    float logicalViewAspect = 1.0f;
};

inline constexpr float kHeldItemSpreadStartAspect = 1.0f;
inline constexpr float kHeldItemSpreadMaximumAspect = 16.0f / 9.0f;
inline constexpr float kHeldItemSpreadMaximumMetres = 0.03f;

float ComputeHeldItemAspectSpread(float logicalViewAspect) noexcept;

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
                                   float heldPropDepth,
                                   bool bulkyLeftHandCarry = false,
                                   float idleTimeSeconds = 0.0f);

std::array<float, 3u> EvaluateSwordBladeAxisInView(float inwardRadians,
                                                   float forwardRadians);

SwordGripBasisInView EvaluateSwordGripBasisInView(float inwardRadians,
                                                  float forwardRadians,
                                                  float gripRollRadians);

FirstPersonSafeFrame EvaluateOwnerFeedbackPortraitSafeFrame(
    const HeldItemKinematicsState& kinematics,
    float portraitAspect);

// Player-held torch sockets come from the Rag runtime asset. OriginalTorch*
// remain the legacy contracts for Keeper and reward scene assets.
HeldItemTransform PlayerRagTorchGripSocketTransform();
HeldItemTransform PlayerRagTorchFlameSocketTransform();
HeldItemTransform PlayerRagTorchLightSocketTransform();
// Original sword mount authored in the runtime player's +Z-forward model
// frame. Hips-relative, blade-down, right hip (model -X), broad face forward.
HeldItemTransform SwordBodyStowFromHips();
// Interpolate the Grip frame itself so a non-origin Grip follows a straight,
// reachable path between the hand and animated body attachment.
HeldItemTransform BlendHeldItemTransformsAtGrip(
    const HeldItemTransform& fromItem,
    const HeldItemTransform& toItem,
    const HeldItemTransform& itemFromGrip,
    float blend);
// Shared conservative roof response, exposed for the actual-rig regression's
// bounded candidate sweep; production kinematics uses this same calculation.
float ComputePlayerTorchOverheadLowering(
    const std::array<float, 3u>& gripWorld,
    const std::array<float, 3u>& viewUp,
    const std::array<float, 3u>& viewForward);

// Resolves the first full-height route collision surface continuously along
// the camera-centre ray. Low floor props such as the chest remain player-solid
// without masquerading as walls for elevated held items. The predicate owns
// the player-radius inset; reward composition separately reserves cage extent.
float ComputeRewardLanternForwardClearance(float cameraX,
                                           float cameraZ,
                                           float forwardX,
                                           float forwardZ);

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
