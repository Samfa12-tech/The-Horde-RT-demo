#include "gameplay/DevelopmentCheckpoints.h"
#include "gameplay/DevelopmentCheckpointSimulation.h"
#include "gameplay/ShowcaseCheckpoints.h"
#include "vulkan/raytracing/DevelopmentStaticAssetPolicy.h"

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
    Check(kDevelopmentCheckpoints.size() == 20u,
          "twenty isolated render-development checkpoints are exposed");
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
              !chestUnlock->productionLanternGlassOnly &&
              chestUnlock->pitch == -0.35f,
          "production chest unlock has a stable mutually exclusive floor-valid reveal framing");
    Check(productionGlass != nullptr && productionGlass->id == 115 &&
              productionGlass->usesProductionRewardProps &&
              productionGlass->productionLanternGlassOnly &&
              productionGlass->cameraX == -10.65f &&
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
              downwardEvidence.consumedAttackEdges == 1u &&
              downwardEvidence.playerSwingEvents == 1u && stagedDownward.Events().Empty(),
          "downward capture must stage one exact shared swing before freezing feedback");
    Check(upward != nullptr &&
              StageDevelopmentCheckpointSimulation(stagedUpward, *upward,
                                                   &upwardEvidence) &&
              stagedUpward.Snapshot().playerCombat.action ==
                  PlayerCombatAction::UpwardSliceActive &&
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
              stagedHigh.Snapshot().lanternPendulum.forwardAngleRadians == 0.0f,
          "held-high checkpoint imports a frozen rest pendulum and real high carry state");
    Check(heldLow != nullptr && StageDevelopmentCheckpointSimulation(stagedLow, *heldLow) &&
              stagedLow.Snapshot().interaction.heldLightPose ==
                  interactions::HeldLightPose::Low &&
              stagedLow.Snapshot().lanternPendulum.strafeAngularVelocity == 0.0f,
          "held-low checkpoint imports a frozen rest pendulum and real low carry state");
    Check(glassTransmission != nullptr &&
              StageDevelopmentCheckpointSimulation(stagedGlassTransmission,
                                                   *glassTransmission) &&
              stagedGlassTransmission.Snapshot().lanternPendulum.forwardAngleRadians == 0.30f &&
              stagedGlassTransmission.Snapshot().lanternPendulum.strafeAngleRadians == -0.16f,
          "glass-transmission checkpoint freezes an exact angled shared body transform");
    Check(motionExtreme != nullptr &&
              StageDevelopmentCheckpointSimulation(stagedMotionExtreme, *motionExtreme) &&
              stagedMotionExtreme.Snapshot().lanternPendulum.forwardAngleRadians == 0.82f &&
              stagedMotionExtreme.Snapshot().lanternPendulum.strafeAngularVelocity == -1.60f &&
              stagedMotionExtreme.Snapshot().lanternPendulum.previousPivotVelocity[0] == 3.8f,
          "motion-extreme checkpoint preserves exact imported angles, velocities, and hinge history");
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
