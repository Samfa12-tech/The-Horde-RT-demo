#include <algorithm>
#include <array>
#include <cmath>
#include <cstdint>
#include <iostream>
#include <memory>

#include "gameplay/simulation/BoundedTransportQueue.h"
#include "gameplay/simulation/GameSimulation.h"
#include "gameplay/simulation/InputMailbox.h"
#include "gameplay/ShowcaseCheckpoints.h"
#include "gameplay/DevelopmentCheckpointSimulation.h"
#include "gameplay/effects/KeeperTorchLighting.h"

namespace
{

using namespace horde::gameplay;
using namespace horde::gameplay::simulation;

std::size_t CountEvents(const BoundedGameplayEventQueue& queue, GameplayEventType type)
{
    return static_cast<std::size_t>(std::count_if(
        queue.Events().begin(),
        queue.Events().end(),
        [type](const GameplayEvent& event) { return event.type == type; }));
}

bool NearlyEqual(float left, float right, float epsilon = 0.0001f)
{
    return std::abs(left - right) <= epsilon;
}

bool KeeperLightsMatch(const SimulationSnapshot& snapshot, const float expectedStrength)
{
    if (snapshot.fireEmitterCount != 3u || snapshot.fireEmitters[0].stableId != 1u)
        return false;
    for (std::size_t index = 0u; index < effects::kKeeperTorchAnchors.size(); ++index)
    {
        const auto& anchor = effects::kKeeperTorchAnchors[index];
        const auto& light = snapshot.fireEmitters[index + 1u];
        const auto item = effects::KeeperTorchWorldFromItem(anchor);
        if (light.stableId != anchor.stableId || light.seed != anchor.seed ||
            light.parentObject != effects::FireEmitterParentObject::WorldObject ||
            light.zone != ShowcaseZone::Finale || !NearlyEqual(light.strength, expectedStrength) ||
            !NearlyEqual(light.fuel, 1.0f) || !std::isfinite(light.phase) ||
            light.phase < 0.0f || light.phase >= 1.0f ||
            light.worldFromFlame != items::MultiplyHeldItemTransforms(item, items::OriginalTorchFlameSocketTransform()) ||
            light.worldFromLight != items::MultiplyHeldItemTransforms(item, items::OriginalTorchLightSocketTransform()))
            return false;
    }
    return snapshot.fireEmitters[1].stableId != snapshot.fireEmitters[2].stableId &&
           snapshot.fireEmitters[1].seed != snapshot.fireEmitters[2].seed;
}

template <typename Check>
void TestKeeperTorchLighting(Check&& check)
{
    static_assert(effects::kFireEmitterCapacity == 4u);
    static_assert(effects::kActiveFireEmitterCapacity == 4u);
    GameSimulation simulation;
    check(KeeperLightsMatch(simulation.Snapshot(), 0.0f) &&
          simulation.Snapshot().fireEmitters[0].strength > 0.0f,
          "new world torches start dark with IDs3/4 while the original opening torch remains lit");
    check(NearlyEqual(simulation.Snapshot().fireEmitters[1].worldFromFlame[13], 1.02f) &&
          NearlyEqual(simulation.Snapshot().fireEmitters[1].worldFromLight[13], 0.99f) &&
          NearlyEqual(simulation.Snapshot().fireEmitters[1].worldFromLight[14] -
                      simulation.Snapshot().fireEmitters[1].worldFromFlame[14], 0.025f),
          "fixed world torches retain the admitted OriginalTorch Flame/Light socket offsets");
    InputSnapshot input;
    input.hasAuthoritativePlayerPose = true;
    input.authoritativePlayerX = kKeeperRetryPosition.x;
    input.authoritativePlayerZ = kKeeperRetryPosition.z;
    input.yawRadians = -1.57079632679f;
    input.damageEnabled = false;
    simulation.StepFixed(input);
    check(!simulation.Snapshot().lich.revealStarted && KeeperLightsMatch(simulation.Snapshot(), 0.0f),
          "arrival or replay pose alone cannot ignite the flanks before the torch-loss reveal gate");
    simulation.ApplyShowcaseCheckpoint(8);
    check(KeeperLightsMatch(simulation.Snapshot(), 0.0f),
          "settled torch approach import does not prematurely light the Keeper room");
    simulation.StepFixed(input);
    check(CountEvents(simulation.Events(), GameplayEventType::KeeperRevealStarted) == 1u &&
          simulation.Snapshot().lich.revealStarted && KeeperLightsMatch(simulation.Snapshot(), 1.0f) &&
          NearlyEqual(simulation.Snapshot().fireEmitters[1].phase, (1.0f / 60.0f) * 0.37f),
          "both world torches ignite on the exact existing reveal-event tick and advance once per fixed step");
    simulation.ClearEvents();
    bool revealLit = true;
    for (int tick = 0; tick < 359; ++tick)
    {
        simulation.StepFixed(input);
        revealLit = revealLit && KeeperLightsMatch(simulation.Snapshot(), 1.0f);
        simulation.ClearEvents();
    }
    check(revealLit && simulation.Snapshot().lich.revealComplete,
          "both stationary flank flames remain continuously lit through all six seconds of reveal");
    const auto beforePause = simulation.Snapshot().fireEmitters;
    InputSnapshot paused = input;
    paused.paused = true;
    simulation.AdvanceFrame(paused, 20.0);
    check(KeeperLightsMatch(simulation.Snapshot(), 1.0f) &&
          simulation.Snapshot().fireEmitters[1].phase == beforePause[1].phase &&
          simulation.Snapshot().fireEmitters[2].phase == beforePause[2].phase &&
          simulation.Snapshot().fireEmitters[1].lowPassNoise == beforePause[1].lowPassNoise &&
          simulation.Snapshot().fireEmitters[2].lowPassNoise == beforePause[2].lowPassNoise,
          "menu or lifecycle suspension freezes flank phase and flicker without extinguishing the encounter");
    InputSnapshot retreat = input;
    retreat.authoritativePlayerX = kTorchBayCenters[3].x;
    retreat.authoritativePlayerZ = kTorchBayCenters[3].z;
    simulation.StepFixed(retreat);
    check(simulation.Snapshot().zone != ShowcaseZone::Finale &&
          KeeperLightsMatch(simulation.Snapshot(), 1.0f) &&
          CountEvents(simulation.Events(), GameplayEventType::KeeperRevealStarted) == 0u,
          "retreat keeps the existing revealed encounter lights without replaying ignition feedback");
    simulation.ClearEvents();

    std::uint64_t attack = 0u;
    bool fightLit = true;
    for (int tick = 0; tick < 1200 && simulation.Snapshot().lich.health > 0; ++tick)
    {
        const auto& before = simulation.Snapshot();
        input.authoritativePlayerX = before.lich.x;
        input.authoritativePlayerZ = before.lich.z;
        if (before.playerCombat.action == PlayerCombatAction::Idle &&
            before.lich.hitCooldownRemaining <= 0.00001f)
            input.commands.attack = ++attack;
        simulation.StepFixed(input);
        fightLit = fightLit && KeeperLightsMatch(simulation.Snapshot(), 1.0f);
        simulation.ClearEvents();
    }
    check(fightLit && simulation.Snapshot().lich.health == 0 &&
          simulation.Snapshot().lich.phase == LichPhase::Dead &&
          !simulation.Snapshot().lich.deathAnimationComplete && KeeperLightsMatch(simulation.Snapshot(), 1.0f),
          "three actual accepted sword hits leave flank flames lit during the lethal tick and visible death clip");
    bool deathLit = true;
    bool unlockedBeforeCompletion = false;
    int deathTicks = 0;
    while (!simulation.Snapshot().lich.deathAnimationComplete && deathTicks < 300)
    {
        input.authoritativePlayerX = simulation.Snapshot().lich.x;
        input.authoritativePlayerZ = simulation.Snapshot().lich.z;
        simulation.StepFixed(input);
        ++deathTicks;
        const auto& state = simulation.Snapshot();
        if (!state.lich.deathAnimationComplete)
        {
            deathLit = deathLit && KeeperLightsMatch(state, 1.0f);
            if (state.chestReward.phase == interactions::ChestRewardPhase::ClosedUnlocked)
                unlockedBeforeCompletion = true;
        }
        simulation.ClearEvents();
    }
    check(deathLit && unlockedBeforeCompletion && deathTicks >= 177 && deathTicks <= 179 &&
          simulation.Snapshot().lich.deathAnimationComplete && KeeperLightsMatch(simulation.Snapshot(), 0.0f),
          "flanks survive the earlier two-second chest unlock and extinguish exactly at the full Dead clip completion");
    for (int tick = 0; tick < 60; ++tick) simulation.StepFixed(input);
    check(KeeperLightsMatch(simulation.Snapshot(), 0.0f),
          "completed death cannot reignite flank flames on later encounter ticks");
    simulation.ClearEvents();
    input.authoritativePlayerX = interactions::kRewardChestInteractionPosition.x + 1.30f;
    input.authoritativePlayerZ = interactions::kRewardChestInteractionPosition.z;
    input.yawRadians = -1.57079632679f;
    ++input.commands.interact;
    simulation.StepFixed(input);
    bool rewardDark = KeeperLightsMatch(simulation.Snapshot(), 0.0f);
    for (int tick = 0; tick < 90; ++tick)
    {
        simulation.StepFixed(input);
        rewardDark = rewardDark && KeeperLightsMatch(simulation.Snapshot(), 0.0f);
    }
    ++input.commands.interact;
    simulation.StepFixed(input);
    check(rewardDark && simulation.Snapshot().chestReward.phase == interactions::ChestRewardPhase::LanternClaimed &&
          simulation.Snapshot().interaction.heldLightKind == interactions::HeldLightKind::RewardLantern &&
          CountEvents(simulation.Events(), GameplayEventType::LanternClaimed) == 1u &&
          KeeperLightsMatch(simulation.Snapshot(), 0.0f),
          "actual chest opening and reward claim preserve the lantern while both completed-encounter flanks remain dark");
    simulation.ClearEvents();
    simulation.StepFixed(input);
    check(simulation.Snapshot().interaction.heldLightKind == interactions::HeldLightKind::RewardLantern &&
          CountEvents(simulation.Events(), GameplayEventType::LanternClaimed) == 0u &&
          KeeperLightsMatch(simulation.Snapshot(), 0.0f),
          "ordinary claimed reward ticks cannot repeat claim feedback or reignite completed flank flames");
    simulation.RetryEncounter();
    check(simulation.Snapshot().lich.revealPhase == KeeperRevealPhase::RetryRecognition &&
          KeeperLightsMatch(simulation.Snapshot(), 1.0f) &&
          simulation.Snapshot().fireEmitters[1].phase == 0.0f &&
          simulation.Snapshot().fireEmitters[2].phase == 0.0f && simulation.Events().Empty(),
          "real retry restores lit one-second recognition with reset independent phases and no new audio events");
    input = {};
    input.damageEnabled = false;
    input.yawRadians = -1.57079632679f;
    for (int tick = 0; tick < 60; ++tick) simulation.StepFixed(input);
    check(simulation.Snapshot().lich.revealComplete && KeeperLightsMatch(simulation.Snapshot(), 1.0f) &&
          CountEvents(simulation.Events(), GameplayEventType::KeeperRevealStarted) == 0u &&
          CountEvents(simulation.Events(), GameplayEventType::KeeperWarning) == 0u,
          "retry recognition retains lit flanks while preserving the existing cue contract");
    simulation.ResetRoute();
    check(KeeperLightsMatch(simulation.Snapshot(), 0.0f) &&
          simulation.Snapshot().fireEmitters[1].phase == 0.0f &&
          simulation.Snapshot().fireEmitters[2].phase == 0.0f &&
          simulation.Snapshot().fireEmitters[0].strength > 0.0f,
          "full route reset extinguishes the world pair and resets phase without dropping the opening light");
    for (const int checkpoint : {0, 6, 8, 9, 10, 11})
    {
        check(simulation.ApplyShowcaseCheckpoint(checkpoint), "Keeper fire coverage imports actual authored checkpoint");
        const float expected = checkpoint == 9 || checkpoint == 10 ? 1.0f : 0.0f;
        check(KeeperLightsMatch(simulation.Snapshot(), expected) &&
              simulation.Snapshot().fireEmitters[1].phase == 0.0f &&
              simulation.Snapshot().fireEmitters[2].phase == 0.0f && simulation.Events().Empty(),
              "checkpoint import lights only already-revealed combat and never dormant or completed-death states");
    }
    const auto claimed = simulation.Snapshot();
    simulation.ImportRewardCheckpoint(claimed.chestReward, claimed.interaction, claimed.finale);
    check(simulation.Snapshot().chestReward.phase == interactions::ChestRewardPhase::LanternClaimed &&
          KeeperLightsMatch(simulation.Snapshot(), 0.0f) &&
          simulation.Events().Empty(),
          "claimed reward checkpoint import preserves terminal dark flanks without emitting ignition cues");
}

template <typename Check>
void TestKeeperReveal(Check&& check)
{
    static_assert(static_cast<int>(GameplayEventType::TorchExtinguished) == 16);
    static_assert(static_cast<int>(GameplayEventType::KeeperRevealStarted) == 17);
    static_assert(static_cast<int>(GameplayEventType::KeeperWarning) == 18);
    static_assert(static_cast<int>(GameplayEventType::KeeperCombatReady) == 19);
    InputSnapshot arrival;
    arrival.hasAuthoritativePlayerPose = true;
    arrival.authoritativePlayerX = kKeeperRetryPosition.x;
    arrival.authoritativePlayerZ = kKeeperRetryPosition.z;
    arrival.yawRadians = -1.57079632679f;

    GameSimulation unsettled;
    unsettled.AdvanceFrame(arrival, 1.0 / 60.0);
    check(!unsettled.Snapshot().lich.revealStarted &&
          CountEvents(unsettled.Events(), GameplayEventType::KeeperRevealStarted) == 0u,
          "keeper threshold cannot awaken before the torch-loss sequence settles");

    for (const int rate : {15, 30, 60, 120})
    {
        GameSimulation simulation;
        check(simulation.ApplyShowcaseCheckpoint(8), "green approach import initializes the settled torch");
        std::array<std::size_t, 3> counts{};
        std::uint64_t previousSequence = 0u;
        bool ordered = true;
        bool safe = true;
        bool aligned = true;
        float previousY = simulation.Snapshot().lich.y;
        for (int frame = 0; frame < rate * 6; ++frame)
        {
            simulation.AdvanceFrame(arrival, 1.0 / static_cast<double>(rate));
            const auto& state = simulation.Snapshot();
            safe = safe && state.lich.health == 3 && state.playerVitals.vitality == 3 &&
                !state.lich.damagePulse && state.lich.staffLightStrength == 0.0f &&
                state.chestReward.phase == interactions::ChestRewardPhase::Locked &&
                KeeperLightsMatch(state, state.lich.revealStarted ? 1.0f : 0.0f);
            aligned = aligned && NearlyEqual(state.lich.x, kKeeperStagingPosition.x) &&
                NearlyEqual(state.lich.z, kKeeperStagingPosition.z) &&
                state.lich.y >= previousY - 0.0001f &&
                state.lich.y - previousY < 0.025f;
            previousY = state.lich.y;
            for (const GameplayEvent& event : simulation.Events().Events())
            {
                if (event.type == GameplayEventType::KeeperRevealStarted ||
                    event.type == GameplayEventType::KeeperWarning ||
                    event.type == GameplayEventType::KeeperCombatReady)
                {
                    const auto index = static_cast<std::size_t>(event.type) - 17u;
                    ++counts[index];
                    ordered = ordered && event.sequence > previousSequence &&
                        event.source == EntityId::Lich && event.target == EntityId::Player &&
                        NearlyEqual(event.worldX, kKeeperStagingPosition.x) &&
                        NearlyEqual(event.listenerX, arrival.authoritativePlayerX) &&
                        NearlyEqual(event.listenerYawRadians, arrival.yawRadians);
                    previousSequence = event.sequence;
                }
            }
            simulation.ClearEvents();
            if (frame < rate * 6 - 1)
                safe = safe && !state.lich.revealComplete;
        }
        check(safe && aligned && ordered && counts == std::array<std::size_t, 3>{1u, 1u, 1u} &&
              simulation.Snapshot().lich.revealComplete &&
              simulation.Snapshot().lich.phase == LichPhase::MaintainingRange &&
              NearlyEqual(simulation.Snapshot().lich.phaseTime, 0.0f) &&
              NearlyEqual(simulation.Snapshot().lich.y, -0.77f + LichEncounter::kRevealRise),
              "six-second reveal must be continuous, harmless and once-only at all delivery rates");

        int chargeTicks = 0;
        while (simulation.Snapshot().lich.phase != LichPhase::Charging && chargeTicks < 600)
        {
            simulation.AdvanceFrame(arrival, 1.0 / 60.0);
            ++chargeTicks;
            simulation.ClearEvents();
        }
        check(chargeTicks >= 39 && chargeTicks < 600,
              "combat must begin with the full existing reposition rather than spending it during warning");
        for (int tick = 0; tick < 71; ++tick)
        {
            simulation.AdvanceFrame(arrival, 1.0 / 60.0);
            check(simulation.Snapshot().lich.phase == LichPhase::Charging &&
                  !simulation.Snapshot().lich.damagePulse,
                  "first attack cannot arrive before all 1.20 seconds of charging");
            simulation.ClearEvents();
        }
        simulation.AdvanceFrame(arrival, 1.0 / 60.0);
        check(simulation.Snapshot().lich.phase == LichPhase::Recovering &&
              simulation.Snapshot().lich.damagePulse,
              "the complete first telegraph ends in the existing damage pulse");
    }

    GameSimulation retreat;
    retreat.ApplyShowcaseCheckpoint(8);
    for (int tick = 0; tick < 200; ++tick) retreat.StepFixed(arrival);
    check(retreat.Snapshot().lich.revealPhase == KeeperRevealPhase::Warning &&
          retreat.Snapshot().lich.titleOpacity > 0.99f,
          "the warning presents a readable title through the immutable snapshot");
    retreat.ClearEvents();
    const auto frozen = retreat.Snapshot().lich;
    InputSnapshot paused = arrival;
    paused.paused = true;
    paused.commands.attack = 1u;
    retreat.AdvanceFrame(paused, 30.0);
    check(NearlyEqual(retreat.Snapshot().lich.revealElapsedSeconds, frozen.revealElapsedSeconds) &&
          NearlyEqual(retreat.Snapshot().lich.titleOpacity, frozen.titleOpacity) && retreat.Events().Empty(),
          "pause suspends reveal/title and consumes attack without replaying one-shot boundaries");
    const auto generation = retreat.Snapshot().enemyRoster.encounters[1].resetGeneration;
    InputSnapshot back = arrival;
    back.authoritativePlayerX = 4.2f;
    back.authoritativePlayerZ = -15.2f;
    back.yawRadians = 1.0f;
    back.commands.attack = 1u;
    retreat.AdvanceFrame(back, 1.0 / 60.0);
    check(retreat.Snapshot().activeEnemyKind == EnemyKind::Lich &&
          NearlyEqual(retreat.Snapshot().playerX, arrival.authoritativePlayerX) &&
          NearlyEqual(retreat.Snapshot().playerZ, arrival.authoritativePlayerZ) &&
          NearlyEqual(retreat.Snapshot().playerYawRadians, back.yawRadians) &&
          retreat.Snapshot().lich.revealElapsedSeconds > frozen.revealElapsedSeconds &&
          NearlyEqual(retreat.Snapshot().lich.x, kKeeperStagingPosition.x),
          "intro holds translation while allowing look and advancing the reveal clock");
    for (int tick = 0; tick < 160; ++tick)
    {
        retreat.AdvanceFrame(back, 1.0 / 60.0);
        retreat.ClearEvents();
    }
    arrival.commands.attack = 1u;
    retreat.AdvanceFrame(arrival, 1.0 / 60.0);
    check(retreat.Snapshot().lich.revealComplete &&
          retreat.Snapshot().enemyRoster.encounters[1].resetGeneration == generation &&
          CountEvents(retreat.Events(), GameplayEventType::KeeperRevealStarted) == 0u &&
          CountEvents(retreat.Events(), GameplayEventType::KeeperWarning) == 0u,
          "threshold reentry preserves the completed encounter attempt without replaying awakening");

    GameSimulation retreatSafety;
    retreatSafety.ApplyShowcaseCheckpoint(8);
    InputSnapshot safeArrival = arrival;
    safeArrival.commands = {};
    for (int tick = 0; tick < 100; ++tick) retreatSafety.StepFixed(safeArrival);
    InputSnapshot nearGuard;
    nearGuard.hasAuthoritativePlayerPose = true;
    nearGuard.authoritativePlayerX = 0.0f;
    nearGuard.authoritativePlayerZ = -4.8f;
    bool stayedInvulnerable = true;
    for (int tick = 0; tick < 240; ++tick)
    {
        retreatSafety.StepFixed(nearGuard);
        stayedInvulnerable = stayedInvulnerable && retreatSafety.Snapshot().playerVitals.vitality == 3 &&
            NearlyEqual(retreatSafety.Snapshot().playerX, safeArrival.authoritativePlayerX) &&
            NearlyEqual(retreatSafety.Snapshot().playerZ, safeArrival.authoritativePlayerZ) &&
            CountEvents(retreatSafety.Events(), GameplayEventType::PlayerDamaged) == 0u &&
            CountEvents(retreatSafety.Events(), GameplayEventType::PlayerKilled) == 0u;
        retreatSafety.ClearEvents();
    }
    check(stayedInvulnerable && !retreatSafety.Snapshot().lich.revealComplete,
          "authoritative pose publications cannot bypass the harmless intro translation hold");

    GameSimulation held;
    held.ApplyShowcaseCheckpoint(8);
    InputSnapshot beforeArrival = arrival;
    beforeArrival.authoritativePlayerX = 4.2f;
    beforeArrival.authoritativePlayerZ = -15.2f;
    beforeArrival.commands.attack = 1u;
    held.StepFixed(beforeArrival);
    check(held.Snapshot().playerCombat.action != PlayerCombatAction::Idle,
          "approach fixture begins with an ordinary live sword cut");
    held.ClearEvents();
    InputSnapshot heldInput = arrival;
    heldInput.commands.attack = 3u;
    heldInput.commands.parry = 1u;
    held.StepFixed(heldInput);
    check(held.Snapshot().lich.revealStarted &&
          held.Snapshot().playerCombat.action == PlayerCombatAction::Idle &&
          CountEvents(held.Events(), GameplayEventType::PlayerSwing) == 0u,
          "arrival cancels an existing cut and drops simultaneous buffered actions");
    held.ClearEvents();
    heldInput.hasAuthoritativePlayerPose = false;
    heldInput.moveForward = 1.0f;
    heldInput.moveStrafe = -1.0f;
    bool allPhasesHeld = true;
    std::array<bool, 3> heldPhases{};
    for (int tick = 1; tick < 360; ++tick)
    {
        const auto phase = held.Snapshot().lich.revealPhase;
        if (phase == KeeperRevealPhase::Awakening) heldPhases[0] = true;
        if (phase == KeeperRevealPhase::Warning) heldPhases[1] = true;
        if (phase == KeeperRevealPhase::Ready) heldPhases[2] = true;
        heldInput.yawRadians += 0.01f;
        ++heldInput.commands.attack;
        ++heldInput.commands.parry;
        ++heldInput.commands.dodge;
        held.StepFixed(heldInput);
        allPhasesHeld = allPhasesHeld &&
            NearlyEqual(held.Snapshot().playerX, arrival.authoritativePlayerX) &&
            NearlyEqual(held.Snapshot().playerZ, arrival.authoritativePlayerZ) &&
            NearlyEqual(held.Snapshot().playerYawRadians, heldInput.yawRadians) &&
            held.Snapshot().playerTravelledThisTick == 0.0f &&
            held.Snapshot().playerCombat.action == PlayerCombatAction::Idle &&
            CountEvents(held.Events(), GameplayEventType::PlayerSwing) == 0u &&
            CountEvents(held.Events(), GameplayEventType::PlayerFootstep) == 0u;
        held.ClearEvents();
    }
    check(allPhasesHeld && heldPhases == std::array<bool, 3>{true, true, true} &&
          held.Snapshot().lich.revealComplete,
          "all initial intro phases hold actions and translation without holding look or time");
    heldInput.moveForward = heldInput.moveStrafe = 0.0f;
    held.StepFixed(heldInput);
    check(held.Snapshot().playerCombat.action == PlayerCombatAction::Idle &&
          held.Snapshot().playerTravelledThisTick == 0.0f &&
          CountEvents(held.Events(), GameplayEventType::PlayerSwing) == 0u,
          "combat handoff cannot replay held attack, parry or dodge edges");

    GameSimulation attacks;
    attacks.ApplyShowcaseCheckpoint(8);
    InputSnapshot close = arrival;
    close.commands = {};
    close.authoritativePlayerX = kKeeperStagingPosition.x;
    close.authoritativePlayerZ = kKeeperStagingPosition.z;
    for (int tick = 0; tick < 351; ++tick) attacks.StepFixed(close);
    close.commands.attack = 1u;
    attacks.StepFixed(close);
    // Multiple reveal-time edges must be consumed without a swing or a
    // continuation leaking into combat after the presentation ends.
    for (int tick = 0; tick < 4; ++tick) attacks.StepFixed(close);
    close.commands.attack = 3u;
    for (int tick = 0; tick < 70; ++tick) attacks.StepFixed(close);
    check(attacks.Snapshot().lich.revealComplete && attacks.Snapshot().lich.health == 3 &&
          CountEvents(attacks.Events(), GameplayEventType::PlayerSwing) == 0u &&
          CountEvents(attacks.Events(), GameplayEventType::EnemyHit) == 0u &&
          CountEvents(attacks.Events(), GameplayEventType::LichDefeated) == 0u,
          "reveal attack edges are dropped without swinging across combat handoff");
    attacks.ClearEvents();
    close.commands.attack = 4u;
    for (int tick = 0; tick < 40; ++tick)
    {
        close.authoritativePlayerX = attacks.Snapshot().lich.x;
        close.authoritativePlayerZ = attacks.Snapshot().lich.z;
        attacks.StepFixed(close);
    }
    check(attacks.Snapshot().lich.health == 2 &&
          CountEvents(attacks.Events(), GameplayEventType::EnemyHit) == 1u,
          "a fresh post-reveal attack uses the unchanged accepted-hit rules");

    attacks.RetryEncounter();
    check(attacks.Snapshot().lich.revealPhase == KeeperRevealPhase::RetryRecognition &&
          NearlyEqual(attacks.Snapshot().playerX, kKeeperRetryPosition.x) &&
          NearlyEqual(attacks.Snapshot().playerZ, kKeeperRetryPosition.z) && attacks.Events().Empty(),
          "live retry uses safe arrival and event-free one-second recognition initialization");
    InputSnapshot retryInput;
    retryInput.yawRadians = -1.57079632679f;
    retryInput.moveForward = 1.0f;
    retryInput.moveStrafe = 1.0f;
    for (int tick = 0; tick < 59; ++tick)
    {
        ++retryInput.commands.attack;
        ++retryInput.commands.parry;
        ++retryInput.commands.dodge;
        attacks.StepFixed(retryInput);
    }
    check(!attacks.Snapshot().lich.revealComplete && attacks.Snapshot().lich.health == 3,
          "retry cannot enable either actor's damage before its complete recognition beat");
    ++retryInput.commands.attack;
    ++retryInput.commands.parry;
    ++retryInput.commands.dodge;
    attacks.StepFixed(retryInput);
    check(attacks.Snapshot().lich.revealComplete &&
          CountEvents(attacks.Events(), GameplayEventType::KeeperRevealStarted) == 0u &&
          CountEvents(attacks.Events(), GameplayEventType::KeeperWarning) == 0u &&
          CountEvents(attacks.Events(), GameplayEventType::KeeperCombatReady) == 1u &&
          NearlyEqual(attacks.Snapshot().lich.phaseTime, 0.0f) &&
          NearlyEqual(attacks.Snapshot().playerX, kKeeperRetryPosition.x) &&
          NearlyEqual(attacks.Snapshot().playerZ, kKeeperRetryPosition.z) &&
          attacks.Snapshot().playerCombat.action == PlayerCombatAction::Idle &&
          CountEvents(attacks.Events(), GameplayEventType::PlayerSwing) == 0u &&
          CountEvents(attacks.Events(), GameplayEventType::PlayerFootstep) == 0u,
          "retry enables full combat once after one second without replaying full reveal cues");
    retryInput.moveForward = retryInput.moveStrafe = 0.0f;
    attacks.ClearEvents();
    attacks.StepFixed(retryInput);
    check(attacks.Snapshot().playerCombat.action == PlayerCombatAction::Idle &&
          NearlyEqual(attacks.Snapshot().playerX, kKeeperRetryPosition.x) &&
          NearlyEqual(attacks.Snapshot().playerZ, kKeeperRetryPosition.z) && attacks.Events().Empty(),
          "last recognition tick leaves no buffered sword, parry or dodge action");
    ++retryInput.commands.attack;
    attacks.StepFixed(retryInput);
    check(CountEvents(attacks.Events(), GameplayEventType::PlayerSwing) == 1u,
          "fresh input after recognition releases the sword action hold");
    attacks.ResetRoute();
    check(!attacks.Snapshot().lich.revealStarted &&
          NearlyEqual(attacks.Snapshot().lich.x, kKeeperStagingPosition.x) &&
          attacks.Snapshot().skeletonEnemies[1].idlePhaseSeconds > 0.0f,
          "full reset restores fresh keeper and bounded per-entity incidental idle phase");

    GameSimulation clearance;
    clearance.ApplyShowcaseCheckpoint(8);
    arrival.commands = {};
    for (int tick = 0; tick < 2; ++tick) clearance.StepFixed(arrival);
    InputSnapshot move;
    move.yawRadians = -1.57079632679f;
    move.moveForward = 1.0f;
    for (int tick = 0; tick < 130; ++tick) clearance.StepFixed(move);
    const auto& stopped = clearance.Snapshot();
    check(NearlyEqual(stopped.playerX, arrival.authoritativePlayerX) &&
          NearlyEqual(stopped.playerZ, arrival.authoritativePlayerZ) &&
          stopped.playerTravelledThisTick == 0.0f &&
          std::hypot(stopped.playerX - stopped.lich.x, stopped.playerZ - stopped.lich.z) >=
              kKeeperPresentationCollisionRadius + kPlayerCollisionRadius - 0.001f &&
          stopped.playerX > stopped.lich.x && NearlyEqual(stopped.playerYawRadians, move.yawRadians),
          "held movement cannot advance or overlap the rising keeper");
    move.moveForward = -1.0f;
    const float contactX = stopped.playerX;
    for (int tick = 0; tick < 10; ++tick) clearance.StepFixed(move);
    check(NearlyEqual(clearance.Snapshot().playerX, contactX),
          "reverse movement is also held during the presentation");
    move.moveForward = 0.0f;
    for (int tick = 0; tick < 218; ++tick) clearance.StepFixed(move);
    move.moveForward = -1.0f;
    for (int tick = 0; tick < 10; ++tick) clearance.StepFixed(move);
    check(clearance.Snapshot().lich.revealComplete && clearance.Snapshot().playerX > contactX + 0.2f,
          "normal translation resumes after the full presentation");

    for (const int captureId : {9, 10, 11})
    {
        GameSimulation capture;
        check(capture.ApplyShowcaseCheckpoint(captureId), "legacy keeper capture import succeeds");
        const auto* checkpoint = FindShowcaseCheckpoint(captureId);
        check(NearlyEqual(capture.Snapshot().playerX, checkpoint->x) &&
              NearlyEqual(capture.Snapshot().playerZ, checkpoint->z) &&
              capture.Snapshot().lich.revealComplete && capture.Events().Empty() &&
              capture.Snapshot().tickIndex == 0u &&
              NearlyEqual(capture.Snapshot().skeletonEnemies[1].idlePhaseSeconds, 0.0f),
              "legacy capture retains authored player/combat state without reveal events or incidental offsets");
    }
    GameSimulation defeated;
    defeated.ApplyShowcaseCheckpoint(11);
    const auto rewardBeforeRetreat = defeated.Snapshot().chestReward;
    defeated.AdvanceFrame(back, 1.0 / 60.0);
    defeated.AdvanceFrame(arrival, 1.0 / 60.0);
    check(defeated.Snapshot().lich.phase == LichPhase::Dead && defeated.Snapshot().lich.health == 0 &&
          defeated.Snapshot().enemyRoster.encounters[1].status == EncounterStatus::Dead &&
          defeated.Snapshot().chestReward.phase == rewardBeforeRetreat.phase &&
          CountEvents(defeated.Events(), GameplayEventType::KeeperRevealStarted) == 0u &&
          CountEvents(defeated.Events(), GameplayEventType::LichDefeated) == 0u &&
          CountEvents(defeated.Events(), GameplayEventType::LanternClaimed) == 0u,
          "defeated keeper and claimed reward stay terminal across route selection and reentry");
}

} // namespace

template <typename Check>
void TestSkeletonIncidental(Check&& check)
{
    static_assert(static_cast<int>(GameplayEventType::SkeletonIncidental) == 20);
    GameSimulationConfig config;
    config.playerStartZ = -7.0f;
    GameSimulation simulation(config);
    InputSnapshot idle;
    idle.hasAuthoritativePlayerPose = true;
    idle.authoritativePlayerZ = -7.0f;
    std::array<EntityId, 3> sources{};
    std::array<int, 3> times{};
    std::size_t cueCount = 0u;
    bool valid = true;
    for (int tick = 1; tick <= 2400; ++tick)
    {
        simulation.StepFixed(idle);
        for (const auto& event : simulation.Events().Events())
        {
            if (event.type != GameplayEventType::SkeletonIncidental) continue;
            valid = valid && cueCount < sources.size() && event.target == EntityId::Invalid &&
                NearlyEqual(event.intensity, 0.30f) &&
                NearlyEqual(event.listenerZ, idle.authoritativePlayerZ);
            if (cueCount < sources.size())
            {
                sources[cueCount] = event.source;
                times[cueCount] = tick;
            }
            ++cueCount;
        }
        simulation.ClearEvents();
        if (tick == 719)
        {
            check(cueCount == 0u, "idle bones remain silent until at least twelve seconds");
            idle.paused = true;
            simulation.SynchronizePausedInput(idle);
            simulation.AdvanceFrame(idle, 60.0);
            check(CountEvents(simulation.Events(), GameplayEventType::SkeletonIncidental) == 0u,
                  "paused lifecycle barrier cannot generate or catch up idle cues");
            idle.paused = false;
        }
    }
    check(valid && cueCount == 3u &&
          sources == std::array<EntityId, 3>{EntityId::SkeletonA, EntityId::SkeletonB, EntityId::SkeletonA} &&
          times == std::array<int, 3>{720, 1080, 2400},
          "sparse incidental cues stagger actors and preserve event-time positional identity");
    simulation.ResetRoute();
    for (int tick = 0; tick < 719; ++tick)
    {
        simulation.StepFixed(idle);
        check(CountEvents(simulation.Events(), GameplayEventType::SkeletonIncidental) == 0u,
              "route reset starts a fresh idle delay without a stale cue");
        simulation.ClearEvents();
    }
    simulation.StepFixed(idle);
    check(CountEvents(simulation.Events(), GameplayEventType::SkeletonIncidental) == 1u,
          "reset deterministic idle cadence restarts once at the minimum delay");
    check(simulation.ApplyShowcaseCheckpoint(0), "idle capture fixture imports");
    for (int tick = 0; tick < 1200; ++tick)
    {
        simulation.StepFixed(idle);
        check(CountEvents(simulation.Events(), GameplayEventType::SkeletonIncidental) == 0u,
              "legacy capture imports keep incidental presentation event-free");
        simulation.ClearEvents();
    }
}

int main()
{
    bool passed = true;
    const auto check = [&passed](bool condition, const char* message)
    {
        if (!condition)
        {
            passed = false;
            std::cerr << "Simulation gameplay test failed: " << message << '\n';
        }
    };
    TestKeeperReveal(check);
    TestKeeperTorchLighting(check);
    TestSkeletonIncidental(check);

    BoundedGameplayEventQueue identityQueue;
    GameplayEvent first;
    first.type = GameplayEventType::EnemyHit;
    first.source = EntityId::Player;
    first.target = EntityId::Skeleton;
    first.worldX = 1.0f;
    first.listenerX = -1.0f;
    first.listenerZ = 2.0f;
    first.listenerYawRadians = 0.25f;
    GameplayEvent second = first;
    second.target = EntityId::Lich;
    second.worldX = 2.0f;
    second.listenerX = -2.0f;
    second.listenerYawRadians = -0.50f;
    check(identityQueue.Push(first) && identityQueue.Push(second),
          "two same-type events must fit the bounded queue");
    check(identityQueue[0].sequence != identityQueue[1].sequence &&
          identityQueue[0].target == EntityId::Skeleton &&
          identityQueue[1].target == EntityId::Lich &&
          identityQueue[0].worldX != identityQueue[1].worldX &&
          identityQueue[0].listenerX != identityQueue[1].listenerX &&
          identityQueue[0].listenerYawRadians != identityQueue[1].listenerYawRadians,
          "same-type events must retain distinct sequences, entities, positions, and listener state");
    for (std::size_t i = identityQueue.Size(); i < BoundedGameplayEventQueue::kCapacity; ++i)
    {
        identityQueue.Push({});
    }
    check(!identityQueue.Push({}) && identityQueue.OverflowCount() == 1u &&
          identityQueue.Size() == BoundedGameplayEventQueue::kCapacity &&
          identityQueue.HighWaterMark() == BoundedGameplayEventQueue::kCapacity &&
          identityQueue.NextSequence() == BoundedGameplayEventQueue::kCapacity + 1u &&
          identityQueue[0].source == EntityId::Player &&
          identityQueue[0].target == EntityId::Skeleton &&
          NearlyEqual(identityQueue[0].listenerX, -1.0f),
          "overflow must be explicit and must retain the ordered event identity without advancing sequence");

    BoundedTransportQueue<GameplayEvent, 2u> platformQueue;
    check(platformQueue.Push(first) && platformQueue.Push(second) &&
          platformQueue.Size() == 2u && platformQueue.HighWaterMark() == 2u,
          "platform transport queue must retain bounded publications in order");
    check(!platformQueue.Push({}) && platformQueue.OverflowCount() == 1u &&
          platformQueue.Size() == 2u && platformQueue[0].target == EntityId::Skeleton &&
          platformQueue[1].target == EntityId::Lich &&
          NearlyEqual(platformQueue[0].listenerX, -1.0f) &&
          NearlyEqual(platformQueue[1].listenerX, -2.0f),
          "platform transport overflow must be visible and must not overwrite queued events");
    platformQueue.Clear();
    check(platformQueue.Size() == 0u && platformQueue.OverflowCount() == 1u &&
          platformQueue.HighWaterMark() == 2u,
          "platform transport drain must clear entries without hiding overflow diagnostics");

    GameSimulationConfig listenerConfig;
    listenerConfig.movementSpeedMetresPerSecond = 30.0f;
    GameSimulation movingListeners(listenerConfig);
    InputSnapshot movingListenerInput;
    movingListenerInput.moveForward = 1.0f;
    movingListenerInput.yawRadians = 0.15f;
    movingListenerInput.damageEnabled = false;
    const std::uint32_t listenerTicks = movingListeners.AdvanceFrame(
        movingListenerInput,
        0.100,
        101u);
    std::size_t movingFootstepCount = 0u;
    std::uint64_t previousFootstepSequence = 0u;
    bool movingFootstepsAreOrdered = true;
    bool movingFootstepsCaptureCurrentListener = true;
    bool atLeastOneListenerDiffersFromFrameEnd = false;
    for (const GameplayEvent& event : movingListeners.Events().Events())
    {
        if (event.type != GameplayEventType::PlayerFootstep)
        {
            continue;
        }
        ++movingFootstepCount;
        movingFootstepsAreOrdered = movingFootstepsAreOrdered &&
            event.sequence > previousFootstepSequence;
        movingFootstepsCaptureCurrentListener = movingFootstepsCaptureCurrentListener &&
            event.source == EntityId::Player &&
            event.target == EntityId::Invalid &&
            NearlyEqual(event.listenerX, event.worldX) &&
            NearlyEqual(event.listenerZ, event.worldZ) &&
            NearlyEqual(event.listenerYawRadians, movingListenerInput.yawRadians);
        atLeastOneListenerDiffersFromFrameEnd = atLeastOneListenerDiffersFromFrameEnd ||
            !NearlyEqual(event.listenerX, movingListeners.Snapshot().playerX) ||
            !NearlyEqual(event.listenerZ, movingListeners.Snapshot().playerZ);
        previousFootstepSequence = event.sequence;
    }
    check(listenerTicks >= 3u && movingFootstepCount >= 2u && movingFootstepsAreOrdered &&
          movingFootstepsCaptureCurrentListener && atLeastOneListenerDiffersFromFrameEnd,
          "events from several fixed ticks in one moving frame must retain each contact's listener state");

    GameSimulation attacks;
    InputSnapshot attackInput;
    attackInput.damageEnabled = false;
    attackInput.commands.attack = 1u;
    attacks.AdvanceFrame(attackInput, 1.0 / 60.0, 1u);
    check(attacks.Snapshot().lastConsumedAttackSequence == 1u &&
          CountEvents(attacks.Events(), GameplayEventType::PlayerSwing) == 1u,
          "attack sequence N must be consumed exactly once");
    for (int frame = 0; frame < 5; ++frame)
    {
        attacks.AdvanceFrame(attackInput, 1.0 / 60.0, 1u);
    }
    check(CountEvents(attacks.Events(), GameplayEventType::PlayerSwing) == 1u,
          "re-reading sequence N must not repeat the attack");
    attackInput.commands.attack = 2u;
    for (int frame = 0; frame < 40; ++frame)
    {
        attacks.AdvanceFrame(attackInput, 1.0 / 60.0, 2u);
    }
    check(attacks.Snapshot().lastConsumedAttackSequence == 2u &&
          CountEvents(attacks.Events(), GameplayEventType::PlayerSwing) == 2u,
          "a natural second press during downward wind-up must buffer one upward slice");

    attackInput.commands.attack = 3u;
    for (int frame = 0; frame < 5; ++frame)
    {
        attacks.AdvanceFrame(attackInput, 1.0 / 60.0, 3u);
    }
    check(attacks.Snapshot().lastConsumedAttackSequence == 3u &&
          CountEvents(attacks.Events(), GameplayEventType::PlayerSwing) == 2u,
          "a third edge during the committed upward action must be consumed without replay");

    const auto swingEvents = attacks.Events().Events();
    std::uint64_t firstSwingSequence = 0u;
    std::uint64_t secondSwingSequence = 0u;
    for (const GameplayEvent& event : swingEvents)
    {
        if (event.type == GameplayEventType::PlayerSwing)
        {
            if (firstSwingSequence == 0u)
            {
                firstSwingSequence = event.sequence;
            }
            else
            {
                secondSwingSequence = event.sequence;
                break;
            }
        }
    }
    check(firstSwingSequence != 0u && secondSwingSequence > firstSwingSequence,
          "buffered downward/upward swings must publish two ordered semantic events");
    check(std::all_of(swingEvents.begin(), swingEvents.end(), [](const GameplayEvent& event)
          {
              return event.type != GameplayEventType::PlayerSwing || event.target == EntityId::Invalid;
          }),
          "an out-of-range player swing must not falsely claim a skeleton target");

    GameSimulation chainedAttack;
    InputSnapshot chainedInput;
    chainedInput.hasAuthoritativePlayerPose = true;
    chainedInput.authoritativePlayerX = 0.0f;
    chainedInput.authoritativePlayerZ = -3.20f;
    chainedInput.damageEnabled = false;
    chainedInput.commands.attack = 1u;
    chainedAttack.StepFixed(chainedInput);
    while (chainedAttack.Snapshot().playerCombat.action != PlayerCombatAction::SwingActive)
    {
        chainedAttack.StepFixed(chainedInput);
    }
    chainedInput.commands.attack = 2u;
    chainedAttack.StepFixed(chainedInput);
    for (int tick = 0; tick < 60; ++tick)
    {
        chainedAttack.StepFixed(chainedInput);
    }
    std::size_t chainedSwingCount = 0u;
    std::size_t chainedHitCount = 0u;
    std::array<std::int32_t, 2u> chainedSwingPayloads{};
    for (const GameplayEvent& event : chainedAttack.Events().Events())
    {
        if (event.type == GameplayEventType::PlayerSwing && chainedSwingCount < 2u)
        {
            chainedSwingPayloads[chainedSwingCount++] = event.payload;
        }
        if (event.type == GameplayEventType::EnemyHit) ++chainedHitCount;
    }
    check(chainedAttack.Snapshot().lastConsumedAttackSequence == 2u &&
          chainedSwingCount == 2u && chainedSwingPayloads[0] == 1 &&
          chainedSwingPayloads[1] == 2 && chainedHitCount == 2u &&
          chainedAttack.Snapshot().openingEncounterComplete,
          "downward then upward cut must consume two edges and publish one identified swing/hit per cut");

    GameSimulation ownerTimedChainedAttack;
    InputSnapshot ownerTimedChainedInput;
    ownerTimedChainedInput.damageEnabled = false;
    ownerTimedChainedInput.commands.attack = 1u;
    // Reproduce the real input trace: the owner's second click arrives about
    // 400 ms after the first, after the 160 ms downstroke has visibly landed.
    for (int tick = 0; tick < 24; ++tick)
    {
        ownerTimedChainedAttack.StepFixed(ownerTimedChainedInput);
    }
    ownerTimedChainedInput.commands.attack = 2u;
    ownerTimedChainedAttack.StepFixed(ownerTimedChainedInput);
    for (int tick = 0; tick < 70; ++tick)
    {
        ownerTimedChainedAttack.StepFixed(ownerTimedChainedInput);
    }
    check(ownerTimedChainedAttack.Snapshot().lastConsumedAttackSequence == 2u &&
          CountEvents(ownerTimedChainedAttack.Events(), GameplayEventType::PlayerSwing) == 2u,
          "a natural 400 ms second press after the downstroke lands must produce the upward slice");

    GameSimulation coalescedAttack;
    InputSnapshot coalescedInput;
    coalescedInput.hasAuthoritativePlayerPose = true;
    coalescedInput.authoritativePlayerX = 0.0f;
    coalescedInput.authoritativePlayerZ = -3.20f;
    coalescedInput.damageEnabled = false;
    coalescedInput.commands.attack = 2u;
    for (int tick = 0; tick < 70; ++tick)
    {
        coalescedAttack.StepFixed(coalescedInput);
    }
    check(coalescedAttack.Snapshot().lastConsumedAttackSequence == 2u &&
          CountEvents(coalescedAttack.Events(), GameplayEventType::PlayerSwing) == 2u &&
          CountEvents(coalescedAttack.Events(), GameplayEventType::EnemyHit) == 2u,
          "a coalesced 0-to-2 publication must preserve both downward and upward edges");

    GameSimulation pausedChain;
    InputSnapshot pausedChainInput;
    pausedChainInput.damageEnabled = false;
    pausedChainInput.commands.attack = 1u;
    pausedChain.StepFixed(pausedChainInput);
    while (pausedChain.Snapshot().playerCombat.action != PlayerCombatAction::SwingActive)
    {
        pausedChain.StepFixed(pausedChainInput);
    }
    pausedChainInput.paused = true;
    pausedChainInput.commands.attack = 2u;
    pausedChain.StepFixed(pausedChainInput);
    pausedChainInput.paused = false;
    pausedChain.StepFixed(pausedChainInput);
    check(pausedChain.Snapshot().lastConsumedAttackSequence == 2u &&
          CountEvents(pausedChain.Events(), GameplayEventType::PlayerSwing) == 1u,
          "a chained edge consumed while paused must not replay or duplicate after resume");

    const auto deliverChainedAttack = [](const int renderRate)
    {
        GameSimulation delivered;
        InputSnapshot input;
        input.hasAuthoritativePlayerPose = true;
        input.authoritativePlayerX = 0.0f;
        input.authoritativePlayerZ = -3.20f;
        input.damageEnabled = false;
        input.commands.attack = 1u;
        bool secondEdgePublished = false;
        for (int frame = 0; frame < renderRate * 2; ++frame)
        {
            if (!secondEdgePublished && frame == renderRate * 2 / 5)
            {
                input.commands.attack = 2u;
                secondEdgePublished = true;
            }
            delivered.AdvanceFrame(input, 1.0 / static_cast<double>(renderRate));
        }
        return std::array<std::uint64_t, 5u>{
            delivered.Snapshot().tickIndex,
            delivered.Snapshot().lastConsumedAttackSequence,
            static_cast<std::uint64_t>(delivered.Snapshot().playerCombat.action),
            CountEvents(delivered.Events(), GameplayEventType::PlayerSwing),
            CountEvents(delivered.Events(), GameplayEventType::EnemyHit)};
    };
    const auto chainedAt30 = deliverChainedAttack(30);
    const auto chainedAt60 = deliverChainedAttack(60);
    const auto chainedAt120 = deliverChainedAttack(120);
    check(chainedAt30 == chainedAt60 && chainedAt60 == chainedAt120 &&
          chainedAt60[0] == 120u && chainedAt60[1] == 2u &&
          chainedAt60[2] == static_cast<std::uint64_t>(PlayerCombatAction::Idle) &&
          chainedAt60[3] == 2u && chainedAt60[4] == 2u,
          "30/60/120 render delivery must preserve the same owner-timed 400 ms two-cut commands, hits, and final phase");

    GameSimulation skeletonPair;
    InputSnapshot pairInput;
    pairInput.hasAuthoritativePlayerPose = true;
    pairInput.authoritativePlayerX = 0.0f;
    pairInput.authoritativePlayerZ = -2.85f;
    pairInput.yawRadians = 0.0f;
    pairInput.damageEnabled = false;
    for (int frame = 0; frame < 100; ++frame)
    {
        skeletonPair.AdvanceFrame(pairInput, 1.0 / 60.0);
    }
    check(skeletonPair.Snapshot().skeletonEnemyCount == 2u &&
          skeletonPair.Snapshot().activeSkeletonCount == 2u &&
          skeletonPair.Snapshot().skeletonEnemies[0].id == EntityId::SkeletonA &&
          skeletonPair.Snapshot().skeletonEnemies[1].id == EntityId::SkeletonB &&
          skeletonPair.Snapshot().skeletonAttackerId == EntityId::SkeletonA,
          "the bounded pair must expose stable A/B IDs and choose A on an equal-distance tie");
    check(std::hypot(skeletonPair.Snapshot().skeletonEnemies[1].x -
                     skeletonPair.Snapshot().skeletonEnemies[0].x,
                     skeletonPair.Snapshot().skeletonEnemies[1].z -
                     skeletonPair.Snapshot().skeletonEnemies[0].z) >= 0.699f,
          "the live skeleton pair must retain the deterministic 0.70 m separation");

    const float distanceToA = std::hypot(skeletonPair.Snapshot().skeletonEnemies[0].x -
                                         pairInput.authoritativePlayerX,
                                         skeletonPair.Snapshot().skeletonEnemies[0].z -
                                         pairInput.authoritativePlayerZ);
    const float distanceToB = std::hypot(skeletonPair.Snapshot().skeletonEnemies[1].x -
                                         pairInput.authoritativePlayerX,
                                         skeletonPair.Snapshot().skeletonEnemies[1].z -
                                         pairInput.authoritativePlayerZ);
    const std::size_t expectedFirstTarget = distanceToA <= distanceToB ? 0u : 1u;
    const std::size_t expectedSecondTarget = 1u - expectedFirstTarget;

    pairInput.commands.attack = 1u;
    for (int frame = 0; frame < 40 && skeletonPair.Snapshot().activeSkeletonCount == 2u; ++frame)
    {
        skeletonPair.AdvanceFrame(pairInput, 1.0 / 60.0);
    }
    check(skeletonPair.Snapshot().skeletonEnemies[expectedFirstTarget].dead &&
          !skeletonPair.Snapshot().skeletonEnemies[expectedSecondTarget].dead &&
          skeletonPair.Snapshot().activeSkeletonCount == 1u,
          "one sword action must kill only the nearest valid skeleton");
    const float defeatedX = skeletonPair.Snapshot().skeletonEnemies[expectedFirstTarget].x;
    const float defeatedZ = skeletonPair.Snapshot().skeletonEnemies[expectedFirstTarget].z;
    bool postDeathSeparationHeld = true;
    for (int frame = 0; frame < 120; ++frame)
    {
        skeletonPair.AdvanceFrame(pairInput, 1.0 / 60.0);
        postDeathSeparationHeld = postDeathSeparationHeld &&
            std::hypot(skeletonPair.Snapshot().skeletonEnemies[1].x -
                       skeletonPair.Snapshot().skeletonEnemies[0].x,
                       skeletonPair.Snapshot().skeletonEnemies[1].z -
                       skeletonPair.Snapshot().skeletonEnemies[0].z) >= 0.699f &&
            NearlyEqual(skeletonPair.Snapshot().skeletonEnemies[expectedFirstTarget].x, defeatedX) &&
            NearlyEqual(skeletonPair.Snapshot().skeletonEnemies[expectedFirstTarget].z, defeatedZ);
    }
    check(skeletonPair.Snapshot().skeletonEnemies[expectedFirstTarget].dead &&
          !skeletonPair.Snapshot().skeletonEnemies[expectedSecondTarget].dead &&
          postDeathSeparationHeld &&
          skeletonPair.Snapshot().skeletonAttackerId ==
              skeletonPair.Snapshot().skeletonEnemies[expectedSecondTarget].id,
          "a defeated skeleton must remain fixed while the separated survivor owns the attack token");

    pairInput.commands.attack = 2u;
    for (int frame = 0; frame < 60 && !skeletonPair.Snapshot().openingEncounterComplete; ++frame)
    {
        skeletonPair.AdvanceFrame(pairInput, 1.0 / 60.0);
    }
    check(skeletonPair.Snapshot().openingEncounterComplete &&
          skeletonPair.Snapshot().activeSkeletonCount == 0u &&
          skeletonPair.Snapshot().enemyRoster.encounters[0].status == EncounterStatus::Dead,
          "defeating both stable entities must complete the opening encounter");

    std::array<EntityId, 2> defeatedTargets{};
    std::size_t defeatedTargetCount = 0u;
    std::uint64_t previousDefeatSequence = 0u;
    bool orderedDistinctDefeats = true;
    bool hitImmediatelyPrecedesDefeat = true;
    GameplayEvent previousEvent{};
    for (const GameplayEvent& event : skeletonPair.Events().Events())
    {
        if (event.type == GameplayEventType::EnemyDefeated)
        {
            orderedDistinctDefeats = orderedDistinctDefeats && event.sequence > previousDefeatSequence;
            hitImmediatelyPrecedesDefeat = hitImmediatelyPrecedesDefeat &&
                previousEvent.type == GameplayEventType::EnemyHit &&
                previousEvent.target == event.target && previousEvent.sequence + 1u == event.sequence;
            previousDefeatSequence = event.sequence;
            if (defeatedTargetCount < defeatedTargets.size())
            {
                defeatedTargets[defeatedTargetCount++] = event.target;
            }
        }
        previousEvent = event;
    }
    check(defeatedTargetCount == 2u && orderedDistinctDefeats && hitImmediatelyPrecedesDefeat &&
          defeatedTargets[0] == skeletonPair.Snapshot().skeletonEnemies[expectedFirstTarget].id &&
          defeatedTargets[1] == skeletonPair.Snapshot().skeletonEnemies[expectedSecondTarget].id,
          "each ordered A/B defeat must immediately follow its entity-aware hit event");
    std::size_t pairSwingCount = 0u;
    bool pairSwingsAreTargetless = true;
    for (const GameplayEvent& event : skeletonPair.Events().Events())
    {
        if (event.type == GameplayEventType::PlayerSwing)
        {
            ++pairSwingCount;
            pairSwingsAreTargetless = pairSwingsAreTargetless && event.target == EntityId::Invalid;
        }
    }
    check(pairSwingCount == 2u && pairSwingsAreTargetless,
          "swing-intent events must stay targetless until an actual entity-aware hit resolves");

    GameSimulation parrySimulation;
    InputSnapshot parryInput;
    parryInput.hasAuthoritativePlayerPose = true;
    parryInput.authoritativePlayerX = -0.75f;
    parryInput.authoritativePlayerZ = -3.20f;
    parryInput.yawRadians = 0.0f;
    parryInput.damageEnabled = true;
    bool parryIssued = false;
    for (int frame = 0; frame < 360 &&
         CountEvents(parrySimulation.Events(), GameplayEventType::PlayerParrySucceeded) == 0u;
         ++frame)
    {
        const auto& attacker = parrySimulation.Snapshot().skeletonEnemies[0];
        if (!parryIssued && attacker.action == EnemyCombatAction::AttackWindup &&
            attacker.actionTime >= 0.98f)
        {
            parryInput.commands.parry = 1u;
            parryIssued = true;
        }
        parrySimulation.AdvanceFrame(parryInput, 1.0 / 60.0, frame + 1u);
    }
    const auto parryEvents = parrySimulation.Events().Events();
    const auto parryEvent = std::find_if(parryEvents.begin(), parryEvents.end(), [](const GameplayEvent& event)
    {
        return event.type == GameplayEventType::PlayerParrySucceeded;
    });
    check(parrySimulation.Snapshot().lastConsumedParrySequence == 1u &&
          parryEvent != parryEvents.end() &&
          parryEvent->source == EntityId::Player &&
          parryEvent->target == EntityId::SkeletonA &&
          parryEvent->tickIndex > 0u &&
          parrySimulation.Snapshot().combatPresentation.parrySuccessActive &&
          parrySimulation.Snapshot().combatPresentation.parrySuccessEventSequence ==
              parryEvent->sequence &&
          parrySimulation.Snapshot().combatPresentation.parrySuccessTickIndex ==
              parryEvent->tickIndex &&
          parrySimulation.Snapshot().combatPresentation.parrySuccessEntity ==
              EntityId::SkeletonA &&
          parrySimulation.Snapshot().playerVitals.vitality == PlayerVitals::kMaxVitality,
          "a successful parry must emit ordered tick/entity provenance and retain independent presentation state");

    InputSnapshot riposteInput = parryInput;
    riposteInput.commands.attack = 1u;
    parrySimulation.AdvanceFrame(riposteInput, 1.0 / 60.0);
    check(parrySimulation.Snapshot().playerCombat.action == PlayerCombatAction::SwingWindup &&
          parrySimulation.Snapshot().combatPresentation.parrySuccessActive &&
          parrySimulation.Snapshot().playerAnimation.reaction == CombatReaction::Parried,
          "the next-tick riposte must start immediately while independent parry presentation remains visible");

    GameSimulation catchUpParry;
    InputSnapshot catchUpParryInput = parryInput;
    catchUpParryInput.commands = {};
    bool catchUpParryIssued = false;
    for (int frame = 0; frame < 360 && !catchUpParryIssued; ++frame)
    {
        const auto& attacker = catchUpParry.Snapshot().skeletonEnemies[0];
        if (attacker.action == EnemyCombatAction::AttackWindup &&
            attacker.actionTime >= 1.05f)
        {
            catchUpParryInput.commands.parry = 1u;
            catchUpParryIssued = true;
            break;
        }
        catchUpParry.AdvanceFrame(catchUpParryInput, 1.0 / 60.0,
                                  static_cast<std::uint64_t>(frame + 1));
    }
    const std::uint32_t catchUpParryTicks = catchUpParry.AdvanceFrame(
        catchUpParryInput, 0.100, 500u);
    const auto catchUpParryEvents = catchUpParry.Events().Events();
    const auto catchUpParryEvent = std::find_if(
        catchUpParryEvents.begin(), catchUpParryEvents.end(), [](const GameplayEvent& event)
        {
            return event.type == GameplayEventType::PlayerParrySucceeded;
        });
    check(catchUpParryIssued && catchUpParryTicks >= 4u &&
          catchUpParryEvent != catchUpParryEvents.end() &&
          catchUpParry.Snapshot().combatPresentation.parrySuccessActive &&
          catchUpParry.Snapshot().combatPresentation.parrySuccessEventSequence ==
              catchUpParryEvent->sequence &&
          catchUpParry.Snapshot().combatPresentation.parrySuccessTickIndex ==
              catchUpParryEvent->tickIndex &&
          catchUpParryEvent->tickIndex < catchUpParry.Snapshot().tickIndex &&
          NearlyEqual(catchUpParry.Snapshot().combatPresentation.parrySuccessRemainingSeconds,
                      horde::gameplay::CombatTimeline::kParryPresentationSeconds) &&
          catchUpParry.Snapshot().playerCombat.action != PlayerCombatAction::ParryActive &&
          catchUpParry.Snapshot().playerAnimation.reaction == CombatReaction::Parried,
          "parry presentation must survive later catch-up ticks without holding the parry action open");

    const double frameRates[] = {15.0, 30.0, 60.0, 120.0};
    bool parryPresentationCadenceStable = true;
    for (const double frameRate : frameRates)
    {
        GameSimulation cadenceParry;
        InputSnapshot cadenceInput = parryInput;
        cadenceInput.commands = {};
        bool commandPublished = false;
        bool eventObserved = false;
        for (int frame = 0; frame < 360 && !eventObserved; ++frame)
        {
            const auto& attacker = cadenceParry.Snapshot().skeletonEnemies[0];
            if (!commandPublished && attacker.action == EnemyCombatAction::AttackWindup &&
                attacker.actionTime >= 1.05f)
            {
                cadenceInput.commands.parry = 1u;
                commandPublished = true;
            }
            cadenceParry.AdvanceFrame(cadenceInput, 1.0 / frameRate,
                                      static_cast<std::uint64_t>(frame + 1));
            eventObserved = CountEvents(cadenceParry.Events(),
                                        GameplayEventType::PlayerParrySucceeded) == 1u;
        }
        parryPresentationCadenceStable = parryPresentationCadenceStable && commandPublished &&
            eventObserved && cadenceParry.Snapshot().combatPresentation.parrySuccessActive &&
            cadenceParry.Snapshot().playerAnimation.reaction == CombatReaction::Parried;
        if (!eventObserved)
        {
            continue;
        }
        cadenceParry.AdvanceFrame(cadenceInput, 1.0 / frameRate);
        parryPresentationCadenceStable = parryPresentationCadenceStable &&
            cadenceParry.Snapshot().combatPresentation.parrySuccessActive &&
            cadenceParry.Snapshot().playerAnimation.reaction == CombatReaction::Parried;
    }
    check(parryPresentationCadenceStable,
          "parry feedback must survive its event frame and one ordinary 15/30/60/120 Hz frame");

    catchUpParry.AdvanceFrame(catchUpParryInput, 1.0 / 120.0);
    check(catchUpParry.Snapshot().simulationTicksThisFrame == 0u &&
          catchUpParry.Snapshot().combatPresentation.parrySuccessActive &&
          catchUpParry.Snapshot().playerAnimation.reaction == CombatReaction::Parried &&
          catchUpParry.Snapshot().heldItemKinematics.successJolt > 0.0f,
          "a zero-tick 120 Hz frame must refresh the shared held-sword presentation pose");
    catchUpParry.AdvanceFrame(catchUpParryInput, 0.100);
    check(catchUpParry.Snapshot().combatPresentation.parrySuccessActive &&
          catchUpParry.Snapshot().combatPresentation.parrySuccessRemainingSeconds > 0.0f,
          "a 100 ms frame contribution must age parry presentation once rather than once per catch-up tick");

    GameSimulation overCapParry;
    InputSnapshot overCapParryInput = parryInput;
    overCapParryInput.commands = {};
    bool overCapParryIssued = false;
    for (int frame = 0; frame < 360 && !overCapParryIssued; ++frame)
    {
        const auto& attacker = overCapParry.Snapshot().skeletonEnemies[0];
        if (attacker.action == EnemyCombatAction::AttackWindup &&
            attacker.actionTime >= 1.05f)
        {
            overCapParryInput.commands.parry = 1u;
            overCapParryIssued = true;
            break;
        }
        overCapParry.AdvanceFrame(overCapParryInput, 1.0 / 60.0,
                                  static_cast<std::uint64_t>(frame + 1));
    }
    overCapParry.AdvanceFrame(overCapParryInput, 0.500, 500u);
    check(overCapParryIssued &&
          CountEvents(overCapParry.Events(), GameplayEventType::PlayerParrySucceeded) == 1u &&
          overCapParry.Snapshot().combatPresentation.parrySuccessTickIndex <
              overCapParry.Snapshot().tickIndex &&
          overCapParry.Snapshot().combatPresentation.parrySuccessActive &&
          NearlyEqual(overCapParry.Snapshot().combatPresentation.parrySuccessRemainingSeconds,
                      horde::gameplay::CombatTimeline::kParryPresentationSeconds) &&
          overCapParry.Snapshot().combatPresentation.parrySuccessRemainingSeconds > 0.0f,
          "an over-cap hitch frame must preserve feedback emitted during its catch-up batch");

    InputSnapshot pausedCatchUp = catchUpParryInput;
    pausedCatchUp.paused = true;
    catchUpParry.AdvanceFrame(pausedCatchUp, 1.0 / 60.0);
    check(!catchUpParry.Snapshot().combatPresentation.parrySuccessActive &&
          catchUpParry.Snapshot().heldItemKinematics.successJolt == 0.0f,
          "lifecycle pause must clear stale parry presentation from the published held-item pose");

    InputSnapshot pausedAtImpact = overCapParryInput;
    pausedAtImpact.paused = true;
    overCapParry.AdvanceFrame(pausedAtImpact, 0.0);
    check(!overCapParry.Snapshot().combatPresentation.parrySuccessActive &&
          overCapParry.Snapshot().heldItemKinematics.successJolt == 0.0f &&
          overCapParry.Snapshot().playerAnimation.reaction != CombatReaction::Parried,
          "pausing at the event presentation boundary must not revive the authoritative one-tick jolt");

    for (int frame = 0; frame < 10; ++frame)
    {
        parrySimulation.AdvanceFrame(parryInput, 1.0 / 60.0);
    }
    check(CountEvents(parrySimulation.Events(), GameplayEventType::PlayerParrySucceeded) == 1u,
          "re-reading one parry sequence must not repeat its semantic event");

    InputSnapshot spamParry = parryInput;
    spamParry.commands.parry = 3u;
    parrySimulation.AdvanceFrame(spamParry, 1.0 / 60.0);
    check(parrySimulation.Snapshot().lastConsumedParrySequence == 3u &&
          CountEvents(parrySimulation.Events(), GameplayEventType::PlayerParrySucceeded) == 1u,
          "unavailable parry commands must be consumed without delayed buffering");

    pairInput.commands.retry = 1u;
    skeletonPair.AdvanceFrame(pairInput, 1.0 / 60.0);
    check(skeletonPair.Snapshot().activeSkeletonCount == 2u &&
          !skeletonPair.Snapshot().openingEncounterComplete &&
          NearlyEqual(skeletonPair.Snapshot().skeletonEnemies[0].x, -0.75f) &&
          NearlyEqual(skeletonPair.Snapshot().skeletonEnemies[1].x, 0.75f),
          "encounter retry must restore both skeletons at their authored spawns");

    GameSimulation damageEvents;
    InputSnapshot damageInput;
    damageInput.hasAuthoritativePlayerPose = true;
    damageInput.authoritativePlayerX = 0.0f;
    damageInput.authoritativePlayerZ = -3.40f;
    damageInput.damageEnabled = true;
    for (int frame = 0;
         frame < 600 && damageEvents.Snapshot().playerVitals.phase == PlayerLifePhase::Alive;
         ++frame)
    {
        damageEvents.AdvanceFrame(damageInput, 1.0 / 60.0, static_cast<std::uint64_t>(frame + 1));
    }
    bool playerDamageEventsIdentifyTheirAttacker = true;
    for (const GameplayEvent& event : damageEvents.Events().Events())
    {
        if (event.type == GameplayEventType::PlayerDamaged ||
            event.type == GameplayEventType::PlayerKilled)
        {
            playerDamageEventsIdentifyTheirAttacker = playerDamageEventsIdentifyTheirAttacker &&
                (event.source == EntityId::SkeletonA || event.source == EntityId::SkeletonB) &&
                event.target == EntityId::Player;
        }
    }
    check(damageEvents.Snapshot().playerVitals.phase == PlayerLifePhase::Dying &&
          CountEvents(damageEvents.Events(), GameplayEventType::PlayerDamaged) == 2u &&
          CountEvents(damageEvents.Events(), GameplayEventType::PlayerKilled) == 1u &&
          playerDamageEventsIdentifyTheirAttacker,
          "two entity-aware nonfatal hits must emit PlayerDamaged while the lethal hit emits only PlayerKilled");

    // Reproduce Restart Route's actual ordering: the UI publishes reset and
    // immediately resumes before the owner gets to consume the mailbox.
    GameSimulation deadBeforeMenu = damageEvents;
    for (int frame = 0; frame < 60; ++frame)
        deadBeforeMenu.StepFixed(damageInput);
    check(deadBeforeMenu.Snapshot().playerVitals.phase == PlayerLifePhase::Dead &&
          deadBeforeMenu.Snapshot().playerVitals.vitality == 0,
          "menu reset regression begins with a genuinely combat-killed player");
    InputSnapshot menuWorldInput;
    menuWorldInput.paused = true;
    menuWorldInput.commands.attack = 4u;
    menuWorldInput.commands.parry = 2u;
    menuWorldInput.commands.dodge = 3u;
    menuWorldInput.commands.interact = 5u;
    menuWorldInput.commands.toggleHeldLightPose = 6u;
    menuWorldInput.commands.routeReset = 1u;
    InputMailbox menuMailbox;
    menuMailbox.Publish(menuWorldInput);
    InputSnapshot menuResume = menuWorldInput;
    menuResume.paused = false;
    menuMailbox.Publish(menuResume);
    const auto coalescedMenu = menuMailbox.ConsumeLatest();
    GameSimulation menuRestart = deadBeforeMenu;
    menuRestart.SynchronizePausedInput(coalescedMenu.snapshot, coalescedMenu.publicationSequence,
        PausedInputPolicy::PreserveWorldCommands);
    menuRestart.SynchronizePausedInput(coalescedMenu.snapshot, coalescedMenu.publicationSequence,
        PausedInputPolicy::PreserveWorldCommands);
    check(menuRestart.Snapshot().playerVitals.phase == PlayerLifePhase::Dead &&
          menuRestart.Snapshot().lastConsumedRouteResetSequence == 0u,
          "menu synchronization neither executes nor acknowledges the retained reset prematurely");
    menuRestart.StepFixed(coalescedMenu.snapshot, 0.0f, coalescedMenu.publicationSequence);
    const auto restarted = menuRestart.Snapshot();
    check(restarted.playerVitals.phase == PlayerLifePhase::Alive && restarted.playerVitals.vitality == 3 &&
          restarted.lastConsumedRouteResetSequence == 1u && restarted.retryGeneration == 0u &&
          NearlyEqual(restarted.playerX, kPlayerSpawn.x) && NearlyEqual(restarted.playerZ, kPlayerSpawn.z) &&
          restarted.lastConsumedAttackSequence == 4u && restarted.lastConsumedParrySequence == 2u &&
          restarted.lastConsumedDodgeSequence == 3u && restarted.lastConsumedInteractSequence == 5u &&
          restarted.lastConsumedToggleHeldLightPoseSequence == 6u && menuRestart.Events().Empty(),
          "reset then resume before owner consumption restores vitality once and discards all competing actions");
    menuRestart.AdvanceFrame(coalescedMenu.snapshot, 0.0, coalescedMenu.publicationSequence);
    check(menuRestart.Snapshot().lastConsumedRouteResetSequence == 1u &&
          menuRestart.Snapshot().tickIndex == restarted.tickIndex && menuRestart.Events().Empty(),
          "the same resumed publication cannot apply a reset twice or advance paused gameplay time");

    GameSimulation menuRetry = deadBeforeMenu;
    InputSnapshot retryMenu = menuWorldInput;
    retryMenu.commands.routeReset = 0u;
    retryMenu.commands.retry = 1u;
    menuRetry.SynchronizePausedInput(retryMenu, 301u, PausedInputPolicy::PreserveWorldCommands);
    menuRetry.AdvanceFrame(retryMenu, 0.0, 301u);
    check(menuRetry.Snapshot().playerVitals.phase == PlayerLifePhase::Alive &&
          menuRetry.Snapshot().playerVitals.vitality == 3 &&
          menuRetry.Snapshot().lastConsumedRetrySequence == 1u &&
          menuRetry.Snapshot().retryGeneration == 1u && menuRetry.Events().Empty(),
          "paused death-menu retry remains responsive without a gameplay tick or competing actions");
    menuRetry.SynchronizePausedInput(retryMenu, 301u, PausedInputPolicy::PreserveWorldCommands);
    menuRetry.AdvanceFrame(retryMenu, 1.0, 301u);
    check(menuRetry.Snapshot().retryGeneration == 1u && menuRetry.Snapshot().simulationTicksThisFrame == 0u &&
          menuRetry.Snapshot().playerVitals.vitality == 3 && menuRetry.Events().Empty(),
          "repeated menu synchronization never duplicates retry or enables paused damage");

    GameSimulation stopOverridesMenu = deadBeforeMenu;
    stopOverridesMenu.SynchronizePausedInput(menuWorldInput, 401u, PausedInputPolicy::PreserveWorldCommands);
    stopOverridesMenu.SynchronizePausedInput(menuWorldInput, 402u); // New actual stop: default discards all.
    stopOverridesMenu.AdvanceFrame(menuResume, 0.0, 403u);
    check(stopOverridesMenu.Snapshot().playerVitals.phase == PlayerLifePhase::Dead &&
          stopOverridesMenu.Snapshot().lastConsumedRouteResetSequence == 1u &&
          stopOverridesMenu.Snapshot().retryGeneration == 0u && stopOverridesMenu.Events().Empty(),
          "a newer genuine lifecycle stop discards an earlier menu reset without reviving the player");

    GameSimulation menuAfterStop = deadBeforeMenu;
    menuAfterStop.SynchronizePausedInput(menuWorldInput, 501u); // Retained lifecycle discard floor.
    InputSnapshot newerMenuReset = menuWorldInput;
    newerMenuReset.commands.routeReset = 2u;
    menuAfterStop.SynchronizePausedInput(newerMenuReset, 502u, PausedInputPolicy::PreserveWorldCommands);
    menuAfterStop.AdvanceFrame(newerMenuReset, 0.0, 502u);
    check(menuAfterStop.Snapshot().playerVitals.phase == PlayerLifePhase::Alive &&
          menuAfterStop.Snapshot().playerVitals.vitality == 3 &&
          menuAfterStop.Snapshot().lastConsumedRouteResetSequence == 2u && menuAfterStop.Events().Empty(),
          "a deliberate newer menu reset survives after older stopped edges were discarded");

    GameSimulation menuTwoWorldCommands = deadBeforeMenu;
    InputSnapshot bothWorld = menuWorldInput;
    bothWorld.commands.retry = 1u;
    menuTwoWorldCommands.SynchronizePausedInput(bothWorld, 601u, PausedInputPolicy::PreserveWorldCommands);
    menuTwoWorldCommands.AdvanceFrame(bothWorld, 0.0, 601u);
    check(menuTwoWorldCommands.Snapshot().lastConsumedRouteResetSequence == 1u &&
          menuTwoWorldCommands.Snapshot().lastConsumedRetrySequence == 0u,
          "preserved coalesced reset retains established priority over retry");
    menuTwoWorldCommands.SynchronizePausedInput(bothWorld, 602u, PausedInputPolicy::PreserveWorldCommands);
    menuTwoWorldCommands.AdvanceFrame(bothWorld, 0.0, 602u);
    menuTwoWorldCommands.AdvanceFrame(bothWorld, 0.0, 602u);
    check(menuTwoWorldCommands.Snapshot().lastConsumedRouteResetSequence == 1u &&
          menuTwoWorldCommands.Snapshot().lastConsumedRetrySequence == 1u &&
          menuTwoWorldCommands.Snapshot().retryGeneration == 1u && menuTwoWorldCommands.Events().Empty(),
          "another menu barrier preserves already-ingested retry until it is applied exactly once");

    GameSimulation skeletonFeedback;
    InputSnapshot skeletonFeedbackInput;
    skeletonFeedbackInput.damageEnabled = false;
    bool sawEnemyFootstep = false;
    bool sawEnemyAttackStarted = false;
    bool skeletonFeedbackIdentityValid = true;
    std::uint64_t previousSkeletonFeedbackSequence = 0u;
    for (int frame = 0; frame < 900 && (!sawEnemyFootstep || !sawEnemyAttackStarted); ++frame)
    {
        skeletonFeedback.AdvanceFrame(
            skeletonFeedbackInput, 1.0 / 60.0, static_cast<std::uint64_t>(frame + 1));
        for (const GameplayEvent& event : skeletonFeedback.Events().Events())
        {
            skeletonFeedbackIdentityValid = skeletonFeedbackIdentityValid &&
                event.sequence > previousSkeletonFeedbackSequence;
            previousSkeletonFeedbackSequence = event.sequence;
            if (event.type == GameplayEventType::EnemyFootstep)
            {
                sawEnemyFootstep = true;
                skeletonFeedbackIdentityValid = skeletonFeedbackIdentityValid &&
                    (event.source == EntityId::SkeletonA || event.source == EntityId::SkeletonB) &&
                    event.target == EntityId::Invalid;
            }
            if (event.type == GameplayEventType::EnemyAttackStarted)
            {
                sawEnemyAttackStarted = true;
                skeletonFeedbackIdentityValid = skeletonFeedbackIdentityValid &&
                    (event.source == EntityId::SkeletonA || event.source == EntityId::SkeletonB) &&
                    event.target == EntityId::Player;
            }
        }
        skeletonFeedback.ClearEvents();
    }
    check(sawEnemyFootstep && sawEnemyAttackStarted && skeletonFeedbackIdentityValid,
          "walking and attacking skeletons must emit ordered entity-aware footstep and attack events");

    GameSimulation lichFeedback;
    check(lichFeedback.ApplyShowcaseCheckpoint(9),
          "mirror checkpoint import must initialise lich feedback coverage");
    InputSnapshot lichFeedbackInput;
    lichFeedbackInput.damageEnabled = false;
    bool sawLichCharge = false;
    bool sawLichImpact = false;
    bool lichFeedbackIdentityValid = true;
    std::uint64_t chargeSequence = 0u;
    std::uint64_t impactSequence = 0u;
    for (int frame = 0; frame < 900 && (!sawLichCharge || !sawLichImpact); ++frame)
    {
        lichFeedback.AdvanceFrame(
            lichFeedbackInput, 1.0 / 60.0, static_cast<std::uint64_t>(frame + 1));
        for (const GameplayEvent& event : lichFeedback.Events().Events())
        {
            if (event.type == GameplayEventType::LichChargeStarted)
            {
                sawLichCharge = true;
                chargeSequence = event.sequence;
                lichFeedbackIdentityValid = lichFeedbackIdentityValid &&
                    event.source == EntityId::Lich && event.target == EntityId::Player &&
                    event.intensity > 0.0f;
            }
            if (event.type == GameplayEventType::LichImpact)
            {
                sawLichImpact = true;
                impactSequence = event.sequence;
                lichFeedbackIdentityValid = lichFeedbackIdentityValid &&
                    event.source == EntityId::Lich && event.target == EntityId::Player;
            }
        }
        lichFeedback.ClearEvents();
    }
    check(sawLichCharge && sawLichImpact && chargeSequence < impactSequence &&
          lichFeedbackIdentityValid,
          "the lich must emit an ordered entity-aware charge then impact sequence");

    GameSimulation lichDefeatFeedback;
    check(lichDefeatFeedback.ApplyShowcaseCheckpoint(10),
          "lich checkpoint import must initialise defeat-event coverage");
    InputSnapshot lichDefeatInput;
    lichDefeatInput.damageEnabled = false;
    std::uint64_t lichAttackCommand = 0u;
    std::size_t lichHitEventCount = 0u;
    std::size_t lichDefeatedEventCount = 0u;
    std::size_t chestUnlockedEventCount = 0u;
    std::uint64_t lichDefeatedSequence = 0u;
    std::uint64_t chestUnlockedSequence = 0u;
    bool lichDefeatIdentityAndOrderValid = true;
    for (int frame = 0; frame < 1200 && lichDefeatFeedback.Snapshot().lich.health > 0; ++frame)
    {
        const auto& before = lichDefeatFeedback.Snapshot();
        lichDefeatInput.hasAuthoritativePlayerPose = true;
        lichDefeatInput.authoritativePlayerX = before.lich.x;
        lichDefeatInput.authoritativePlayerZ = before.lich.z;
        if (before.playerCombat.action == PlayerCombatAction::Idle &&
            before.lich.hitCooldownRemaining <= 0.00001f)
        {
            lichDefeatInput.commands.attack = ++lichAttackCommand;
        }
        lichDefeatFeedback.AdvanceFrame(
            lichDefeatInput, 1.0 / 60.0, static_cast<std::uint64_t>(frame + 1));
        GameplayEventType previousType = GameplayEventType::PlayerFootstep;
        bool hasPrevious = false;
        for (const GameplayEvent& event : lichDefeatFeedback.Events().Events())
        {
            if (event.type == GameplayEventType::EnemyHit && event.target == EntityId::Lich)
            {
                ++lichHitEventCount;
                lichDefeatIdentityAndOrderValid = lichDefeatIdentityAndOrderValid &&
                    event.source == EntityId::Player;
            }
            if (event.type == GameplayEventType::LichDefeated)
            {
                ++lichDefeatedEventCount;
                lichDefeatedSequence = event.sequence;
                lichDefeatIdentityAndOrderValid = lichDefeatIdentityAndOrderValid &&
                    hasPrevious && previousType == GameplayEventType::EnemyHit &&
                    event.source == EntityId::Player && event.target == EntityId::Lich;
            }
            if (event.type == GameplayEventType::ChestUnlocked)
            {
                ++chestUnlockedEventCount;
                chestUnlockedSequence = event.sequence;
                lichDefeatIdentityAndOrderValid = lichDefeatIdentityAndOrderValid &&
                    event.source == EntityId::Lich && event.target == EntityId::RewardChest;
            }
            previousType = event.type;
            hasPrevious = true;
        }
        lichDefeatFeedback.ClearEvents();
    }
    check(lichDefeatFeedback.Snapshot().chestReward.phase ==
              horde::gameplay::interactions::ChestRewardPhase::Locked &&
          lichDefeatFeedback.Snapshot().chestReward.unlockPending &&
          chestUnlockedEventCount == 0u,
          "the lethal tick must arm, but not emit, the two-second chest unlock");
    for (int frame = 0; frame < 118; ++frame)
    {
        lichDefeatFeedback.AdvanceFrame(lichDefeatInput, 1.0 / 60.0);
        for (const GameplayEvent& event : lichDefeatFeedback.Events().Events())
        {
            if (event.type == GameplayEventType::LichDefeated) ++lichDefeatedEventCount;
            if (event.type == GameplayEventType::ChestUnlocked) ++chestUnlockedEventCount;
        }
        lichDefeatFeedback.ClearEvents();
    }
    check(lichDefeatFeedback.Snapshot().chestReward.phase ==
              horde::gameplay::interactions::ChestRewardPhase::Locked &&
          lichDefeatFeedback.Snapshot().chestReward.unlockPending &&
          chestUnlockedEventCount == 0u,
          "the chest must remain locked through 1.983 fixed seconds after the lethal tick begins");
    lichDefeatFeedback.AdvanceFrame(lichDefeatInput, 1.0 / 60.0);
    for (const GameplayEvent& event : lichDefeatFeedback.Events().Events())
    {
        if (event.type == GameplayEventType::LichDefeated) ++lichDefeatedEventCount;
        if (event.type == GameplayEventType::ChestUnlocked)
        {
            ++chestUnlockedEventCount;
            chestUnlockedSequence = event.sequence;
            lichDefeatIdentityAndOrderValid = lichDefeatIdentityAndOrderValid &&
                event.source == EntityId::Lich &&
                event.target == EntityId::RewardChest;
        }
    }
    lichDefeatFeedback.ClearEvents();
    std::cout << "lich/chest delayed finale events hits/defeat/unlock="
              << lichHitEventCount << '/' << lichDefeatedEventCount << '/'
              << chestUnlockedEventCount << " phase="
              << static_cast<int>(lichDefeatFeedback.Snapshot().chestReward.phase)
              << " pending="
              << lichDefeatFeedback.Snapshot().chestReward.unlockPending
              << " time="
              << lichDefeatFeedback.Snapshot().chestReward.phaseTime
              << " sequence=" << lichDefeatedSequence << '/'
              << chestUnlockedSequence << '\n';
    check(lichDefeatFeedback.Snapshot().lich.health == 0 && lichHitEventCount == 3u &&
          lichDefeatedEventCount == 1u && chestUnlockedEventCount == 1u &&
          lichDefeatFeedback.Snapshot().chestReward.phase ==
              horde::gameplay::interactions::ChestRewardPhase::ClosedUnlocked &&
          lichDefeatIdentityAndOrderValid &&
          chestUnlockedSequence > lichDefeatedSequence,
          "three accepted hits must emit one lich defeat followed exactly two fixed seconds later by one chest unlock");

    GameSimulation retry;
    check(retry.ApplyShowcaseCheckpoint(9), "legacy combat fixture imports its exact authored state");
    InputSnapshot finaleInput;
    finaleInput.hasAuthoritativePlayerPose = true;
    finaleInput.authoritativePlayerX = -33.70f;
    finaleInput.authoritativePlayerZ = -15.20f;
    finaleInput.yawRadians = -1.57079632679f;
    finaleInput.damageEnabled = false;
    retry.AdvanceFrame(finaleInput, 1.0 / 60.0);
    check(retry.Snapshot().activeEnemyKind == EnemyKind::Lich &&
          retry.Snapshot().retryCheckpoint == 9,
          "entering the finale must select the persistent lich encounter and mirror retry");
    const std::uint32_t lichGeneration = retry.Snapshot().enemyRoster.encounters[1].resetGeneration;

    InputSnapshot lichHitInput = finaleInput;
    lichHitInput.authoritativePlayerX = retry.Snapshot().lich.x;
    lichHitInput.authoritativePlayerZ = retry.Snapshot().lich.z;
    lichHitInput.commands.attack = 1u;
    retry.AdvanceFrame(lichHitInput, 1.0 / 60.0);
    check(retry.Snapshot().lich.health == 3,
          "the lich must not take damage on the swing input edge");
    for (int frame = 0; frame < 20 && retry.Snapshot().lich.health == 3; ++frame)
    {
        retry.AdvanceFrame(lichHitInput, 1.0 / 60.0);
    }
    check(retry.Snapshot().lich.health == 2,
          "the live lich must take exactly one hit when the sword enters its active window");

    InputSnapshot outsideInput = lichHitInput;
    outsideInput.authoritativePlayerX = 50.0f;
    outsideInput.authoritativePlayerZ = 50.0f;
    retry.AdvanceFrame(outsideInput, 1.0 / 60.0);
    InputSnapshot returnInput = finaleInput;
    returnInput.commands.attack = 1u;
    retry.AdvanceFrame(returnInput, 1.0 / 60.0);
    check(retry.Snapshot().activeEnemyKind == EnemyKind::Lich &&
          retry.Snapshot().enemyRoster.encounters[1].resetGeneration == lichGeneration &&
          retry.Snapshot().lich.health == 2 &&
          retry.Snapshot().playerVitals.vitality == PlayerVitals::kMaxVitality,
          "leaving and re-entering an outside seam must not heal or reset a live selected encounter");

    finaleInput.commands.attack = 1u;
    finaleInput.commands.retry = 1u;
    retry.AdvanceFrame(finaleInput, 1.0 / 60.0);
    check(retry.Snapshot().lastConsumedRetrySequence == 1u &&
          retry.Snapshot().retryGeneration == 1u &&
          retry.Snapshot().activeEnemyKind == EnemyKind::Lich &&
          NearlyEqual(retry.Snapshot().playerX, kKeeperRetryPosition.x) &&
          NearlyEqual(retry.Snapshot().playerZ, kKeeperRetryPosition.z) &&
          retry.Snapshot().torchFailure.phase == TorchFailurePhase::Settled &&
          retry.Snapshot().lich.revealPhase == KeeperRevealPhase::RetryRecognition &&
          retry.Snapshot().playerVitals.vitality == PlayerVitals::kMaxVitality,
          "retry must restore the authored mirror player, torch failure, lich, and vitality state exactly once");
    retry.AdvanceFrame(finaleInput, 1.0 / 60.0);
    check(retry.Snapshot().retryGeneration == 1u,
          "re-reading the same retry sequence must not apply a second retry");

    InputSnapshot resetInput = finaleInput;
    resetInput.paused = true;
    resetInput.commands.routeReset = 1u;
    retry.AdvanceFrame(resetInput, 1.0);
    check(retry.Snapshot().lastConsumedRouteResetSequence == 1u &&
          retry.Snapshot().activeEnemyKind == EnemyKind::Skeleton &&
          retry.Snapshot().retryCheckpoint == 0 &&
          NearlyEqual(retry.Snapshot().playerX, kPlayerSpawn.x) &&
          NearlyEqual(retry.Snapshot().playerZ, kPlayerSpawn.z) &&
          NearlyEqual(retry.Snapshot().playerPitchRadians, 0.0f),
          "a paused route reset must restore the live opening pose with zero pitch once");

    GameSimulation deadRejectsParry;
    InputSnapshot lethalInput;
    lethalInput.hasAuthoritativePlayerPose = true;
    lethalInput.authoritativePlayerX = -0.75f;
    lethalInput.authoritativePlayerZ = -3.40f;
    lethalInput.damageEnabled = true;
    for (int frame = 0; frame < 700 && deadRejectsParry.Snapshot().playerAlive; ++frame)
    {
        deadRejectsParry.AdvanceFrame(lethalInput, 1.0 / 60.0);
    }
    lethalInput.commands.parry = 1u;
    deadRejectsParry.AdvanceFrame(lethalInput, 1.0 / 60.0);
    check(deadRejectsParry.Snapshot().lastConsumedParrySequence == 1u &&
          deadRejectsParry.Snapshot().playerCombat.action == PlayerCombatAction::Idle,
          "death-state parry input must be consumed without starting or buffering an action");

    GameSimulation pausedRejectsParry;
    InputSnapshot pausedParryInput;
    pausedParryInput.paused = true;
    pausedParryInput.commands.parry = 1u;
    pausedRejectsParry.StepFixed(pausedParryInput);
    pausedParryInput.paused = false;
    pausedRejectsParry.StepFixed(pausedParryInput);
    check(pausedRejectsParry.Snapshot().lastConsumedParrySequence == 1u &&
          pausedRejectsParry.Snapshot().playerCombat.action == PlayerCombatAction::Idle,
          "paused parry input must be consumed without starting after resume");

    GameSimulation directionalDodge;
    InputSnapshot dodgeInput;
    dodgeInput.damageEnabled = false;
    dodgeInput.moveForward = 1.0f;
    dodgeInput.commands.dodge = 1u;
    directionalDodge.StepFixed(dodgeInput);
    dodgeInput.moveForward = 0.0f;
    for (int tick = 1; tick < 12; ++tick)
    {
        directionalDodge.StepFixed(dodgeInput);
    }
    const float forwardDodgeDistance = std::hypot(
        directionalDodge.Snapshot().playerX - kPlayerSpawn.x,
        directionalDodge.Snapshot().playerZ - kPlayerSpawn.z);
    check(directionalDodge.Snapshot().lastConsumedDodgeSequence == 1u &&
          forwardDodgeDistance > 0.82f && forwardDodgeDistance < 0.98f &&
          directionalDodge.Snapshot().playerZ < kPlayerSpawn.z,
          "one dodge command must latch the left-stick direction and travel a bounded distance");
    const float settledDodgeZ = directionalDodge.Snapshot().playerZ;
    for (int tick = 0; tick < 12; ++tick)
    {
        directionalDodge.StepFixed(dodgeInput);
    }
    check(NearlyEqual(directionalDodge.Snapshot().playerZ, settledDodgeZ) &&
          directionalDodge.Snapshot().lastConsumedDodgeSequence == 1u,
          "re-reading one dodge sequence must not repeat movement");

    GameSimulation diagonalDodge;
    InputSnapshot diagonalDodgeInput;
    diagonalDodgeInput.damageEnabled = false;
    diagonalDodgeInput.moveForward = 1.0f;
    diagonalDodgeInput.moveStrafe = 1.0f;
    diagonalDodgeInput.commands.dodge = 1u;
    diagonalDodge.StepFixed(diagonalDodgeInput);
    diagonalDodgeInput.moveForward = 0.0f;
    diagonalDodgeInput.moveStrafe = 0.0f;
    for (int tick = 1; tick < 12; ++tick)
    {
        diagonalDodge.StepFixed(diagonalDodgeInput);
    }
    const float diagonalDistance = std::hypot(
        diagonalDodge.Snapshot().playerX - kPlayerSpawn.x,
        diagonalDodge.Snapshot().playerZ - kPlayerSpawn.z);
    check(diagonalDistance > 0.82f && diagonalDistance < 0.98f,
          "diagonal left-stick dodge direction must be normalized");

    GameSimulation neutralDodge;
    InputSnapshot neutralDodgeInput;
    neutralDodgeInput.damageEnabled = false;
    neutralDodgeInput.yawRadians = 1.57079632679f;
    neutralDodgeInput.commands.dodge = 1u;
    for (int tick = 0; tick < 12; ++tick)
    {
        neutralDodge.StepFixed(neutralDodgeInput);
    }
    check(neutralDodge.Snapshot().playerX > kPlayerSpawn.x + 0.82f &&
          std::abs(neutralDodge.Snapshot().playerZ - kPlayerSpawn.z) < 0.04f,
          "neutral-stick dodge must fall back to current facing");

    GameSimulation collisionDodge(GameSimulationConfig{.playerStartX = 1.65f,
                                                        .playerStartZ = 1.85f,
                                                        .playerStartYawRadians = 0.0f});
    InputSnapshot collisionDodgeInput;
    collisionDodgeInput.damageEnabled = false;
    collisionDodgeInput.moveStrafe = 1.0f;
    collisionDodgeInput.commands.dodge = 1u;
    for (int tick = 0; tick < 12; ++tick)
    {
        collisionDodge.StepFixed(collisionDodgeInput);
        collisionDodgeInput.moveStrafe = 0.0f;
    }
    check(IsShowcasePlayerPositionWalkable(collisionDodge.Snapshot().playerX,
                                           collisionDodge.Snapshot().playerZ) &&
          collisionDodge.Snapshot().playerX < 1.85f,
          "dodge displacement must remain inside the shared corridor collision route");

    GameSimulation pausedRejectsDodge;
    InputSnapshot pausedDodgeInput;
    pausedDodgeInput.paused = true;
    pausedDodgeInput.commands.dodge = 1u;
    pausedRejectsDodge.StepFixed(pausedDodgeInput);
    pausedDodgeInput.paused = false;
    pausedRejectsDodge.StepFixed(pausedDodgeInput);
    check(pausedRejectsDodge.Snapshot().lastConsumedDodgeSequence == 1u &&
          !pausedRejectsDodge.Snapshot().dodgeActive &&
          NearlyEqual(pausedRejectsDodge.Snapshot().playerX, kPlayerSpawn.x) &&
          NearlyEqual(pausedRejectsDodge.Snapshot().playerZ, kPlayerSpawn.z),
          "paused dodge input must be consumed without buffering movement after resume");

    GameSimulation torchDrenchFeedback;
    InputSnapshot drenchedTorchInput;
    drenchedTorchInput.hasAuthoritativePlayerPose = true;
    drenchedTorchInput.authoritativePlayerX = -2.10f;
    drenchedTorchInput.authoritativePlayerZ = -15.20f;
    drenchedTorchInput.damageEnabled = false;
    torchDrenchFeedback.StepFixed(drenchedTorchInput);
    check(CountEvents(torchDrenchFeedback.Events(),
                      GameplayEventType::TorchExtinguished) == 1u &&
              torchDrenchFeedback.Snapshot().torchFailure.triggered,
          "entering roof water must emit one shared positional torch-extinguish cue");
    bool extinguishUsesTriggerPosition = false;
    for (const GameplayEvent& event : torchDrenchFeedback.Events().Events())
    {
        if (event.type == GameplayEventType::TorchExtinguished)
        {
            extinguishUsesTriggerPosition =
                NearlyEqual(event.worldX, -2.10f) &&
                NearlyEqual(event.worldZ, -15.20f) &&
                NearlyEqual(event.listenerX, -2.10f) &&
                NearlyEqual(event.listenerZ, -15.20f);
        }
    }
    check(extinguishUsesTriggerPosition,
          "torch-extinguish audio must retain exact event-time source/listener state");
    torchDrenchFeedback.ClearEvents();
    for (int tick = 0; tick < 120; ++tick)
    {
        torchDrenchFeedback.StepFixed(drenchedTorchInput);
    }
    check(CountEvents(torchDrenchFeedback.Events(),
                      GameplayEventType::TorchExtinguished) == 0u,
          "guttering, drop, settle, and repeated polling must not duplicate the extinguish cue");
    drenchedTorchInput.paused = true;
    torchDrenchFeedback.StepFixed(drenchedTorchInput);
    drenchedTorchInput.paused = false;
    torchDrenchFeedback.StepFixed(drenchedTorchInput);
    check(CountEvents(torchDrenchFeedback.Events(),
                      GameplayEventType::TorchExtinguished) == 0u,
          "pause/resume must not replay the already-consumed extinguish cue");

    GameSimulation resetParity;
    check(resetParity.ApplyShowcaseCheckpoint(0) &&
          NearlyEqual(resetParity.Snapshot().playerPitchRadians, -0.05f) &&
          resetParity.Snapshot().skeletonEnemyCount == 1u &&
          resetParity.Snapshot().activeSkeletonCount == 1u &&
          NearlyEqual(resetParity.Snapshot().skeletonEnemies[0].x, 0.0f) &&
          NearlyEqual(resetParity.Snapshot().skeletonEnemies[0].z, -4.65f) &&
          resetParity.Snapshot().swordCombat.enemyAnimation == EnemyAnimation::Walking &&
          NearlyEqual(resetParity.Snapshot().swordCombat.enemyAnimationTime, 0.0f) &&
          resetParity.Snapshot().tickIndex == 0u &&
          resetParity.Events().Empty(),
          "exact checkpoint 0 import must retain capture pitch and zero-time walking renderer state without a tick or event");
    bool historicalCheckpointsRemainSingle = true;
    for (std::int32_t checkpointId = 0; checkpointId < 12; ++checkpointId)
    {
        GameSimulation historicalCapture;
        historicalCheckpointsRemainSingle = historicalCheckpointsRemainSingle &&
            historicalCapture.ApplyShowcaseCheckpoint(checkpointId) &&
            historicalCapture.Snapshot().skeletonEnemyCount == 1u &&
            historicalCapture.Snapshot().activeSkeletonCount == 1u &&
            NearlyEqual(historicalCapture.Snapshot().skeletonEnemies[0].x, 0.0f) &&
            NearlyEqual(historicalCapture.Snapshot().skeletonEnemies[0].z, -4.65f);
    }
    check(historicalCheckpointsRemainSingle,
          "all twelve historical authored checkpoints must retain the original one-skeleton capture state");
    GameSimulation twoEnemyCapture;
    check(twoEnemyCapture.ApplyShowcaseCheckpoint(12) &&
          twoEnemyCapture.Snapshot().activeEnemyKind == EnemyKind::Skeleton &&
          twoEnemyCapture.Snapshot().skeletonEnemyCount == 2u &&
          twoEnemyCapture.Snapshot().activeSkeletonCount == 2u &&
          NearlyEqual(twoEnemyCapture.Snapshot().skeletonEnemies[0].x, -0.75f) &&
          NearlyEqual(twoEnemyCapture.Snapshot().skeletonEnemies[1].x, 0.75f) &&
          twoEnemyCapture.Snapshot().tickIndex == 0u &&
          twoEnemyCapture.Events().Empty(),
          "two-enemy-combat checkpoint import must retain the exact fresh bounded pair without a tick or event");
    resetParity.ResetRoute();
    check(resetParity.Snapshot().skeletonEnemyCount == 2u &&
          resetParity.Snapshot().activeSkeletonCount == 2u &&
          NearlyEqual(resetParity.Snapshot().skeletonEnemies[0].x, -0.75f) &&
          NearlyEqual(resetParity.Snapshot().skeletonEnemies[1].x, 0.75f) &&
          NearlyEqual(resetParity.Snapshot().playerYawRadians, 0.0f) &&
          NearlyEqual(resetParity.Snapshot().playerPitchRadians, 0.0f),
          "live ResetRoute must restore the pair and override checkpoint pose with configured yaw and pitch");

    auto waterfallConfig = ProductionGameSimulationConfig();
    waterfallConfig.swordStartsStowed = true;
    waterfallConfig.waterfallSkeletonEncounter = true;
    auto waterfallEncounter = std::make_unique<GameSimulation>(waterfallConfig);
    const auto& stagedPair = waterfallEncounter->Snapshot().skeletonEnemies;
    check(ProductionGameSimulationConfig().swordStartsStowed == false &&
          ProductionGameSimulationConfig().waterfallSkeletonEncounter == false &&
          waterfallEncounter->Snapshot().skeletonEnemyCount == 2u &&
          waterfallEncounter->Snapshot().activeSkeletonCount == 2u &&
          stagedPair[0].id == EntityId::SkeletonA &&
          stagedPair[1].id == EntityId::SkeletonB &&
          stagedPair[0].health == 1 && stagedPair[1].health == 1 &&
          NearlyEqual(stagedPair[0].x, kWaterfallSkeletonPairCenter.x) &&
          NearlyEqual(stagedPair[1].x, kWaterfallSkeletonPairCenter.x) &&
          NearlyEqual(stagedPair[0].z, kWaterfallSkeletonPairCenter.z - 0.75f) &&
          NearlyEqual(stagedPair[1].z, kWaterfallSkeletonPairCenter.z + 0.75f) &&
          NearlyEqual(stagedPair[0].facingRadians, 1.57079632679f) &&
          NearlyEqual(stagedPair[1].facingRadians, 1.57079632679f) &&
          NearlyEqual(stagedPair[0].animationTime, 0.0f) &&
          NearlyEqual(stagedPair[1].animationTime, 0.65f) &&
          waterfallEncounter->Snapshot().heldItems[1].parentMode ==
              items::HeldItemParentMode::BodyStow &&
          NearlyEqual(waterfallEncounter->Snapshot().heldItems[1].visualStowBlend, 1.0f) &&
          NearlyEqual(waterfallEncounter->Snapshot().heldItemKinematics.swordStowBlend,
                      waterfallEncounter->Snapshot().heldItems[1].visualStowBlend),
          "opt-in Waterfall encounter stages the same two stable skeleton IDs west of the wetline while production waits for rendered BodyStow support");

    auto waterfallReset = std::make_unique<GameSimulation>(waterfallConfig);
    waterfallReset->ResetRoute();
    const auto& resetWaterfallGuards = waterfallReset->Snapshot().skeletonEnemies;
    check(waterfallReset->Snapshot().skeletonEnemyCount == 2u &&
          resetWaterfallGuards[0].id == EntityId::SkeletonA &&
          resetWaterfallGuards[1].id == EntityId::SkeletonB &&
          resetWaterfallGuards[0].health == 1 && resetWaterfallGuards[1].health == 1 &&
          NearlyEqual(resetWaterfallGuards[0].x, kWaterfallSkeletonPairCenter.x) &&
          NearlyEqual(resetWaterfallGuards[1].x, kWaterfallSkeletonPairCenter.x) &&
          NearlyEqual(resetWaterfallGuards[0].z, kWaterfallSkeletonPairCenter.z - 0.75f) &&
          NearlyEqual(resetWaterfallGuards[1].z, kWaterfallSkeletonPairCenter.z + 0.75f) &&
          NearlyEqual(resetWaterfallGuards[0].facingRadians, 1.57079632679f) &&
          NearlyEqual(resetWaterfallGuards[1].facingRadians, 1.57079632679f),
          "Waterfall route reset restores the same two IDs and east-facing lateral guard layout");

    auto waterfallWalkPhase = std::make_unique<GameSimulation>(waterfallConfig);
    InputSnapshot waterfallWalkInput;
    waterfallWalkInput.hasAuthoritativePlayerPose = true;
    waterfallWalkInput.authoritativePlayerX = -2.0f;
    waterfallWalkInput.authoritativePlayerZ = kWaterfallSkeletonPairCenter.z;
    waterfallWalkInput.damageEnabled = false;
    for (int tick = 0; tick < 6; ++tick)
    {
        waterfallWalkPhase->StepFixed(waterfallWalkInput);
    }
    waterfallWalkInput.authoritativePlayerX = -3.3f;
    waterfallWalkInput.authoritativePlayerZ = kWaterfallSkeletonPairCenter.z;
    waterfallWalkPhase->StepFixed(waterfallWalkInput);
    const auto& walkingGuards = waterfallWalkPhase->Snapshot().skeletonEnemies;
    check(IsWaterfallSkeletonArena(waterfallWalkInput.authoritativePlayerX,
                                   waterfallWalkInput.authoritativePlayerZ) &&
          walkingGuards[0].animation == EnemyAnimation::Walking &&
          walkingGuards[1].animation == EnemyAnimation::Walking &&
          walkingGuards[0].action == EnemyCombatAction::Locomotion &&
          walkingGuards[1].action == EnemyCombatAction::Locomotion &&
          NearlyEqual(walkingGuards[1].animationTime - walkingGuards[0].animationTime,
                      0.65f),
          "Waterfall guards resume their shared-authority approach walk out of phase after waiting outside the arena");
    waterfallWalkInput.authoritativePlayerX = -2.0f;
    for (int tick = 0; tick < 6; ++tick)
    {
        waterfallWalkPhase->StepFixed(waterfallWalkInput);
    }
    waterfallWalkInput.authoritativePlayerX = -3.3f;
    waterfallWalkPhase->StepFixed(waterfallWalkInput);
    const auto& reenteredGuards = waterfallWalkPhase->Snapshot().skeletonEnemies;
    check(reenteredGuards[0].animation == EnemyAnimation::Walking &&
          reenteredGuards[1].animation == EnemyAnimation::Walking &&
          NearlyEqual(reenteredGuards[1].animationTime - reenteredGuards[0].animationTime,
                      0.65f),
          "Waterfall guards reapply their selected gait phase after leaving and re-entering the room");
    waterfallWalkPhase->ResetRoute();
    waterfallWalkInput.authoritativePlayerX = -2.0f;
    for (int tick = 0; tick < 6; ++tick)
    {
        waterfallWalkPhase->StepFixed(waterfallWalkInput);
    }
    waterfallWalkInput.authoritativePlayerX = -3.3f;
    waterfallWalkPhase->StepFixed(waterfallWalkInput);
    const auto& resetWalkingGuards = waterfallWalkPhase->Snapshot().skeletonEnemies;
    check(resetWalkingGuards[0].animation == EnemyAnimation::Walking &&
          resetWalkingGuards[1].animation == EnemyAnimation::Walking &&
          NearlyEqual(resetWalkingGuards[1].animationTime - resetWalkingGuards[0].animationTime,
                      0.65f),
          "Waterfall route reset restores both authored walk phases after an outside-arena wait");

    InputSnapshot waterfallInput;
    waterfallInput.hasAuthoritativePlayerPose = true;
    waterfallInput.authoritativePlayerX = -2.0f;
    waterfallInput.authoritativePlayerZ = kWaterfallSkeletonPairCenter.z;
    waterfallInput.damageEnabled = false;
    check(!IsWaterfallSkeletonArena(waterfallInput.authoritativePlayerX,
                                    waterfallInput.authoritativePlayerZ) &&
          !IsRouteAudioObstructed(waterfallInput.authoritativePlayerX,
                                  waterfallInput.authoritativePlayerZ,
                                  kWaterfallSkeletonPairCenter.x,
                                  kWaterfallSkeletonPairCenter.z),
          "Waterfall warning test point is an unobstructed east-side approach outside the aggro circle");
    waterfallEncounter->StepFixed(waterfallInput);
    const std::span<const GameplayEvent> warningEvents =
        waterfallEncounter->Events().Events();
    const auto warningEvent = std::find_if(
        warningEvents.begin(), warningEvents.end(),
        [](const GameplayEvent& event)
        {
            return event.type == GameplayEventType::SkeletonEncounterWarning;
        });
    check(waterfallEncounter->Snapshot().playerX > -2.5f &&
          waterfallEncounter->Snapshot().heldItems[1].transition.active &&
          waterfallEncounter->Snapshot().automaticSwordDrawBlocksDefense &&
          warningEvent != warningEvents.end() &&
          warningEvent->source == EntityId::SkeletonA &&
          warningEvent->target == EntityId::Player && warningEvent->tickIndex > 0u &&
          CountEvents(waterfallEncounter->Events(), GameplayEventType::SkeletonEncounterWarning) == 1u &&
          CountEvents(waterfallEncounter->Events(), GameplayEventType::PlayerSwordDrawStarted) == 1u &&
          CountEvents(waterfallEncounter->Events(), GameplayEventType::PlayerSwing) == 0u,
          "clear LOS emits a skeleton warning and starts the draw before the wetline without stowed contact");

    waterfallInput.authoritativePlayerX = kWaterfallSkeletonPairCenter.x;
    waterfallInput.commands.attack = 1u;
    waterfallEncounter->StepFixed(waterfallInput);
    waterfallInput.commands.attack = 2u;
    waterfallEncounter->StepFixed(waterfallInput);
    waterfallInput.commands.attack = 3u;
    waterfallEncounter->StepFixed(waterfallInput);
    check(CountEvents(waterfallEncounter->Events(), GameplayEventType::PlayerSwing) == 0u &&
          waterfallEncounter->Snapshot().lastConsumedAttackSequence == 3u,
          "drawing permits only one queued attack and consumes extra attack edges without early contact");
    for (int tick = 0; tick < 24; ++tick)
    {
        waterfallEncounter->StepFixed(waterfallInput);
    }
    check(waterfallEncounter->Snapshot().heldItems[1].parentMode ==
              items::HeldItemParentMode::HandSocket &&
          !waterfallEncounter->Snapshot().heldItems[1].transition.active &&
          !waterfallEncounter->Snapshot().automaticSwordDrawBlocksDefense &&
          NearlyEqual(waterfallEncounter->Snapshot().heldItems[1].visualStowBlend, 0.0f) &&
          NearlyEqual(waterfallEncounter->Snapshot().heldItemKinematics.swordStowBlend, 0.0f) &&
          CountEvents(waterfallEncounter->Events(), GameplayEventType::PlayerSwordAttachmentChanged) == 1u &&
          CountEvents(waterfallEncounter->Events(), GameplayEventType::PlayerSwing) == 1u,
          "one queued attack starts only after the fixed-tick hand attachment and draw completion");

    auto waterfallParry = std::make_unique<GameSimulation>(waterfallConfig);
    InputSnapshot waterfallParryInput;
    waterfallParryInput.hasAuthoritativePlayerPose = true;
    waterfallParryInput.authoritativePlayerX = kWaterfallSkeletonPairCenter.x;
    waterfallParryInput.authoritativePlayerZ = kWaterfallSkeletonPairCenter.z;
    waterfallParryInput.damageEnabled = false;
    waterfallParryInput.commands.parry = 1u;
    waterfallParry->StepFixed(waterfallParryInput);
    for (int tick = 0; tick < 30; ++tick)
        waterfallParry->StepFixed(waterfallParryInput);
    check(waterfallParry->Snapshot().playerCombat.action == PlayerCombatAction::Idle &&
          CountEvents(waterfallParry->Events(), GameplayEventType::PlayerParrySucceeded) == 0u &&
          CountEvents(waterfallParry->Events(), GameplayEventType::PlayerSwordDrawStarted) == 1u,
          "a parry pressed while stowed is discarded rather than delayed into the drawn state");

    auto stowedCombatConfig = GameSimulationConfig{};
    stowedCombatConfig.swordStartsStowed = true;
    auto manualDrawAttack = std::make_unique<GameSimulation>(stowedCombatConfig);
    InputSnapshot manualDrawAttackInput;
    manualDrawAttackInput.damageEnabled = false;
    manualDrawAttackInput.commands.attack = 1u;
    manualDrawAttack->StepFixed(manualDrawAttackInput);
    check(manualDrawAttack->Snapshot().heldItems[1].transition.active &&
          !manualDrawAttack->Snapshot().automaticSwordDrawBlocksDefense &&
          CountEvents(manualDrawAttack->Events(), GameplayEventType::PlayerSwing) == 0u,
          "a manual attack starts a draw without early contact or forced-draw immunity");
    for (int tick = 0; tick < 30; ++tick)
        manualDrawAttack->StepFixed(manualDrawAttackInput);
    check(CountEvents(manualDrawAttack->Events(), GameplayEventType::PlayerSwordDrawStarted) == 1u &&
          CountEvents(manualDrawAttack->Events(), GameplayEventType::PlayerSwing) == 1u &&
          manualDrawAttack->Snapshot().heldItems[1].parentMode == items::HeldItemParentMode::HandSocket,
          "one manual attack edge draws then starts exactly one ordinary attack after ready");
    auto stowedAtContact = std::make_unique<GameSimulation>(stowedCombatConfig);
    InputSnapshot stowedContactInput;
    stowedContactInput.hasAuthoritativePlayerPose = true;
    stowedContactInput.authoritativePlayerX = -0.75f;
    stowedContactInput.authoritativePlayerZ = -3.20f;
    stowedContactInput.damageEnabled = false;
    for (int tick = 0; tick < 200; ++tick)
    {
        const auto& attacker = stowedAtContact->Snapshot().skeletonEnemies[0];
        if (attacker.action == EnemyCombatAction::AttackWindup &&
            attacker.actionTime >= 1.04f)
            break;
        stowedAtContact->StepFixed(stowedContactInput);
    }
    bool stowedAttackWindupAtEdge =
        stowedAtContact->Snapshot().skeletonEnemies[0].action ==
            EnemyCombatAction::AttackWindup &&
        stowedAtContact->Snapshot().skeletonEnemies[0].actionTime >= 1.04f;
    stowedContactInput.commands.attack = 1u;
    stowedContactInput.damageEnabled = true;
    if (stowedAttackWindupAtEdge)
        stowedAtContact->StepFixed(stowedContactInput);
    bool manualDrawContactPulseSeen =
        stowedAtContact->Snapshot().swordCombat.combatants[0].playerHitPulse;
    bool manualDrawDamagedAtContact =
        CountEvents(stowedAtContact->Events(), GameplayEventType::PlayerDamaged) > 0u;
    float manualDrawContactActionTime =
        stowedAtContact->Snapshot().swordCombat.combatants[0].actionTime;
    float manualDrawContactProgress =
        stowedAtContact->Snapshot().heldItems[1].transition.progress;
    for (int tick = 0; tick < 10 && stowedAttackWindupAtEdge &&
                        !manualDrawContactPulseSeen; ++tick)
    {
        stowedAtContact->StepFixed(stowedContactInput);
        const SimulationSnapshot& sample = stowedAtContact->Snapshot();
        manualDrawContactPulseSeen = sample.swordCombat.combatants[0].playerHitPulse;
        manualDrawDamagedAtContact =
            CountEvents(stowedAtContact->Events(), GameplayEventType::PlayerDamaged) > 0u;
        manualDrawContactActionTime = sample.swordCombat.combatants[0].actionTime;
        manualDrawContactProgress = sample.heldItems[1].transition.progress;
    }
    const bool manualDrawContactExpected = stowedAttackWindupAtEdge &&
        manualDrawContactPulseSeen &&
        stowedAtContact->Snapshot().heldItems[1].transition.active &&
        !stowedAtContact->Snapshot().automaticSwordDrawBlocksDefense &&
        manualDrawDamagedAtContact &&
        CountEvents(stowedAtContact->Events(), GameplayEventType::PlayerSwing) == 0u;
    if (!manualDrawContactExpected)
    {
        std::cerr << "manual draw contact diagnostic: windup=" << stowedAttackWindupAtEdge
                  << " pulse=" << manualDrawContactPulseSeen
                  << " actionTime=" << manualDrawContactActionTime
                  << " transitionActive=" << stowedAtContact->Snapshot().heldItems[1].transition.active
                  << " transitionProgress=" << manualDrawContactProgress
                  << " automaticDefenseLockout="
                  << stowedAtContact->Snapshot().automaticSwordDrawBlocksDefense
                  << " damaged=" << manualDrawDamagedAtContact
                  << " damageEvents=" << CountEvents(stowedAtContact->Events(),
                                                       GameplayEventType::PlayerDamaged)
                  << '\n';
    }
    check(manualDrawContactExpected,
          "a manual attack-triggered draw does not grant invulnerability during enemy contact");

    auto stableStowedCombat = std::make_unique<GameSimulation>(stowedCombatConfig);
    InputSnapshot stableStowedInput = stowedContactInput;
    stableStowedInput.commands.attack = 0u;
    bool stableStowTookDamage = false;
    for (int tick = 0; tick < 600 && !stableStowTookDamage; ++tick)
    {
        stableStowedCombat->StepFixed(stableStowedInput);
        stableStowTookDamage =
            CountEvents(stableStowedCombat->Events(), GameplayEventType::PlayerDamaged) > 0u;
    }
    check(stableStowTookDamage &&
          stableStowedCombat->Snapshot().heldItems[1].parentMode ==
              items::HeldItemParentMode::BodyStow &&
          !stableStowedCombat->Snapshot().heldItems[1].transition.active,
          "a stable BodyStowed sword does not grant damage immunity without an active forced draw");

    auto waterfallRetreat = std::make_unique<GameSimulation>(waterfallConfig);
    InputSnapshot waterfallRouteInput;
    waterfallRouteInput.hasAuthoritativePlayerPose = true;
    waterfallRouteInput.damageEnabled = false;
    const std::array<RoutePosition, 6> waterfallRoute{{
        kWaterfallSkeletonPairCenter,
        {-3.0f, -15.20f},
        {-4.15f, -15.20f},
        {-6.55f, -15.20f},
        {-5.50f, -13.25f},
        {-2.0f, -15.20f},
    }};
    bool waterfallPairStayedInRoom = true;
    bool waterfallRetreatKeptAggro = true;
    for (std::size_t routeIndex = 0; routeIndex < waterfallRoute.size(); ++routeIndex)
    {
        const RoutePosition& position = waterfallRoute[routeIndex];
        waterfallRouteInput.authoritativePlayerX = position.x;
        waterfallRouteInput.authoritativePlayerZ = position.z;
        for (int tick = 0; tick < 18; ++tick)
            waterfallRetreat->StepFixed(waterfallRouteInput);
        if (routeIndex == 1u)
        {
            waterfallRetreatKeptAggro =
                !IsWaterfallSkeletonArena(position.x, position.z) &&
                IsWaterfallSkeletonRoom(position.x, position.z) &&
                waterfallRetreat->Snapshot().swordCombat.attackerIndex >= 0 &&
                waterfallRetreat->Snapshot().swordCombat.combatants[
                    static_cast<std::size_t>(waterfallRetreat->Snapshot().swordCombat.attackerIndex)].action !=
                    EnemyCombatAction::Locomotion;
        }
        for (std::size_t index = 0; index < waterfallRetreat->Snapshot().skeletonEnemyCount; ++index)
        {
            const auto& enemy = waterfallRetreat->Snapshot().skeletonEnemies[index];
            waterfallPairStayedInRoom = waterfallPairStayedInRoom &&
                enemy.x < -2.5f &&
                IsWaterfallSkeletonPositionWalkable(enemy.x, enemy.z) &&
                IsWaterfallSkeletonWalkableSweep({enemy.x, enemy.z},
                                                  kWaterfallSkeletonPairCenter);
        }
    }
    float waterfallBoundaryX = -2.75f;
    float waterfallBoundaryZ = -15.20f;
    ResolveWaterfallSkeletonEnemyCollision(
        waterfallBoundaryX, waterfallBoundaryZ, waterfallBoundaryX, waterfallBoundaryZ);
    float proposedAcrossWetlineX = -2.40f;
    float proposedAcrossWetlineZ = -15.20f;
    ResolveWaterfallSkeletonEnemyCollision(
        waterfallBoundaryX, waterfallBoundaryZ,
        proposedAcrossWetlineX, proposedAcrossWetlineZ);
    check(waterfallPairStayedInRoom && waterfallRetreatKeptAggro &&
          waterfallRetreat->Snapshot().skeletonEnemyCount == 2u &&
          proposedAcrossWetlineX <= -2.5f &&
          !IsWaterfallSkeletonPositionWalkable(-2.40f, -15.20f),
          "zigzag, retreat and re-entry preserve both reachable skeletons behind the wetline and block nav through the room boundary");

    GameSimulation mirrorCapture;
    check(mirrorCapture.ApplyShowcaseCheckpoint(9),
          "mirror checkpoint import must succeed");
    const float mirrorFacing = std::atan2(mirrorCapture.Snapshot().playerX - mirrorCapture.Snapshot().lich.x,
                                          mirrorCapture.Snapshot().playerZ - mirrorCapture.Snapshot().lich.z);
    check(mirrorCapture.Snapshot().activeEnemyKind == EnemyKind::Lich &&
          NearlyEqual(mirrorCapture.Snapshot().lich.facingRadians, mirrorFacing) &&
          mirrorCapture.Snapshot().tickIndex == 0u &&
          mirrorCapture.Events().Empty(),
          "mirror import must finalize zero-delta lich facing without a tick or event");

    GameSimulation finaleRoofCapture;
    check(finaleRoofCapture.ApplyShowcaseCheckpoint(11) &&
          finaleRoofCapture.Snapshot().lich.phase == LichPhase::Dead &&
          finaleRoofCapture.Snapshot().lich.deathAnimationComplete &&
          NearlyEqual(finaleRoofCapture.Snapshot().lich.finaleSkylightOpenProgress, 1.0f, 0.003f) &&
          finaleRoofCapture.Snapshot().tickIndex == 0u &&
          finaleRoofCapture.Events().Empty(),
          "finale-roof zero-delta finalization must preserve the authored dead/open-roof state without a tick or event");

    GameSimulation legacyMount;
    GameSimulationConfig anatomicalMountConfig{};
    anatomicalMountConfig.playerMountProfile = items::PlayerMountProfile::AnatomicalBody;
    GameSimulation anatomicalMount(anatomicalMountConfig);
    GameSimulation productionMount(ProductionGameSimulationConfig());
    check(productionMount.Snapshot().playerMountProfile == items::PlayerMountProfile::AnatomicalBody &&
          productionMount.Snapshot().heldItemKinematics.leftHandLocal ==
              anatomicalMount.Snapshot().heldItemKinematics.leftHandLocal &&
          productionMount.Snapshot().heldItemKinematics.rightHandLocal ==
              anatomicalMount.Snapshot().heldItemKinematics.rightHandLocal,
          "production application configuration must preserve the exact accepted anatomical targets");
    check(legacyMount.Snapshot().playerMountProfile == items::PlayerMountProfile::LegacyViewRelative &&
          NearlyEqual(legacyMount.Snapshot().heldItemKinematics.heldPropDepth,
                      GameSimulation(GameSimulationConfig{}).Snapshot().heldItemKinematics.heldPropDepth),
          "default game simulation retains the legacy view-relative mount profile and target");
    {
        // The entry ceiling now requires additional Rag clearance. Compare
        // authored mount heights at the actual open shaft, independently of
        // that safety response, while keeping the production-entry check above.
        GameSimulationConfig openMountConfig;
        openMountConfig.playerStartX = kSkylightChamberCenter.x;
        openMountConfig.playerStartZ = kSkylightChamberCenter.z + 0.70f;
        const auto openLegacy = std::make_unique<GameSimulation>(openMountConfig);
        openMountConfig.playerMountProfile = items::PlayerMountProfile::AnatomicalBody;
        const auto openAnatomical = std::make_unique<GameSimulation>(openMountConfig);
        std::cout << "open shaft actual X/Z=" << openLegacy->Snapshot().playerX << '/'
                  << openLegacy->Snapshot().playerZ << " mount torch lowering/left Y legacy="
                  << openLegacy->Snapshot().heldItemKinematics.torchOverheadLowering << '/'
                  << openLegacy->Snapshot().heldItemKinematics.leftHandLocal[1]
                  << " anatomical="
                  << openAnatomical->Snapshot().heldItemKinematics.torchOverheadLowering << '/'
                  << openAnatomical->Snapshot().heldItemKinematics.leftHandLocal[1] << '\n';
        check(openLegacy->Snapshot().heldItemKinematics.torchOverheadLowering == 0.0f &&
              openAnatomical->Snapshot().heldItemKinematics.torchOverheadLowering == 0.0f &&
              openAnatomical->Snapshot().playerMountProfile == items::PlayerMountProfile::AnatomicalBody &&
              NearlyEqual(openAnatomical->Snapshot().heldItemKinematics.leftHandLocal[1] -
                          openLegacy->Snapshot().heldItemKinematics.leftHandLocal[1], 0.10f) &&
              !NearlyEqual(openAnatomical->Snapshot().heldItemKinematics.leftShoulderLocal[0],
                           openLegacy->Snapshot().heldItemKinematics.leftShoulderLocal[0]),
              "configured anatomical profile publishes the authored hand height and shoulder frame in the open shaft");
    }
    for (const auto name : {"waterfall-guards-entry", "waterfall-guards-walk-early",
                            "waterfall-guards-walk-later"})
    {
        const auto* checkpoint = FindDevelopmentCheckpoint(name);
        auto guardPreview = std::make_unique<GameSimulation>(ProductionGameSimulationConfig());
        check(checkpoint != nullptr && StageDevelopmentCheckpointSimulation(*guardPreview, *checkpoint),
              "explicit guard-room development capture stages through shared simulation");
        if (checkpoint == nullptr) continue;
        const auto& preview = guardPreview->Snapshot();
        check(preview.activeEnemyKind == EnemyKind::Skeleton &&
              preview.enemyRoster.selectedEnemy == EnemyKind::Skeleton,
              "waterfall room keeps the relocated guards selected for rendering and combat");
        // The zero-delta authoritative pose import is one StepFixed boundary;
        // the requested count then advances that many actual timed fixed ticks.
        check(preview.tickIndex == checkpoint->waterfallGuardFixedTicks + 1u &&
              NearlyEqual(preview.playerX, checkpoint->cameraX) &&
              NearlyEqual(preview.playerZ, checkpoint->cameraZ) &&
              preview.playerAlive && preview.playerVitals.vitality == 3 &&
              preview.skeletonEnemyCount == 2u && preview.activeSkeletonCount == 2u &&
              preview.skeletonEnemies[0].id == EntityId::SkeletonA &&
              preview.skeletonEnemies[1].id == EntityId::SkeletonB &&
              preview.skeletonEnemies[0].health == 1 && preview.skeletonEnemies[1].health == 1,
              "guard preview retains exact bounded ticks, camera, identities and health");
        if (checkpoint->waterfallGuardFixedTicks == 0u)
            check(NearlyEqual(preview.skeletonEnemies[0].facingRadians, 1.57079632679f) &&
                  NearlyEqual(preview.skeletonEnemies[1].facingRadians, 1.57079632679f) &&
                  preview.skeletonEnemies[0].z < preview.playerZ &&
                  preview.skeletonEnemies[1].z > preview.playerZ,
                  "entry capture shows both guards facing the arriving player in lateral lanes");
        else
            check(preview.skeletonEnemies[0].animation == EnemyAnimation::Walking &&
                  preview.skeletonEnemies[1].animation == EnemyAnimation::Walking &&
                  preview.skeletonEnemies[0].x > kWaterfallSkeletonPairCenter.x &&
                  preview.skeletonEnemies[1].x > kWaterfallSkeletonPairCenter.x &&
                  NearlyEqual(preview.skeletonEnemies[1].animationTime -
                              preview.skeletonEnemies[0].animationTime, 0.65f),
                  "walk preview observes actual shared locomotion with distinct gait samples");
        InputSnapshot freeze;
        freeze.paused = true;
        const auto frozenTick = preview.tickIndex;
        guardPreview->AdvanceFrame(freeze, 0.0, preview.inputPublicationSequence + 1u);
        check(guardPreview->Snapshot().tickIndex == frozenTick,
              "capture freeze does not advance the staged guard ticks");
    }
    {
        auto guardRoomImport = std::make_unique<GameSimulation>(waterfallConfig);
        check(guardRoomImport->ApplyShowcaseCheckpoint(4) &&
              guardRoomImport->Snapshot().activeEnemyKind == EnemyKind::Skeleton &&
              guardRoomImport->Snapshot().enemyRoster.selectedEnemy == EnemyKind::Skeleton &&
              guardRoomImport->Snapshot().skeletonEnemyCount == 2u,
              "direct room checkpoint selects the relocated pair instead of the historical Keeper route gate");
        InputSnapshot leaveRoom;
        leaveRoom.hasAuthoritativePlayerPose = true;
        leaveRoom.authoritativePlayerX = -11.0f;
        leaveRoom.authoritativePlayerZ = -15.2f;
        leaveRoom.damageEnabled = false;
        guardRoomImport->StepFixed(leaveRoom, 0.0f);
        check(guardRoomImport->Snapshot().activeEnemyKind == EnemyKind::Lich,
              "leaving the guard room retains ordinary Keeper route selection");
    }
    {
        auto boundedPreview = std::make_unique<GameSimulation>(ProductionGameSimulationConfig());
        DevelopmentCheckpoint excessive = *FindDevelopmentCheckpoint("waterfall-guards-walk-later");
        excessive.waterfallGuardFixedTicks = 31u;
        check(!StageDevelopmentCheckpointSimulation(*boundedPreview, excessive) &&
              boundedPreview->Snapshot().tickIndex == 0u &&
              !ProductionGameSimulationConfig().waterfallSkeletonEncounter &&
              !ProductionGameSimulationConfig().swordStartsStowed,
              "guard capture rejects unbounded staging and retains production defaults");
    }
    for (const auto profile : {items::PlayerMountProfile::LegacyViewRelative,
                               items::PlayerMountProfile::AnatomicalBody})
    {
        GameSimulationConfig boundedLookConfig;
        boundedLookConfig.playerMountProfile = profile;
        boundedLookConfig.playerStartPitchRadians = -4.0f;
        GameSimulation boundedLook(boundedLookConfig);
        check(NearlyEqual(boundedLook.Snapshot().playerPitchRadians, -0.32f),
              "both player mounts use the established gameplay look limit at construction");
        InputSnapshot extremeLook;
        extremeLook.pitchRadians = -4.0f;
        boundedLook.StepFixed(extremeLook);
        check(NearlyEqual(boundedLook.Snapshot().playerPitchRadians, -0.32f),
              "an extreme diagnostic input cannot extend normal gameplay camera pitch");
        extremeLook.pitchRadians = 1.0f;
        boundedLook.StepFixed(extremeLook);
        check(NearlyEqual(boundedLook.Snapshot().playerPitchRadians, 0.28f),
              "both player mounts retain the normal upper look limit");
        boundedLook.ResetRoute();
        check(NearlyEqual(boundedLook.Snapshot().playerPitchRadians, -0.32f),
              "reset retains the normal gameplay pitch boundary");
    }
    check(anatomicalMount.ApplyShowcaseCheckpoint(0) &&
          anatomicalMount.Snapshot().playerMountProfile == items::PlayerMountProfile::AnatomicalBody,
          "authored checkpoint import preserves the configured player mount profile");
    anatomicalMount.RetryEncounter();
    check(anatomicalMount.Snapshot().playerMountProfile == items::PlayerMountProfile::AnatomicalBody,
          "encounter retry preserves the configured player mount profile");
    anatomicalMount.ResetRoute();
    check(anatomicalMount.Snapshot().playerMountProfile == items::PlayerMountProfile::AnatomicalBody,
          "route reset preserves the configured player mount profile");
    anatomicalMount.ImportRewardCheckpoint({}, {}, {});
    check(anatomicalMount.Snapshot().playerMountProfile == items::PlayerMountProfile::AnatomicalBody,
          "reward checkpoint import preserves the configured player mount profile");

    GameSimulation pausedRetry;
    InputSnapshot pausedFinale = finaleInput;
    pausedFinale.commands = {};
    pausedRetry.AdvanceFrame(pausedFinale, 1.0 / 60.0);
    InputSnapshot queuedSwing = pausedFinale;
    queuedSwing.commands.attack = 1u;
    pausedRetry.AdvanceFrame(queuedSwing, 1.0 / 60.0);
    check(!pausedRetry.Events().Empty(), "pre-retry semantic events must exist for stale-event coverage");
    InputSnapshot pausedRetryInput = pausedFinale;
    pausedRetryInput.paused = true;
    pausedRetryInput.commands.attack = 1u;
    pausedRetryInput.commands.retry = 1u;
    pausedRetry.AdvanceFrame(pausedRetryInput, 1.0);
    check(pausedRetry.Snapshot().lastConsumedRetrySequence == 1u &&
          pausedRetry.Snapshot().lastConsumedAttackSequence == 1u &&
          pausedRetry.Snapshot().retryGeneration == 1u &&
          pausedRetry.Events().Empty() &&
          NearlyEqual(pausedRetry.Snapshot().playerX, kKeeperRetryPosition.x) &&
          NearlyEqual(pausedRetry.Snapshot().playerZ, kKeeperRetryPosition.z),
          "paused retry must consume the competing attack exactly once, discard stale events, and clear catch-up time");

    for (const auto name : {"glass-transport", "glass-fire-transport",
                            "glass-tinted-transport", "glass-millimetre-closed",
                            "glass-edge-fresnel"})
    {
        GameSimulation glassCheckpoint(ProductionGameSimulationConfig());
        const auto* checkpoint = FindDevelopmentCheckpoint(name);
        check(checkpoint != nullptr &&
              StageDevelopmentCheckpointSimulation(glassCheckpoint, *checkpoint) &&
              glassCheckpoint.Snapshot().zone == ShowcaseZone::SkylightChamber,
              "glass captures use the staged camera's skylight zone, not the borrowed lighting preset's opening zone");
    }

    if (!passed)
    {
        return 1;
    }
    std::cout << "Shared simulation command, event, seam-persistence, and retry tests passed.\n";
    return 0;
}
