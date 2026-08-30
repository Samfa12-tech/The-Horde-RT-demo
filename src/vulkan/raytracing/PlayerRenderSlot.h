#pragma once

#include <array>
#include <cstdint>
#include <initializer_list>
#include <limits>
#include <string>
#include <vector>

#include "gameplay/animation/PlayerAnimationState.h"
#include "scene/assets/SkinnedMeshAsset.h"

namespace horde::vulkan::raytracing
{

inline constexpr float kPlayerGripSocketToleranceMetres = 0.015f;
inline constexpr float kPlayerGripOrientationToleranceRadians = 0.02f;
inline constexpr float kPlayerBootGroundingSafetyMetres = 0.00025f;

enum class PlayerRenderRoute : std::uint8_t
{
    Procedural,
    Skinned,
    // Stable first-person fallback: the skinned character remains available to
    // reflection/shadow rays, while four procedural arm segments own the
    // primary-camera view. This keeps the authored rig/socket authority without
    // presenting the deferred hand mesh in the player's view.
    HybridBlockPrimary,
};

struct PlayerRouteMasks
{
    std::array<std::uint8_t, 20u> instanceMasks{};
};

PlayerRouteMasks BuildPlayerRouteMasks(PlayerRenderRoute route);

// The imported player is authored +Z forward with anatomical Left on +X.
// Facing the gameplay -Z direction therefore requires a 180-degree Y rotation:
// model +X maps to gameplay left, not a reflection through the Z plane.
struct PlayerModelWorldBasis
{
    std::array<float, 3u> modelXInWorld{};
    std::array<float, 3u> modelYInWorld{{0.0f, 1.0f, 0.0f}};
    std::array<float, 3u> modelZInWorld{};
};

PlayerModelWorldBasis BuildPlayerModelWorldBasis(
    const std::array<float, 3u>& gameplayRightInWorld,
    const std::array<float, 3u>& gameplayForwardInWorld);

std::array<float, 3u> PlayerModelVectorToWorld(
    const PlayerModelWorldBasis& basis,
    const std::array<float, 3u>& modelVector);

std::array<float, 3u> WorldVectorToPlayerModel(
    const PlayerModelWorldBasis& basis,
    const std::array<float, 3u>& worldVector);

float PlayerModelWorldBasisDeterminant(const PlayerModelWorldBasis& basis);

struct ProductionSceneVisibilityInput
{
    PlayerRenderRoute requestedPlayerRoute = PlayerRenderRoute::Procedural;
    bool glassFixtureVisible = false;
    bool productionInspection = false;
    bool rewardLanternClaimed = false;
};

struct ProductionSceneVisibility
{
    bool rewardWorldVisible = false;
    bool inspectionOverride = false;
    PlayerRenderRoute playerRoute = PlayerRenderRoute::Procedural;
    std::uint8_t torchMask = 0u;
    std::uint8_t swordMask = 0u;
    std::uint8_t playerMask = 0u;
    bool playerPrimaryVisible = false;
    bool playerReflectionVisible = false;
};

ProductionSceneVisibility BuildProductionSceneVisibility(
    const ProductionSceneVisibilityInput& input);

// The torso is a stable player-space frame, not the midpoint of two IK
// effectors. A raised or retracted one-handed prop may move one clavicle, but
// must never drag the complete skinned body toward the camera.
std::array<float, 3u> EvaluatePlayerTorsoAnchorLocal(
    const horde::gameplay::animation::PlayerAnimationSnapshot& animation);

std::array<float, 3u> GroundPlayerRootOnRouteFloor(
    const std::array<float, 3u>& shoulderAnchoredRootWorld,
    float routeFloorWorldY,
    float assetGroundingOffsetMetres);

enum class PlayerPrimitiveSemantic : std::uint8_t
{
    Body,
    Head,
    NearFace,
};

struct PlayerPrimitiveVisibility
{
    bool primaryVisible = true;
    bool shadowVisible = true;
    bool reflectionVisible = true;
};

std::vector<PlayerPrimitiveVisibility> BuildPlayerPrimitiveVisibility(
    std::initializer_list<PlayerPrimitiveSemantic> semantics);

struct PlayerSocketPlan
{
    horde::gameplay::animation::TwoBoneIkSolution leftArm{};
    horde::gameplay::animation::TwoBoneIkSolution rightArm{};
    horde::gameplay::items::HeldItemTransform localFromLeftHand{};
    horde::gameplay::items::HeldItemTransform localFromRightHand{};
    float leftErrorMetres = 0.0f;
    float rightErrorMetres = 0.0f;
};

PlayerSocketPlan EvaluatePlayerSocketPlan(
    const horde::gameplay::animation::PlayerAnimationSnapshot& animation);

bool ResolvePlayerHeldItemVisuals(
    const horde::gameplay::items::HeldItemStates& authoritativeItems,
    const horde::gameplay::items::HeldItemTransform& worldFromLeftHandBone,
    const horde::gameplay::items::HeldItemTransform& worldFromRightHandBone,
    horde::gameplay::items::HeldItemStates& renderItems,
    std::string& diagnostic);

struct PlayerGripAgreement
{
    float positionErrorMetres = 0.0f;
    float orientationErrorRadians = 0.0f;
};

PlayerGripAgreement MeasurePlayerGripAgreement(
    const horde::gameplay::items::HeldItemState& authoritativeItem,
    const horde::gameplay::items::HeldItemState& renderedItem);

PlayerGripAgreement MeasureTransformAgreement(
    const horde::gameplay::items::HeldItemTransform& intended,
    const horde::gameplay::items::HeldItemTransform& rendered);

horde::gameplay::items::HeldItemTransform InverseRigidHeldItemTransform(
    const horde::gameplay::items::HeldItemTransform& transform);

struct RewardLanternVisualTransforms
{
    horde::gameplay::items::HeldItemTransform worldFromRing{};
    horde::gameplay::items::HeldItemTransform worldFromHinge{};
    horde::gameplay::items::HeldItemTransform worldFromBody{};
    PlayerGripAgreement gripAgreement{};
};

bool ComposeClaimedRewardLanternVisuals(
    const horde::gameplay::items::HeldItemTransform& worldFromFinalLeftGrip,
    const horde::gameplay::items::HeldItemTransform& ringFromGripRing,
    const horde::gameplay::items::HeldItemTransform& ringFromHinge,
    const horde::gameplay::items::HeldItemTransform& authoritativeWorldFromHinge,
    const horde::gameplay::items::HeldItemTransform& authoritativeWorldFromBody,
    float uniformScale,
    RewardLanternVisualTransforms& output,
    std::string& diagnostic);

enum class PlayerCpuSkinCadence : std::uint8_t
{
    Hz30,
    Hz60,
    RequiresReviewedBackend,
};

struct PlayerCpuCadenceMeasurements
{
    double hz30CostMilliseconds = 0.0;
    double hz60CostMilliseconds = 0.0;
    float hz30MaximumMotionErrorMetres = 0.0f;
    float hz60MaximumMotionErrorMetres = 0.0f;
};

PlayerCpuSkinCadence ChoosePlayerCpuCadence(
    const PlayerCpuCadenceMeasurements& measurements,
    float acceptedMotionErrorMetres = 0.01f,
    double acceptedCpuCostMilliseconds = 1.0);

bool PlayerPoseNeedsRefresh(std::uint64_t tickIndex,
                            std::uint64_t lastSkinnedTick,
                            PlayerCpuSkinCadence cadence);

class PlayerRenderSlot
{
public:
    bool LoadAsset(const std::string& runtimeGlbPath, std::string& diagnostic);
    bool PreparePose(const horde::gameplay::animation::PlayerAnimationSnapshot& animation,
                     std::uint64_t tickIndex,
                     PlayerCpuSkinCadence cadence,
                     bool& poseUpdated,
                     std::string& diagnostic);
    bool ShoulderCenter(const horde::gameplay::animation::PlayerAnimationSnapshot& animation,
                        std::array<float, 3u>& center,
                        std::string& diagnostic) const;
    bool ResolveHeldItemVisuals(
        const horde::gameplay::items::HeldItemStates& authoritativeItems,
        const horde::gameplay::items::HeldItemTransform& worldFromLeftGrip,
        const horde::gameplay::items::HeldItemTransform& worldFromRightGrip,
        horde::gameplay::items::HeldItemStates& renderItems,
        std::string& diagnostic);
    float BootGroundingOffsetMetres(
        const horde::gameplay::animation::PlayerAnimationSnapshot& animation) const;

    bool IsLoaded() const { return asset_.IsLoaded(); }
    const std::vector<horde::scene::TexturedSkinnedRtVertex>& UniqueVertices() const
    {
        return uniqueVertices_;
    }
    const std::vector<horde::scene::SkinnedPbrTangent>& UniqueTangents() const
    {
        return uniqueTangents_;
    }
    const horde::scene::SkinnedPlayerSockets& BoneSockets() const { return sockets_; }
    float LeftSocketErrorMetres() const { return leftSocketErrorMetres_; }
    float RightSocketErrorMetres() const { return rightSocketErrorMetres_; }
    const PlayerGripAgreement& LeftGripAgreement() const { return leftGripAgreement_; }
    const PlayerGripAgreement& RightGripAgreement() const { return rightGripAgreement_; }
    const horde::gameplay::items::HeldItemTransform& FinalWorldFromLeftGrip() const
    {
        return finalWorldFromLeftGrip_;
    }

private:
    bool DeriveAssetGripSockets(std::string& diagnostic);
    bool BuildBootGroundingProfiles(std::string& diagnostic);

    horde::scene::SkinnedMeshAsset asset_;
    std::vector<horde::scene::TexturedSkinnedRtVertex> uniqueVertices_;
    std::vector<horde::scene::SkinnedPbrTangent> uniqueTangents_;
    horde::scene::SkinnedPlayerSockets sockets_{};
    std::uint64_t lastSkinnedTick_ = std::numeric_limits<std::uint64_t>::max();
    horde::gameplay::animation::PlayerAnimationSnapshot lastPreparedAnimation_{};
    bool hasLastPreparedAnimation_ = false;
    float leftSocketErrorMetres_ = 0.0f;
    float rightSocketErrorMetres_ = 0.0f;
    horde::gameplay::items::HeldItemTransform leftHandFromGripSocket_ =
        horde::gameplay::items::IdentityHeldItemTransform();
    horde::gameplay::items::HeldItemTransform rightHandFromGripSocket_ =
        horde::gameplay::items::IdentityHeldItemTransform();
    PlayerGripAgreement leftGripAgreement_{};
    PlayerGripAgreement rightGripAgreement_{};
    horde::gameplay::items::HeldItemTransform finalWorldFromLeftGrip_ =
        horde::gameplay::items::IdentityHeldItemTransform();
    bool stableGripBasesReady_ = false;
    bool bootGroundingProfilesReady_ = false;
};

} // namespace horde::vulkan::raytracing
