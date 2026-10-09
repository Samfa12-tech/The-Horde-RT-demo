#include "telemetry/CombatTimingTrace.h"

#include <iostream>
#include <sstream>

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
    return pass ? 0 : 1;
}
