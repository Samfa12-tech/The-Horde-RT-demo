#include <array>
#include <cmath>
#include <cstdint>
#include <iostream>
#include <limits>
#include <span>

#include "audio/MusicDirector.h"
#include "gameplay/simulation/GameSimulation.h"

namespace
{

using namespace horde::audio;
using namespace horde::gameplay;
using namespace horde::gameplay::simulation;

bool passed = true;

void Check(const bool condition, const char* message)
{
    if (!condition)
    {
        passed = false;
        std::cerr << "Music director test failed: " << message << '\n';
    }
}

bool Near(const double left, const double right, const double epsilon = 0.0001)
{
    return std::abs(left - right) <= epsilon;
}

GameplayEvent Event(const GameplayEventType type,
                    const std::uint64_t sequence,
                    const EntityId source = EntityId::Player,
                    const EntityId target = EntityId::Invalid)
{
    GameplayEvent event{};
    event.type = type;
    event.sequence = sequence;
    event.source = source;
    event.target = target;
    return event;
}

void TestOpeningEngagementAndPersistentBeds()
{
    MusicDirector director;
    SimulationSnapshot snapshot{};
    auto result = director.Update(snapshot, {}, 0.0, 1u);
    Check(result.cue == MusicCue::A && result.looping,
          "living but disengaged opening enemies remain in A");

    snapshot.skeletonAttackerId = EntityId::SkeletonA;
    result = director.Update(snapshot, {}, 1.0, 1u);
    Check(result.cue == MusicCue::B && result.looping && Near(result.positionSeconds, 0.0),
          "a valid attacker starts B at its beginning");

    snapshot.skeletonAttackerId = EntityId::Invalid;
    std::array<GameplayEvent, 1> defeated{Event(
        GameplayEventType::EnemyDefeated, 1u, EntityId::Player, EntityId::SkeletonA)};
    result = director.Update(snapshot, defeated, 2.0, 1u);
    Check(result.cue == MusicCue::B && Near(result.positionSeconds, 1.0),
          "one of two defeated skeletons does not end the engaged bed");
    defeated[0].type = GameplayEventType::PlayerDamaged;
    defeated[0].sequence = 2u;
    defeated[0].source = EntityId::SkeletonB;
    defeated[0].target = EntityId::Player;
    const auto eventBefore = defeated[0];
    result = director.Update(snapshot, defeated, 3.0, 1u);
    Check(result.cue == MusicCue::B && Near(result.positionSeconds, 2.0),
          "skeleton damage keeps B engaged through wind-up/recovery gaps");
    Check(defeated[0].sequence == eventBefore.sequence &&
          defeated[0].type == eventBefore.type &&
          defeated[0].source == eventBefore.source &&
          defeated[0].target == eventBefore.target,
          "resolver leaves the caller-owned const event storage unchanged");

    snapshot.openingEncounterComplete = true;
    result = director.Update(snapshot, {}, 4.0, 1u);
    Check(result.cue == MusicCue::A,
          "completed opening encounter exits B");

    GameSimulation simulation;
    MusicDirector realSnapshotDirector;
    result = realSnapshotDirector.Update(simulation.Snapshot(), {}, 0.0, 4u);
    Check(result.cue == MusicCue::A &&
          simulation.Snapshot().activeSkeletonCount == kSkeletonEnemyCapacity,
          "real initial GameSimulation snapshot with two enemies does not imply combat");
}

void TestLichAndRewardPriority()
{
    MusicDirector director;
    SimulationSnapshot snapshot{};
    snapshot.activeEnemyKind = EnemyKind::Lich;
    snapshot.lich.phase = LichPhase::MaintainingRange;
    auto result = director.Update(snapshot, {}, 10.0, 1u);
    Check(result.cue == MusicCue::E && result.looping,
          "active lich selects E");

    snapshot.lich.phase = LichPhase::Charging;
    result = director.Update(snapshot, {}, 13.0, 1u);
    Check(result.cue == MusicCue::E && Near(result.positionSeconds, 3.0),
          "lich charge does not restart E");
    snapshot.lich.phase = LichPhase::Recovering;
    result = director.Update(snapshot, {}, 14.0, 1u);
    Check(result.cue == MusicCue::E && Near(result.positionSeconds, 4.0),
          "lich recovery keeps E continuous");

    snapshot.lich.phase = LichPhase::Dead;
    snapshot.finale.lichDefeated = true;
    snapshot.finale.phase = interactions::FinaleSequencePhase::LichFalling;
    result = director.Update(snapshot, {}, 15.0, 1u);
    Check(result.cue == MusicCue::F && result.looping,
          "defeated lich immediately selects F");
    result = director.Update(snapshot, {}, 75.0, 1u);
    Check(result.cue == MusicCue::F && Near(result.positionSeconds, 0.0),
          "F remains the bed after a 60-second reward wait and exact loop periods");

    snapshot.finale.phase = interactions::FinaleSequencePhase::RaisingLantern;
    result = director.Update(snapshot, {}, 75.5, 1u);
    Check(result.cue == MusicCue::F && Near(result.positionSeconds, 0.5),
          "lantern raise keeps F rather than jumping to G");
    snapshot.finale.phase = interactions::FinaleSequencePhase::RevealingLantern;
    result = director.Update(snapshot, {}, 76.0, 1u);
    Check(result.cue == MusicCue::F && Near(result.positionSeconds, 1.0),
          "lantern reveal keeps F");
}

void TestTorchFailureAndOneShotDeduplication()
{
    MusicDirector director;
    SimulationSnapshot snapshot{};
    snapshot.torchFailure.triggered = true;
    snapshot.torchFailure.phase = TorchFailurePhase::Guttering;
    snapshot.torchFailure.heldByPlayer = true;
    snapshot.torchFailure.flameStrength = 0.0f;
    std::array<GameplayEvent, 1> torchEvent{
        Event(GameplayEventType::TorchExtinguished, 1u)};

    auto result = director.Update(snapshot, torchEvent, 0.0, 1u);
    Check(result.cue == MusicCue::A,
          "triggered or Guttering torch state is not yet actual extinction");
    result = director.Update(snapshot, torchEvent, 0.5, 1u);
    Check(result.cue == MusicCue::A,
          "duplicate early event remains pending until torch release");

    snapshot.torchFailure.phase = TorchFailurePhase::Falling;
    snapshot.torchFailure.heldByPlayer = false;
    result = director.Update(snapshot, torchEvent, 0.6, 1u);
    Check(result.cue == MusicCue::C && !result.looping &&
          Near(result.positionSeconds, 0.0),
          "pending event triggers C only once durable actual failure begins");
    result = director.Update(snapshot, {}, 3.59, 1u);
    Check(result.cue == MusicCue::C && Near(result.positionSeconds, 2.99),
          "C advances on the audio clock for exactly three seconds");
    result = director.Update(snapshot, {}, 3.60, 1u);
    Check(result.cue == MusicCue::D && result.looping &&
          Near(result.positionSeconds, 0.0),
          "C ends at three seconds and recomputes to dark exploration D");

    result = director.Update(snapshot, torchEvent, 4.0, 1u);
    Check(result.cue == MusicCue::D,
          "same event sequence cannot retrigger C after its one-shot latch");

    MusicDirector lateAttach;
    result = lateAttach.Update(snapshot, {}, 0.0, 1u);
    Check(result.cue == MusicCue::D,
          "late attachment after durable torch failure reconstructs D without C");

    MusicDirector cancel;
    snapshot.finale = {};
    snapshot.torchFailure.phase = TorchFailurePhase::Guttering;
    snapshot.torchFailure.heldByPlayer = true;
    cancel.Update(snapshot, torchEvent, 0.0, 1u);
    snapshot.torchFailure.phase = TorchFailurePhase::Falling;
    snapshot.torchFailure.heldByPlayer = false;
    result = cancel.Update(snapshot, {}, 0.1, 1u);
    Check(result.cue == MusicCue::C,
          "torch event starts C when the actual failure arrives");
    snapshot.playerAlive = false;
    result = cancel.Update(snapshot, {}, 0.2, 1u);
    Check(result.cue == MusicCue::None,
          "death cancels C to silence");
    snapshot.playerAlive = true;
    result = cancel.Update(snapshot, {}, 0.3, 1u);
    Check(result.cue == MusicCue::D,
          "cancelled one-shot is not replayed when alive state returns");
}

void TestResetAndQueueLifetimes()
{
    MusicDirector director;
    SimulationSnapshot snapshot{};
    snapshot.torchFailure.phase = TorchFailurePhase::Falling;
    snapshot.torchFailure.heldByPlayer = false;
    std::array<GameplayEvent, 1> event{
        Event(GameplayEventType::TorchExtinguished, 8u)};
    auto result = director.Update(snapshot, event, 0.0, 7u);
    Check(result.cue == MusicCue::C,
          "first newly supplied torch event can begin the one-shot");
    result = director.Update(snapshot, {}, 3.0, 7u);
    Check(result.cue == MusicCue::D,
          "completed C holds its session latch");

    result = director.Update(snapshot, event, 4.0, 8u);
    Check(result.cue == MusicCue::C,
          "new event-queue lifetime resets event sequence and one-shot latches");

    snapshot.retryGeneration = 1u;
    event[0].sequence = 9u;
    result = director.Update(snapshot, event, 4.1, 8u);
    Check(result.cue == MusicCue::C && result.discontinuity,
          "retry clears route latches while a monotonic queue accepts the next event");
    event[0].sequence = 8u;
    result = director.Update(snapshot, event, 4.15, 8u);
    Check(result.cue == MusicCue::C && Near(result.positionSeconds, 0.05),
          "older sequence is rejected without replaying the active one-shot");

    result = director.Update(snapshot, {}, 7.1, 8u);
    Check(result.cue == MusicCue::D,
          "retry one-shot completes normally");
    snapshot.retryGeneration += 1u;
    snapshot.lastConsumedRouteResetSequence += 1u;
    snapshot.torchFailure.phase = TorchFailurePhase::Held;
    snapshot.torchFailure.heldByPlayer = true;
    snapshot.torchFailure.triggered = false;
    snapshot.skeletonAttackerId = EntityId::Invalid;
    event[0].sequence = 1u;
    result = director.Update(snapshot, {}, 7.2, 8u);
    Check(result.cue == MusicCue::A && result.discontinuity,
          "route reset clears the previous torch failure and chooses the fresh route bed");
    result = director.Update(snapshot, event, 7.3, 8u);
    Check(result.cue == MusicCue::A,
          "stale torch event cannot fire after route reset before actual failure");

    MusicDirector routeCombat;
    SimulationSnapshot combatSnapshot{};
    std::array<GameplayEvent, 1> attack{
        Event(GameplayEventType::EnemyAttackStarted, 5u,
              EntityId::SkeletonA, EntityId::Player)};
    result = routeCombat.Update(combatSnapshot, attack, 0.0, 9u);
    Check(result.cue == MusicCue::B,
          "newly supplied skeleton event engages the current route");
    ++combatSnapshot.retryGeneration;
    ++combatSnapshot.lastConsumedRouteResetSequence;
    result = routeCombat.Update(combatSnapshot, {}, 0.1, 9u);
    Check(result.cue == MusicCue::A && result.discontinuity,
          "route reset clears combat latches without changing queue lifetime");
    result = routeCombat.Update(combatSnapshot, attack, 0.2, 9u);
    Check(result.cue == MusicCue::A,
          "already-consumed event cannot relatch combat after route reset");
}

void TestProductionTorchSequence()
{
    auto config = ProductionGameSimulationConfig();
    config.playerStartX = -1.80f;
    config.playerStartZ = -15.20f;
    GameSimulation simulation(config);
    MusicDirector director;
    director.Update(simulation.Snapshot(), {}, 0.0, 1u);
    InputSnapshot input{};
    bool observedEvent = false;
    bool observedRelease = false;
    std::uint64_t eventSequence = 0;
    double releaseTime = 0.0;
    for (int tick = 1; tick <= 240; ++tick)
    {
        simulation.StepFixed(input);
        const auto& snapshot = simulation.Snapshot();
        const auto events = simulation.Events().Events();
        const auto count = events.size();
        for (const auto& event : events)
        {
            if (event.type == GameplayEventType::TorchExtinguished)
            {
                Check(!observedEvent && event.sequence > 0,
                      "production torch publishes one monotonic event");
                observedEvent = true;
                eventSequence = event.sequence;
                Check(snapshot.torchFailure.phase == TorchFailurePhase::Guttering &&
                      snapshot.torchFailure.heldByPlayer && snapshot.torchFailure.flameStrength > 0,
                      "production event announces guttering before physical extinction");
            }
        }
        const double audioTime = tick / 60.0;
        const auto result = director.Update(snapshot, events, audioTime, 1u);
        Check(simulation.Events().Size() == count,
              "music never drains events still needed by production SFX");
        if (!snapshot.torchFailure.heldByPlayer && !observedRelease)
        {
            observedRelease = true;
            releaseTime = audioTime;
            Check(result.cue == MusicCue::C && Near(result.positionSeconds, 0),
                  "pending production sting begins at actual torch extinction/release");
        }
        if (snapshot.torchFailure.phase == TorchFailurePhase::Guttering)
        {
            Check(result.cue == MusicCue::A,
                  "lit production guttering does not prematurely select the dark bed");
        }
        if (observedRelease && audioTime >= releaseTime + 3.0 + 0.0001)
        {
            Check(result.cue == MusicCue::D,
                  "production sting returns to persistent dark exploration after three seconds");
        }
        simulation.ClearEvents(); // Existing SFX consumer remains the queue owner.
    }
    Check(observedEvent && observedRelease && eventSequence > 0,
          "real simulation exercised event-to-release-to-dark transitions");
}

void TestSkylightAndDawnContinuity()
{
    MusicDirector director;
    SimulationSnapshot snapshot{};
    snapshot.finale.lichDefeated = true;
    snapshot.finale.phase = interactions::FinaleSequencePhase::SkylightOpening;
    snapshot.finale.endingPhase = interactions::FinaleEndingPhase::SkylightOpening;
    snapshot.finale.phaseTime = 1.25f;
    auto result = director.Update(snapshot, {}, 0.0, 1u);
    Check(result.cue == MusicCue::G && !result.looping &&
          Near(result.positionSeconds, 1.25),
          "late opening attachment seeks G to the actual phase time");

    snapshot.finale.phaseTime = 1.25f;
    result = director.Update(snapshot, {}, 3.0, 1u);
    Check(result.cue == MusicCue::G && Near(result.positionSeconds, 4.25),
          "G advances from monotonic audio time while the opening phase persists");

    snapshot.finale.phase = interactions::FinaleSequencePhase::DawnRevealed;
    snapshot.finale.endingPhase = interactions::FinaleEndingPhase::DawnRevealed;
    snapshot.finale.phaseTime = 0.0f;
    result = director.Update(snapshot, {}, 3.1, 1u);
    Check(result.cue == MusicCue::G && Near(result.positionSeconds, 4.5) &&
          result.discontinuity,
          "Dawn transition preserves G at the 4.50-second harmonic boundary");

    snapshot.finale.phaseTime = 0.5f;
    MusicDirector lateDawn;
    result = lateDawn.Update(snapshot, {}, 0.0, 1u);
    Check(result.cue == MusicCue::G && Near(result.positionSeconds, 5.0),
          "late Dawn attachment reconstructs G at 4.50 seconds plus phase time");

    result = director.Update(snapshot, {}, 4.59, 1u);
    Check(result.cue == MusicCue::G && Near(result.positionSeconds, 5.99),
          "G remains through its 5.99-second boundary");
    result = director.Update(snapshot, {}, 4.60, 1u);
    Check(result.cue == MusicCue::H && result.looping &&
          Near(result.positionSeconds, 0.0),
          "G ends at six musical seconds and H begins once");
    result = director.Update(snapshot, {}, 4.70, 1u);
    Check(result.cue == MusicCue::H && Near(result.positionSeconds, 0.1),
          "H remains latched and advances if Dawn snapshot phase time is stale");

    snapshot.finale.phase = interactions::FinaleSequencePhase::Complete;
    snapshot.finale.endingPhase = interactions::FinaleEndingPhase::Complete;
    snapshot.finale.phaseTime = 0.0f;
    snapshot.finaleComplete = true;
    result = director.Update(snapshot, {}, 5.0, 1u);
    Check(result.cue == MusicCue::H && Near(result.positionSeconds, 0.4),
          "complete ending stays on H and preserves its loop phase");

    MusicDirector lateComplete;
    result = lateComplete.Update(snapshot, {}, 0.0, 1u);
    Check(result.cue == MusicCue::H && result.looping,
          "late completed-finale attachment starts H directly");
}

void TestSuspensionClockAndLoopPeriods()
{
    MusicDirector director;
    SimulationSnapshot snapshot{};
    auto result = director.Update(snapshot, {}, 100.0, 1u);
    result = director.Update(snapshot, {}, 106.0, 1u);
    Check(result.cue == MusicCue::A && Near(result.positionSeconds, 6.0),
          "loop position advances by audio-clock delta");
    result = director.Update(snapshot, {}, 112.0, 1u);
    Check(Near(result.positionSeconds, 0.0),
          "12-second loop period excludes release tails and wraps exactly");
    for (int period = 1; period <= 20; ++period)
    {
        result = director.Update(snapshot, {}, 112.0 + period * 12.0, 1u);
    }
    Check(result.cue == MusicCue::A && Near(result.positionSeconds, 0.0),
          "twenty exact twelve-second loops preserve the authored loop period");

    result = director.Update(snapshot, {}, 353.0, 1u, true);
    Check(result.suspended && Near(result.positionSeconds, 0.0),
          "external lifecycle suspension freezes the music clock");
    result = director.Update(snapshot, {}, 413.0, 1u, false);
    Check(!result.suspended && Near(result.positionSeconds, 0.0),
          "resume does not count suspended wall/audio-clock time");
    result = director.Update(snapshot, {}, 414.5, 1u, false);
    Check(Near(result.positionSeconds, 1.5),
          "music resumes from the frozen musical position");

    MusicDirector gResume;
    snapshot.finale.lichDefeated = true;
    snapshot.finale.phase = interactions::FinaleSequencePhase::SkylightOpening;
    snapshot.finale.phaseTime = 1.0f;
    result = gResume.Update(snapshot, {}, 0.0, 1u);
    Check(result.cue == MusicCue::G && Near(result.positionSeconds, 1.0),
          "initial G enters from durable finale phase time");
    snapshot.paused = true;
    snapshot.finale.phaseTime = 1.0f;
    result = gResume.Update(snapshot, {}, 5.0, 1u);
    Check(result.suspended && Near(result.positionSeconds, 1.0),
          "paused finale freezes its G cue");
    snapshot.paused = false;
    snapshot.finale.phaseTime = 2.25f;
    result = gResume.Update(snapshot, {}, 500.0, 1u);
    Check(!result.suspended && result.cue == MusicCue::G &&
          Near(result.positionSeconds, 2.25),
          "resume reconstructs G from the durable finale phase clock");

    MusicDirector invalidClock;
    snapshot = {};
    invalidClock.Update(snapshot, {}, 4.0, 1u);
    result = invalidClock.Update(snapshot, {}, 5.0, 1u);
    Check(Near(result.positionSeconds, 1.0), "finite clock advances the bed");
    result = invalidClock.Update(snapshot, {}, 4.5, 1u);
    Check(!result.clockValid && result.discontinuity &&
          Near(result.positionSeconds, 1.0),
          "backwards audio clock is explicit and cannot rewind the current cue");
    result = invalidClock.Update(snapshot, {}, 5.5, 1u);
    Check(result.clockValid && Near(result.positionSeconds, 2.0),
          "clock rebase resumes forward progression after a backwards sample");
    invalidClock.Update(snapshot, {}, std::numeric_limits<double>::quiet_NaN(), 1u);
    result = invalidClock.Update(snapshot, {}, 100.0, 1u);
    Check(result.clockValid && Near(result.positionSeconds, 2.0),
          "non-finite clock invalidates one interval without adding an unbounded jump");
}

void TestIrregularUpdateEquivalence()
{
    SimulationSnapshot snapshot{};
    MusicDirector regular;
    MusicDirector irregular;
    regular.Update(snapshot, {}, 0.0, 1u);
    irregular.Update(snapshot, {}, 0.0, 1u);
    for (int step = 1; step <= 33; ++step)
    {
        regular.Update(snapshot, {}, step * 0.25, 1u);
    }
    const auto result = irregular.Update(snapshot, {}, 8.25, 1u);
    const auto regularResult = regular.Update(snapshot, {}, 8.25, 1u);
    Check(result.cue == MusicCue::A && regularResult.cue == MusicCue::A &&
          Near(result.positionSeconds, regularResult.positionSeconds),
          "irregular update cadence matches the same elapsed audio clock");
}

} // namespace

int main()
{
    TestOpeningEngagementAndPersistentBeds();
    TestLichAndRewardPriority();
    TestTorchFailureAndOneShotDeduplication();
    TestResetAndQueueLifetimes();
    TestProductionTorchSequence();
    TestSkylightAndDawnContinuity();
    TestSuspensionClockAndLoopPeriods();
    TestIrregularUpdateEquivalence();
    if (!passed)
    {
        return 1;
    }
    std::cout << "Shared adaptive music resolver tests passed.\n";
    return 0;
}
