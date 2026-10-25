#include "telemetry/CombatTimingTrace.h"
#include "gameplay/simulation/GameSimulation.h"

#include <iostream>
#include <cmath>
#include <sstream>

namespace
{
bool TimestampedHeldGuardStartsAtScheduledTick(const auto& check)
{
    using namespace horde::gameplay;
    using namespace horde::gameplay::simulation;
    auto config = ProductionGameSimulationConfig();
    config.swordStartsStowed = false;
    config.waterfallSkeletonEncounter = false;
    GameSimulation simulation(config);
    InputSnapshot input;
    input.tutorialEnabled = false;
    input.damageEnabled = false;
    constexpr std::uint64_t anchor = 5'000'000'000u;
    constexpr std::uint64_t interval = 16'666'667u;
    simulation.AdvanceFrame(input, 0.0, 1u, anchor);

    // Android publishes continuous held state and timestamped edges together.
    // In a catch-up frame, ticks1..5 therefore see held=true before the fresh
    // edge is legitimately due at tick6. Those earlier updates must not expire
    // its one-shot press ownership or move the parry to an earlier tick.
    input.guardHeld = true;
    input.commands.parry = 1u;
    RecordCombatInputEdge(input, CombatInputEdgeKind::Parry, anchor + 85'000'000u);
    const auto steps = simulation.AdvanceFrame(input, .100, 2u, anchor + 100'000'000u);
    const auto& timing = simulation.Snapshot().combatInputTiming;
    const auto traceIndex = (timing.nextTraceIndex +
        static_cast<std::uint32_t>(timing.traces.size()) - 1u) %
        static_cast<std::uint32_t>(timing.traces.size());
    const auto consumed = timing.traces[traceIndex];
    bool pass = check(steps == 6u && simulation.Snapshot().tickIndex == 6u &&
        consumed.kind == CombatInputEdgeKind::Parry && consumed.commandSequence == 1u &&
        consumed.targetTick == 6u && consumed.actualTick == 6u &&
        consumed.status == CombatInputTimingStatus::Consumed &&
        simulation.Snapshot().lastConsumedParrySequence == 1u,
        "A late-frame held parry preserves timestamp admission at tick6 after five earlier held ticks.");
    pass &= check(simulation.Snapshot().playerCombat.action == PlayerCombatAction::ParryStartup,
        "A consumed timestamped fresh press must actually start parry, not lose its held token during catch-up.");

    std::uint64_t now = anchor + 100'000'000u;
    std::uint64_t publication = 2u;
    bool reachedGuard = false, restartedAfterGuard = false;
    for (int frame = 0; frame < 45; ++frame)
    {
        now += interval;
        simulation.AdvanceFrame(input, 1.0 / 60.0, ++publication, now);
        const auto action = simulation.Snapshot().playerCombat.action;
        if (reachedGuard && (action == PlayerCombatAction::ParryStartup ||
                             action == PlayerCombatAction::ParryActive))
            restartedAfterGuard = true;
        reachedGuard = reachedGuard || action == PlayerCombatAction::Guard;
    }
    pass &= check(reachedGuard && !restartedAfterGuard &&
        simulation.Snapshot().playerCombat.action == PlayerCombatAction::Guard,
        "The timestamped press completes into held guard without replaying the original parry window.");

    // Another correctly timestamped command without a physical release is not
    // another fresh press. Retain command consumption while rejecting defense.
    ++input.commands.parry;
    RecordCombatInputEdge(input, CombatInputEdgeKind::Parry, now + interval / 2u);
    now += interval;
    simulation.AdvanceFrame(input, 1.0 / 60.0, ++publication, now);
    pass &= check(simulation.Snapshot().lastConsumedParrySequence == 2u &&
        simulation.Snapshot().playerCombat.action == PlayerCombatAction::Guard,
        "A scheduled repeated command while held cannot manufacture a fresh parry window.");
    input.guardHeld = false;
    now += interval;
    simulation.AdvanceFrame(input, 1.0 / 60.0, ++publication, now);
    input.guardHeld = true;
    ++input.commands.parry;
    RecordCombatInputEdge(input, CombatInputEdgeKind::Parry, now + interval / 2u);
    now += interval;
    simulation.AdvanceFrame(input, 1.0 / 60.0, ++publication, now);
    pass &= check(simulation.Snapshot().lastConsumedParrySequence == 3u &&
        simulation.Snapshot().playerCombat.action == PlayerCombatAction::ParryStartup,
        "A physical release and new scheduled press rearms exactly one ordinary startup.");
    return pass;
}

bool TimestampedBusyGuardPressIsDiscarded(const auto& check)
{
    using namespace horde::gameplay;
    using namespace horde::gameplay::simulation;
    auto config = ProductionGameSimulationConfig();
    config.swordStartsStowed = false;
    config.waterfallSkeletonEncounter = false;
    GameSimulation simulation(config);
    InputSnapshot input;
    input.tutorialEnabled = false;
    input.damageEnabled = false;
    constexpr std::uint64_t anchor = 8'000'000'000u;
    constexpr std::uint64_t interval = 16'666'667u;
    simulation.AdvanceFrame(input, 0.0, 1u, anchor);
    input.commands.attack = 1u;
    RecordCombatInputEdge(input, CombatInputEdgeKind::Attack, anchor + 8'000'000u);
    std::uint64_t now = anchor + interval;
    std::uint64_t publication = 2u;
    simulation.AdvanceFrame(input, 1.0 / 60.0, publication, now);
    bool pass = check(simulation.Snapshot().playerCombat.action == PlayerCombatAction::SwingWindup,
        "Busy guard-discard fixture begins with a real timestamped attack.");

    // This fresh hold arrives during the attack. Its edge is due only on the
    // last catch-up tick, so it retains a valid token until deliberate busy
    // rejection; that rejection must then consume the token permanently.
    input.guardHeld = true;
    input.commands.parry = 1u;
    RecordCombatInputEdge(input, CombatInputEdgeKind::Parry, now + 85'000'000u);
    now += 100'000'000u;
    const auto steps = simulation.AdvanceFrame(input, .100, ++publication, now);
    const auto& timing = simulation.Snapshot().combatInputTiming;
    const auto traceIndex = (timing.nextTraceIndex +
        static_cast<std::uint32_t>(timing.traces.size()) - 1u) %
        static_cast<std::uint32_t>(timing.traces.size());
    const auto consumed = timing.traces[traceIndex];
    pass &= check(steps == 6u && consumed.kind == CombatInputEdgeKind::Parry &&
        consumed.commandSequence == 1u && consumed.targetTick == 7u && consumed.actualTick == 7u &&
        consumed.status == CombatInputTimingStatus::Consumed &&
        simulation.Snapshot().lastConsumedParrySequence == 1u &&
        simulation.Snapshot().playerCombat.action == PlayerCombatAction::SwingWindup,
        "A delayed guard press is consumed at busy tick7 without interrupting the attack.");

    bool reachedGuard = false, replayedParry = false;
    for (int frame = 0; frame < 90; ++frame)
    {
        now += interval;
        simulation.AdvanceFrame(input, 1.0 / 60.0, ++publication, now);
        const auto action = simulation.Snapshot().playerCombat.action;
        replayedParry = replayedParry || action == PlayerCombatAction::ParryStartup ||
                       action == PlayerCombatAction::ParryActive;
        if (action == PlayerCombatAction::Guard) { reachedGuard = true; break; }
    }
    pass &= check(reachedGuard && !replayedParry &&
        simulation.Snapshot().tickIndex / 60.0 + 0.00001 >= SwordCombat::kSwordDuration - 1.0 / 60.0,
        "A busy held press returns only to ordinary guard after full attack recovery, never delayed parry.");
    for (std::uint64_t command = 2u; command <= 4u; ++command)
    {
        input.commands.parry = command;
        RecordCombatInputEdge(input, CombatInputEdgeKind::Parry, now + interval / 2u);
        now += interval;
        simulation.AdvanceFrame(input, 1.0 / 60.0, ++publication, now);
        pass &= check(simulation.Snapshot().lastConsumedParrySequence == command &&
            simulation.Snapshot().playerCombat.action == PlayerCombatAction::Guard,
            "Post-recovery edge spam cannot reuse a fresh token discarded while the attack was busy.");
    }
    return pass;
}
}

int main()
{
    using namespace horde::gameplay::simulation;
    horde::telemetry::CombatTimingTrace reporter;
    SimulationSnapshot state;
    state.tickIndex = 81u;
    state.combatInputTiming.traceCount = 1u;
    state.combatInputTiming.nextTraceIndex = 1u;
    auto& edge = state.combatInputTiming.traces[0];
    edge.kind = CombatInputEdgeKind::Parry;
    edge.commandSequence = 1u;
    edge.edgeSteadyTimeNanoseconds = 1'010'000'000u;
    edge.publicationSequence = 5u;
    edge.targetTick = 80u;
    edge.actualTick = 80u;
    edge.status = CombatInputTimingStatus::Consumed;
    horde::telemetry::RtSubmittedFrameIdentity identity;
    identity.submissionSerial = 9u;
    identity.frame.simulationTick = 80u;
    identity.frame.recordSerial = 8u;
    std::ostringstream output;
    const auto check = [](bool pass, const char* message) {
        if (!pass) std::cerr << "FAIL: " << message << '\n';
        return pass;
    };
    bool pass = check(reporter.WriteAcceptedPresent(output, state, identity, 1'100'000'000u, true) == 0u,
        "A different recorded pose tick cannot be attributed to this frame.");
    pass &= TimestampedHeldGuardStartsAtScheduledTick(check);
    pass &= TimestampedBusyGuardPressIsDiscarded(check);
    identity.frame.simulationTick = 81u;
    pass &= check(reporter.WriteAcceptedPresent(output, state, identity, 1'100'000'000u, false) == 0u,
        "An unaccepted present cannot produce a presentation row.");
    pass &= check(reporter.WriteAcceptedPresent(output, state, identity, 1'100'000'000u, true) == 1u,
        "The first accepted owning RT frame must join input, tick and pose.");
    const auto first = output.str();
    pass &= check(first.find("\"edgeNativeReceiptNs\":1010000000") != std::string::npos &&
        first.find("\"consumedTick\":80") != std::string::npos &&
        first.find("\"poseTick\":81") != std::string::npos &&
        first.find("\"submission\":9") != std::string::npos &&
        first.find("\"displayTimeMeasured\":false") != std::string::npos,
        "Trace provenance must retain distinct input/tick/pose/presentation clocks and honest display limitation.");
    pass &= check(reporter.WriteAcceptedPresent(output, state, identity, 1'110'000'000u, true) == 0u,
        "Repeated polling cannot duplicate the same edge presentation.");
    edge.semanticEventSequence = 20u;
    edge.semanticEventTick = 82u;
    state.tickIndex = identity.frame.simulationTick = 83u;
    state.combatPresentation.parrySuccessActive = true;
    pass &= check(reporter.WriteAcceptedPresent(output, state, identity, 1'120'000'000u, true) == 1u,
        "Later parry feedback must retain its source edge and owning later frame.");
    for (std::uint64_t command = 2u; command <= 140u; ++command)
    {
        edge.commandSequence = command;
        reporter.WriteAcceptedPresent(output, state, identity, 1'120'000'000u + command, true);
    }
    std::uint32_t rows = 0u;
    for (const char character : output.str()) if (character == '\n') ++rows;
    pass &= check(rows == horde::telemetry::CombatTimingTrace::kMaximumRows && !reporter.HasReportable(state),
        "Diagnostic output must stop at its fixed session bound.");
    {
        horde::telemetry::CombatTimingTrace contacts;
        SimulationSnapshot contactState;
        contactState.tickIndex = 90;
        CombatContactSample sample;
        sample.tick = 88; sample.commandSequence = 7; sample.consumedTick = 80;
        sample.attackId = 5; sample.phase = horde::gameplay::PlayerCombatAction::SwingActive;
        sample.cut = horde::gameplay::PlayerAttackCut::DownwardCut;
        sample.outcome = CombatContactOutcome::AcceptedSwordContact;
        sample.target = EntityId::SkeletonA; sample.semanticEventSequence = 21;
        sample.phaseSeconds = .12f; sample.hasBlade = true;
        sample.bladeStart = {.1f,.2f,.3f}; sample.bladeEnd = {.4f,.5f,.6f};
        sample.hasSeparation = true; sample.separationMetres = -.001f;
        contactState.combatContactTrace.Push(sample);
        const auto frozenContactState = contactState;
        contactState.combatContactTrace.samples[0].targetX = 2.0f;
        pass &= check(frozenContactState.combatContactTrace.samples[0].targetX == 0.0f &&
            contactState.combatContactTrace.samples.size() == CombatContactTraceSnapshot::kCapacity,
            "bounded heap-backed contact samples remain independently owned in immutable snapshot copies");
        identity.frame.simulationTick = 89;
        std::ostringstream contactOutput;
        pass &= check(contacts.WriteAcceptedPresent(contactOutput,contactState,identity,2000,true)==0,
            "contact observations cannot join the wrong owning rendered snapshot");
        identity.frame.simulationTick = 90;
        pass &= check(contacts.WriteAcceptedPresent(contactOutput,contactState,identity,2000,false)==0,
            "unaccepted present never promotes contact evidence");
        pass &= check(contacts.WriteAcceptedPresent(contactOutput,contactState,identity,2000,true)==1,
            "a real owning present can join bounded contact history");
        const auto contactText=contactOutput.str();
        pass &= check(contactText.find("\"contactSampleTick\":88")!=std::string::npos &&
            contactText.find("\"poseTick\":90")!=std::string::npos &&
            contactText.find("\"sampleTickWasPresented\":false")!=std::string::npos &&
            contactText.find("\"targetReactionMayReplaceContactPose\":true")!=std::string::npos &&
            contactText.find("\"attackId\":5")!=std::string::npos &&
            contactText.find("\"semanticEvent\":21")!=std::string::npos,
            "catch-up preserves input/cut/contact/result identity without claiming a skipped pose was displayed");
        contactState.combatContactTrace.Clear();
        sample.tick = 90; sample.bladeStart[0]=NAN; sample.separationMetres=NAN;
        contactState.combatContactTrace.Push(sample);
        pass &= check(contacts.WriteAcceptedPresent(contactOutput,contactState,identity,2010,true)==1 &&
            contactOutput.str().find("\"separationMetres\":null")!=std::string::npos &&
            contactOutput.str().find("\"bladeStart\":null")!=std::string::npos,
            "new generations retain monotonic identity and nonfinite samples remain unavailable");
        pass &= check(contacts.WriteAcceptedPresent(contactOutput,contactState,identity,2020,true)==0,
            "repeated present polling cannot duplicate a contact row");
    }
    return pass ? 0 : 1;
}
