#include "ui/DiagnosticOverlay.h"

#include <cmath>
#include <iomanip>
#include <sstream>

#include "vulkan/RtCapabilityReport.h"

namespace horde::ui
{

namespace
{

const char* PlayerCombatActionName(const gameplay::PlayerCombatAction action)
{
    switch (action)
    {
    case gameplay::PlayerCombatAction::Idle: return "idle";
    case gameplay::PlayerCombatAction::SwingWindup: return "swing-windup";
    case gameplay::PlayerCombatAction::SwingActive: return "swing-active";
    case gameplay::PlayerCombatAction::SwingRecovery: return "swing-recovery";
    case gameplay::PlayerCombatAction::ParryStartup: return "parry-startup";
    case gameplay::PlayerCombatAction::ParryActive: return "parry-active";
    case gameplay::PlayerCombatAction::ParryRecovery: return "parry-recovery";
    }
    return "unknown";
}

const char* EnemyCombatActionName(const gameplay::EnemyCombatAction action)
{
    switch (action)
    {
    case gameplay::EnemyCombatAction::Locomotion: return "locomotion";
    case gameplay::EnemyCombatAction::AttackWindup: return "attack-windup";
    case gameplay::EnemyCombatAction::AttackActive: return "attack-active";
    case gameplay::EnemyCombatAction::AttackRecovery: return "attack-recovery";
    case gameplay::EnemyCombatAction::Staggered: return "staggered";
    case gameplay::EnemyCombatAction::Dead: return "dead";
    }
    return "unknown";
}

} // namespace

std::string BuildDiagnosticOverlayText(const vulkan::DeviceCapabilities& capabilities)
{
    return vulkan::BuildCapabilityTextReport(capabilities);
}

std::string BuildUnsupportedDeviceText(const vulkan::DeviceCapabilities& capabilities)
{
    std::ostringstream out;
    out << "Unsupported Vulkan RT device\n";
    out << "This project is RT or nothing. No fake fallback will be used.\n\n";
    out << vulkan::BuildCapabilityTextReport(capabilities);
    return out.str();
}

std::string BuildDeveloperOverlayText(const DeveloperOverlaySnapshot& snapshot)
{
    std::ostringstream out;
    out << std::fixed << std::setprecision(1);
    out << "DEV  " << snapshot.buildIdentity << "  shader " << snapshot.shaderIdentity << '\n';
    out << snapshot.gpuName << "  |  Vulkan " << snapshot.vulkanApi << '\n';
    out << "RT " << snapshot.rtMode << "  |  presented " << (snapshot.presented ? "YES" : "NO")
        << "  |  scale " << std::lround(snapshot.renderScale * 100.0f) << "%\n";
    out << "FRAME " << snapshot.frameTimeMs << " ms  |  " << snapshot.fps << " FPS  |  "
        << snapshot.internalWidth << 'x' << snapshot.internalHeight << " -> "
        << snapshot.presentationWidth << 'x' << snapshot.presentationHeight << '\n';
    if (snapshot.gpuRtTimingValid)
    {
        out << "GPU RT " << snapshot.gpuRtLatestMs << " ms  |  avg "
            << snapshot.gpuRtAverageMs << " ms  |  " << snapshot.gpuRtSampleCount
            << " samples\n";
    }
    else
    {
        out << "GPU RT N/A  |  " << snapshot.gpuRtTimingStatus << '\n';
    }
    out << "SCENE " << snapshot.routeZone << "  |  torch failure " << snapshot.torchFailurePhase
        << "  |  " << snapshot.selectedEnemy << ' ' << snapshot.encounterPhase;
    if (snapshot.enemyHealth >= 0)
    {
        out << " hp " << snapshot.enemyHealth;
    }
    out << '\n';
    out << "PLAYER " << snapshot.playerLifePhase << "  |  vitality "
        << snapshot.playerVitality << '/' << snapshot.playerMaxVitality
        << "  |  damage " << (snapshot.playerDamageEnabled ? "ON" : "OFF") << '\n';
    out << "SIM " << snapshot.simulationTicksThisFrame << " ticks  |  accum "
        << snapshot.fixedStepAccumulatorSeconds * 1000.0 << " ms  |  overruns "
        << snapshot.catchUpOverrunCount << '\n';
    out << "EVENTS " << snapshot.queuedEventCount << " queued / "
        << snapshot.eventQueueHighWaterMark << " high / " << snapshot.eventQueueOverflowCount
        << " overflow  |  input " << snapshot.inputPublicationSequence << "  |  cmd "
        << snapshot.consumedAttackSequence << '/' << snapshot.consumedParrySequence << '/'
        << snapshot.consumedRouteResetSequence
        << '/' << snapshot.consumedRetrySequence << '\n';
    out << "AS " << snapshot.blasCount << " BLAS / " << snapshot.tlasCount << " TLAS / "
        << snapshot.tlasInstanceCount << " inst  |  skinned " << snapshot.activeSkinnedEnemies << '\n';
    out << "COMBAT " << snapshot.activeEnemyEntityCount << " active  |  attacker ";
    if (snapshot.attackerEntityId >= 0)
    {
        out << snapshot.attackerEntityId;
    }
    else
    {
        out << "none";
    }
    out << "  |  player " << PlayerCombatActionName(snapshot.playerCombatAction)
        << "  |  enemy " << (snapshot.hasAttackerCombatAction
            ? EnemyCombatActionName(snapshot.attackerCombatAction) : "none")
        << "  |  pose buckets " << snapshot.skeletonPoseBucketCount << '\n';
    out << "MAT " << snapshot.materialEncoding;
    return out.str();
}

} // namespace horde::ui
