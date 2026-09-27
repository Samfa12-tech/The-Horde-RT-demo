#pragma once

#include "gameplay/DevelopmentCheckpoints.h"
#include "gameplay/simulation/GameSimulation.h"

namespace horde::gameplay
{

struct DevelopmentCheckpointStageEvidence
{
    std::uint32_t consumedAttackEdges = 0u;
    std::uint32_t consumedParryEdges = 0u;
    std::uint32_t playerSwingEvents = 0u;
    std::uint32_t playerParrySucceededEvents = 0u;
    std::uint32_t playerDamagedEvents = 0u;
    std::uint32_t playerKilledEvents = 0u;
    std::uint32_t enemyHitEvents = 0u;
    PlayerCombatAction action = PlayerCombatAction::Idle;
    float actionTime = 0.0f;
};

// Optional, non-owning boundary callbacks for the helper's exact shared
// StepFixed invocations. The observer must not modify simulation inputs/state.
struct DevelopmentCheckpointStepFixedObservation
{
    void* user = nullptr;
    void (*beginStepFixed)(void*) noexcept = nullptr;
    void (*completeStepFixed)(void*) noexcept = nullptr;
};

// Stages a debug-only visual checkpoint by advancing the same fixed-step
// command/combat/animation authority used by live play. Platforms only select
// the requested checkpoint; no renderer or platform-owned animation state is
// introduced.
inline bool StageDevelopmentCheckpointSimulation(
    simulation::GameSimulation& gameSimulation,
    const DevelopmentCheckpoint& checkpoint,
    DevelopmentCheckpointStageEvidence* evidence = nullptr,
    const DevelopmentCheckpointStepFixedObservation* stepObservation = nullptr)
{
    if (checkpoint.requiresAnatomicalPlayerMount &&
        gameSimulation.Snapshot().playerMountProfile != items::PlayerMountProfile::AnatomicalBody)
        return false;
    if (!gameSimulation.ApplyShowcaseCheckpoint(checkpoint.baseShowcaseCheckpointId))
        return false;

    const auto stepFixed = [&](const simulation::InputSnapshot& input,
                               const float fixedDeltaSeconds,
                               const std::uint64_t inputPublicationSequence)
    {
        const bool observe = stepObservation != nullptr &&
            stepObservation->beginStepFixed != nullptr &&
            stepObservation->completeStepFixed != nullptr;
        if (observe)
            stepObservation->beginStepFixed(stepObservation->user);
        gameSimulation.StepFixed(input, fixedDeltaSeconds, inputPublicationSequence);
        if (observe)
            stepObservation->completeStepFixed(stepObservation->user);
    };

    const std::uint64_t initialConsumedAttackSequence =
        gameSimulation.Snapshot().lastConsumedAttackSequence;
    const std::uint64_t initialConsumedParrySequence =
        gameSimulation.Snapshot().lastConsumedParrySequence;
    const auto finalize = [&](const bool staged)
    {
        if (evidence != nullptr)
        {
            *evidence = {};
            evidence->consumedAttackEdges = static_cast<std::uint32_t>(
                gameSimulation.Snapshot().lastConsumedAttackSequence -
                initialConsumedAttackSequence);
            evidence->consumedParryEdges = static_cast<std::uint32_t>(
                gameSimulation.Snapshot().lastConsumedParrySequence -
                initialConsumedParrySequence);
            evidence->action = gameSimulation.Snapshot().playerCombat.action;
            evidence->actionTime = gameSimulation.Snapshot().playerCombat.actionTime;
            for (const simulation::GameplayEvent& event : gameSimulation.Events().Events())
            {
                if (event.type == simulation::GameplayEventType::PlayerSwing)
                    ++evidence->playerSwingEvents;
                if (event.type == simulation::GameplayEventType::PlayerParrySucceeded)
                    ++evidence->playerParrySucceededEvents;
                if (event.type == simulation::GameplayEventType::PlayerDamaged)
                    ++evidence->playerDamagedEvents;
                if (event.type == simulation::GameplayEventType::PlayerKilled)
                    ++evidence->playerKilledEvents;
                if (event.type == simulation::GameplayEventType::EnemyHit)
                    ++evidence->enemyHitEvents;
            }
        }
        gameSimulation.ClearEvents();
        return staged;
    };

    simulation::InputSnapshot input;
    input.damageEnabled = false;
    input.hasAuthoritativePlayerPose = true;
    input.authoritativePlayerX = checkpoint.cameraX;
    input.authoritativePlayerZ = checkpoint.cameraZ;
    input.yawRadians = checkpoint.yaw;
    input.pitchRadians = checkpoint.pitch;
    input.torchLightStrength = 1.8f;
    stepFixed(input, 0.0f,
              gameSimulation.Snapshot().inputPublicationSequence + 1u);
    if (checkpoint.stagesUnlockedChest)
    {
        using namespace horde::gameplay::interactions;
        ChestRewardSnapshot chest;
        chest.phase = ChestRewardPhase::ClosedUnlocked;
        InteractionState interaction = gameSimulation.Snapshot().interaction;
        FinaleSequenceSnapshot finale;
        finale.phase = FinaleSequencePhase::LichFalling;
        finale.endingPhase = FinaleEndingPhase::LichFalling;
        finale.lichDefeated = true;
        gameSimulation.ImportRewardCheckpoint(chest, interaction, finale);
    }
    if (checkpoint.rewardPose != DevelopmentRewardPose::None)
    {
        using namespace horde::gameplay::interactions;
        ChestRewardSnapshot chest;
        chest.phase = ChestRewardPhase::LanternClaimed;
        chest.lidOpenProgress = 1.0f;
        InteractionState interaction;
        interaction.heldLightKind = HeldLightKind::RewardLantern;
        interaction.heldLightPose = checkpoint.rewardPose == DevelopmentRewardPose::HeldLow
            ? HeldLightPose::Low
            : HeldLightPose::High;
        interaction.heldLightPoseProgress = 1.0f;
        FinaleSequenceSnapshot finale;
        finale.phase = FinaleSequencePhase::RevealingLantern;
        finale.endingPhase = FinaleEndingPhase::LichFalling;
        finale.lichDefeated = true;
        finale.lanternClaimed = true;

        // Resolve the exact high/low shared hand target before authoring the
        // frozen body state beneath it. The final import then preserves those
        // finite angles/velocities without advancing a simulation tick.
        gameSimulation.ImportRewardCheckpoint(chest, interaction, finale);
        LanternPendulumSnapshot pendulum;
        const auto& hinge = gameSimulation.Snapshot().rewardLanternWorldFromHinge;
        pendulum.initialized = true;
        pendulum.previousPivotPosition = {{hinge[12], hinge[13], hinge[14]}};
        pendulum.forwardAngleRadians = checkpoint.rewardForwardAngleRadians;
        pendulum.strafeAngleRadians = checkpoint.rewardStrafeAngleRadians;
        pendulum.forwardAngularVelocity = checkpoint.rewardForwardAngularVelocity;
        pendulum.strafeAngularVelocity = checkpoint.rewardStrafeAngularVelocity;
        pendulum.torsionAngleRadians = checkpoint.rewardTorsionAngleRadians;
        pendulum.torsionAngularVelocity = checkpoint.rewardTorsionAngularVelocity;
        pendulum.previousPivotVelocity = checkpoint.rewardPreviousPivotVelocity;
        pendulum.previousHandForward = {{hinge[8], hinge[9], hinge[10]}};
        pendulum.worldFromBody = ComposeLanternPendulumBodyTransform(
            hinge, pendulum.forwardAngleRadians, pendulum.strafeAngleRadians,
            pendulum.torsionAngleRadians,
            gameSimulation.Snapshot().heldItemKinematics.
                rewardLanternPresentationYawRadians);
        gameSimulation.ImportRewardCheckpoint(chest, interaction, finale, &pendulum);
    }
    if (checkpoint.combatPose == DevelopmentCombatPose::Rest)
        return finalize(true);
    constexpr float fixedDelta =
        static_cast<float>(simulation::FixedStepRunner::kFixedDeltaSeconds);

    if (checkpoint.combatPose == DevelopmentCombatPose::ParryActive)
    {
        // Drive the same monotonic parry command and fixed-step combat path as
        // live play, then freeze the first 60 Hz sample at/after 0.10 s into
        // the 0.22 s active window (the authored sample is 0.11 s).
        input.commands.parry =
            gameSimulation.Snapshot().lastConsumedParrySequence + 1u;
        constexpr float kParryCaptureActionTimeSeconds = 0.10f;
        for (std::uint32_t tick = 0u; tick < 20u; ++tick)
        {
            stepFixed(input, fixedDelta,
                      gameSimulation.Snapshot().inputPublicationSequence + 1u);
            const PlayerCombatSnapshot& after = gameSimulation.Snapshot().playerCombat;
            if (after.action == PlayerCombatAction::ParryActive &&
                after.actionTime >= kParryCaptureActionTimeSeconds)
                return finalize(true);
        }
        return finalize(false);
    }

    input.commands.attack = gameSimulation.Snapshot().lastConsumedAttackSequence + 1u;
    bool upwardEdgePublished = false;
    for (std::uint32_t tick = 0u; tick < 90u; ++tick)
    {
        const PlayerCombatSnapshot& before = gameSimulation.Snapshot().playerCombat;
        if (checkpoint.combatPose == DevelopmentCombatPose::UpwardSliceActive &&
            !upwardEdgePublished && before.action == PlayerCombatAction::SwingActive &&
            before.actionTime >= 0.05f)
        {
            ++input.commands.attack;
            upwardEdgePublished = true;
        }
        stepFixed(input, fixedDelta,
                  gameSimulation.Snapshot().inputPublicationSequence + 1u);
        const PlayerCombatSnapshot& after = gameSimulation.Snapshot().playerCombat;
        const bool reachedDownward =
            checkpoint.combatPose == DevelopmentCombatPose::DownwardCutActive &&
            after.action == PlayerCombatAction::SwingActive &&
            after.actionTime >= SwordCombat::kSwingActiveDuration - 0.025f;
        const bool reachedUpward =
            checkpoint.combatPose == DevelopmentCombatPose::UpwardSliceActive &&
            after.action == PlayerCombatAction::UpwardSliceActive &&
            after.actionTime >= SwordCombat::kUpwardSliceActiveDuration - 0.025f;
        if (reachedDownward || reachedUpward)
            return finalize(true);
    }
    return finalize(false);
}

} // namespace horde::gameplay
