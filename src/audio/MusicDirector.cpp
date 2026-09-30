#include "audio/MusicDirector.h"

#include <algorithm>
#include <cmath>

namespace horde::audio
{
namespace
{

using gameplay::EnemyKind;
using gameplay::LichPhase;
using gameplay::TorchFailurePhase;
using gameplay::interactions::FinaleSequencePhase;
using gameplay::simulation::EntityId;
using gameplay::simulation::GameplayEvent;
using gameplay::simulation::GameplayEventType;
using gameplay::simulation::SimulationSnapshot;

constexpr double kLoopSeconds = 12.0;
constexpr double kTorchStingSeconds = 3.0;
constexpr double kSkylightCueSeconds = 6.0;
constexpr double kSkylightOpeningSeconds = 4.5;
// Delta accumulation over irregular PCM chunks may land just below the exact
// body boundary. This is less than 0.00005 of a 48 kHz sample, not a timing window.
constexpr double kSampleBoundaryEpsilon = 0.000000001;

bool IsSkeleton(const EntityId id) noexcept
{
    return id == EntityId::SkeletonA || id == EntityId::SkeletonB;
}

bool IsActualTorchFailure(const SimulationSnapshot& snapshot) noexcept
{
    // Guttering is an authored transition, not extinguishment. In this
    // sequence the torch remains held until the falling phase begins.
    return !snapshot.torchFailure.heldByPlayer ||
           snapshot.torchFailure.phase == TorchFailurePhase::Falling ||
           snapshot.torchFailure.phase == TorchFailurePhase::Settled;
}

} // namespace

MusicSelection MusicDirector::Update(
    const SimulationSnapshot& snapshot,
    const std::span<const GameplayEvent> events,
    const double monotonicAudioClockSeconds,
    const std::uint64_t eventLifetimeToken,
    const bool externallySuspended) noexcept
{
    bool discontinuity = false;
    bool sessionChanged = false;

    if (!lifetimeInitialized_)
    {
        lifetimeInitialized_ = true;
        eventLifetimeToken_ = eventLifetimeToken;
    }
    else if (eventLifetimeToken_ != eventLifetimeToken)
    {
        eventLifetimeToken_ = eventLifetimeToken;
        ResetSession();
        sessionChanged = true;
    }

    if (!routeIdentityInitialized_)
    {
        routeIdentityInitialized_ = true;
        retryGeneration_ = snapshot.retryGeneration;
        routeResetSequence_ = snapshot.lastConsumedRouteResetSequence;
    }
    else if (retryGeneration_ != snapshot.retryGeneration ||
             routeResetSequence_ != snapshot.lastConsumedRouteResetSequence)
    {
        retryGeneration_ = snapshot.retryGeneration;
        routeResetSequence_ = snapshot.lastConsumedRouteResetSequence;
        ResetSession(false);
        sessionChanged = true;
    }

    const bool suspended = snapshot.paused || externallySuspended;
    double clockDelta = 0.0;
    bool clockValid = std::isfinite(monotonicAudioClockSeconds);
    const bool resuming = clockInitialized_ && previousUpdateSuspended_ && !suspended;

    if (sessionChanged)
    {
        clockInitialized_ = false;
        previousUpdateSuspended_ = suspended;
    }

    if (clockValid)
    {
        if (!clockInitialized_)
        {
            lastAudioClockSeconds_ = monotonicAudioClockSeconds;
            clockInitialized_ = true;
        }
        else if (monotonicAudioClockSeconds < lastAudioClockSeconds_)
        {
            // Rebase on a backwards clock but do not convert it into negative
            // elapsed audio time or replay a one-shot.
            lastAudioClockSeconds_ = monotonicAudioClockSeconds;
            clockValid = false;
            discontinuity = true;
        }
        else
        {
            if (!suspended && !previousUpdateSuspended_)
            {
                clockDelta = monotonicAudioClockSeconds - lastAudioClockSeconds_;
            }
            lastAudioClockSeconds_ = monotonicAudioClockSeconds;
        }
    }
    else
    {
        // The next finite clock sample becomes a baseline rather than
        // including an unknowable interval.
        clockInitialized_ = false;
        discontinuity = true;
    }
    previousUpdateSuspended_ = suspended;

    for (const GameplayEvent& event : events)
    {
        if (event.sequence == 0u || event.sequence <= lastEventSequence_)
        {
            continue;
        }
        lastEventSequence_ = event.sequence;

        if (event.type == GameplayEventType::TorchExtinguished)
        {
            torchStingPending_ = true;
        }

        if (snapshot.openingEncounterComplete ||
            snapshot.activeEnemyKind != EnemyKind::Skeleton)
        {
            continue;
        }

        const bool skeletonEvent = IsSkeleton(event.source) || IsSkeleton(event.target);
        if (!skeletonEvent)
        {
            continue;
        }

        if (event.type == GameplayEventType::EnemyAttackStarted ||
            event.type == GameplayEventType::EnemyHit ||
            event.type == GameplayEventType::EnemyDefeated ||
            (event.type == GameplayEventType::PlayerDamaged &&
             IsSkeleton(event.source)))
        {
            skeletonEngaged_ = true;
        }
    }

    if (snapshot.openingEncounterComplete)
    {
        skeletonEngaged_ = false;
    }
    else if (snapshot.activeEnemyKind == EnemyKind::Skeleton &&
             IsSkeleton(snapshot.skeletonAttackerId))
    {
        skeletonEngaged_ = true;
    }

    torchFailureLatched_ = torchFailureLatched_ || IsActualTorchFailure(snapshot);

    const FinaleSequencePhase finalePhase = snapshot.finale.phase;
    const bool endingFlowStarted = snapshot.finale.lichDefeated ||
                                   finalePhase != FinaleSequencePhase::Inactive;
    const bool dead = !snapshot.playerAlive;
    if (dead || endingFlowStarted)
    {
        torchStingPending_ = false;
        torchStingActive_ = false;
    }

    if (dead)
    {
        SetCue(MusicCue::None, false, 0.0, discontinuity, discontinuity);
    }
    else if (skylightHReached_)
    {
        torchStingActive_ = false;
        const double nextPosition = cue_ == MusicCue::H && looping_
            ? std::fmod(positionSeconds_ + clockDelta, kLoopSeconds)
            : 0.0;
        SetCue(MusicCue::H, true, nextPosition, false, discontinuity);
    }
    else if (finalePhase == FinaleSequencePhase::Complete || snapshot.finaleComplete)
    {
        torchStingActive_ = false;
        skylightHReached_ = true;
        SetCue(MusicCue::H, true, 0.0, false, discontinuity);
    }
    else if (finalePhase == FinaleSequencePhase::SkylightOpening ||
             finalePhase == FinaleSequencePhase::DawnRevealed)
    {
        torchStingActive_ = false;
        double skylightPosition = ReconstructSkylightPosition(snapshot);
        const bool phaseChanged = !finalePhaseInitialized_ || finalePhase != previousFinalePhase_;
        const bool seekFromSnapshot = phaseChanged || resuming || !clockValid || sessionChanged;
        if (cue_ == MusicCue::G && !seekFromSnapshot)
        {
            skylightPosition = positionSeconds_ + clockDelta;
        }
        if (skylightPosition + kSampleBoundaryEpsilon >= kSkylightCueSeconds)
        {
            const double remainder = std::max(0.0, skylightPosition - kSkylightCueSeconds);
            skylightHReached_ = true;
            SetCue(MusicCue::H, true, remainder, seekFromSnapshot, discontinuity);
        }
        else
        {
            SetCue(MusicCue::G, false, skylightPosition, seekFromSnapshot, discontinuity);
        }
    }
    else if (snapshot.finale.lichDefeated || snapshot.lich.phase == LichPhase::Dead)
    {
        torchStingActive_ = false;
        const double nextPosition = cue_ == MusicCue::F && looping_
            ? std::fmod(positionSeconds_ + clockDelta, kLoopSeconds)
            : 0.0;
        SetCue(MusicCue::F, true, nextPosition, false, discontinuity);
    }
    else if (torchStingPending_ && torchFailureLatched_ && !torchStingUsed_)
    {
        torchStingPending_ = false;
        torchStingUsed_ = true;
        torchStingActive_ = true;
        SetCue(MusicCue::C, false, 0.0, true, discontinuity);
    }
    else if (torchStingActive_)
    {
        const double nextPosition = positionSeconds_ + clockDelta;
        if (nextPosition + kSampleBoundaryEpsilon >= kTorchStingSeconds)
        {
            torchStingActive_ = false;
            const MusicCue bed = ResolvePersistentBed(snapshot);
            const double remainder = std::max(0.0, nextPosition - kTorchStingSeconds);
            const double bedPosition = bed == MusicCue::None
                ? 0.0 : std::fmod(remainder, kLoopSeconds);
            SetCue(bed, bed != MusicCue::None, bedPosition, false, discontinuity);
        }
        else
        {
            SetCue(MusicCue::C, false, nextPosition, false, discontinuity);
        }
    }
    else
    {
        const MusicCue bed = ResolvePersistentBed(snapshot);
        double nextPosition = 0.0;
        if (bed == cue_ && looping_)
        {
            nextPosition = std::fmod(positionSeconds_ + clockDelta, kLoopSeconds);
        }
        SetCue(bed, bed != MusicCue::None, nextPosition, false, discontinuity);
    }

    finalePhaseInitialized_ = true;
    previousFinalePhase_ = finalePhase;

    if (forceDiscontinuity_)
    {
        discontinuity = true;
        forceDiscontinuity_ = false;
    }

    return {
        .cue = cue_,
        .looping = looping_,
        .suspended = suspended,
        .discontinuity = discontinuity,
        .clockValid = clockValid,
        .positionSeconds = positionSeconds_,
        .revision = revision_,
    };
}

void MusicDirector::Reset() noexcept
{
    ResetSession();
    lifetimeInitialized_ = false;
    routeIdentityInitialized_ = false;
    clockInitialized_ = false;
    finalePhaseInitialized_ = false;
}

void MusicDirector::ResetSession(const bool resetEventSequence) noexcept
{
    if (resetEventSequence)
    {
        lastEventSequence_ = 0u;
    }
    skeletonEngaged_ = false;
    torchFailureLatched_ = false;
    torchStingPending_ = false;
    torchStingUsed_ = false;
    torchStingActive_ = false;
    if (cue_ != MusicCue::None || looping_ || positionSeconds_ != 0.0)
    {
        ++revision_;
    }
    cue_ = MusicCue::None;
    looping_ = false;
    positionSeconds_ = 0.0;
    clockInitialized_ = false;
    finalePhaseInitialized_ = false;
    skylightHReached_ = false;
    forceDiscontinuity_ = true;
}

void MusicDirector::SetCue(const MusicCue cue,
                           const bool looping,
                           const double positionSeconds,
                           const bool forceDiscontinuity,
                           bool& discontinuity) noexcept
{
    const double safePosition = std::isfinite(positionSeconds)
        ? std::max(0.0, positionSeconds) : 0.0;
    const bool changed = cue_ != cue || looping_ != looping;
    const bool seek = forceDiscontinuity &&
                      std::abs(positionSeconds_ - safePosition) > 0.000001;
    if (changed || seek)
    {
        ++revision_;
        discontinuity = true;
    }
    cue_ = cue;
    looping_ = looping;
    positionSeconds_ = cue == MusicCue::None
        ? 0.0
        : (looping ? std::fmod(safePosition, kLoopSeconds) : safePosition);
}

double MusicDirector::ReconstructSkylightPosition(
    const SimulationSnapshot& snapshot) const noexcept
{
    const double phaseTime = std::isfinite(snapshot.finale.phaseTime)
        ? std::max(0.0, static_cast<double>(snapshot.finale.phaseTime)) : 0.0;
    if (snapshot.finale.phase == FinaleSequencePhase::DawnRevealed)
    {
        return kSkylightOpeningSeconds + phaseTime;
    }
    return phaseTime;
}

MusicCue MusicDirector::ResolvePersistentBed(
    const SimulationSnapshot& snapshot) const noexcept
{
    if (snapshot.finale.phase == FinaleSequencePhase::SkylightOpening ||
        snapshot.finale.phase == FinaleSequencePhase::DawnRevealed ||
        snapshot.finale.phase == FinaleSequencePhase::Complete ||
        snapshot.finaleComplete)
    {
        return MusicCue::H;
    }
    if (snapshot.finale.lichDefeated || snapshot.lich.phase == LichPhase::Dead)
    {
        return MusicCue::F;
    }
    if (snapshot.activeEnemyKind == EnemyKind::Lich &&
        (snapshot.lich.phase == LichPhase::MaintainingRange ||
         snapshot.lich.phase == LichPhase::Charging ||
         snapshot.lich.phase == LichPhase::Recovering))
    {
        return MusicCue::E;
    }
    if (snapshot.activeEnemyKind == EnemyKind::Skeleton &&
        !snapshot.openingEncounterComplete && skeletonEngaged_)
    {
        return MusicCue::B;
    }
    return torchFailureLatched_ ? MusicCue::D : MusicCue::A;
}

} // namespace horde::audio
