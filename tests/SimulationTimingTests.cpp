#include <cmath>
#include <algorithm>
#include <cstdint>
#include <iostream>

#include "gameplay/simulation/GameSimulation.h"
#include "gameplay/simulation/InputMailbox.h"

namespace
{

using horde::gameplay::simulation::GameSimulation;
using horde::gameplay::simulation::InputMailbox;
using horde::gameplay::simulation::InputSnapshot;
using horde::gameplay::simulation::CombatInputEdgeKind;
using horde::gameplay::simulation::CombatInputTimingDisposition;
using horde::gameplay::simulation::CombatInputTimingStatus;
using horde::gameplay::simulation::GameplayEventType;
using horde::gameplay::simulation::PublishedInput;
using horde::gameplay::simulation::RecordCombatInputEdge;
using horde::gameplay::simulation::SimulationSnapshot;

bool NearlyEqual(float left, float right, float epsilon = 0.00002f)
{
    return std::abs(left - right) <= epsilon;
}

SimulationSnapshot RunCadence(int frameRate, double seconds, float forward, float strafe)
{
    GameSimulation simulation;
    InputSnapshot input;
    input.moveForward = forward;
    input.moveStrafe = strafe;
    input.damageEnabled = false;
    const int frames = static_cast<int>(frameRate * seconds);
    for (int frame = 0; frame < frames; ++frame)
    {
        simulation.AdvanceFrame(input, 1.0 / static_cast<double>(frameRate), frame + 1u);
    }
    return simulation.Snapshot();
}

SimulationSnapshot RunCombatCadence(int frameRate, bool insertHitch)
{
    GameSimulation simulation;
    InputSnapshot input;
    input.hasAuthoritativePlayerPose = true;
    input.authoritativePlayerX = -0.75f;
    input.authoritativePlayerZ = -3.20f;
    input.yawRadians = 0.0f;
    input.damageEnabled = false;
    input.commands.attack = 1u;
    const int ordinaryFrames = frameRate - (insertHitch ? 6 : 0);
    for (int frame = 0; frame < ordinaryFrames; ++frame)
    {
        simulation.AdvanceFrame(input, 1.0 / static_cast<double>(frameRate), frame + 1u);
        if (insertHitch && frame == frameRate / 4)
        {
            simulation.AdvanceFrame(input, 0.100, frameRate + 1u);
        }
    }
    return simulation.Snapshot();
}

const horde::gameplay::simulation::CombatInputTimingTrace& LatestTimingTrace(
    const SimulationSnapshot& snapshot)
{
    const auto& timing = snapshot.combatInputTiming;
    const std::uint32_t index = (timing.nextTraceIndex +
        static_cast<std::uint32_t>(timing.traces.size()) - 1u) %
        static_cast<std::uint32_t>(timing.traces.size());
    return timing.traces[index];
}

bool CheckTimestampedCadence(const int frameRate, const auto& check)
{
    GameSimulation simulation;
    InputSnapshot input;
    const std::uint64_t anchorNs = 5'000'000'000u;
    const std::uint64_t intervalNs = 1'000'000'000u /
        static_cast<std::uint64_t>(frameRate);
    const double frameDelta = 1.0 / static_cast<double>(frameRate);
    simulation.AdvanceFrame(input, 0.0, 1u, anchorNs);
    ++input.commands.attack;
    RecordCombatInputEdge(input, CombatInputEdgeKind::Attack,
                          anchorNs + intervalNs / 2u);
    const std::uint32_t frameTicks = simulation.AdvanceFrame(
        input, frameDelta, 2u, anchorNs + intervalNs);
    const auto& scheduledTrace = LatestTimingTrace(simulation.Snapshot());
    const std::uint64_t expectedSteps = std::max<std::uint64_t>(
        1u, static_cast<std::uint64_t>(std::ceil(
            (frameDelta * 0.5 - 1.0e-12) /
            horde::gameplay::simulation::FixedStepRunner::kFixedDeltaSeconds)));
    check(scheduledTrace.targetTick == expectedSteps,
          "a timestamped attack must target the first tick boundary after its frame phase");
    if (frameTicks == 0u)
    {
        check(scheduledTrace.status == CombatInputTimingStatus::Scheduled,
              "120 Hz zero-tick edge must remain scheduled until the next fixed tick");
        simulation.AdvanceFrame(input, frameDelta, 3u,
                                anchorNs + intervalNs * 2u);
    }
    const auto& consumedTrace = LatestTimingTrace(simulation.Snapshot());
    check(simulation.Snapshot().lastConsumedAttackSequence == 1u &&
          consumedTrace.status == CombatInputTimingStatus::Consumed &&
          consumedTrace.actualTick == consumedTrace.targetTick,
          "timestamped attack must be consumed on its target tick at 15/30/60/120 Hz");
    return true;
}

void RunCombatInputTimingTests(const auto& check)
{
    CheckTimestampedCadence(15, check);
    CheckTimestampedCadence(30, check);
    CheckTimestampedCadence(60, check);
    CheckTimestampedCadence(120, check);

    const auto checkBatchEdge = [&check](const double frameDelta,
                                         const std::uint64_t rawSpanNs,
                                         const std::uint64_t edgeOffsetNs,
                                         const std::uint64_t expectedTarget,
                                         const char* message)
    {
        GameSimulation simulation;
        InputSnapshot input;
        constexpr std::uint64_t batchAnchor = 6'000'000'000u;
        simulation.AdvanceFrame(input, 0.0, 1u, batchAnchor);
        ++input.commands.attack;
        RecordCombatInputEdge(input, CombatInputEdgeKind::Attack,
                              batchAnchor + edgeOffsetNs);
        const std::uint32_t ticks = simulation.AdvanceFrame(
            input, frameDelta, 2u, batchAnchor + rawSpanNs);
        const std::uint32_t expectedBatchTicks = static_cast<std::uint32_t>(std::floor(
            (frameDelta + 1.0e-12) /
            horde::gameplay::simulation::FixedStepRunner::kFixedDeltaSeconds));
        check(ticks == expectedBatchTicks &&
              LatestTimingTrace(simulation.Snapshot()).targetTick == expectedTarget &&
              LatestTimingTrace(simulation.Snapshot()).actualTick == expectedTarget,
              message);
    };
    checkBatchEdge(1.0 / 30.0, 33'333'333u, 2'000'000u, 1u,
                   "an early edge in a 2-tick batch must run on tick one");
    checkBatchEdge(1.0 / 30.0, 33'333'333u, 30'000'000u, 2u,
                   "a late edge in a 2-tick batch must run on tick two");
    checkBatchEdge(0.100, 100'000'000u, 5'000'000u, 1u,
                   "an early edge in a 6-tick hitch batch must run on tick one");
    checkBatchEdge(0.100, 100'000'000u, 95'000'000u, 6u,
                   "a late edge in a 6-tick hitch batch must run on tick six");

    struct ParryBatchOutcome
    {
        std::uint64_t targetTick = 0u;
        std::uint64_t actualTick = 0u;
        std::uint64_t semanticEventSequence = 0u;
        std::uint64_t semanticEventTick = 0u;
        std::uint32_t parrySuccesses = 0u;
        std::uint32_t playerDamages = 0u;
        std::int32_t vitality = 0;
    };
    const auto runParryBatch = [](const std::uint64_t edgeOffsetNs)
    {
        GameSimulation simulation;
        InputSnapshot input;
        input.hasAuthoritativePlayerPose = true;
        input.authoritativePlayerX = -0.75f;
        input.authoritativePlayerZ = -3.20f;
        input.damageEnabled = true;
        constexpr std::uint64_t startNs = 9'000'000'000u;
        std::uint64_t ownerNs = startNs;
        std::uint64_t publication = 1u;
        simulation.AdvanceFrame(input, 0.0, publication, ownerNs);
        for (std::uint32_t frame = 0u; frame < 360u; ++frame)
        {
            const auto& attacker = simulation.Snapshot().skeletonEnemies[0];
            if (attacker.action == horde::gameplay::EnemyCombatAction::AttackWindup &&
                attacker.actionTime >= 1.0f && attacker.actionTime < 1.05f)
                break;
            ownerNs += 16'666'667u;
            ++publication;
            simulation.AdvanceFrame(input, 1.0 / 60.0, publication, ownerNs);
        }
        const auto& beforeBatch = simulation.Snapshot().skeletonEnemies[0];
        const bool startedNearContact =
            beforeBatch.action == horde::gameplay::EnemyCombatAction::AttackWindup &&
            beforeBatch.actionTime >= 1.0f && beforeBatch.actionTime < 1.05f;
        ++input.commands.parry;
        RecordCombatInputEdge(input, CombatInputEdgeKind::Parry,
                              ownerNs + edgeOffsetNs);
        ++publication;
        simulation.AdvanceFrame(input, 0.100, publication,
                                ownerNs + 100'000'000u);
        ownerNs += 100'000'000u;
        ++publication;
        simulation.AdvanceFrame(input, 1.0 / 60.0, publication,
                                ownerNs + 16'666'667u);

        ParryBatchOutcome outcome;
        const auto& timing = simulation.Snapshot().combatInputTiming;
        const std::uint32_t traceIndex = (timing.nextTraceIndex +
            static_cast<std::uint32_t>(timing.traces.size()) - 1u) %
            static_cast<std::uint32_t>(timing.traces.size());
        const auto& trace = timing.traces[traceIndex];
        outcome.targetTick = trace.targetTick;
        outcome.actualTick = trace.actualTick;
        outcome.semanticEventSequence = trace.semanticEventSequence;
        outcome.semanticEventTick = trace.semanticEventTick;
        outcome.vitality = simulation.Snapshot().playerVitals.vitality;
        for (const auto& event : simulation.Events().Events())
        {
            if (event.type == GameplayEventType::PlayerParrySucceeded)
                ++outcome.parrySuccesses;
            if (event.type == GameplayEventType::PlayerDamaged)
                ++outcome.playerDamages;
        }
        if (!startedNearContact)
            outcome.targetTick = 0u;
        return outcome;
    };
    const ParryBatchOutcome earlyParry = runParryBatch(10'000'000u);
    const ParryBatchOutcome lateParry = runParryBatch(90'000'000u);
    check(earlyParry.targetTick > 0u && earlyParry.targetTick + 5u ==
              lateParry.targetTick && earlyParry.actualTick == earlyParry.targetTick &&
          lateParry.actualTick == lateParry.targetTick &&
          earlyParry.parrySuccesses == 1u && earlyParry.semanticEventSequence > 0u &&
          earlyParry.semanticEventTick > earlyParry.actualTick &&
          earlyParry.playerDamages == 0u &&
          lateParry.parrySuccesses == 0u && lateParry.semanticEventSequence == 0u &&
          lateParry.playerDamages == 1u && lateParry.vitality < earlyParry.vitality,
          "a late parry edge in a 100 ms catch-up frame must not be backdated into the earlier contact ticks");

    GameSimulation exactHitch;
    InputSnapshot exactInput;
    constexpr std::uint64_t anchor = 8'000'000'000u;
    exactHitch.AdvanceFrame(exactInput, 0.0, 1u, anchor);
    ++exactInput.commands.dodge;
    exactInput.moveForward = 1.0f;
    RecordCombatInputEdge(exactInput, CombatInputEdgeKind::Dodge,
                          anchor + 90'000'000u);
    exactHitch.AdvanceFrame(exactInput, 0.100, 2u, anchor + 100'000'000u);
    const auto& exactTrace = LatestTimingTrace(exactHitch.Snapshot());
    check(exactHitch.Snapshot().tickIndex == 6u && exactTrace.targetTick == 6u &&
          exactTrace.actualTick == 6u && exactTrace.disposition ==
              CombatInputTimingDisposition::Timestamped,
          "an edge at 90 ms of an exact 100 ms frame must map to the sixth tick");

    GameSimulation overCapHitch;
    InputSnapshot overCapInput;
    overCapHitch.AdvanceFrame(overCapInput, 0.0, 1u, anchor);
    ++overCapInput.commands.parry;
    RecordCombatInputEdge(overCapInput, CombatInputEdgeKind::Parry,
                          anchor + 150'000'000u);
    overCapHitch.AdvanceFrame(overCapInput, 0.200, 2u, anchor + 200'000'000u);
    const auto& overCapTrace = LatestTimingTrace(overCapHitch.Snapshot());
    check(overCapHitch.Snapshot().tickIndex == 6u && overCapTrace.targetTick == 6u &&
          overCapTrace.actualTick == 6u && overCapTrace.disposition ==
              CombatInputTimingDisposition::HitchTailNewestTick,
          "an edge in the dropped hitch span must run at the newest accepted tick");

    GameSimulation missingMetadata;
    InputSnapshot missingInput;
    missingMetadata.AdvanceFrame(missingInput, 0.0, 1u, anchor);
    missingInput.commands.attack = 1000u;
    missingMetadata.AdvanceFrame(missingInput, 1.0 / 60.0, 2u,
                                 anchor + 16'666'667u);
    check(missingMetadata.Snapshot().lastConsumedAttackSequence == 1u &&
          missingMetadata.Snapshot().combatInputTiming.timestampFallbackCount == 1000u &&
          LatestTimingTrace(missingMetadata.Snapshot()).disposition ==
              CombatInputTimingDisposition::MissingMetadataFallback,
          "missing metadata must preserve large monotonic deltas without expanding a loop");

    GameSimulation overflowMetadata;
    InputSnapshot overflowInput;
    overflowMetadata.AdvanceFrame(overflowInput, 0.0, 1u, anchor);
    for (std::uint64_t sequence = 0u; sequence < 33u; ++sequence)
    {
        ++overflowInput.commands.attack;
        RecordCombatInputEdge(overflowInput, CombatInputEdgeKind::Attack,
                              anchor + sequence);
    }
    overflowMetadata.AdvanceFrame(overflowInput, 0.100, 2u,
                                  anchor + 100'000'000u);
    check(overflowInput.combatEdgeHistory.overwriteCount == 1u &&
          overflowMetadata.Snapshot().combatInputTiming.timestampFallbackCount == 33u &&
          overflowMetadata.Snapshot().lastConsumedAttackSequence == 6u,
          "history overflow must be visible and preserve counter edges via bounded fallback");

    GameSimulation wrappedHistory;
    InputSnapshot wrappedInput;
    for (std::uint64_t sequence = 0u; sequence < 33u; ++sequence)
    {
        ++wrappedInput.commands.attack;
        RecordCombatInputEdge(wrappedInput, CombatInputEdgeKind::Attack,
                              anchor + sequence);
    }
    wrappedHistory.SynchronizePausedInput(wrappedInput);
    wrappedHistory.AdvanceFrame(wrappedInput, 0.0, 2u, anchor);
    ++wrappedInput.commands.attack;
    RecordCombatInputEdge(wrappedInput, CombatInputEdgeKind::Attack,
                          anchor + 10'000'000u);
    wrappedHistory.AdvanceFrame(wrappedInput, 1.0 / 60.0, 3u,
                                anchor + 16'666'667u);
    check(wrappedHistory.Snapshot().combatInputTiming.inputHistoryOverwriteCount == 2u &&
          wrappedHistory.Snapshot().combatInputTiming.timestampFallbackCount == 0u &&
          LatestTimingTrace(wrappedHistory.Snapshot()).disposition ==
              CombatInputTimingDisposition::Timestamped,
          "ordinary ring wrap after consumed edges must not force missing-metadata fallback");

    GameSimulation paused;
    InputSnapshot pauseInput;
    paused.AdvanceFrame(pauseInput, 0.0, 1u, anchor);
    ++pauseInput.commands.attack;
    RecordCombatInputEdge(pauseInput, CombatInputEdgeKind::Attack,
                          anchor + 8'000'000u);
    paused.AdvanceFrame(pauseInput, 1.0 / 120.0, 2u,
                        anchor + 8'333'333u);
    pauseInput.paused = true;
    paused.AdvanceFrame(pauseInput, 1.0 / 60.0, 3u,
                        anchor + 25'000'000u);
    const auto& discardedTrace = LatestTimingTrace(paused.Snapshot());
    pauseInput.paused = false;
    paused.AdvanceFrame(pauseInput, 1.0 / 60.0, 4u,
                        anchor + 41'666'667u);
    check(discardedTrace.status == CombatInputTimingStatus::Discarded &&
          paused.Snapshot().lastConsumedAttackSequence == 1u &&
          paused.Snapshot().combatInputTiming.scheduledEdgeCount == 0u,
          "pause must discard a zero-tick scheduled edge without replay on resume");

    GameSimulation reset;
    InputSnapshot resetInput;
    reset.AdvanceFrame(resetInput, 0.0, 1u, anchor);
    ++resetInput.commands.attack;
    RecordCombatInputEdge(resetInput, CombatInputEdgeKind::Attack,
                          anchor + 8'000'000u);
    ++resetInput.commands.retry;
    reset.AdvanceFrame(resetInput, 1.0 / 120.0, 2u,
                       anchor + 8'333'333u);
    check(reset.Snapshot().retryGeneration == 1u &&
          reset.Snapshot().lastConsumedAttackSequence == 1u &&
          reset.Snapshot().combatInputTiming.scheduledEdgeCount == 0u &&
          LatestTimingTrace(reset.Snapshot()).status == CombatInputTimingStatus::Discarded,
          "retry must clear a scheduled combat edge from the same coherent publication");

    GameSimulation late;
    InputSnapshot lateInput;
    late.AdvanceFrame(lateInput, 0.0, 1u, anchor);
    ++lateInput.commands.attack;
    RecordCombatInputEdge(lateInput, CombatInputEdgeKind::Attack,
                          anchor - 1u);
    late.AdvanceFrame(lateInput, 1.0 / 60.0, 2u,
                      anchor + 16'666'667u);
    check(LatestTimingTrace(late.Snapshot()).disposition ==
              CombatInputTimingDisposition::LateEdgeNextTick &&
          LatestTimingTrace(late.Snapshot()).actualTick == 1u,
          "an edge committed with a timestamp before the preceding sample must use the next tick");

    GameSimulation future;
    InputSnapshot futureInput;
    future.AdvanceFrame(futureInput, 0.0, 1u, anchor);
    ++futureInput.commands.attack;
    RecordCombatInputEdge(futureInput, CombatInputEdgeKind::Attack,
                          anchor + 20'000'000u);
    future.AdvanceFrame(futureInput, 1.0 / 60.0, 2u,
                        anchor + 16'666'667u);
    check(LatestTimingTrace(future.Snapshot()).disposition ==
              CombatInputTimingDisposition::LateEdgeNextTick &&
          LatestTimingTrace(future.Snapshot()).targetTick == 1u &&
          LatestTimingTrace(future.Snapshot()).actualTick == 1u,
          "a future/out-of-interval edge timestamp must not backdate into the current batch");

    GameSimulation regressedClock;
    InputSnapshot regressedInput;
    regressedClock.AdvanceFrame(regressedInput, 0.0, 1u, anchor);
    ++regressedInput.commands.attack;
    RecordCombatInputEdge(regressedInput, CombatInputEdgeKind::Attack,
                          anchor + 1'000'000u);
    regressedClock.AdvanceFrame(regressedInput, 1.0 / 60.0, 2u,
                                anchor - 1'000'000u);
    check(LatestTimingTrace(regressedClock.Snapshot()).disposition ==
              CombatInputTimingDisposition::LateEdgeNextTick &&
          LatestTimingTrace(regressedClock.Snapshot()).actualTick == 1u,
          "a regressed owner clock must schedule the edge at the next tick and keep a monotonic anchor");
}

} // namespace

int main()
{
    bool passed = true;
    const auto check = [&passed](bool condition, const char* message)
    {
        if (!condition)
        {
            passed = false;
            std::cerr << "Simulation timing test failed: " << message << '\n';
        }
    };

    RunCombatInputTimingTests(check);

    const SimulationSnapshot at30 = RunCadence(30, 2.0, 1.0f, 0.0f);
    const SimulationSnapshot at60 = RunCadence(60, 2.0, 1.0f, 0.0f);
    const SimulationSnapshot at120 = RunCadence(120, 2.0, 1.0f, 0.0f);
    const auto sameEnemy = [](const auto& left, const auto& right)
    {
        return left.id == right.id && NearlyEqual(left.x, right.x) &&
               NearlyEqual(left.z, right.z) && NearlyEqual(left.facingRadians, right.facingRadians) &&
               NearlyEqual(left.animationTime, right.animationTime) &&
               NearlyEqual(left.damageFlash, right.damageFlash) && left.health == right.health &&
               left.animation == right.animation && left.dead == right.dead &&
               left.playerHitPulse == right.playerHitPulse;
    };
    check(at30.tickIndex == 120u && at60.tickIndex == 120u && at120.tickIndex == 120u,
          "30/60/120 Hz delivery must execute the same 120 fixed ticks");
    check(NearlyEqual(at30.playerX, at60.playerX) && NearlyEqual(at60.playerX, at120.playerX) &&
          NearlyEqual(at30.playerZ, at60.playerZ) && NearlyEqual(at60.playerZ, at120.playerZ),
          "render cadence must not change final player position");
    check(at30.activeEnemyKind == at60.activeEnemyKind && at60.activeEnemyKind == at120.activeEnemyKind &&
          at30.playerVitals.vitality == at60.playerVitals.vitality &&
          at60.playerVitals.vitality == at120.playerVitals.vitality &&
          at30.activeSkeletonCount == at60.activeSkeletonCount &&
          at60.activeSkeletonCount == at120.activeSkeletonCount &&
          at30.skeletonEnemyCount == at60.skeletonEnemyCount &&
          at60.skeletonEnemyCount == at120.skeletonEnemyCount &&
          at30.skeletonAttackerId == at60.skeletonAttackerId &&
          at60.skeletonAttackerId == at120.skeletonAttackerId &&
          at30.openingEncounterComplete == at60.openingEncounterComplete &&
          at60.openingEncounterComplete == at120.openingEncounterComplete &&
          sameEnemy(at30.skeletonEnemies[0], at60.skeletonEnemies[0]) &&
          sameEnemy(at60.skeletonEnemies[0], at120.skeletonEnemies[0]) &&
          sameEnemy(at30.skeletonEnemies[1], at60.skeletonEnemies[1]) &&
          sameEnemy(at60.skeletonEnemies[1], at120.skeletonEnemies[1]),
          "render cadence must preserve encounter, vitality, and action state");

    GameSimulation baseline;
    GameSimulation hitch;
    InputSnapshot forward;
    forward.moveForward = 1.0f;
    forward.damageEnabled = false;
    for (int i = 0; i < 120; ++i)
    {
        baseline.AdvanceFrame(forward, 1.0 / 60.0);
    }
    for (int i = 0; i < 30; ++i)
    {
        hitch.AdvanceFrame(forward, 1.0 / 60.0);
    }
    hitch.AdvanceFrame(forward, 0.100);
    for (int i = 0; i < 84; ++i)
    {
        hitch.AdvanceFrame(forward, 1.0 / 60.0);
    }
    check(hitch.Snapshot().tickIndex == baseline.Snapshot().tickIndex &&
          NearlyEqual(hitch.Snapshot().playerX, baseline.Snapshot().playerX) &&
          NearlyEqual(hitch.Snapshot().playerZ, baseline.Snapshot().playerZ),
          "a bounded 100 ms hitch must retain deterministic time and motion parity");
    check(hitch.Snapshot().catchUpOverrunCount == 0u,
          "an exactly 100 ms hitch must fit the supported catch-up bound");

    const SimulationSnapshot combat30 = RunCombatCadence(30, false);
    const SimulationSnapshot combat60 = RunCombatCadence(60, false);
    const SimulationSnapshot combat120 = RunCombatCadence(120, false);
    const SimulationSnapshot combatHitch = RunCombatCadence(60, true);
    check(combat30.tickIndex == combat60.tickIndex && combat60.tickIndex == combat120.tickIndex &&
          combatHitch.tickIndex == combat60.tickIndex &&
          combat30.activeSkeletonCount == 1u &&
          combat30.activeSkeletonCount == combat60.activeSkeletonCount &&
          combat60.activeSkeletonCount == combat120.activeSkeletonCount &&
          combat120.activeSkeletonCount == combatHitch.activeSkeletonCount &&
          combat30.playerCombat.action == combat60.playerCombat.action &&
          combat60.playerCombat.action == combat120.playerCombat.action &&
          combat120.playerCombat.action == combatHitch.playerCombat.action,
          "swing contact and action completion must retain 30/60/120 Hz and bounded-hitch parity");

    const SimulationSnapshot axial = RunCadence(60, 0.75, 1.0f, 0.0f);
    const SimulationSnapshot diagonal = RunCadence(60, 0.75, 1.0f, 1.0f);
    const float axialDistance = std::hypot(axial.playerX, axial.playerZ - 1.85f);
    const float diagonalDistance = std::hypot(diagonal.playerX, diagonal.playerZ - 1.85f);
    check(NearlyEqual(axialDistance, diagonalDistance, 0.0001f),
          "full diagonal input must have no speed advantage over one axis");

    GameSimulation paused;
    paused.AdvanceFrame(forward, 1.0 / 120.0);
    const float beforePauseX = paused.Snapshot().playerX;
    const float beforePauseZ = paused.Snapshot().playerZ;
    const std::uint64_t beforePauseTick = paused.Snapshot().tickIndex;
    InputSnapshot pauseInput = forward;
    pauseInput.paused = true;
    paused.AdvanceFrame(pauseInput, 1.0);
    check(paused.Snapshot().tickIndex == beforePauseTick &&
          NearlyEqual(paused.Snapshot().playerX, beforePauseX) &&
          NearlyEqual(paused.Snapshot().playerZ, beforePauseZ),
          "pause must not advance movement or combat");
    paused.AdvanceFrame(forward, 1.0 / 120.0);
    paused.AdvanceFrame(forward, 1.0 / 120.0);
    check(paused.Snapshot().tickIndex == beforePauseTick + 1u,
          "resume must discard the stale partial accumulator and advance only fresh time");

    GameSimulation oversized;
    const std::uint32_t oversizedTicks = oversized.AdvanceFrame(forward, 0.5);
    check(oversizedTicks == 6u && oversizedTicks <= 8u &&
          oversized.Snapshot().catchUpOverrunCount == 1u,
          "an oversized stall must clamp to 100 ms, stay within eight ticks, and report one overrun");

    GameSimulation windowsStyle;
    GameSimulation androidMailboxStyle;
    InputMailbox mailbox;
    InputSnapshot equivalentInput;
    equivalentInput.moveForward = 0.72f;
    equivalentInput.moveStrafe = -0.31f;
    equivalentInput.yawRadians = 0.38f;
    equivalentInput.pitchRadians = -0.07f;
    equivalentInput.torchLightStrength = 1.8f;
    equivalentInput.damageEnabled = false;
    for (std::uint64_t frame = 1u; frame <= 90u; ++frame)
    {
        if (frame == 12u || frame == 48u)
        {
            ++equivalentInput.commands.attack;
        }
        if (frame == 24u || frame == 72u)
        {
            ++equivalentInput.commands.dodge;
        }
        windowsStyle.AdvanceFrame(equivalentInput, 1.0 / 60.0, frame);
        mailbox.Publish(equivalentInput);
        const PublishedInput published = mailbox.ConsumeLatest();
        androidMailboxStyle.AdvanceFrame(
            published.snapshot, 1.0 / 60.0, published.publicationSequence);
    }
    const SimulationSnapshot& windowsSnapshot = windowsStyle.Snapshot();
    const SimulationSnapshot& androidSnapshot = androidMailboxStyle.Snapshot();
    check(windowsSnapshot.tickIndex == androidSnapshot.tickIndex &&
          NearlyEqual(windowsSnapshot.playerX, androidSnapshot.playerX) &&
          NearlyEqual(windowsSnapshot.playerZ, androidSnapshot.playerZ) &&
          windowsSnapshot.activeEnemyKind == androidSnapshot.activeEnemyKind &&
          windowsSnapshot.playerVitals.vitality == androidSnapshot.playerVitals.vitality &&
          windowsSnapshot.lastConsumedAttackSequence == androidSnapshot.lastConsumedAttackSequence &&
          windowsSnapshot.lastConsumedDodgeSequence == androidSnapshot.lastConsumedDodgeSequence,
          "equivalent direct Windows and coherent Android-mailbox input must produce the same simulation snapshot");

    if (!passed)
    {
        return 1;
    }
    std::cout << "Shared simulation cadence, hitch, diagonal, pause, and overrun tests passed.\n";
    return 0;
}
