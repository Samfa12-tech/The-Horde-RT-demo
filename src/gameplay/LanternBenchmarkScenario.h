#pragma once

#include "gameplay/BenchmarkWorkload.h"
#include "gameplay/simulation/GameSimulation.h"

namespace horde::gameplay
{

// Allowlisted production-state staging for the benchmark harness. This does
// not expose Debug checkpoints or renderer tuning to normal gameplay.
inline bool StageLanternBenchmark(simulation::GameSimulation& simulation,
                                  const BenchmarkWorkload workload)
{
    if (!IsLanternBenchmark(workload) || !simulation.ApplyShowcaseCheckpoint(5))
    {
        return false;
    }

    simulation::InputSnapshot input{};
    input.damageEnabled = false;
    input.hasAuthoritativePlayerPose = true;
    input.authoritativePlayerX = kLanternBenchmarkX;
    input.authoritativePlayerZ = kLanternBenchmarkZ;
    input.yawRadians = kLanternBenchmarkYaw;
    input.pitchRadians = kLanternBenchmarkPitch;
    input.torchLightStrength = 1.8f;
    simulation.StepFixed(input, 0.0f,
                         simulation.Snapshot().inputPublicationSequence + 1u);

    using namespace interactions;
    ChestRewardSnapshot chest;
    chest.phase = ChestRewardPhase::LanternClaimed;
    chest.lidOpenProgress = 1.0f;
    InteractionState interaction;
    interaction.heldLightKind = HeldLightKind::RewardLantern;
    interaction.heldLightPose = workload == BenchmarkWorkload::LanternHeldLow
        ? HeldLightPose::Low : HeldLightPose::High;
    interaction.heldLightPoseProgress = 1.0f;
    FinaleSequenceSnapshot finale;
    finale.phase = workload == BenchmarkWorkload::LanternRevealSequence
        ? FinaleSequencePhase::RaisingLantern : FinaleSequencePhase::RevealingLantern;
    finale.endingPhase = FinaleEndingPhase::LichFalling;
    finale.lichDefeated = true;
    finale.lanternClaimed = true;
    simulation.ImportRewardCheckpoint(chest, interaction, finale);

    const auto& hinge = simulation.Snapshot().rewardLanternWorldFromHinge;
    LanternPendulumSnapshot pendulum;
    pendulum.initialized = true;
    pendulum.previousPivotPosition = {{hinge[12], hinge[13], hinge[14]}};
    pendulum.previousHandForward = {{hinge[8], hinge[9], hinge[10]}};
    if (workload == BenchmarkWorkload::LanternGrazing)
    {
        pendulum.forwardAngleRadians = 0.78539816339f;
        pendulum.strafeAngleRadians = 0.45f;
    }
    else if (workload == BenchmarkWorkload::LanternMotionExtreme)
    {
        pendulum.forwardAngleRadians = 0.82f;
        pendulum.strafeAngleRadians = -0.42f;
        pendulum.forwardAngularVelocity = 2.40f;
        pendulum.strafeAngularVelocity = -1.60f;
        pendulum.torsionAngleRadians = 0.28f;
        pendulum.torsionAngularVelocity = -0.90f;
        pendulum.previousPivotVelocity = {{3.8f, 0.25f, -2.2f}};
    }
    pendulum.worldFromBody = ComposeLanternPendulumBodyTransform(
        hinge, pendulum.forwardAngleRadians, pendulum.strafeAngleRadians,
        pendulum.torsionAngleRadians,
        simulation.Snapshot().heldItemKinematics.rewardLanternPresentationYawRadians);
    simulation.ImportRewardCheckpoint(chest, interaction, finale, &pendulum);
    simulation.ResetTiming();
    simulation.ClearEvents();
    return true;
}

} // namespace horde::gameplay
