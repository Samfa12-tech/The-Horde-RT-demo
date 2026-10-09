#include "vulkan/raytracing/SimulationFrameAdapter.h"

#include <algorithm>

namespace horde::vulkan::raytracing
{
RtSceneFrameInputs BuildEntryMenuFrameInputs(const horde::graphics::EntryMenuSession &session,
                                             const float outputExposure,
                                             const FireEmitterQuality fireDetail,
                                             const horde::graphics::ShadowQuality shadowQuality)
{
    const auto entry = session.Snapshot();
    RtSceneFrameInputs frame{};
    frame.tickIndex = entry.tick;
    frame.cameraX = entry.camera.x;
    frame.cameraZ = entry.camera.z;
    frame.cameraYaw = entry.camera.yaw;
    frame.cameraPitch = entry.camera.pitch;
    frame.outputExposure = outputExposure * (1.0f - entry.fade);
    frame.waterQuality = WaterQuality::Off;
    frame.fireDetail = fireDetail;
    frame.shadowQuality = shadowQuality;
    frame.torchLightStrength = 0.0f;
    frame.lich.staffLightStrength = 0.0f;
    frame.rewardLanternWorldFromHinge = entry.hinge;
    frame.lanternPendulum = entry.pendulum;
    frame.fireEmitters[0] = entry.fire;
    frame.fireEmitterCount = 1;
    return frame;
}

RtSceneFrameInputs BuildGraphicsPreviewFrameInputs(
    const horde::graphics::GraphicsPreviewSession& session,
    const float outputExposure, const WaterQuality waterQuality,
    const FireEmitterQuality fireDetail, const RtSceneTuning& tuning,
    const std::optional<horde::graphics::ShadowQuality> shadowQuality)
{
    const auto preview = session.Snapshot();
    RtSceneFrameInputs frame{};
    frame.tickIndex = preview.tick;
    frame.cameraX = preview.camera.x;
    frame.cameraZ = preview.camera.z;
    frame.cameraYaw = preview.camera.yaw;
    frame.cameraPitch = preview.camera.pitch;
    frame.walkTime = static_cast<float>(preview.timeSeconds);
    frame.outputExposure = outputExposure;
    frame.waterQuality = waterQuality;
    frame.fireDetail = fireDetail;
    frame.shadowQuality = shadowQuality;
    frame.previewMotion = preview.motionTest;
    frame.torchLightStrength = 1.8f;
    frame.tuning = ClampRtSceneTuning(tuning);
    frame.lich.staffLightStrength = 0.0f;
    frame.roster.selectedEnemy = horde::gameplay::EnemyKind::Skeleton;
    frame.roster.renderedEnemies[0] = horde::gameplay::EnemyKind::Skeleton;
    frame.roster.renderedEnemyCount = 1u;
    frame.skeletonEnemyCount = 1u;
    auto& skeleton = frame.skeletonEnemies[0];
    skeleton.id = horde::gameplay::simulation::EntityId::SkeletonA;
    skeleton.x = preview.skeleton.x;
    skeleton.z = preview.skeleton.z;
    skeleton.facingRadians = preview.skeleton.facing;
    skeleton.animationTime = preview.skeleton.animationTime;
    skeleton.animation = horde::gameplay::EnemyAnimation::Idle;
    skeleton.action = horde::gameplay::EnemyCombatAction::Locomotion;
    frame.fireEmitterCount = preview.fireEmitters.size();
    std::copy(preview.fireEmitters.begin(), preview.fireEmitters.end(), frame.fireEmitters.begin());
    return frame;
}

RtSceneFrameInputs BuildRtSceneFrameInputs(
    const horde::gameplay::simulation::SimulationSnapshot& simulation,
    const float outputExposure,
    const WaterQuality waterQuality)
{
    return BuildRtSceneFrameInputs(simulation, outputExposure, waterQuality, RtSceneTuning{}, std::nullopt);
}

RtSceneFrameInputs BuildRtSceneFrameInputs(
    const horde::gameplay::simulation::SimulationSnapshot& simulation,
    const float outputExposure,
    const RtSceneTuning& tuning,
    const WaterQuality waterQuality,
    const std::optional<FireEmitterQuality> fireDetail,
    const std::optional<horde::graphics::ShadowQuality> shadowQuality)
{
    return BuildRtSceneFrameInputs(simulation, outputExposure, waterQuality, tuning, fireDetail, shadowQuality);
}

RtSceneFrameInputs BuildRtSceneFrameInputs(
    const horde::gameplay::simulation::SimulationSnapshot& simulation,
    const float outputExposure,
    const WaterQuality waterQuality,
    const RtSceneTuning& tuning,
    const std::optional<FireEmitterQuality> fireDetail,
    const std::optional<horde::graphics::ShadowQuality> shadowQuality)
{
    RtSceneFrameInputs frame;
    frame.playerRenderRoute = kProductionPlayerRenderRoute;
    frame.tickIndex = simulation.tickIndex;
    frame.cameraYaw = simulation.playerYawRadians;
    frame.cameraPitch = simulation.playerPitchRadians;
    frame.torchLightStrength = simulation.torchLightStrength * simulation.torchFailure.flameStrength;
    frame.walkTime = simulation.walkTime;
    frame.cameraX = simulation.playerX;
    frame.cameraZ = simulation.playerZ;
    frame.walkAmount = simulation.walkAmount;
    frame.outputExposure = outputExposure;
    frame.waterQuality = waterQuality;
    frame.fireDetail = fireDetail;
    frame.shadowQuality = shadowQuality;
    frame.tuning = ClampRtSceneTuning(tuning);
    frame.combat = simulation.swordCombat;
    frame.playerCombat = simulation.playerCombat;
    frame.combat.damageFlash = simulation.playerVitals.damageFlash;
    frame.skeletonEnemies = simulation.skeletonEnemies;
    frame.skeletonEnemyCount = simulation.skeletonEnemyCount;
    frame.torchFailure = simulation.torchFailure;
    frame.heldItems = simulation.heldItems;
    frame.heldItemKinematics = simulation.heldItemKinematics;
    frame.playerAnimation = simulation.playerAnimation;
    frame.playerMountProfile = simulation.playerMountProfile;
    frame.heldLight = simulation.heldLight;
    frame.interaction = simulation.interaction;
    frame.chestReward = simulation.chestReward;
    frame.finale = simulation.finale;
    frame.lanternPendulum = simulation.lanternPendulum;
    frame.rewardLanternWorldFromHinge = simulation.rewardLanternWorldFromHinge;
    frame.roster = simulation.enemyRoster;
    frame.lich = simulation.lich;
    frame.zone = simulation.zone;
    frame.fireEmitters = simulation.fireEmitters;
    frame.fireEmitterCount = simulation.fireEmitterCount;
    frame.lich.finaleSkylightOpenProgress = ResolveRtFinaleRoofOpen(
        simulation.lich.finaleSkylightOpenProgress, frame.tuning);
    frame.lich.finaleDawnRevealProgress = ResolveRtFinaleDawnReveal(
        simulation.lich.finaleDawnRevealProgress, frame.tuning);
    return frame;
}

} // namespace horde::vulkan::raytracing
