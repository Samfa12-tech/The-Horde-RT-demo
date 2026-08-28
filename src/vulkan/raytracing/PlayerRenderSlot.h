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

enum class PlayerRenderRoute : std::uint8_t
{
    Procedural,
    Skinned,
};

struct PlayerRouteMasks
{
    std::array<std::uint8_t, 20u> instanceMasks{};
};

PlayerRouteMasks BuildPlayerRouteMasks(PlayerRenderRoute route);

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
        const horde::gameplay::items::HeldItemTransform& worldFromLeftHandBone,
        const horde::gameplay::items::HeldItemTransform& worldFromRightHandBone,
        horde::gameplay::items::HeldItemStates& renderItems,
        std::string& diagnostic);

    bool IsLoaded() const { return asset_.IsLoaded(); }
    const std::vector<horde::scene::TexturedSkinnedRtVertex>& UniqueVertices() const
    {
        return uniqueVertices_;
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
    bool DeriveStableRestGripBases(std::string& diagnostic);

    horde::scene::SkinnedMeshAsset asset_;
    std::vector<horde::scene::TexturedSkinnedRtVertex> uniqueVertices_;
    horde::scene::SkinnedPlayerSockets sockets_{};
    std::uint64_t lastSkinnedTick_ = std::numeric_limits<std::uint64_t>::max();
    float leftSocketErrorMetres_ = 0.0f;
    float rightSocketErrorMetres_ = 0.0f;
    horde::gameplay::items::HeldItemTransform leftBoneFromGripSocket_ =
        horde::gameplay::items::IdentityHeldItemTransform();
    horde::gameplay::items::HeldItemTransform rightBoneFromGripSocket_ =
        horde::gameplay::items::IdentityHeldItemTransform();
    horde::gameplay::items::HeldItemTransform leftRestHandOrientation_ =
        horde::gameplay::items::IdentityHeldItemTransform();
    horde::gameplay::items::HeldItemTransform rightRestHandOrientation_ =
        horde::gameplay::items::IdentityHeldItemTransform();
    horde::gameplay::items::HeldItemTransform leftRestGripBasisInPlayer_ =
        horde::gameplay::items::IdentityHeldItemTransform();
    horde::gameplay::items::HeldItemTransform rightRestGripBasisInPlayer_ =
        horde::gameplay::items::IdentityHeldItemTransform();
    PlayerGripAgreement leftGripAgreement_{};
    PlayerGripAgreement rightGripAgreement_{};
    horde::gameplay::items::HeldItemTransform finalWorldFromLeftGrip_ =
        horde::gameplay::items::IdentityHeldItemTransform();
    bool stableGripBasesReady_ = false;
};

} // namespace horde::vulkan::raytracing
