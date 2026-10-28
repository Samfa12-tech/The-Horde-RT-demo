#pragma once

#include "gameplay/DevelopmentCheckpoints.h"
#include "gameplay/simulation/GameSimulation.h"

namespace horde::gameplay
{

enum class DevelopmentCheckpointStageFailure : std::uint8_t
{
    None,
    AuthoredTerrainRequired,
    MissingTerrain,
    StaleTerrain,
    BaseCheckpointRejected,
    SimulationStateNotReached,
};

inline constexpr const char* DevelopmentCheckpointStageFailureName(
    DevelopmentCheckpointStageFailure failure) noexcept
{
    switch (failure)
    {
    case DevelopmentCheckpointStageFailure::None: return "none";
    case DevelopmentCheckpointStageFailure::AuthoredTerrainRequired: return "authored-terrain-required";
    case DevelopmentCheckpointStageFailure::MissingTerrain: return "missing-terrain";
    case DevelopmentCheckpointStageFailure::StaleTerrain: return "stale-terrain";
    case DevelopmentCheckpointStageFailure::BaseCheckpointRejected: return "base-checkpoint-rejected";
    case DevelopmentCheckpointStageFailure::SimulationStateNotReached: return "simulation-state-not-reached";
    }
    return "unknown";
}

struct DevelopmentCheckpointStageEvidence
{
    DevelopmentCheckpointStageFailure failure = DevelopmentCheckpointStageFailure::None;
    // Admission-time identities, before checkpoint imports can change them.
    std::uint64_t worldGeneration = 0u;
    std::uint64_t terrainGeneration = 0u;
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
    if (evidence != nullptr)
    {
        *evidence = {};
        evidence->worldGeneration = gameSimulation.worldRoute_.generation;
        evidence->terrainGeneration = gameSimulation.chapterTerrainGeneration_;
        evidence->failure = DevelopmentCheckpointStageFailure::SimulationStateNotReached;
    }
    const auto reject = [&](DevelopmentCheckpointStageFailure failure)
    {
        if (evidence != nullptr) evidence->failure = failure;
        return false;
    };
    const auto chapterTerrain=gameSimulation.chapterTerrain_;
    if (checkpoint.stagesRescueExterior)
    {
        if (!gameSimulation.config_.authoredChapterTerrain)
            return reject(DevelopmentCheckpointStageFailure::AuthoredTerrainRequired);
        if (!chapterTerrain)
            return reject(DevelopmentCheckpointStageFailure::MissingTerrain);
        if (gameSimulation.chapterTerrainGeneration_ != gameSimulation.worldRoute_.generation)
            return reject(DevelopmentCheckpointStageFailure::StaleTerrain);
    }
    if (checkpoint.stagesWaterfallGuards &&
        (checkpoint.waterfallGuardFixedTicks > 30u ||
         checkpoint.combatPose != DevelopmentCombatPose::Rest ||
         !gameSimulation.BeginMotionEvidenceEquipmentSeed()))
        return false;
    if (!gameSimulation.ApplyShowcaseCheckpoint(checkpoint.baseShowcaseCheckpointId))
    {
        if (checkpoint.stagesWaterfallGuards) gameSimulation.EndMotionEvidenceEquipmentSeed();
        return reject(DevelopmentCheckpointStageFailure::BaseCheckpointRejected);
    }

    if(checkpoint.developmentWorldRoute || gameSimulation.Snapshot().developmentWorldRoute)
        gameSimulation.SetDevelopmentWorldRoute(checkpoint.developmentWorldRoute,checkpoint.stagedWorldPreparation);

    if (checkpoint.developmentSupportFixture || gameSimulation.Snapshot().developmentSupportFixture)
        gameSimulation.SetDevelopmentSupportFixture(checkpoint.developmentSupportFixture,
            gameSimulation.Snapshot().playerSupportGeneration + 1u);

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
    const auto accumulateEvents = [&]()
    {
        if (evidence == nullptr) return;
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
    };
    const auto finalize = [&](const bool staged)
    {
        if (evidence != nullptr)
        {
            evidence->failure = staged ? DevelopmentCheckpointStageFailure::None
                : DevelopmentCheckpointStageFailure::SimulationStateNotReached;
            evidence->consumedAttackEdges = static_cast<std::uint32_t>(
                gameSimulation.Snapshot().lastConsumedAttackSequence -
                initialConsumedAttackSequence);
            evidence->consumedParryEdges = static_cast<std::uint32_t>(
                gameSimulation.Snapshot().lastConsumedParrySequence -
                initialConsumedParrySequence);
            evidence->action = gameSimulation.Snapshot().playerCombat.action;
            evidence->actionTime = gameSimulation.Snapshot().playerCombat.actionTime;
        }
        accumulateEvents();
        gameSimulation.ClearEvents();
        return staged;
    };

    if(checkpoint.stagesRescueExterior) {
        // A diagnostic exterior seed is a new authored chapter presentation,
        // even in a process which has already walked this route. Retaining the
        // first-arrival flag skips the genuine lowering step on repeated seeds;
        // retaining Kit/proof would also borrow the preceding run's progression.
        // Ordinary checkpoint/retry/save paths deliberately keep their policy.
        gameSimulation.chapterDialogue_.Reset();
        gameSimulation.dialogueDucking_.Reset();
        gameSimulation.chapterCompanion_.Reset();
        gameSimulation.chapterFirstExterior_=false;
        gameSimulation.chapterVisibleRaise_=0;
        // Explicit companion evidence first executes the accepted-hit/death
        // seed, then the same rope/native-grounding path as ordinary captures.
        // Historical route captures retain their existing imported state.
        if(checkpoint.keeperDeathFixedTicks != 0u) {
            const auto* death=FindDevelopmentCheckpoint("chapter-keeper-death-retired");
            if(!death || !StageDevelopmentCheckpointSimulation(gameSimulation,*death)) return finalize(false);
        }
        using namespace interactions;
        ChestRewardSnapshot chest;chest.phase=ChestRewardPhase::LanternClaimed;chest.lidOpenProgress=1;
        InteractionState interaction;ResetInteractionState(interaction,HeldLightKind::RewardLantern);
        FinaleSequenceSnapshot finale;finale.lichDefeated=true;
        gameSimulation.ImportRewardCheckpoint(chest,interaction,finale);
        for(std::size_t i=1;i<simulation::kWorldZones.size();++i)
            if(!gameSimulation.PublishChapterTerrain({gameSimulation.worldRoute_.generation,
                static_cast<simulation::WorldZoneId>(i)},chapterTerrain))return finalize(false);
        simulation::InputSnapshot routeInput;routeInput.damageEnabled=false;routeInput.tutorialEnabled=false;
        routeInput.yawRadians=checkpoint.yaw;routeInput.pitchRadians=checkpoint.pitch;
        const auto advance=[&](){
            stepFixed(routeInput,1.0f/60.0f,gameSimulation.Snapshot().inputPublicationSequence+1);
            // Diagnostic exterior preparation can cover thousands of ticks in
            // one owner call. Observe and retain evidence before draining each
            // step, as live owners do; never erase a recorded overflow count.
            // Combat checkpoint staging retains its existing finalizer path.
            accumulateEvents();
            gameSimulation.ClearEvents();
        };
        for(int tick=0;tick<420&&!gameSimulation.Snapshot().rescue.ropeReady;++tick)advance();
        if(!gameSimulation.Snapshot().rescue.ropeReady)return finalize(false);
        routeInput.commands.interact=gameSimulation.Snapshot().lastConsumedInteractSequence+1;advance();
        if(!gameSimulation.Snapshot().rescue.equipmentStowed)return finalize(false);
        const auto freezeTraversal=[&]() {
            stepFixed(routeInput,0,gameSimulation.Snapshot().inputPublicationSequence+1);
            return finalize(gameSimulation.Snapshot().rescue.equipmentStowed);
        };
        if(checkpoint.rescueTraversalCapture==1u || checkpoint.rescueTraversalCapture==2u) {
            for(int tick=0;tick<1200 && gameSimulation.Snapshot().rescue.equipmentStowed;++tick) {
                const auto& s=gameSimulation.Snapshot().rescue;
                const bool match=checkpoint.rescueTraversalCapture==1u
                    ?s.phase==traversal::Phase::Ascent&&s.supportWorldY>.25f
                    :s.phase==traversal::Phase::PullUp&&s.plantedPelvisDropMetres>.35f&&s.ropeHandsActive;
                if(match)return freezeTraversal();
                advance();
            }
            return finalize(false);
        }
        for(int tick=0;tick<1200&&gameSimulation.Snapshot().rescue.equipmentStowed;++tick)advance();
        if(!gameSimulation.Snapshot().rescue.exteriorSide || gameSimulation.Snapshot().rescue.equipmentStowed)return finalize(false);
        if(checkpoint.rescueTraversalCapture==3u || checkpoint.rescueTraversalCapture==4u) {
            routeInput.commands.interact=gameSimulation.Snapshot().lastConsumedInteractSequence+1;advance();
            if(!gameSimulation.Snapshot().rescue.equipmentStowed)return finalize(false);
            for(int tick=0;tick<1200 && gameSimulation.Snapshot().rescue.equipmentStowed;++tick) {
                const auto& s=gameSimulation.Snapshot().rescue;
                if(checkpoint.rescueTraversalCapture==3u && s.phase==traversal::Phase::Descent&&s.ropeHandsActive)
                    return freezeTraversal();
                advance();
            }
            if(checkpoint.rescueTraversalCapture==3u || gameSimulation.Snapshot().rescue.exteriorSide)
                return finalize(false);
            routeInput.commands.interact=gameSimulation.Snapshot().lastConsumedInteractSequence+1;advance();
            for(int tick=0;tick<1200 && gameSimulation.Snapshot().rescue.equipmentStowed;++tick) {
                const auto& s=gameSimulation.Snapshot().rescue;
                if(s.phase==traversal::Phase::Ascent&&s.supportWorldY>.25f)return freezeTraversal();
                advance();
            }
            return finalize(false);
        }
        if(checkpoint.rescueTraversalCapture!=0u)return finalize(false);
        // Capture route stations through real fixed-step movement and native
        // support/collision after ascent, never a teleport through blockers.
        if(checkpoint.exteriorRouteStation >= simulation::kWorldRoutePoints.size())return finalize(false);
        for(std::size_t station=3;station<=checkpoint.exteriorRouteStation;++station) {
            const auto destination=simulation::kWorldRoutePoints[station];
            bool reached=false;
            for(int tick=0;tick<2600;++tick) {
                const auto& state=gameSimulation.Snapshot();
                const float dx=destination.x-state.playerX,dz=destination.z-state.playerZ;
                const float distance=std::hypot(dx,dz);
                if(distance<.008f) {reached=true;break;}
                routeInput.yawRadians=std::atan2(dx,-dz);
                routeInput.moveForward=std::min(1.0f,distance/.055f);
                routeInput.runHeld=distance>.3f;
                advance();
            }
            if(!reached)return finalize(false);
        }
        routeInput.moveForward=0;routeInput.runHeld=false;
        // The native replay really ascends before freezing. Never seed a
        // camera-only exterior or invent ready resources for a missing scene.
        routeInput.yawRadians=checkpoint.yaw;routeInput.pitchRadians=checkpoint.pitch;
        // Route shots are taken after stopping and looking at the bend. Let
        // the physical lantern and locomotion pose settle in simulation time;
        // render-only settling frames cannot advance those owners.
        for(int tick=0;tick<120;++tick)advance();
        if(checkpoint.raisesLanternForCompanion) {
            routeInput.commands.toggleHeldLightPose=gameSimulation.Snapshot().lastConsumedToggleHeldLightPoseSequence+1;
            for(int tick=0;tick<100;++tick) {
                advance();
                const auto& state=gameSimulation.Snapshot();
                // Diagnostic acknowledgement of the exact current raised pose;
                // this is not evidence of an owner completing the reunion.
                gameSimulation.AcknowledgeChapterPresentation(state.tickIndex,state.chapterDialogue.generation);
            }
            if(!gameSimulation.Snapshot().chapterDialogue.proofAccepted) return finalize(false);
        }
        stepFixed(routeInput,0,gameSimulation.Snapshot().inputPublicationSequence+1);
        return finalize(std::hypot(gameSimulation.Snapshot().playerX-checkpoint.cameraX,
            gameSimulation.Snapshot().playerZ-checkpoint.cameraZ)<.01f);
    }

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
    if (checkpoint.keeperDeathFixedTicks != 0u)
    {
        if (!gameSimulation.config_.developmentRescueJourney ||
            checkpoint.keeperDeathFixedTicks > 360u) return finalize(false);
        // Explicit diagnostic setup, not evidence of a player-completed fight.
        // Preserve legacy checkpoint imports. This separate seed uses live
        // recognition, accepted-hit lockout and the actual death clock.
        gameSimulation.legacyCombatCheckpoint_ = false;
        auto& keeper = gameSimulation.lichEncounter_;
        keeper.SetReadableCombat(true);
        keeper.SetChapterDeathPresentation(true);
        keeper.BeginRetryRecognition();
        constexpr float tickSeconds = 1.0f / 60.0f;
        for (int tick = 0; tick < 180 && !keeper.Snapshot().revealComplete; ++tick)
            keeper.Update(tickSeconds, checkpoint.cameraX, checkpoint.cameraZ, false, true);
        keeper.Update(tickSeconds, checkpoint.cameraX, checkpoint.cameraZ, false, true);
        for (int hit = 0; hit < 3; ++hit)
        {
            if (!keeper.TryAcceptPlayerHit(keeper.Snapshot().x, keeper.Snapshot().z))
                return finalize(false);
            if (hit < 2)
                for (int tick = 0; tick < 120; ++tick)
                    keeper.Update(tickSeconds, checkpoint.cameraX, checkpoint.cameraZ, false, true);
        }
        if (gameSimulation.finaleSequence_.NotifyLichDefeated())
            gameSimulation.chestRewardSequence_.BeginUnlockCountdown();
        input.tutorialEnabled = false;
        for (std::uint32_t tick = 0; tick < checkpoint.keeperDeathFixedTicks; ++tick)
            stepFixed(input, tickSeconds, gameSimulation.Snapshot().inputPublicationSequence + 1u);
        if(gameSimulation.Snapshot().lich.phase != LichPhase::Dead)return finalize(false);
        // Explicit composed diagnostic: retain the real death clock while
        // importing a separately requested frozen reward-hand pose below.
        // Existing death-only views keep their original early completion.
        if(checkpoint.rewardPose==DevelopmentRewardPose::None)return finalize(true);
    }
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
        gameSimulation.ImportRewardCheckpointState(chest, interaction, finale, nullptr,
            simulation::GameSimulation::RewardCheckpointPlacement::AuthoredCapture);
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
        gameSimulation.ImportRewardCheckpointState(chest, interaction, finale, nullptr,
            simulation::GameSimulation::RewardCheckpointPlacement::AuthoredCapture);
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
        gameSimulation.ImportRewardCheckpointState(chest, interaction, finale, &pendulum,
            simulation::GameSimulation::RewardCheckpointPlacement::AuthoredCapture);
    }
    constexpr float fixedDelta =
        static_cast<float>(simulation::FixedStepRunner::kFixedDeltaSeconds);
    if (checkpoint.stagesWaterfallGuards)
    {
        for (std::uint32_t tick = 0u; tick < checkpoint.waterfallGuardFixedTicks; ++tick)
            stepFixed(input, fixedDelta,
                      gameSimulation.Snapshot().inputPublicationSequence + 1u);
        const auto& state = gameSimulation.Snapshot();
        const bool valid = state.skeletonEnemyCount == 2u && state.activeSkeletonCount == 2u &&
            state.skeletonEnemies[0].id == simulation::EntityId::SkeletonA &&
            state.skeletonEnemies[1].id == simulation::EntityId::SkeletonB &&
            state.skeletonEnemies[0].health == 1 && state.skeletonEnemies[1].health == 1 &&
            IsWaterfallSkeletonRoom(state.playerX, state.playerZ);
        if (!valid) gameSimulation.EndMotionEvidenceEquipmentSeed();
        // Successful staging deliberately remains scoped until the capture owner
        // retires it, so the frozen RT snapshot retains the real guard poses.
        return finalize(valid);
    }
    if (checkpoint.combatPose == DevelopmentCombatPose::Rest)
        return finalize(true);

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

// Platform owner-thread boundary. A publisher consumes actual scene readiness;
// the returned result must gate capture/benchmark admission, not just its label.
template<class PublishReadyWorld>
[[nodiscard]] inline bool StageDevelopmentCheckpointForScene(
    simulation::GameSimulation& gameSimulation,
    const DevelopmentCheckpoint& checkpoint,
    PublishReadyWorld&& publishReadyWorld,
    bool liveJourney = false,
    DevelopmentCheckpointStageEvidence* evidence = nullptr,
    const DevelopmentCheckpointStepFixedObservation* stepObservation = nullptr)
{
    // Early platform-boundary rejection must not retain a prior successful
    // capture's events, identities or failure disposition.
    if (evidence != nullptr) *evidence = {};
    const bool chapter = DevelopmentCheckpointRequiresAuthoredChapter(checkpoint);
    if (checkpoint.id == 192)
    {
        if (liveJourney)
            gameSimulation.StartLiveDevelopmentRescueJourney(true);
        else
        {
            if (!gameSimulation.ApplyShowcaseCheckpoint(checkpoint.baseShowcaseCheckpointId))
            {
                if (evidence != nullptr)
                    evidence->failure = DevelopmentCheckpointStageFailure::BaseCheckpointRejected;
                return false;
            }
            gameSimulation.SetDevelopmentRescueJourney(true, true);
        }
        publishReadyWorld();
        return true; // The live lower safe start does not require exterior readiness yet.
    }
    if (chapter)
    {
        // Explicit checkpoint import changes the generation. Publish actual
        // renderer readiness only afterwards; old callbacks cannot populate it.
        gameSimulation.SetDevelopmentRescueJourney(true, true);
        publishReadyWorld();
    }
    else if (gameSimulation.Snapshot().developmentRescueJourney)
        gameSimulation.SetDevelopmentRescueJourney(false);
    return StageDevelopmentCheckpointSimulation(gameSimulation, checkpoint, evidence, stepObservation);
}

} // namespace horde::gameplay
