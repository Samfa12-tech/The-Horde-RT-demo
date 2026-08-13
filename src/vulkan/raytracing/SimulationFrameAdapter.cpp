#include "vulkan/raytracing/SimulationFrameAdapter.h"

namespace horde::vulkan::raytracing
{

RtSceneFrameInputs BuildRtSceneFrameInputs(
    const horde::gameplay::simulation::SimulationSnapshot& simulation,
    const float outputExposure)
{
    RtSceneFrameInputs frame;
    frame.cameraYaw = simulation.playerYawRadians;
    frame.cameraPitch = simulation.playerPitchRadians;
    frame.lanternStrength = simulation.lanternStrength * simulation.lantern.flameStrength;
    frame.walkTime = simulation.walkTime;
    frame.cameraX = simulation.playerX;
    frame.cameraZ = simulation.playerZ;
    frame.walkAmount = simulation.walkAmount;
    frame.outputExposure = outputExposure;
    frame.combat = simulation.swordCombat;
    frame.playerCombat = simulation.playerCombat;
    frame.combat.damageFlash = simulation.playerVitals.damageFlash;
    frame.skeletonEnemies = simulation.skeletonEnemies;
    frame.skeletonEnemyCount = simulation.skeletonEnemyCount;
    frame.lantern = simulation.lantern;
    frame.roster = simulation.enemyRoster;
    frame.lich = simulation.lich;
    return frame;
}

} // namespace horde::vulkan::raytracing
