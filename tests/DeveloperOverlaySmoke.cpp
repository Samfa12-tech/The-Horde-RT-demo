#include <iostream>
#include <string>

#include "ui/DiagnosticOverlay.h"
#include "vulkan/GpuFrameTimer.h"

namespace
{

bool RequireContains(const std::string& text, const std::string& expected)
{
    if (text.find(expected) != std::string::npos)
    {
        return true;
    }
    std::cerr << "Developer overlay output is missing: " << expected << '\n';
    return false;
}

} // namespace

int main()
{
    static_assert(horde::vulkan::GpuFrameTimerHasCurrentSample(
        horde::vulkan::GpuFrameTimerStatus::Available));
    static_assert(!horde::vulkan::GpuFrameTimerHasCurrentSample(
        horde::vulkan::GpuFrameTimerStatus::ResultUnavailable));
    static_assert(!horde::vulkan::GpuFrameTimerHasCurrentSample(
        horde::vulkan::GpuFrameTimerStatus::QueryError));

    horde::ui::DeveloperOverlaySnapshot snapshot;
    snapshot.buildIdentity = "0.1.3-alpha.1 DEBUG";
    snapshot.shaderIdentity = "0123456789ab";
    snapshot.gpuName = "Test RTX";
    snapshot.vulkanApi = "1.4.350";
    snapshot.rtMode = "RayTracingPipeline";
    snapshot.routeZone = "finale";
    snapshot.materialEncoding = "ASTC test route";
    snapshot.torchFailurePhase = "settled";
    snapshot.selectedEnemy = "lich";
    snapshot.encounterPhase = "charging";
    snapshot.playerLifePhase = "dying";
    snapshot.playerVitality = 0;
    snapshot.playerMaxVitality = 3;
    snapshot.playerDamageEnabled = false;
    snapshot.internalWidth = 1080;
    snapshot.internalHeight = 2235;
    snapshot.presentationWidth = 1440;
    snapshot.presentationHeight = 2980;
    snapshot.blasCount = 9;
    snapshot.tlasCount = 1;
    snapshot.tlasInstanceCount = 19;
    snapshot.activeSkinnedEnemies = 2;
    snapshot.activeEnemyEntityCount = 2;
    snapshot.attackerEntityId = 2;
    snapshot.playerCombatAction = horde::gameplay::PlayerCombatAction::ParryActive;
    snapshot.attackerCombatAction = horde::gameplay::EnemyCombatAction::AttackActive;
    snapshot.hasAttackerCombatAction = true;
    snapshot.skeletonPoseBucketCount = 2;
    snapshot.enemyHealth = 2;
    snapshot.renderScale = 0.75f;
    snapshot.fps = 83.815f;
    snapshot.frameTimeMs = 11.931f;
    snapshot.gpuRtTimingValid = true;
    snapshot.gpuRtLatestMs = 7.425f;
    snapshot.gpuRtAverageMs = 7.812f;
    snapshot.gpuRtSampleCount = 120u;
    snapshot.gpuRtTimingStatus = "Available";
    snapshot.simulationTicksThisFrame = 1;
    snapshot.fixedStepAccumulatorSeconds = 0.0025;
    snapshot.catchUpOverrunCount = 2;
    snapshot.queuedEventCount = 3;
    snapshot.eventQueueHighWaterMark = 7;
    snapshot.eventQueueOverflowCount = 0;
    snapshot.inputPublicationSequence = 42;
    snapshot.consumedAttackSequence = 5;
    snapshot.consumedParrySequence = 4;
    snapshot.consumedRouteResetSequence = 2;
    snapshot.consumedRetrySequence = 1;
    snapshot.presented = true;

    const std::string text = horde::ui::BuildDeveloperOverlayText(snapshot);
    bool ok = true;
    ok &= RequireContains(text, "DEV  0.1.3-alpha.1 DEBUG  shader 0123456789ab");
    ok &= RequireContains(text, "RT RayTracingPipeline  |  presented YES  |  scale 75%");
    ok &= RequireContains(text, "FRAME 11.9 ms  |  83.8 FPS  |  1080x2235 -> 1440x2980");
    ok &= RequireContains(text, "GPU RT 7.4 ms  |  avg 7.8 ms  |  120 samples");
    ok &= RequireContains(text, "SCENE finale  |  torch failure settled  |  lich charging hp 2");
    ok &= RequireContains(text, "PLAYER dying  |  vitality 0/3  |  damage OFF");
    ok &= RequireContains(text, "SIM 1 ticks  |  accum 2.5 ms  |  overruns 2");
    ok &= RequireContains(text, "EVENTS 3 queued / 7 high / 0 overflow  |  input 42  |  cmd 5/4/2/1");
    ok &= RequireContains(text, "AS 9 BLAS / 1 TLAS / 19 inst  |  skinned 2");
    ok &= RequireContains(text,
                          "COMBAT 2 active  |  attacker 2  |  player parry-active  |  enemy attack-active  |  pose buckets 2");
    ok &= RequireContains(text, "MAT ASTC test route");

    snapshot.presented = false;
    snapshot.selectedEnemy = "none";
    snapshot.encounterPhase = "inactive";
    snapshot.enemyHealth = -1;
    snapshot.gpuRtTimingValid = false;
    snapshot.gpuRtTimingStatus = "Queue timestamps unsupported";
    snapshot.activeEnemyEntityCount = 0;
    snapshot.attackerEntityId = -1;
    snapshot.playerCombatAction = horde::gameplay::PlayerCombatAction::Idle;
    snapshot.hasAttackerCombatAction = false;
    snapshot.skeletonPoseBucketCount = 0;
    const std::string inactiveText = horde::ui::BuildDeveloperOverlayText(snapshot);
    ok &= RequireContains(inactiveText, "presented NO");
    ok &= RequireContains(inactiveText, "none inactive");
    ok &= RequireContains(inactiveText, "GPU RT N/A  |  Queue timestamps unsupported");
    ok &= RequireContains(inactiveText,
                          "COMBAT 0 active  |  attacker none  |  player idle  |  enemy none  |  pose buckets 0");
    if (inactiveText.find(" hp ") != std::string::npos)
    {
        std::cerr << "Developer overlay printed health for an encounter without a health value.\n";
        ok = false;
    }
    return ok ? 0 : 1;
}
