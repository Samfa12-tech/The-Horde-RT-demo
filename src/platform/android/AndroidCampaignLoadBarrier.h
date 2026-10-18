#pragma once

#include <cmath>
#include <cstdint>
#include <limits>

namespace horde::platform::android
{
// Owner-thread policy only. This class neither pauses a simulation nor certifies
// GPU output. The caller holds its menu/lifecycle pause while KeepsPaused() and
// supplies identities from the frame that actually completed RT presentation.
class AndroidCampaignLoadBarrier
{
public:
    enum class Phase { Idle, Preparing, AwaitingJavaRebase, AwaitingPresentation, Complete, Failure };

    struct Status
    {
        Phase phase = Phase::Idle;
        std::uint64_t request = 0;
        std::uint64_t surfaceGeneration = 0;
        std::uint64_t rebaseToken = 0;
        float yaw = 0.0f;
        float pitch = 0.0f;
        std::uint64_t loadedTick = 0;
        std::uint64_t loadedWorldGeneration = 0;
        bool loaded = false;

        bool MatchesRebasePose(float incomingYaw, float incomingPitch) const noexcept
        {
            // Match GameSimulation's restore publication floor exactly. Java
            // receives these float bits; a nearby pose cannot release that floor.
            return std::isfinite(incomingYaw) && std::isfinite(incomingPitch) &&
                incomingYaw == yaw && incomingPitch == pitch;
        }
    };

    const Status& State() const noexcept { return state_; }

    bool KeepsPaused() const noexcept
    {
        return state_.phase != Phase::Idle && state_.phase != Phase::Complete;
    }

    bool HasPendingCommittedLoad() const noexcept
    {
        return state_.loaded && KeepsPaused();
    }

    // Requests and surface generations are monotonic, nonzero caller identities.
    // A pending committed load cannot be superseded by a second load request.
    bool Begin(std::uint64_t request, std::uint64_t surfaceGeneration) noexcept
    {
        if (!request || request <= lastRequest_ || !surfaceGeneration ||
            surfaceGeneration < highestSurfaceGeneration_ ||
            (KeepsPaused() && (state_.loaded || state_.phase != Phase::Failure)) ||
            lastRebaseToken_ == std::numeric_limits<std::uint64_t>::max()) return false;
        lastRequest_ = request;
        highestSurfaceGeneration_ = surfaceGeneration;
        state_ = {};
        state_.phase = Phase::Preparing;
        state_.request = request;
        state_.surfaceGeneration = surfaceGeneration;
        return true;
    }

    // Call only after the matching logical restore has committed. Validate pose
    // and identities before the logical commit; failure here must not be treated
    // as permission to unpause an already restored simulation.
    bool CommitLoaded(std::uint64_t request, std::uint64_t surfaceGeneration,
        float yaw, float pitch, std::uint64_t loadedTick,
        std::uint64_t loadedWorldGeneration) noexcept
    {
        if (state_.phase != Phase::Preparing || state_.request != request ||
            !surfaceGeneration || state_.surfaceGeneration != surfaceGeneration ||
            !loadedWorldGeneration || !std::isfinite(yaw) || !std::isfinite(pitch) ||
            lastRebaseToken_ == std::numeric_limits<std::uint64_t>::max()) return false;
        state_.loaded = true;
        state_.yaw = yaw;
        state_.pitch = pitch;
        state_.loadedTick = loadedTick;
        state_.loadedWorldGeneration = loadedWorldGeneration;
        state_.rebaseToken = ++lastRebaseToken_;
        state_.phase = Phase::AwaitingJavaRebase;
        return true;
    }

    // The caller must first validate a newer neutral input publication whose
    // absolute camera pose matches State(). This token alone is not input proof.
    bool Acknowledge(std::uint64_t rebaseToken, std::uint64_t surfaceGeneration) noexcept
    {
        if (state_.phase != Phase::AwaitingJavaRebase || !rebaseToken ||
            state_.rebaseToken != rebaseToken || !surfaceGeneration ||
            state_.surfaceGeneration != surfaceGeneration) return false;
        state_.phase = Phase::AwaitingPresentation;
        return true;
    }

    bool Presented(std::uint64_t surfaceGeneration, std::uint64_t submittedSimulationTick,
        std::uint64_t submittedWorldGeneration) noexcept
    {
        if (state_.phase != Phase::AwaitingPresentation || !surfaceGeneration ||
            state_.surfaceGeneration != surfaceGeneration ||
            submittedWorldGeneration != state_.loadedWorldGeneration ||
            submittedSimulationTick < state_.loadedTick) return false;
        state_.phase = Phase::Complete;
        return true;
    }

    // Cancellation/failure revoke preparation only. Once committed, keep the
    // acknowledgement/presentation obligation and pause; there is no rollback.
    bool Cancel(std::uint64_t request) noexcept
    {
        if (!request || request != state_.request || state_.loaded ||
            (state_.phase != Phase::Preparing && state_.phase != Phase::Failure)) return false;
        state_ = {};
        return true;
    }

    bool Fail(std::uint64_t request) noexcept
    {
        if (!request || request != state_.request || state_.loaded ||
            state_.phase != Phase::Preparing) return false;
        state_.phase = Phase::Failure;
        state_.rebaseToken = 0;
        return true;
    }

    bool SurfaceRetired(std::uint64_t surfaceGeneration) noexcept
    {
        if (!surfaceGeneration || state_.surfaceGeneration != surfaceGeneration ||
            !KeepsPaused()) return false;
        if (!state_.loaded)
        {
            state_ = {};
            return true;
        }
        state_.surfaceGeneration = 0;
        state_.rebaseToken = 0;
        state_.phase = Phase::AwaitingJavaRebase;
        return true;
    }

    // Home can suspend a surface without replacing it. Call after lifecycle
    // suspension has retired outstanding work; a pre-suspend acknowledgement
    // must not release the restored game when that same surface resumes.
    bool InputSuspended(std::uint64_t surfaceGeneration) noexcept
    {
        if (!state_.loaded || !KeepsPaused() || !surfaceGeneration ||
            state_.surfaceGeneration != surfaceGeneration) return false;
        if (lastRebaseToken_ == std::numeric_limits<std::uint64_t>::max())
        {
            state_.phase = Phase::Failure;
            state_.rebaseToken = 0;
            return false;
        }
        state_.rebaseToken = ++lastRebaseToken_;
        state_.phase = Phase::AwaitingJavaRebase;
        return true;
    }
    // Renderer-only resource recreation can advance the retained game's world
    // identity without replacing its logical campaign. The caller must prove
    // that fact; arbitrary world/reset commands may not use this recovery path.
    bool ResourcesRecreated(std::uint64_t surfaceGeneration, std::uint64_t currentTick,
        std::uint64_t currentWorldGeneration) noexcept
    {
        if (!state_.loaded || !KeepsPaused() || !surfaceGeneration ||
            state_.surfaceGeneration != surfaceGeneration ||
            !currentWorldGeneration || currentWorldGeneration <= state_.loadedWorldGeneration ||
            currentTick < state_.loadedTick) return false;
        if (lastRebaseToken_ == std::numeric_limits<std::uint64_t>::max())
        {
            state_.phase = Phase::Failure;
            state_.rebaseToken = 0;
            return false;
        }
        state_.loadedTick = currentTick;
        state_.loadedWorldGeneration = currentWorldGeneration;
        state_.rebaseToken = ++lastRebaseToken_;
        state_.phase = Phase::AwaitingJavaRebase;
        return true;
    }
    // A pending committed game survives surface retirement. Reissue the camera
    // handshake on a strictly newer surface, rejecting every old acknowledgement
    // and presentation. Ordinary surface changes after Complete are not a load.
    bool ResumeOnSurface(std::uint64_t surfaceGeneration) noexcept
    {
        if (!state_.loaded || !KeepsPaused() || state_.surfaceGeneration != 0 ||
            !surfaceGeneration || surfaceGeneration <= highestSurfaceGeneration_) return false;
        if (lastRebaseToken_ == std::numeric_limits<std::uint64_t>::max())
        {
            state_.phase = Phase::Failure; // Never wrap an acknowledgement identity.
            return false;
        }
        highestSurfaceGeneration_ = surfaceGeneration;
        state_.surfaceGeneration = surfaceGeneration;
        state_.rebaseToken = ++lastRebaseToken_;
        state_.phase = Phase::AwaitingJavaRebase;
        return true;
    }

private:
    Status state_;
    std::uint64_t lastRequest_ = 0;
    std::uint64_t lastRebaseToken_ = 0;
    std::uint64_t highestSurfaceGeneration_ = 0;
};
}


