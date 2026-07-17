#pragma once

#include <cstddef>
#include <cstdint>
#include <string>
#include <vector>

#include "gameplay/ShowcaseReplay.h"

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

struct ShowcaseBenchmarkMetadata
{
    std::string timestampUtc;
    std::string buildIdentity;
    std::string shaderIdentity;
    std::string gpuName;
    std::string vulkanApi;
    std::string rtMode;
    std::string presentMode;
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

    void Start(std::uint32_t laps = kDefaultLaps);
    ShowcaseBenchmarkAdvance Advance();
    void RecordFrame(double frameTimeMs, bool rtFramePresented);
    void Cancel();

    ShowcaseBenchmarkStatus Status() const { return status_; }
    bool IsRunning() const { return status_ == ShowcaseBenchmarkStatus::Running; }
    bool HasStarted() const { return status_ != ShowcaseBenchmarkStatus::Idle; }
    bool Passed() const;
    std::uint32_t CurrentLap() const { return currentLap_; }
    std::uint32_t CompletedLaps() const { return completedLaps_; }
    std::uint32_t TotalLaps() const { return totalLaps_; }
    std::size_t ReachedWaypoints() const { return reachedWaypoints_; }
    bool PresentedEveryFrame() const { return presentedEveryFrame_; }
    const ShowcaseReplaySnapshot& ReplaySnapshot() const { return replay_.Snapshot(); }
    const std::vector<ShowcaseBenchmarkFrame>& Frames() const { return frames_; }

    ShowcaseBenchmarkStatistics OverallStatistics() const;
    ShowcaseBenchmarkStatistics ZoneStatistics(ShowcaseZone zone) const;
    std::string ProgressText() const;
    std::string BuildTextReport(const ShowcaseBenchmarkMetadata& metadata) const;
    std::string BuildJsonReport(const ShowcaseBenchmarkMetadata& metadata) const;

private:
    ShowcaseBenchmarkStatistics StatisticsFor(ShowcaseZone zone, bool filterZone) const;

    ShowcaseRouteReplay replay_;
    ShowcaseBenchmarkStatus status_ = ShowcaseBenchmarkStatus::Idle;
    std::uint32_t totalLaps_ = kDefaultLaps;
    std::uint32_t currentLap_ = 0u;
    std::uint32_t completedLaps_ = 0u;
    std::size_t reachedWaypoints_ = 0u;
    std::uint32_t lapFrames_ = 0u;
    bool pendingLapRestart_ = false;
    bool presentedEveryFrame_ = true;
    std::vector<ShowcaseBenchmarkFrame> frames_;
};

const char* ShowcaseBenchmarkStatusName(ShowcaseBenchmarkStatus status);

} // namespace horde::gameplay
