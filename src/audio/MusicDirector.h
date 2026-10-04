#pragma once

#include <cstdint>
#include <span>

#include "gameplay/simulation/SimulationSnapshot.h"

namespace horde::audio
{

enum class MusicCue : std::uint8_t
{
    None,
    A,
    B,
    C,
    D,
    E,
    F,
    G,
    H,
};

struct MusicSelection
{
    MusicCue cue = MusicCue::None;
    bool looping = false;
    bool suspended = false;
    bool discontinuity = false;
    bool clockValid = true;
    double positionSeconds = 0.0;
    std::uint64_t revision = 0;
    // Presentation envelope, multiplied by independent user music volume at
    // the existing device output. It never changes saved volume or cue PCM.
    float revealGain = 1.0f;
};

// Resolves shared gameplay state into a cue and musical position. It owns no
// playback resources and never mutates or drains the simulation event span.
// events is the existing queue's ordered, newly observed span, not a historical
// replay. eventLifetimeToken must change whenever the caller starts a new queue
// whose sequence numbers may restart. Route/retry resets retain that queue's
// sequence high-water mark. Call Reset for out-of-band checkpoint/session imports.
class MusicDirector
{
public:
    MusicSelection Update(
        const gameplay::simulation::SimulationSnapshot& snapshot,
        std::span<const gameplay::simulation::GameplayEvent> events,
        double monotonicAudioClockSeconds,
        std::uint64_t eventLifetimeToken,
        bool externallySuspended = false) noexcept;

    void Reset() noexcept;

private:
    void ResetSession(bool resetEventSequence = true) noexcept;
    void SetCue(MusicCue cue, bool looping, double positionSeconds,
                bool forceDiscontinuity, bool& discontinuity) noexcept;
    double ReconstructSkylightPosition(
        const gameplay::simulation::SimulationSnapshot& snapshot) const noexcept;
    MusicCue ResolvePersistentBed(
        const gameplay::simulation::SimulationSnapshot& snapshot) const noexcept;

    MusicCue cue_ = MusicCue::None;
    bool looping_ = false;
    double positionSeconds_ = 0.0;
    std::uint64_t revision_ = 0;
    bool forceDiscontinuity_ = true;

    bool lifetimeInitialized_ = false;
    std::uint64_t eventLifetimeToken_ = 0;
    std::uint64_t lastEventSequence_ = 0;
    bool routeIdentityInitialized_ = false;
    std::uint32_t retryGeneration_ = 0;
    std::uint64_t routeResetSequence_ = 0;

    bool skeletonEngaged_ = false;
    bool torchFailureLatched_ = false;
    bool torchStingPending_ = false;
    bool torchStingUsed_ = false;
    bool torchStingActive_ = false;

    bool clockInitialized_ = false;
    double lastAudioClockSeconds_ = 0.0;
    bool previousUpdateSuspended_ = false;
    float revealGain_ = 1.0f;
    bool finalePhaseInitialized_ = false;
    bool skylightHReached_ = false;
    gameplay::interactions::FinaleSequencePhase previousFinalePhase_ =
        gameplay::interactions::FinaleSequencePhase::Inactive;
};

} // namespace horde::audio
