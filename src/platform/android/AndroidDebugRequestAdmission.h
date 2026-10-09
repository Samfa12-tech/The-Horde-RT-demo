#pragma once

namespace horde::platform::android
{
struct DebugRequestAdmissionState
{
    bool surfaceReady = false;
    bool practicePending = false;
    bool checkpointPending = false;
    bool fixtureCheckpointActive = false;
    bool capturePending = false;
    bool replayPending = false;
    bool benchmarkPendingOrActive = false;
    bool motionPendingOrActive = false;
    bool entryModeActive = false;
};

inline bool CanRequestCombatPractice(const DebugRequestAdmissionState& state)
{
    return state.surfaceReady && !state.practicePending && !state.checkpointPending &&
        !state.fixtureCheckpointActive &&
        !state.capturePending && !state.replayPending && !state.benchmarkPendingOrActive &&
        !state.motionPendingOrActive && !state.entryModeActive;
}

inline bool CanRequestCompetingDebugMode(const DebugRequestAdmissionState& state)
{
    return !state.practicePending;
}
} // namespace horde::platform::android
