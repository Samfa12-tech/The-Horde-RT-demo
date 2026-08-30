#include "gameplay/DevelopmentCheckpoints.h"
#include "gameplay/DevelopmentCheckpointSimulation.h"
#include "gameplay/ShowcaseCheckpoints.h"
#include "vulkan/raytracing/DevelopmentStaticAssetPolicy.h"

#include <cmath>
#include <filesystem>
#include <iostream>
#include <string>
#include <string_view>

namespace
{

int failures = 0;

void Check(bool condition, std::string_view message)
{
    if (!condition)
    {
        std::cerr << "FAIL: " << message << '\n';
        ++failures;
    }
}

} // namespace

int main()
{
    using namespace horde::gameplay;
    using namespace horde::vulkan::raytracing;

    Check(kShowcaseCheckpoints.size() == 13u,
          "the existing release checkpoint array remains exactly 13 entries");
    Check(FindShowcaseCheckpoint("pbr-sword-closeup") == nullptr,
          "development proof does not enter the release checkpoint lookup");
    Check(FindShowcaseCheckpoint("pbr-torch-fire") == nullptr,
          "torch development proof does not enter the release checkpoint lookup");
    Check(FindShowcaseCheckpoint("player-body-grips") == nullptr,
          "player-body proof does not enter the release checkpoint lookup");
    Check(kDevelopmentCheckpoints.size() == 36u,
          "thirty-six isolated render-development, lantern stress, wall, pitch, and chest-clearance checkpoints are exposed");
    const DevelopmentCheckpoint* checkpoint = FindDevelopmentCheckpoint("pbr-sword-closeup");
    Check(checkpoint != nullptr && checkpoint->id == 100 && checkpoint->baseShowcaseCheckpointId == 0 &&
              checkpoint->name == std::string_view("pbr-sword-closeup") &&
              checkpoint->cameraX == 0.0f && checkpoint->cameraZ == 1.85f,
          "development close-up has a stable identity and imports the opening gameplay state");
    const DevelopmentCheckpoint* torch = FindDevelopmentCheckpoint("pbr-torch-fire");
    Check(torch != nullptr && torch->id == 101 && torch->baseShowcaseCheckpointId == 0 &&
              torch->cameraX == 0.0f && torch->cameraZ == 1.85f,
          "production torch/fire close-up has a stable isolated identity");
    const DevelopmentCheckpoint* player = FindDevelopmentCheckpoint("player-body-grips");
    Check(player != nullptr && player->id == 102 && player->baseShowcaseCheckpointId == 0 &&
              player->cameraX == 0.0f && player->cameraZ == 1.85f &&
              player->pitch < -0.28f,
          "player body/grips proof has a stable downward-view identity");
    const DevelopmentCheckpoint* fallback = FindDevelopmentCheckpoint("player-fallback-grips");
    Check(fallback != nullptr && fallback->baseShowcaseCheckpointId == 0 &&
              fallback->pitch == player->pitch,
          "procedural player A/B checkpoint uses the exact skinned grip view");
    Check(FindDevelopmentCheckpoint(102) == player && FindDevelopmentCheckpoint(105) == fallback,
          "Android debug automation resolves bounded development checkpoints by ID");
    const DevelopmentCheckpoint* ownerFeedback =
        FindDevelopmentCheckpoint("player-body-owner-feedback");
    Check(ownerFeedback != nullptr && ownerFeedback->id == 106 &&
              ownerFeedback->baseShowcaseCheckpointId == 0 && ownerFeedback->pitch < -0.25f,
          "owner arm-root and sword-cant regression has a stable first-person checkpoint");
    const DevelopmentCheckpoint* downward =
        FindDevelopmentCheckpoint("player-body-downward-cut");
    const DevelopmentCheckpoint* upward =
        FindDevelopmentCheckpoint("player-body-upward-slice");
    Check(downward != nullptr && downward->id == 107 &&
              downward->combatPose == DevelopmentCombatPose::DownwardCutActive &&
              upward != nullptr && upward->id == 108 &&
              upward->combatPose == DevelopmentCombatPose::UpwardSliceActive,
          "authoritative downward/upward owner regression poses have stable debug identities");
    const DevelopmentCheckpoint* glass =
        FindDevelopmentCheckpoint("glass-transport");
    Check(glass != nullptr && glass->id == 109 && glass->baseShowcaseCheckpointId == 4 &&
              glass->usesGlassFixture,
          "generic dielectric inspection has a stable debug identity and explicitly enables its fixture");
    const DevelopmentCheckpoint* glassFire =
        FindDevelopmentCheckpoint("glass-fire-transport");
    Check(glassFire != nullptr && glassFire->id == 110 &&
              glassFire->baseShowcaseCheckpointId == 0 && glassFire->usesGlassFixture,
          "fire-on dielectric proof reuses the exact imported fixture and camera");
    const DevelopmentCheckpoint* tintedGlass =
        FindDevelopmentCheckpoint("glass-tinted-transport");
    Check(tintedGlass != nullptr && tintedGlass->id == 111 &&
              tintedGlass->baseShowcaseCheckpointId == 0 && tintedGlass->usesGlassFixture &&
              tintedGlass->glassAttenuationColor[0] < 0.2f &&
              tintedGlass->glassAttenuationColor[1] > 0.7f &&
              tintedGlass->glassAttenuationDistance < 0.5f,
          "tinted dielectric proof has an intentionally channel-separated Beer-Lambert profile");
    const DevelopmentCheckpoint* millimetreGlass =
        FindDevelopmentCheckpoint("glass-millimetre-closed");
    Check(millimetreGlass != nullptr && millimetreGlass->id == 112 &&
              millimetreGlass->baseShowcaseCheckpointId == 4 &&
              millimetreGlass->usesGlassFixture &&
              millimetreGlass->glassDepthScale == 0.005f,
          "millimetre dielectric proof scales the same 0.2 m closed fixture to exactly 1 mm");
    const DevelopmentCheckpoint* edgeGlass =
        FindDevelopmentCheckpoint("glass-edge-fresnel");
    Check(edgeGlass != nullptr && edgeGlass->id == 113 &&
              edgeGlass->baseShowcaseCheckpointId == 4 && edgeGlass->usesGlassFixture &&
              edgeGlass->yaw < -0.50f && edgeGlass->yaw > -0.80f,
          "edge Fresnel proof retains a stable glancing camera and imported fixture route");
    const DevelopmentCheckpoint* chestUnlock =
        FindDevelopmentCheckpoint("lantern-chest-unlock");
    const DevelopmentCheckpoint* productionGlass =
        FindDevelopmentCheckpoint("lantern-glass-production");
    Check(chestUnlock != nullptr && chestUnlock->id == 114 &&
              chestUnlock->usesProductionRewardProps &&
              chestUnlock->stagesUnlockedChest &&
              !chestUnlock->productionLanternGlassOnly &&
              chestUnlock->cameraX == kRewardChestRoutePosition.x + 1.85f &&
              chestUnlock->cameraZ == kRewardChestRoutePosition.z &&
              QueryShowcaseZone(chestUnlock->cameraX, chestUnlock->cameraZ) ==
                  ShowcaseZone::Finale &&
              chestUnlock->pitch == -0.35f,
          "production chest unlock has a stable finale-room interaction framing");
    horde::gameplay::simulation::GameSimulation stagedChestGuidance;
    Check(chestUnlock != nullptr &&
              StageDevelopmentCheckpointSimulation(stagedChestGuidance,
                                                   *chestUnlock) &&
              stagedChestGuidance.Snapshot().chestReward.phase ==
                  interactions::ChestRewardPhase::ClosedUnlocked &&
              !stagedChestGuidance.Snapshot().chestReward.unlockPending &&
              stagedChestGuidance.Snapshot().finale.lichDefeated &&
              !stagedChestGuidance.Snapshot().finale.lanternClaimed,
          "chest-unlock checkpoint must freeze the exact post-delay light-on state before opening or claiming");
    Check(productionGlass != nullptr && productionGlass->id == 115 &&
              productionGlass->usesProductionRewardProps &&
              productionGlass->productionLanternGlassOnly &&
              productionGlass->cameraX == kRewardChestRoutePosition.x + 1.85f &&
              productionGlass->cameraZ == kRewardChestRoutePosition.z &&
              productionGlass->pitch == -0.35f,
          "production lantern glass has a stable full-bounds close inspection route");
    const DevelopmentCheckpoint* heldHigh = FindDevelopmentCheckpoint("lantern-held-high");
    const DevelopmentCheckpoint* heldLow = FindDevelopmentCheckpoint("lantern-held-low");
    const DevelopmentCheckpoint* glassTransmission =
        FindDevelopmentCheckpoint("lantern-glass-transmission");
    const DevelopmentCheckpoint* motionExtreme =
        FindDevelopmentCheckpoint("lantern-motion-extreme");
    Check(heldHigh != nullptr && heldHigh->id == 116 &&
              heldHigh->rewardPose == DevelopmentRewardPose::HeldHigh &&
              heldLow != nullptr && heldLow->id == 117 &&
              heldLow->rewardPose == DevelopmentRewardPose::HeldLow &&
              glassTransmission != nullptr && glassTransmission->id == 118 &&
              glassTransmission->rewardPose == DevelopmentRewardPose::GlassTransmission &&
              motionExtreme != nullptr && motionExtreme->id == 119 &&
              motionExtreme->rewardPose == DevelopmentRewardPose::MotionExtreme,
          "Task 8 held/glass/extreme checkpoints append stable IDs 116 through 119");
    horde::gameplay::simulation::GameSimulation stagedDownward;
    horde::gameplay::simulation::GameSimulation stagedUpward;
    DevelopmentCheckpointStageEvidence downwardEvidence{};
    DevelopmentCheckpointStageEvidence upwardEvidence{};
    Check(downward != nullptr &&
              StageDevelopmentCheckpointSimulation(stagedDownward, *downward,
                                                   &downwardEvidence) &&
              stagedDownward.Snapshot().playerCombat.action ==
                  PlayerCombatAction::SwingActive &&
              downwardEvidence.actionTime >=
                  SwordCombat::kSwingActiveDuration - 0.025f &&
              downwardEvidence.consumedAttackEdges == 1u &&
              downwardEvidence.playerSwingEvents == 1u && stagedDownward.Events().Empty(),
          "downward capture must stage one exact shared swing before freezing feedback");
    Check(upward != nullptr &&
              StageDevelopmentCheckpointSimulation(stagedUpward, *upward,
                                                   &upwardEvidence) &&
              stagedUpward.Snapshot().playerCombat.action ==
                  PlayerCombatAction::UpwardSliceActive &&
              upwardEvidence.actionTime >=
                  SwordCombat::kUpwardSliceActiveDuration - 0.025f &&
              upwardEvidence.consumedAttackEdges == 2u &&
              upwardEvidence.playerSwingEvents == 2u && stagedUpward.Events().Empty(),
          "upward capture must stage two exact shared swings before freezing feedback");
    horde::gameplay::simulation::GameSimulation stagedHigh;
    horde::gameplay::simulation::GameSimulation stagedLow;
    horde::gameplay::simulation::GameSimulation stagedGlassTransmission;
    horde::gameplay::simulation::GameSimulation stagedMotionExtreme;
    Check(heldHigh != nullptr && StageDevelopmentCheckpointSimulation(stagedHigh, *heldHigh) &&
              stagedHigh.Snapshot().interaction.heldLightPose ==
                  interactions::HeldLightPose::High &&
              stagedHigh.Snapshot().lanternPendulum.forwardAngleRadians == 0.0f &&
              stagedHigh.Snapshot().lanternPendulum.torsionAngleRadians == 0.0f,
          "held-high checkpoint imports a frozen rest pendulum and real high carry state");
    Check(heldLow != nullptr && StageDevelopmentCheckpointSimulation(stagedLow, *heldLow) &&
              stagedLow.Snapshot().interaction.heldLightPose ==
                  interactions::HeldLightPose::Low &&
              stagedLow.Snapshot().lanternPendulum.strafeAngularVelocity == 0.0f &&
              stagedLow.Snapshot().lanternPendulum.torsionAngularVelocity == 0.0f,
          "held-low checkpoint imports a frozen rest pendulum and real low carry state");
    Check(glassTransmission != nullptr &&
              StageDevelopmentCheckpointSimulation(stagedGlassTransmission,
                                                   *glassTransmission) &&
              stagedGlassTransmission.Snapshot().lanternPendulum.forwardAngleRadians == 0.30f &&
              stagedGlassTransmission.Snapshot().lanternPendulum.strafeAngleRadians == -0.16f &&
              stagedGlassTransmission.Snapshot().lanternPendulum.torsionAngleRadians == 0.12f &&
              stagedGlassTransmission.Snapshot().lanternPendulum.torsionAngularVelocity == -0.40f,
          "glass-transmission checkpoint freezes exact swing/torsion shared body state");
    Check(motionExtreme != nullptr &&
              StageDevelopmentCheckpointSimulation(stagedMotionExtreme, *motionExtreme) &&
              stagedMotionExtreme.Snapshot().lanternPendulum.forwardAngleRadians == 0.82f &&
              stagedMotionExtreme.Snapshot().lanternPendulum.strafeAngularVelocity == -1.60f &&
              stagedMotionExtreme.Snapshot().lanternPendulum.torsionAngleRadians == 0.28f &&
              stagedMotionExtreme.Snapshot().lanternPendulum.torsionAngularVelocity == -0.90f &&
              stagedMotionExtreme.Snapshot().lanternPendulum.previousPivotVelocity[0] == 3.8f,
          "motion-extreme checkpoint preserves exact imported swing/torsion and hinge history");
    const DevelopmentCheckpoint* wallHigh =
        FindDevelopmentCheckpoint("lantern-wall-high");
    const DevelopmentCheckpoint* wallLow =
        FindDevelopmentCheckpoint("lantern-wall-low");
    Check(wallHigh != nullptr && wallHigh->id == 132 &&
              wallHigh->baseShowcaseCheckpointId == 2 &&
              wallHigh->cameraX == 0.0f && wallHigh->cameraZ == -9.70f &&
              wallHigh->yaw == 0.0f &&
              wallHigh->rewardPose == DevelopmentRewardPose::HeldHigh &&
              wallHigh->rewardForwardAngleRadians == 0.0f &&
              wallHigh->rewardStrafeAngleRadians == 0.0f &&
              wallHigh->rewardTorsionAngleRadians == 0.0f,
          "near-wall high checkpoint appends stable ID 132 at the real z=-10 fixture");
    Check(wallLow != nullptr && wallLow->id == 133 &&
              wallLow->baseShowcaseCheckpointId == 2 &&
              wallLow->cameraX == 0.0f && wallLow->cameraZ == -9.70f &&
              wallLow->yaw == 0.0f &&
              wallLow->rewardPose == DevelopmentRewardPose::HeldLow &&
              wallLow->rewardForwardAngleRadians == 0.0f &&
              wallLow->rewardStrafeAngleRadians == 0.52359877560f &&
              wallLow->rewardTorsionAngleRadians == 0.34906585040f,
          "near-wall low checkpoint appends stable ID 133 with representative swing and torsion");
    const DevelopmentCheckpoint* lookUp =
        FindDevelopmentCheckpoint("lantern-held-look-up");
    Check(lookUp != nullptr && lookUp->id == 134 &&
              lookUp->rewardPose == DevelopmentRewardPose::HeldHigh &&
              lookUp->pitch == 0.28f,
          "maximum-upward-look lantern checkpoint protects the block forearm cull regression");
    const DevelopmentCheckpoint* chestHeldHigh =
        FindDevelopmentCheckpoint("lantern-chest-held-high");
    Check(chestHeldHigh != nullptr && chestHeldHigh->id == 135 &&
              chestHeldHigh->baseShowcaseCheckpointId == 10 &&
              chestHeldHigh->cameraX == kRewardChestRoutePosition.x + 1.30f &&
              chestHeldHigh->cameraZ == kRewardChestRoutePosition.z &&
              chestHeldHigh->yaw == -1.57079632679f &&
              chestHeldHigh->rewardPose == DevelopmentRewardPose::HeldHigh &&
              QueryShowcaseZone(chestHeldHigh->cameraX, chestHeldHigh->cameraZ) ==
                  ShowcaseZone::Finale,
          "post-claim chest checkpoint freezes the exact finale-room carry pose above the low chest footprint");
    horde::gameplay::simulation::GameSimulation stagedWallHigh;
    horde::gameplay::simulation::GameSimulation stagedWallLow;
    horde::gameplay::simulation::GameSimulation stagedLookUp;
    horde::gameplay::simulation::GameSimulation stagedChestHeldHigh;
    Check(lookUp != nullptr &&
              StageDevelopmentCheckpointSimulation(stagedLookUp, *lookUp) &&
              stagedLookUp.Snapshot().playerPitchRadians == 0.28f &&
              stagedLookUp.Snapshot().interaction.heldLightPose ==
                  interactions::HeldLightPose::High,
          "maximum-upward-look checkpoint freezes the exact claimed-lantern pitch and pose");
    Check(chestHeldHigh != nullptr &&
              StageDevelopmentCheckpointSimulation(stagedChestHeldHigh,
                                                   *chestHeldHigh) &&
              stagedChestHeldHigh.Snapshot().playerX ==
                  kRewardChestRoutePosition.x + 1.30f &&
              stagedChestHeldHigh.Snapshot().playerZ ==
                  kRewardChestRoutePosition.z &&
              stagedChestHeldHigh.Snapshot().interaction.heldLightPose ==
                  interactions::HeldLightPose::High &&
              stagedChestHeldHigh.Snapshot().heldItemKinematics.
                  leftHandLocal[2] >= 1.02f &&
              stagedChestHeldHigh.Snapshot().heldItemKinematics.
                  leftHandLocal[2] <= 1.05f &&
              items::ComputeRewardLanternForwardClearance(
                  stagedChestHeldHigh.Snapshot().playerX,
                  stagedChestHeldHigh.Snapshot().playerZ,
                  std::sin(stagedChestHeldHigh.Snapshot().playerYawRadians),
                  -std::cos(stagedChestHeldHigh.Snapshot().playerYawRadians)) >
                  2.65f &&
              items::ComputeRewardLanternForwardClearance(
                  stagedChestHeldHigh.Snapshot().playerX,
                  stagedChestHeldHigh.Snapshot().playerZ,
                  std::sin(stagedChestHeldHigh.Snapshot().playerYawRadians),
                  -std::cos(stagedChestHeldHigh.Snapshot().playerYawRadians)) <
                  2.67f,
          "post-claim chest checkpoint preserves open held-prop clearance while player movement remains blocked by the floor chest");
    Check(wallHigh != nullptr &&
              StageDevelopmentCheckpointSimulation(stagedWallHigh, *wallHigh) &&
              stagedWallHigh.Snapshot().playerX == 0.0f &&
              stagedWallHigh.Snapshot().playerZ == -9.70f &&
              stagedWallHigh.Snapshot().interaction.heldLightPose ==
                  interactions::HeldLightPose::High &&
              stagedWallHigh.Snapshot().heldItemKinematics.
                  rewardLanternPresentationYawRadians > 1.50f &&
              stagedWallHigh.Snapshot().rewardLanternWorldFromHinge[14] > -9.90f &&
              stagedWallHigh.Snapshot().rewardLanternWorldFromHinge[14] < -9.715f &&
              stagedWallHigh.Snapshot().lanternPendulum.worldFromBody ==
                  interactions::ComposeLanternPendulumBodyTransform(
                      stagedWallHigh.Snapshot().rewardLanternWorldFromHinge,
                      0.0f, 0.0f, 0.0f,
                      stagedWallHigh.Snapshot().heldItemKinematics.
                          rewardLanternPresentationYawRadians),
          "near-wall high checkpoint freezes the shared wall-retracted hinge and authoritative sideways body camera-side of the fixture");
    Check(wallLow != nullptr &&
              StageDevelopmentCheckpointSimulation(stagedWallLow, *wallLow) &&
              stagedWallLow.Snapshot().playerX == 0.0f &&
              stagedWallLow.Snapshot().playerZ == -9.70f &&
              stagedWallLow.Snapshot().interaction.heldLightPose ==
                  interactions::HeldLightPose::Low &&
              stagedWallLow.Snapshot().lanternPendulum.strafeAngleRadians ==
                  wallLow->rewardStrafeAngleRadians &&
              stagedWallLow.Snapshot().lanternPendulum.torsionAngleRadians ==
                  wallLow->rewardTorsionAngleRadians &&
              stagedWallLow.Snapshot().lanternPendulum.worldFromBody ==
                  interactions::ComposeLanternPendulumBodyTransform(
                      stagedWallLow.Snapshot().rewardLanternWorldFromHinge,
                      wallLow->rewardForwardAngleRadians,
                      wallLow->rewardStrafeAngleRadians,
                      wallLow->rewardTorsionAngleRadians,
                      stagedWallLow.Snapshot().heldItemKinematics.
                          rewardLanternPresentationYawRadians),
          "near-wall low checkpoint freezes raw pendulum continuity plus the exact authoritative collision-bounded body transform");
    std::size_t lanternSweepCount = 0u;
    bool sweepHasHigh = false;
    bool sweepHasLow = false;
    bool sweepHasAlternateCamera = false;
    for (const DevelopmentCheckpoint& candidate : kDevelopmentCheckpoints)
    {
        if (!candidate.name.starts_with("lantern-sweep-")) continue;
        ++lanternSweepCount;
        sweepHasHigh = sweepHasHigh ||
            candidate.rewardPose == DevelopmentRewardPose::HeldHigh;
        sweepHasLow = sweepHasLow ||
            candidate.rewardPose == DevelopmentRewardPose::HeldLow;
        sweepHasAlternateCamera = sweepHasAlternateCamera ||
            candidate.name.ends_with("alt-camera");
        const float cone = std::sqrt(
            candidate.rewardForwardAngleRadians * candidate.rewardForwardAngleRadians +
            candidate.rewardStrafeAngleRadians * candidate.rewardStrafeAngleRadians);
        Check(cone <= 0.95994f,
              "every GPU lantern stress import remains inside the 55-degree hard cone");
        horde::gameplay::simulation::GameSimulation stagedSweep;
        Check(StageDevelopmentCheckpointSimulation(stagedSweep, candidate) &&
                  stagedSweep.Snapshot().lanternPendulum.forwardAngleRadians ==
                      candidate.rewardForwardAngleRadians &&
                  stagedSweep.Snapshot().lanternPendulum.strafeAngleRadians ==
                      candidate.rewardStrafeAngleRadians &&
                  stagedSweep.Snapshot().lanternPendulum.torsionAngleRadians ==
                      candidate.rewardTorsionAngleRadians &&
                  stagedSweep.Snapshot().lanternPendulum.torsionAngularVelocity ==
                      candidate.rewardTorsionAngularVelocity,
              "each GPU lantern stress checkpoint imports and freezes its exact authoritative pendulum");
    }
    Check(lanternSweepCount == 12u && sweepHasHigh && sweepHasLow &&
              sweepHasAlternateCamera,
          "reachable-angle GPU stress covers cone cardinals, hard diagonals, both poses, and alternate cameras");
    Check(FindDevelopmentCheckpoint("opening") == nullptr,
          "release checkpoint names cannot resolve through the development lookup");

    const std::filesystem::path sourceRoot{"C:/source/horde"};
    const auto enabled = ResolveDevelopmentStaticAssetDirectory(true, true, sourceRoot);
    Check(enabled == sourceRoot / "assets/models/weapons/meshy/runtime-development",
          "Debug development capture resolves only the audited source-tree derivative");
    Check(ResolveDevelopmentStaticAssetDirectory(true, false, sourceRoot).empty(),
          "ordinary Debug gameplay does not silently activate source-tree assets");
    Check(ResolveDevelopmentStaticAssetDirectory(false, true, sourceRoot).empty(),
          "packaged Release disables source-tree fallback even when a path is supplied");
    Check(UseGenericStaticAssetForCheckpoint("pbr-sword-closeup"),
          "the explicit development checkpoint activates generic static metadata");
    Check(UseGenericStaticAssetForCheckpoint("pbr-torch-fire"),
          "the production torch/fire checkpoint activates the generic static slot");
    Check(!UseGenericStaticAssetForCheckpoint("lantern-drop"),
          "the historical external checkpoint literal does not activate the generic proof");

    if (failures != 0)
    {
        std::cerr << failures << " development static asset assertion(s) failed\n";
        return 1;
    }
    std::cout << "Development static asset policy passed\n";
    return 0;
}
