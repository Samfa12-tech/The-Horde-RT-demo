#pragma once

#include "gameplay/DevelopmentCheckpoints.h"
#include "gameplay/simulation/GameSimulation.h"

namespace horde::gameplay
{

struct DevelopmentCheckpointStageEvidence
{
    std::uint32_t consumedAttackEdges = 0u;
    std::uint32_t playerSwingEvents = 0u;
    std::uint32_t enemyHitEvents = 0u;
    PlayerCombatAction action = PlayerCombatAction::Idle;
    float actionTime = 0.0f;
};

// Stages a debug-only visual checkpoint by advancing the same fixed-step
// command/combat/animation authority used by live play. Platforms only select
// the requested checkpoint; no renderer or platform-owned animation state is
// introduced.
inline bool StageDevelopmentCheckpointSimulation(
    simulation::GameSimulation& gameSimulation,
    const DevelopmentCheckpoint& checkpoint,
    DevelopmentCheckpointStageEvidence* evidence = nullptr)
{
    if (!gameSimulation.ApplyShowcaseCheckpoint(checkpoint.baseShowcaseCheckpointId))
        return false;

    const std::uint64_t initialConsumedAttackSequence =
        gameSimulation.Snapshot().lastConsumedAttackSequence;
    const auto finalize = [&](const bool staged)
    {
        if (evidence != nullptr)
        {
            *evidence = {};
            evidence->consumedAttackEdges = static_cast<std::uint32_t>(
                gameSimulation.Snapshot().lastConsumedAttackSequence -
                initialConsumedAttackSequence);
            evidence->action = gameSimulation.Snapshot().playerCombat.action;
            evidence->actionTime = gameSimulation.Snapshot().playerCombat.actionTime;
            for (const simulation::GameplayEvent& event : gameSimulation.Events().Events())
            {
                if (event.type == simulation::GameplayEventType::PlayerSwing)
                    ++evidence->playerSwingEvents;
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
    gameSimulation.StepFixed(input, 0.0f,
                             gameSimulation.Snapshot().inputPublicationSequence + 1u);
    if (checkpoint.combatPose == DevelopmentCombatPose::Rest)
        return finalize(true);

    input.commands.attack = gameSimulation.Snapshot().lastConsumedAttackSequence + 1u;
    bool upwardEdgePublished = false;
    constexpr float fixedDelta =
        static_cast<float>(simulation::FixedStepRunner::kFixedDeltaSeconds);
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
        gameSimulation.StepFixed(input, fixedDelta,
                                 gameSimulation.Snapshot().inputPublicationSequence + 1u);
        const PlayerCombatSnapshot& after = gameSimulation.Snapshot().playerCombat;
        const bool reachedDownward =
            checkpoint.combatPose == DevelopmentCombatPose::DownwardCutActive &&
            after.action == PlayerCombatAction::SwingActive && after.actionTime >= 0.07f;
        const bool reachedUpward =
            checkpoint.combatPose == DevelopmentCombatPose::UpwardSliceActive &&
            after.action == PlayerCombatAction::UpwardSliceActive && after.actionTime >= 0.07f;
        if (reachedDownward || reachedUpward)
            return finalize(true);
    }
    return finalize(false);
}

} // namespace horde::gameplay
