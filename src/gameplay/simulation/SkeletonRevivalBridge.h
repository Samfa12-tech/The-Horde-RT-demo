#pragma once

#include <algorithm>
#include <cmath>
#include <cstdint>

namespace horde::gameplay::simulation
{
enum class SkeletonRevivalPhase : std::uint8_t
{
    Disabled, Practice, FirstFight, Signal, Rising, Rearming, Live, Complete
};

struct SkeletonRevivalInput
{
    bool practiceFinished = false;
    bool practiceSkipped = false;
    bool bothDead = false; // Actual two-actor owner state, never a lesson award.
    bool bothAlive = false; // Both same stable actor records, not merely !bothDead.
    bool playerAlive = false;
    bool paused = false;
    bool presentationReady = false;
    bool actorClearance = false;
    // Sampled only when consuming the signal, after presentation admission.
    float riseDurationSeconds = 0.0f;
};

struct SkeletonRevivalSnapshot
{
    bool enabled = false, consumed = false;
    SkeletonRevivalPhase phase = SkeletonRevivalPhase::Disabled;
    float phaseSeconds = 0.0f;
    std::uint64_t generation = 1u;
    bool signalPulse = false, revivePulse = false, waitingForClearance = false;

    bool SuppressesActors() const
    {
        return phase == SkeletonRevivalPhase::Signal || phase == SkeletonRevivalPhase::Rising ||
            phase == SkeletonRevivalPhase::Rearming;
    }
};

// Pure fixed-tick ownership only: no actors, poses, audio, saves or renderer.
// Integration must admit the presentation, suppress actor contact/targeting in
// the indicated phases, and apply revivePulse to the same two actors atomically.
// Explicit Skip retains the existing opt-out: no forced second fight. A missing
// presentation cannot produce a signal, a rising clock or fake completion.
class SkeletonRevivalBridge
{
public:
    static constexpr float kSignalLeadSeconds = 0.35f;
    static constexpr float kRearmingSeconds = 0.5f;
    static constexpr float kMaximumStepSeconds = 0.05f;

    void Reset(bool enabled = false)
    {
        const auto generation = NextGeneration();
        snapshot_ = {}; snapshot_.generation = generation; snapshot_.enabled = enabled;
        snapshot_.phase = enabled ? SkeletonRevivalPhase::Practice : SkeletonRevivalPhase::Disabled;
        riseDurationSeconds_ = 0.0f; livePairSeen_ = restoredCompleted_ = false;
    }
    void Retry(bool finished, bool skipped)
    {
        snapshot_.generation = NextGeneration(); CancelTransient(); riseDurationSeconds_ = 0.0f;
        livePairSeen_ = false;
        if (restoredCompleted_) { Enter(SkeletonRevivalPhase::Complete); return; }
        if (!snapshot_.enabled) { Enter(SkeletonRevivalPhase::Disabled); return; }
        if (snapshot_.consumed || skipped)
        {
            snapshot_.consumed = true; Enter(SkeletonRevivalPhase::Live);
            // The caller's ordinary retry initialises standing actors. Observe
            // that state before accepting a later final defeat; do not reuse
            // the pre-retry/pre-revive all-dead publication.
            return;
        }
        Enter(finished ? SkeletonRevivalPhase::FirstFight : SkeletonRevivalPhase::Practice);
    }
    void RestoreCompleted()
    {
        snapshot_.generation = NextGeneration(); CancelTransient(); snapshot_.consumed = true;
        riseDurationSeconds_ = 0.0f; livePairSeen_ = false; restoredCompleted_ = true;
        Enter(SkeletonRevivalPhase::Complete);
    }
    void CancelTransient() { snapshot_.signalPulse = snapshot_.revivePulse = false; }

    const SkeletonRevivalSnapshot& Step(float deltaSeconds, const SkeletonRevivalInput& input)
    {
        CancelTransient();
        if (!snapshot_.enabled || snapshot_.phase == SkeletonRevivalPhase::Complete ||
            !std::isfinite(deltaSeconds) || deltaSeconds <= 0.0f || input.paused || !input.playerAlive)
            return snapshot_;

        if (snapshot_.phase == SkeletonRevivalPhase::Practice || snapshot_.phase == SkeletonRevivalPhase::FirstFight)
        {
            if (input.practiceSkipped)
            {
                snapshot_.consumed = true; livePairSeen_ = true; Enter(SkeletonRevivalPhase::Live);
                return snapshot_; // The existing fight may already be defeated.
            }
            if (snapshot_.phase == SkeletonRevivalPhase::Practice)
            {
                if (!input.practiceFinished) return snapshot_;
                Enter(SkeletonRevivalPhase::FirstFight);
            }
            if (input.bothDead && input.presentationReady &&
                std::isfinite(input.riseDurationSeconds) && input.riseDurationSeconds > 0.0f)
            {
                // Commit consumption before publishing the one-shot signal.
                snapshot_.consumed = true; riseDurationSeconds_ = input.riseDurationSeconds;
                Enter(SkeletonRevivalPhase::Signal); snapshot_.signalPulse = true;
            }
            return snapshot_;
        }
        if (snapshot_.phase == SkeletonRevivalPhase::Live)
        {
            if (input.bothAlive && !input.bothDead) livePairSeen_ = true;
            else if (input.bothDead && !input.bothAlive && livePairSeen_) Enter(SkeletonRevivalPhase::Complete);
            return snapshot_;
        }
        if (!input.presentationReady) return snapshot_;

        float remaining = std::min(deltaSeconds, kMaximumStepSeconds);
        if (snapshot_.phase == SkeletonRevivalPhase::Signal)
        {
            if (!Advance(kSignalLeadSeconds, remaining)) return snapshot_;
            Enter(SkeletonRevivalPhase::Rising);
        }
        if (snapshot_.phase == SkeletonRevivalPhase::Rising)
        {
            if (!Advance(riseDurationSeconds_, remaining)) return snapshot_;
            Enter(SkeletonRevivalPhase::Rearming);
        }
        if (snapshot_.phase == SkeletonRevivalPhase::Rearming)
        {
            if (!Advance(kRearmingSeconds, remaining)) return snapshot_;
            if (!input.actorClearance) snapshot_.waitingForClearance = true;
            else
            {
                Enter(SkeletonRevivalPhase::Live); livePairSeen_ = false;
                snapshot_.revivePulse = true;
            }
        }
        return snapshot_;
    }
    const SkeletonRevivalSnapshot& Snapshot() const { return snapshot_; }

private:
    std::uint64_t NextGeneration() const
    { return snapshot_.generation == UINT64_MAX ? UINT64_MAX : snapshot_.generation + 1u; }
    void Enter(SkeletonRevivalPhase phase)
    { snapshot_.phase = phase; snapshot_.phaseSeconds = 0.0f; snapshot_.waitingForClearance = false; }
    bool Advance(float duration, float& remaining)
    {
        const float needed = std::max(0.0f, duration - snapshot_.phaseSeconds);
        if (remaining < needed) { snapshot_.phaseSeconds += remaining; remaining = 0.0f; return false; }
        remaining -= needed; snapshot_.phaseSeconds = duration; return true;
    }
    SkeletonRevivalSnapshot snapshot_{};
    float riseDurationSeconds_ = 0.0f;
    bool livePairSeen_ = false, restoredCompleted_ = false;
};
} // namespace horde::gameplay::simulation
