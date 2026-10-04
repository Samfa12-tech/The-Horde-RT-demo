#pragma once

#include <algorithm>
#include <cmath>
#include <cstdint>
#include <optional>

namespace horde::graphics
{
// Unadmitted CPU experiment: the application does not instantiate this
// controller or expose a DRS setting. Only owning, presented GPU timings can
// inform it; command-record duration is deliberately not an input.
struct DynamicResolutionObservation
{
    std::uint64_t submissionSequence = 0;
    double gpuMilliseconds = 0.0;
    double timestampResolutionMilliseconds = 0.0;
    std::uint64_t elapsedTicks = 0;
    bool validGpuTiming = false;
    bool successfullyPresented = false;
    bool foregroundGameplay = false;
    bool capped = false;
    bool transitionInProgress = false;
};

class BoundedDynamicResolution
{
public:
    explicit BoundedDynamicResolution(std::uint32_t initialPercent = 100,
                                     std::uint32_t minimumPercent = 50,
                                     std::uint32_t maximumPercent = 100,
                                     double targetMilliseconds = 1000.0 / 60.0)
        : minimum_(std::clamp(minimumPercent, 50u, 100u)),
          maximum_(std::clamp(maximumPercent, minimum_, 100u)),
          target_(std::isfinite(targetMilliseconds) && targetMilliseconds > 0.0
                      ? targetMilliseconds : 1000.0 / 60.0)
    {
        Reset(initialPercent);
    }

    void Reset(std::uint32_t effectivePercent)
    {
        effective_ = std::clamp(effectivePercent, minimum_, maximum_);
        pending_.reset();
        average_.reset();
        lastSequence_ = 0;
        dwell_ = slow_ = fast_ = 0;
    }

    std::optional<std::uint32_t> Observe(const DynamicResolutionObservation& sample)
    {
        // Query collection is asynchronous: duplicates/out-of-order slots must
        // never count toward dwell or hysteresis.
        if (sample.submissionSequence == 0 || sample.submissionSequence <= lastSequence_)
            return std::nullopt;
        lastSequence_ = sample.submissionSequence;
        const double tickDuration = static_cast<double>(sample.elapsedTicks) *
                                    sample.timestampResolutionMilliseconds;
        if (pending_ || !sample.validGpuTiming || !sample.successfullyPresented ||
            !sample.foregroundGameplay || sample.capped || sample.transitionInProgress ||
            !std::isfinite(sample.gpuMilliseconds) || sample.gpuMilliseconds <= 0.0 ||
            !std::isfinite(sample.timestampResolutionMilliseconds) ||
            sample.timestampResolutionMilliseconds <= 0.0 || sample.elapsedTicks < 32 ||
            sample.timestampResolutionMilliseconds > target_ * 0.01 ||
            !std::isfinite(tickDuration) ||
            std::abs(tickDuration-sample.gpuMilliseconds) >
                std::max(sample.timestampResolutionMilliseconds*2, sample.gpuMilliseconds*0.01))
        {
            // Neither stale background averages nor interrupted consecutive
            // streaks should change quality on resume.
            average_.reset();
            dwell_ = slow_ = fast_ = 0;
            return std::nullopt;
        }
        average_ = average_ ? *average_ * 0.8 + sample.gpuMilliseconds * 0.2
                            : sample.gpuMilliseconds;
        dwell_ = std::min(dwell_ + 1u, 30u);
        slow_ = *average_ > target_ * 1.08 ? std::min(slow_ + 1u, 3u) : 0u;
        fast_ = *average_ < target_ * 0.82 ? std::min(fast_ + 1u, 12u) : 0u;
        if (dwell_ < 30u) return std::nullopt;
        std::uint32_t requested = effective_;
        if (slow_ == 3u && effective_ > minimum_)
            requested = effective_ - std::min(5u, effective_ - minimum_);
        else if (fast_ == 12u && effective_ < maximum_)
            requested = effective_ + std::min(5u, maximum_ - effective_);
        if (requested != effective_) pending_ = requested;
        return pending_;
    }

    bool CompleteResize(std::uint32_t requestedPercent, bool succeeded)
    {
        if (!pending_ || *pending_ != requestedPercent) return false;
        if (succeeded) effective_ = requestedPercent;
        pending_.reset();
        average_.reset();
        dwell_ = slow_ = fast_ = 0;
        return true;
    }

    std::uint32_t EffectivePercent() const { return effective_; }
    std::optional<std::uint32_t> PendingPercent() const { return pending_; }

private:
    std::uint32_t minimum_, maximum_, effective_ = 100;
    double target_;
    std::uint64_t lastSequence_ = 0;
    std::uint32_t dwell_ = 0, slow_ = 0, fast_ = 0;
    std::optional<double> average_;
    std::optional<std::uint32_t> pending_;
};
} // namespace horde::graphics
