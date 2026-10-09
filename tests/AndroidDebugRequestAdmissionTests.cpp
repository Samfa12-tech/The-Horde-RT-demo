#include "platform/android/AndroidDebugRequestAdmission.h"

#include <iostream>

int main()
{
    using namespace horde::platform::android;
    bool passed = true;
    const auto check = [&](bool value, const char* message) {
        if (!value) { std::cerr << message << '\n'; passed = false; }
    };
    DebugRequestAdmissionState available{};
    available.surfaceReady = true;
    check(CanRequestCombatPractice(available), "active clean session admits debug practice");
    check(!CanRequestCombatPractice({}), "practice requires an active surface");
    const DebugRequestAdmissionState conflicts[] = {
        [&] { auto s = available; s.practicePending = true; return s; }(),
        [&] { auto s = available; s.checkpointPending = true; return s; }(),
        [&] { auto s = available; s.fixtureCheckpointActive = true; return s; }(),
        [&] { auto s = available; s.capturePending = true; return s; }(),
        [&] { auto s = available; s.replayPending = true; return s; }(),
        [&] { auto s = available; s.benchmarkPendingOrActive = true; return s; }(),
        [&] { auto s = available; s.motionPendingOrActive = true; return s; }(),
        [&] { auto s = available; s.entryModeActive = true; return s; }(),
    };
    for (const auto& state : conflicts)
        check(!CanRequestCombatPractice(state), "practice rejects every competing mode in either direction");
    DebugRequestAdmissionState practiceQueued = available;
    practiceQueued.practicePending = true;
    check(!CanRequestCompetingDebugMode(practiceQueued),
          "capture, motion, replay, checkpoint and benchmark endpoints reject queued practice");
    check(CanRequestCompetingDebugMode(available), "ordinary debug automation remains available without practice");
    return passed ? 0 : 1;
}
