#pragma once

#include <algorithm>
#include <cmath>
#include <cstdint>
#include <utility>

namespace horde::gameplay::simulation
{

class FixedStepRunner
{
public:
    static constexpr double kFixedDeltaSeconds = 1.0 / 60.0;
    static constexpr double kMaximumFrameContributionSeconds = 0.100;
    static constexpr std::uint32_t kMaximumStepsPerAdvance = 8u;

    template <typename StepFunction>
    std::uint32_t Advance(double frameDeltaSeconds, bool paused, StepFunction&& step)
    {
        ticksLastAdvance_ = 0u;
        resetRequested_ = false;
        if (paused)
        {
            accumulatorSeconds_ = 0.0;
            return 0u;
        }

        if (!std::isfinite(frameDeltaSeconds) || frameDeltaSeconds < 0.0)
        {
            frameDeltaSeconds = 0.0;
            ++overrunCount_;
        }
        if (frameDeltaSeconds > kMaximumFrameContributionSeconds)
        {
            frameDeltaSeconds = kMaximumFrameContributionSeconds;
            ++overrunCount_;
        }
        accumulatorSeconds_ += frameDeltaSeconds;

        constexpr double epsilon = 1.0e-12;
        while (accumulatorSeconds_ + epsilon >= kFixedDeltaSeconds &&
               ticksLastAdvance_ < kMaximumStepsPerAdvance)
        {
            std::forward<StepFunction>(step)(static_cast<float>(kFixedDeltaSeconds));
            ++ticksLastAdvance_;
            ++totalTicks_;
            if (resetRequested_)
            {
                accumulatorSeconds_ = 0.0;
                resetRequested_ = false;
                break;
            }
            accumulatorSeconds_ = std::max(0.0, accumulatorSeconds_ - kFixedDeltaSeconds);
        }

        if (accumulatorSeconds_ + epsilon >= kFixedDeltaSeconds)
        {
            accumulatorSeconds_ = std::fmod(accumulatorSeconds_, kFixedDeltaSeconds);
            ++overrunCount_;
        }
        return ticksLastAdvance_;
    }

    void ResetAccumulator()
    {
        accumulatorSeconds_ = 0.0;
        resetRequested_ = true;
    }

    double AccumulatorSeconds() const { return accumulatorSeconds_; }
    std::uint64_t OverrunCount() const { return overrunCount_; }
    std::uint64_t TotalTicks() const { return totalTicks_; }
    std::uint32_t TicksLastAdvance() const { return ticksLastAdvance_; }

private:
    double accumulatorSeconds_ = 0.0;
    std::uint64_t overrunCount_ = 0u;
    std::uint64_t totalTicks_ = 0u;
    std::uint32_t ticksLastAdvance_ = 0u;
    bool resetRequested_ = false;
};

} // namespace horde::gameplay::simulation
