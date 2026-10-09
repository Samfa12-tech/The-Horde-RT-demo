#pragma once

#include "gameplay/items/HeldItemState.h"
#include "gameplay/simulation/SimulationSnapshot.h"

#include <string_view>
#include <cstddef>
#include <cstdint>
#include <cmath>

namespace horde::platform::windows
{

constexpr bool IsCaptureSwordFullyStowed(
    const horde::gameplay::items::HeldItemState& sword)
{
    using namespace horde::gameplay::items;
    return sword.id == HeldItemId::Sword && sword.hand == HeldHand::RightHand &&
        sword.parentMode == HeldItemParentMode::BodyStow && !sword.detached &&
        !sword.transition.active && sword.visualStowBlend == 1.0f &&
        sword.visualGripBlend == 0.0f;
}

struct CapturedPlayerGeometryEvidence
{
    bool currentUploadCaptured = false;
    bool allVertexPositionsFinite = false;
    std::size_t vertexCount = 0u;
    std::size_t faceCount = 0u;
};

constexpr bool IsValidCapturedPlayerGeometry(
    const CapturedPlayerGeometryEvidence& geometry)
{
    return geometry.currentUploadCaptured && geometry.allVertexPositionsFinite &&
        geometry.vertexCount >= 3u && geometry.faceCount != 0u;
}

inline bool HasVerticalProofRaisedCeilingRetractionWitness(
    const horde::gameplay::simulation::SimulationSnapshot& snapshot) noexcept
{
    // This validates only the authored raised proof pose. The caller separately
    // binds it to checkpoint 171, completed RT work, ownership and captured geometry.
    using namespace horde::gameplay;
    using namespace horde::gameplay::items;
    constexpr float poseTolerance = 0.00001f;
    const auto poseMatches = [=](const float actual, const float expected)
    {
        return std::isfinite(actual) && std::abs(actual - expected) <= poseTolerance;
    };
    const auto& kinematics = snapshot.heldItemKinematics;
    const auto& torch = snapshot.heldItems[0];
    return snapshot.developmentSupportFixture && snapshot.playerGrounded &&
        snapshot.playerSupportId == horde::gameplay::simulation::PlayerSupportId::ProofPlatform &&
        poseMatches(snapshot.playerX, 0.0f) && poseMatches(snapshot.playerZ, 0.0f) &&
        poseMatches(snapshot.playerYawRadians, 0.0f) &&
        poseMatches(snapshot.playerPitchRadians, -0.05f) &&
        poseMatches(snapshot.playerHeightDelta, horde::gameplay::simulation::kProofSupportHeight) &&
        poseMatches(snapshot.playerSupportWorldY,
             kRouteFloorWorldY + horde::gameplay::simulation::kProofSupportHeight) &&
        poseMatches(horde::gameplay::simulation::PlayerEyeWorldY(snapshot.playerSupportWorldY), 1.05f) &&
        snapshot.interaction.heldLightKind == interactions::HeldLightKind::Torch &&
        torch.id == HeldItemId::OriginalTorch && torch.hand == HeldHand::LeftHand &&
        torch.parentMode == HeldItemParentMode::HandSocket && !torch.detached &&
        !torch.transition.active &&
        IsCaptureSwordFullyStowed(snapshot.heldItems[1]) &&
        poseMatches(kinematics.swordStowBlend, 1.0f) &&
        poseMatches(kinematics.swordHandGripBlend, 0.0f) &&
        std::isfinite(kinematics.torchOverheadLowering) &&
        kinematics.torchOverheadLowering > 0.0f &&
        std::isfinite(kinematics.torchOverheadRetraction) &&
        kinematics.torchOverheadRetraction > 0.0f;
}

struct VerticalProofRaisedCaptureEvidence
{
    std::uint32_t checkpointId = 0u;
    std::string_view checkpointName{};
    bool checkpointAllowsCroppedArms = false;
    const horde::gameplay::simulation::SimulationSnapshot* simulation = nullptr;
    bool completedRtDispatch = false;
    bool completedRtPresentation = false;
    bool rtStorageImageCopied = false;
    bool dedicatedPlayerOwnership = false;
    bool primaryPlayerVisible = false;
    bool primaryPixelCounterAvailable = false;
    std::uint32_t primaryArmPixels = 0u;
    CapturedPlayerGeometryEvidence viewmodelGeometry{};
    CapturedPlayerGeometryEvidence worldBodyGeometry{};
};

inline bool AdmitsVerticalProofRaisedCeilingRetraction(
    const VerticalProofRaisedCaptureEvidence& evidence) noexcept
{
    return evidence.checkpointId == 171u &&
        evidence.checkpointName == "vertical-proof-raised" &&
        !evidence.checkpointAllowsCroppedArms && evidence.simulation != nullptr &&
        evidence.completedRtDispatch && evidence.completedRtPresentation &&
        evidence.rtStorageImageCopied && evidence.dedicatedPlayerOwnership &&
        evidence.primaryPlayerVisible && evidence.primaryPixelCounterAvailable &&
        evidence.primaryArmPixels == 0u &&
        IsValidCapturedPlayerGeometry(evidence.viewmodelGeometry) &&
        IsValidCapturedPlayerGeometry(evidence.worldBodyGeometry) &&
        HasVerticalProofRaisedCeilingRetractionWitness(*evidence.simulation);
}

inline bool HasExpectedPlayerCaptureVisibility(
    const bool dedicatedPlayerOwnership,
    const bool primaryPlayerVisible,
    const bool pixelCountersAvailable,
    const std::uint32_t primaryArmPixels,
    const bool primaryArmsMayBeOutsideFrame,
    const VerticalProofRaisedCaptureEvidence* ceilingRetractionEvidence = nullptr)
{
    return dedicatedPlayerOwnership && primaryPlayerVisible &&
        (!pixelCountersAvailable || primaryArmPixels != 0u ||
         primaryArmsMayBeOutsideFrame ||
         (ceilingRetractionEvidence != nullptr &&
          AdmitsVerticalProofRaisedCeilingRetraction(*ceilingRetractionEvidence)));
}

struct ClaimedRewardCapturePixelPolicy
{
    bool requirePlayerPixels = true;
    bool requireRewardRingPixels = false;
    bool requireRewardBodyPixels = true;
    bool requireSwordPixels = false;
    bool permitsCompleteWallRetraction = false;
};

constexpr ClaimedRewardCapturePixelPolicy ClaimedRewardCapturePolicy(
    const std::string_view checkpointName, const bool swordFullyStowed = false)
{
    if (checkpointName == "finale-roof")
    {
        // The authored finale now occurs after the route-local reward claim.
        // Prove that the reward replaced only the failed torch while the
        // skinned hand, top ring and hanging body remain visible. A fully
        // body-stowed sword can be outside primary view; a held sword must remain visible.
        return {
            .requirePlayerPixels = true,
            .requireRewardRingPixels = true,
            .requireRewardBodyPixels = true,
            .requireSwordPixels = !swordFullyStowed,
            .permitsCompleteWallRetraction = false,
        };
    }
    if (checkpointName == "lantern-wall-high" ||
        checkpointName == "lantern-wall-low")
    {
        // These checkpoints stage the exact maximum-clearance collision stop.
        // The shared carry path may retract the hand, ring, and body fully out
        // of the primary camera while the sword remains as a visible negative
        // control when held. Stable body stow may put it outside primary view.
        // Instance masks and GripRing authority are still mandatory.
        return {
            .requirePlayerPixels = false,
            .requireRewardRingPixels = false,
            .requireRewardBodyPixels = false,
            .requireSwordPixels = !swordFullyStowed,
            .permitsCompleteWallRetraction = true,
        };
    }
    if (checkpointName == "lantern-chest-held-high")
    {
        // Exact live post-claim stand-off: the chest collision footprint must
        // retract the carry safely without hiding the anatomical-left arm,
        // top ring, hanging body, or right-hand sword on a phone viewport.
        return {
            .requirePlayerPixels = true,
            .requireRewardRingPixels = true,
            .requireRewardBodyPixels = true,
            .requireSwordPixels = !swordFullyStowed,
            .permitsCompleteWallRetraction = false,
        };
    }
    return {};
}

} // namespace horde::platform::windows
