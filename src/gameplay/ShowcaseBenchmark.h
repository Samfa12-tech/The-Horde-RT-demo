#pragma once

#include <array>
#include <cstddef>
#include <cstdint>
#include <string>
#include <vector>

#include "gameplay/ShowcaseReplay.h"
#include "gameplay/BenchmarkWorkload.h"

namespace horde::telemetry { class RtBenchmarkEvidenceRun; }

namespace horde::gameplay
{

enum class ShowcaseBenchmarkStatus
{
    Idle,
    Running,
    Complete,
    Failed,
    Cancelled,
};

struct ShowcaseBenchmarkAdvance
{
    ShowcaseReplaySnapshot replay;
    std::uint32_t frameInLap = 0u;
    bool lapStarted = false;
    bool lapCompleted = false;
    bool finished = false;
};

struct ShowcaseBenchmarkFrame
{
    double frameTimeMs = 0.0;
    ShowcaseZone zone = ShowcaseZone::Outside;
    std::uint32_t lap = 0u;
};

struct ShowcaseBenchmarkStatistics
{
    std::size_t frames = 0u;
    double averageMs = 0.0;
    double medianMs = 0.0;
    double p95Ms = 0.0;
    double onePercentLowFps = 0.0;
};

// Existing platform render-loop intervals, not GPU-only cost or display pacing.
struct ShowcaseBenchmarkLiveTiming
{
    std::size_t frames = 0u;
    double averageMs = 0.0;
    double fps = 0.0;
};

struct ShowcaseBenchmarkMetadata
{
    std::string runId;
    std::string timestampUtc;
    std::string buildIdentity;
    std::string shaderIdentity;
    std::string gpuName;
    std::string vulkanApi;
    std::string rtMode;
    std::string executionBackend;
    std::string presentMode;
    std::string legacyFrameTimingScope = "unspecified-platform-interval";
    std::string materialEncoding;
    std::uint32_t renderScalePercent = 100u;
    std::uint32_t internalWidth = 0u;
    std::uint32_t internalHeight = 0u;
    std::uint32_t presentationWidth = 0u;
    std::uint32_t presentationHeight = 0u;
};

class ShowcaseBenchmarkRun
{
public:
    static constexpr std::uint32_t kDefaultLaps = 2u;
    static constexpr std::uint32_t kMaximumFramesPerLap = 4000u;
    static constexpr std::size_t kLiveTimingWindowFrames = 60u;

    void Start(std::uint32_t laps = kDefaultLaps,
               BenchmarkWorkload workload = BenchmarkWorkload::ShowcaseRoute,
               bool showLiveFps = false);
    ShowcaseBenchmarkAdvance Advance();
    void RecordFrame(double frameTimeMs, bool rtFramePresented);
    void Cancel();

    ShowcaseBenchmarkStatus Status() const { return status_; }
    bool IsRunning() const { return status_ == ShowcaseBenchmarkStatus::Running; }
    bool HasStarted() const { return status_ != ShowcaseBenchmarkStatus::Idle; }
    bool Passed() const;
    std::uint32_t CurrentLap() const { return currentLap_; }
    std::uint32_t FrameInLap() const { return lapFrames_; }
    std::uint32_t CompletedLaps() const { return completedLaps_; }
    std::uint32_t TotalLaps() const { return totalLaps_; }
    std::size_t ReachedWaypoints() const { return IsLanternBenchmark(workload_) ? 0u : reachedWaypoints_; }
    bool PresentedEveryFrame() const { return presentedEveryFrame_; }
    BenchmarkWorkload Workload() const { return workload_; }
    const ShowcaseReplaySnapshot& ReplaySnapshot() const { return currentReplay_; }
    const std::vector<ShowcaseBenchmarkFrame>& Frames() const { return frames_; }

    ShowcaseBenchmarkStatistics OverallStatistics() const;
    ShowcaseBenchmarkStatistics ZoneStatistics(ShowcaseZone zone) const;
    std::string ProgressText() const;
    ShowcaseBenchmarkLiveTiming LiveTiming() const;
    bool ShowsLiveFps() const { return showLiveFps_ && IsRunning(); }
    bool ConsumeLiveProgressUpdate();
    std::string BuildTextReport(const ShowcaseBenchmarkMetadata& metadata,
        const horde::telemetry::RtBenchmarkEvidenceRun* evidence = nullptr) const;
    std::string BuildJsonReport(const ShowcaseBenchmarkMetadata& metadata,
        const horde::telemetry::RtBenchmarkEvidenceRun* evidence = nullptr) const;

private:
    ShowcaseBenchmarkStatistics StatisticsFor(ShowcaseZone zone, bool filterZone) const;
    void ResetLiveTiming();

    ShowcaseRouteReplay replay_;
    ShowcaseReplaySnapshot currentReplay_{};
    BenchmarkWorkload workload_ = BenchmarkWorkload::ShowcaseRoute;
    ShowcaseBenchmarkStatus status_ = ShowcaseBenchmarkStatus::Idle;
    std::uint32_t totalLaps_ = kDefaultLaps;
    std::uint32_t currentLap_ = 0u;
    std::uint32_t completedLaps_ = 0u;
    std::size_t reachedWaypoints_ = 0u;
    std::uint32_t lapFrames_ = 0u;
    bool pendingLapRestart_ = false;
    bool presentedEveryFrame_ = true;
    std::vector<ShowcaseBenchmarkFrame> frames_;
    // Opt-in interactive HUD only: no extra collector/formatting in automated or
    // frozen A/B runs, and no change to persisted benchmark evidence/statistics.
    bool showLiveFps_ = false;
    bool liveProgressDue_ = false;
    std::array<double, kLiveTimingWindowFrames> liveIntervals_{};
    std::size_t liveCount_ = 0u;
    std::size_t liveNext_ = 0u;
    double liveTotalMs_ = 0.0;
    double liveRefreshMs_ = 0.0;
};

const char* ShowcaseBenchmarkStatusName(ShowcaseBenchmarkStatus status);

} // namespace horde::gameplay
