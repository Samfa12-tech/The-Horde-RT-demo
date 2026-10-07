#include "telemetry/InputPresentationTrace.h"
#include "gameplay/simulation/GameSimulation.h"

#include <iostream>
#include <limits>
#include <sstream>

int main()
{
    using namespace horde::gameplay::simulation;
    using horde::telemetry::InputPresentationTrace;
    const auto check = [](const bool pass, const char* message) {
        if (!pass) std::cerr << "FAIL: " << message << '\n';
        return pass;
    };
    GameSimulation simulation;
    InputSnapshot input;
    input.moveForward = 0.75f;
    input.moveStrafe = -0.25f;
    simulation.AdvanceFrame(input, 1.0 / 60.0, 7u);
    auto state = simulation.Snapshot();
    bool pass = check(state.inputPublicationSequence == 7u &&
        state.inputMoveForward == 0.75f && state.inputMoveStrafe == -0.25f,
        "A coherent simulation snapshot must retain its actual continuous input publication.");
    input.moveForward = input.moveStrafe = 0.0f;
    simulation.AdvanceFrame(input, 0.0, 8u);
    state = simulation.Snapshot();
    pass &= check(state.inputPublicationSequence == 8u && state.inputMoveForward == 0.0f &&
        state.inputMoveStrafe == 0.0f,
        "A cancel/release publication must be observable even on a zero-tick frame.");
    simulation.SynchronizePausedInput(input, 9u);
    state = simulation.Snapshot();
    pass &= check(state.paused && state.inputPublicationSequence == 9u &&
        state.inputMoveForward == 0.0f && state.inputMoveStrafe == 0.0f,
        "Pause synchronization must retain zero controls and its own publication.");

    InputPresentationTrace reporter;
    horde::telemetry::RtSubmittedFrameIdentity identity;
    identity.frame.sceneEpoch = 2u;
    identity.frame.recordSerial = 10u;
    identity.submissionSerial = 11u;
    identity.frame.simulationTick = state.tickIndex + 1u;
    std::ostringstream output;
    pass &= check(!reporter.WriteAcceptedPresent(output, state, identity, 1'000'000'000u, true),
        "A different recorded pose cannot be attributed to the current controls.");
    identity.frame.simulationTick = state.tickIndex;
    pass &= check(!reporter.WriteAcceptedPresent(output, state, identity, 1'000'000'000u, false),
        "Unaccepted RT presentation must not emit an input observation.");
    pass &= check(reporter.WriteAcceptedPresent(output, state, identity, 1'000'000'000u, true),
        "The first owning accepted RT present must join controls, pose and presentation.");
    pass &= check(output.str().find("\"moveForward\":0") != std::string::npos &&
        output.str().find("\"paused\":true") != std::string::npos &&
        output.str().find("\"submission\":11") != std::string::npos &&
        output.str().find("\"displayTimeMeasured\":false") != std::string::npos,
        "The row must identify continuous controls, paused state, owning submission and its display limit.");
    ++identity.frame.recordSerial;
    ++identity.submissionSerial;
    pass &= check(!reporter.WriteAcceptedPresent(output, state, identity, 1'010'000'000u, true),
        "Unchanged controls must respect the idle observation interval.");
    pass &= check(reporter.WriteAcceptedPresent(output, state, identity, 1'250'000'000u, true),
        "A later owning frame must allow stationary post-cancel position observation.");
    ++state.inputPublicationSequence;
    pass &= check(!reporter.WriteAcceptedPresent(output, state, identity, 1'260'000'000u, true),
        "A repeated submitted frame cannot be reused for a new input publication.");
    ++identity.frame.recordSerial;
    ++identity.submissionSerial;
    state.inputMoveForward = 1.0f;
    pass &= check(reporter.WriteAcceptedPresent(output, state, identity, 1'260'000'000u, true),
        "A fresh input publication must be observable before the next idle interval.");
    ++identity.submissionSerial;
    state.inputMoveForward = std::numeric_limits<float>::quiet_NaN();
    pass &= check(!reporter.WriteAcceptedPresent(output, state, identity, 1'510'000'000u, true),
        "Nonfinite optional telemetry must be omitted rather than produce invalid JSON.");
    state.inputMoveForward = 0.0f;
    for (std::uint64_t index = 0u; index < 300u; ++index)
    {
        ++state.inputPublicationSequence;
        ++identity.frame.recordSerial;
        ++identity.submissionSerial;
        reporter.WriteAcceptedPresent(output, state, identity, 1'520'000'000u + index, true);
    }
    std::uint32_t rows = 0u;
    for (const char character : output.str()) if (character == '\n') ++rows;
    pass &= check(rows == InputPresentationTrace::kMaximumRows &&
        !reporter.HasReportable(state, 2'000'000'000u),
        "Continuous input inspection must stop at its independent session quota.");
    return pass ? 0 : 1;
}
