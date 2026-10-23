#pragma once

#include "gameplay/BenchmarkProfile.h"
#include "platform/windows/WindowsOutputResizeLaunch.h"

namespace horde::platform::windows
{
enum class WindowsBenchmarkProfilePhase { Idle, Applying, Measuring, Restoring, Restored, Failed };

// This owns only the nonpersistent tuple and acknowledgement protocol. The
// platform owns the HWND/resources and must arm the exact just-presented owner,
// drain it, and verify both its RT copy and window placement before observing.
class WindowsBenchmarkProfileSession
{
public:
    bool Begin(const gameplay::BenchmarkProfileConfiguration& original,
               const gameplay::BenchmarkProfileId profile = gameplay::BenchmarkProfileId::WindowsProvisionalV1) noexcept
    {
        if (phase_ != WindowsBenchmarkProfilePhase::Idle) return false;
        working_ = original; original_ = original;
        if (!run_.Begin(profile, working_)) return false;
        phase_ = WindowsBenchmarkProfilePhase::Applying; return true;
    }
    bool Arm(const OutputResizeOwnedFrame& owner) noexcept
    {
        if ((phase_ != WindowsBenchmarkProfilePhase::Applying && phase_ != WindowsBenchmarkProfilePhase::Restoring) ||
            armed_ || !IsCurrentOutputResizeCompletion(owner, owner, 1)) return false;
        expectedOwner_ = owner; armed_ = true; return true;
    }
    bool Observe(const gameplay::BenchmarkProfileConfiguration& actual,
                 const OutputResizeOwnedFrame& completed, const std::uint64_t completionSerial) noexcept
    {
        if (!armed_ || !IsCurrentOutputResizeCompletion(expectedOwner_, completed, completionSerial) ||
            completionSerial <= previousCompletion_) return false;
        if (phase_ == WindowsBenchmarkProfilePhase::Applying) {
            if (!run_.Observe(actual)) return false;
            phase_ = WindowsBenchmarkProfilePhase::Measuring;
        } else if (phase_ == WindowsBenchmarkProfilePhase::Restoring) {
            if (!RestorationMatches(actual)) return false;
            phase_ = WindowsBenchmarkProfilePhase::Restored;
        } else return false;
        previousCompletion_ = completionSerial; armed_ = false; return true;
    }
    bool ObserveMeasuring(const gameplay::BenchmarkProfileConfiguration& actual) noexcept
    { return phase_ == WindowsBenchmarkProfilePhase::Measuring && run_.Observe(actual); }
    bool BeginRestore(const bool measurementPassed, const bool cancelled) noexcept
    {
        if (phase_ != WindowsBenchmarkProfilePhase::Applying && phase_ != WindowsBenchmarkProfilePhase::Measuring) return false;
        measured_ = measurementPassed && phase_ == WindowsBenchmarkProfilePhase::Measuring;
        if (measured_) measured_ = run_.Complete();
        else if (cancelled) run_.Cancel();
        else run_.Fail();
        working_ = original_; armed_ = false;
        phase_ = WindowsBenchmarkProfilePhase::Restoring; return true;
    }
    void RestorationFailed() noexcept
    { run_.Fail(); phase_ = WindowsBenchmarkProfilePhase::Failed; measured_ = false; }
    WindowsBenchmarkProfilePhase Phase() const noexcept { return phase_; }
    const gameplay::BenchmarkProfileConfiguration& Working() const noexcept { return working_; }
    bool Succeeded() const noexcept { return phase_ == WindowsBenchmarkProfilePhase::Restored && measured_; }
    bool Restored() const noexcept { return phase_ == WindowsBenchmarkProfilePhase::Restored; }
private:
    bool RestorationMatches(gameplay::BenchmarkProfileConfiguration actual) const noexcept
    {
        if (!gameplay::ValidBenchmarkProfileConfiguration(actual) ||
            std::abs(actual.cameraAspect - original_.cameraAspect) > 1e-5) return false;
        actual.cameraAspect = original_.cameraAspect;
        return actual == original_;
    }
    // The run's borrowed working tuple outlives its destructor.
    gameplay::BenchmarkProfileConfiguration working_{}, original_{};
    gameplay::BenchmarkProfileRun run_;
    WindowsBenchmarkProfilePhase phase_ = WindowsBenchmarkProfilePhase::Idle;
    OutputResizeOwnedFrame expectedOwner_{};
    std::uint64_t previousCompletion_ = 0;
    bool armed_ = false, measured_ = false;
};
}
