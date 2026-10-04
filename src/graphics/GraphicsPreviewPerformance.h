#pragma once
#include <array>
#include <cmath>
#include <cstddef>
#include <cstdint>
#include <optional>

namespace horde::graphics
{
struct GraphicsPreviewPerformanceSample {
    double wallEndSeconds = 0.0, loopMilliseconds = 0.0, cpuRenderMilliseconds = 0.0;
    std::optional<double> gpuMilliseconds;
    bool successfulPresent = false, transition = false;
};
struct GraphicsPreviewPerformanceSnapshot {
    std::uint64_t scopeEpoch = 0u;
    std::size_t sampleCount = 0u, successfulPresentCount = 0u, transitionCount = 0u, stableSampleCount = 0u;
    double successfulPresentsPerSecond = 0.0, meanLoopMilliseconds = 0.0, meanCpuRenderMilliseconds = 0.0;
    std::optional<double> meanGpuMilliseconds;
    // Overlap is possible on shared-memory hardware: never sum these classifications.
    std::optional<std::uint64_t> trackedDeviceLocalBytes, trackedHostVisibleBytes;
    std::array<GraphicsPreviewPerformanceSample, 128u> samples{}; // Oldest first, transitions retained.
};
// One render-owner instance. Publish copied snapshots at UI cadence; no readback/logging.
class GraphicsPreviewPerformance {
public:
    void BeginScope(const std::uint64_t epoch) noexcept {
        epoch_ = epoch; count_ = next_ = 0u; cumulativeSeconds_ = 0.0;
        deviceBytes_.reset(); hostBytes_.reset();
    }
    void SetTrackedAllocations(const std::uint64_t deviceLocal, const std::uint64_t hostVisible) noexcept {
        deviceBytes_ = deviceLocal; hostBytes_ = hostVisible;
    }
    bool RecordFrame(const double loopWallSeconds, const double cpuRenderMilliseconds,
                     const std::optional<double> gpuMilliseconds, const bool successfulPresent,
                     const bool transition) noexcept {
        if (!std::isfinite(loopWallSeconds) || loopWallSeconds <= 0.0 ||
            !std::isfinite(cpuRenderMilliseconds) || cpuRenderMilliseconds < 0.0) return false;
        std::optional<double> gpu;
        if (gpuMilliseconds && std::isfinite(*gpuMilliseconds) && *gpuMilliseconds >= 0.0) gpu = gpuMilliseconds;
        if (!std::isfinite(cumulativeSeconds_ + loopWallSeconds)) return false;
        cumulativeSeconds_ += loopWallSeconds;
        samples_[next_] = {cumulativeSeconds_, loopWallSeconds * 1000.0, cpuRenderMilliseconds,
                          gpu, successfulPresent, transition};
        next_ = (next_ + 1u) % samples_.size();
        if (count_ < samples_.size()) ++count_;
        return true;
    }
    GraphicsPreviewPerformanceSnapshot Snapshot() const noexcept {
        GraphicsPreviewPerformanceSnapshot result;
        result.scopeEpoch = epoch_; result.sampleCount = count_;
        result.trackedDeviceLocalBytes = deviceBytes_; result.trackedHostVisibleBytes = hostBytes_;
        double wallSeconds = 0.0, gpuTotal = 0.0;
        std::size_t gpuCount = 0u;
        const auto first = (next_ + samples_.size() - count_) % samples_.size();
        for (std::size_t i = 0u; i < count_; ++i) {
            const auto& sample = samples_[(first + i) % samples_.size()];
            result.samples[i] = sample;
            wallSeconds += sample.loopMilliseconds / 1000.0;
            if (sample.successfulPresent) ++result.successfulPresentCount;
            if (sample.transition) { ++result.transitionCount; continue; }
            ++result.stableSampleCount;
            result.meanLoopMilliseconds += sample.loopMilliseconds;
            result.meanCpuRenderMilliseconds += sample.cpuRenderMilliseconds;
            if (sample.gpuMilliseconds) { gpuTotal += *sample.gpuMilliseconds; ++gpuCount; }
        }
        if (wallSeconds > 0.0) result.successfulPresentsPerSecond = result.successfulPresentCount / wallSeconds;
        if (result.stableSampleCount > 0u) {
            result.meanLoopMilliseconds /= result.stableSampleCount;
            result.meanCpuRenderMilliseconds /= result.stableSampleCount;
        }
        if (gpuCount > 0u) result.meanGpuMilliseconds = gpuTotal / gpuCount;
        return result;
    }
private:
    std::uint64_t epoch_ = 0u;
    std::size_t count_ = 0u, next_ = 0u;
    double cumulativeSeconds_ = 0.0;
    std::array<GraphicsPreviewPerformanceSample, 128u> samples_{};
    std::optional<std::uint64_t> deviceBytes_, hostBytes_;
};
} // namespace horde::graphics
