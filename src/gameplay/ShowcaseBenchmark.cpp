#include "gameplay/ShowcaseBenchmark.h"

#include <algorithm>
#include <array>
#include <cmath>
#include <iomanip>
#include <numeric>
#include <sstream>

namespace horde::gameplay
{
namespace
{

constexpr std::array<ShowcaseZone, 10> kReportedZones{{
    ShowcaseZone::Opening,
    ShowcaseZone::SkeletonRoom,
    ShowcaseZone::ShadowCorridor,
    ShowcaseZone::SkylightChamber,
    ShowcaseZone::YellowTorchBay,
    ShowcaseZone::BlueTorchBay,
    ShowcaseZone::RedTorchBay,
    ShowcaseZone::GreenTorchBay,
    ShowcaseZone::TransmissionThreshold,
    ShowcaseZone::Finale,
}};

std::string JsonEscape(const std::string& value)
{
    std::string escaped;
    escaped.reserve(value.size());
    for (const char character : value)
    {
        switch (character)
        {
        case '\\': escaped += "\\\\"; break;
        case '"': escaped += "\\\""; break;
        case '\n': escaped += "\\n"; break;
        case '\r': escaped += "\\r"; break;
        case '\t': escaped += "\\t"; break;
        default: escaped += character; break;
        }
    }
    return escaped;
}

double Percentile(const std::vector<double>& sorted, const double percentile)
{
    if (sorted.empty())
    {
        return 0.0;
    }
    const std::size_t index = std::min(
        sorted.size() - 1u,
        static_cast<std::size_t>(std::ceil(percentile * static_cast<double>(sorted.size()))) - 1u);
    return sorted[index];
}

double Median(const std::vector<double>& sorted)
{
    if (sorted.empty())
    {
        return 0.0;
    }
    const std::size_t middle = sorted.size() / 2u;
    return (sorted.size() & 1u) != 0u
        ? sorted[middle]
        : (sorted[middle - 1u] + sorted[middle]) * 0.5;
}

} // namespace

const char* ShowcaseBenchmarkStatusName(const ShowcaseBenchmarkStatus status)
{
    switch (status)
    {
    case ShowcaseBenchmarkStatus::Running: return "running";
    case ShowcaseBenchmarkStatus::Complete: return "complete";
    case ShowcaseBenchmarkStatus::Failed: return "failed";
    case ShowcaseBenchmarkStatus::Cancelled: return "cancelled";
    default: return "idle";
    }
}

void ShowcaseBenchmarkRun::Start(const std::uint32_t laps)
{
    totalLaps_ = std::max(1u, laps);
    currentLap_ = 1u;
    completedLaps_ = 0u;
    reachedWaypoints_ = 0u;
    lapFrames_ = 0u;
    pendingLapRestart_ = false;
    presentedEveryFrame_ = true;
    frames_.clear();
    replay_.Reset();
    status_ = ShowcaseBenchmarkStatus::Running;
}

ShowcaseBenchmarkAdvance ShowcaseBenchmarkRun::Advance()
{
    ShowcaseBenchmarkAdvance result;
    result.replay = replay_.Snapshot();
    if (!IsRunning())
    {
        result.finished = status_ == ShowcaseBenchmarkStatus::Complete ||
                          status_ == ShowcaseBenchmarkStatus::Failed;
        return result;
    }

    if (pendingLapRestart_)
    {
        replay_.Reset();
        currentLap_ = completedLaps_ + 1u;
        lapFrames_ = 0u;
        pendingLapRestart_ = false;
        result.lapStarted = true;
    }

    result.replay = replay_.Update();
    if (++lapFrames_ > 4000u)
    {
        status_ = ShowcaseBenchmarkStatus::Failed;
        result.finished = true;
        return result;
    }
    if (result.replay.waypointReached)
    {
        ++reachedWaypoints_;
    }
    if (result.replay.failed)
    {
        status_ = ShowcaseBenchmarkStatus::Failed;
        result.finished = true;
        return result;
    }
    if (result.replay.complete)
    {
        ++completedLaps_;
        result.lapCompleted = true;
        if (completedLaps_ >= totalLaps_)
        {
            status_ = ShowcaseBenchmarkStatus::Complete;
            result.finished = true;
        }
        else
        {
            pendingLapRestart_ = true;
        }
    }
    return result;
}

void ShowcaseBenchmarkRun::RecordFrame(const double frameTimeMs, const bool rtFramePresented)
{
    if (!HasStarted() || status_ == ShowcaseBenchmarkStatus::Cancelled ||
        currentLap_ < totalLaps_ || !std::isfinite(frameTimeMs) || frameTimeMs <= 0.0)
    {
        return;
    }
    frames_.push_back({frameTimeMs, replay_.Snapshot().zone, currentLap_});
    presentedEveryFrame_ = presentedEveryFrame_ && rtFramePresented;
}

void ShowcaseBenchmarkRun::Cancel()
{
    if (IsRunning())
    {
        status_ = ShowcaseBenchmarkStatus::Cancelled;
    }
}

bool ShowcaseBenchmarkRun::Passed() const
{
    return status_ == ShowcaseBenchmarkStatus::Complete &&
           completedLaps_ == totalLaps_ &&
           reachedWaypoints_ == static_cast<std::size_t>(totalLaps_) * kShowcaseReplayPath.size() &&
           presentedEveryFrame_ && !frames_.empty();
}

ShowcaseBenchmarkStatistics ShowcaseBenchmarkRun::StatisticsFor(const ShowcaseZone zone,
                                                                 const bool filterZone) const
{
    std::vector<double> values;
    values.reserve(frames_.size());
    for (const ShowcaseBenchmarkFrame& frame : frames_)
    {
        if (!filterZone || frame.zone == zone)
        {
            values.push_back(frame.frameTimeMs);
        }
    }
    ShowcaseBenchmarkStatistics statistics;
    statistics.frames = values.size();
    if (values.empty())
    {
        return statistics;
    }
    statistics.averageMs = std::accumulate(values.begin(), values.end(), 0.0) /
                           static_cast<double>(values.size());
    std::sort(values.begin(), values.end());
    statistics.medianMs = Median(values);
    statistics.p95Ms = Percentile(values, 0.95);
    const std::size_t slowFrameCount = std::max<std::size_t>(
        1u, static_cast<std::size_t>(std::ceil(static_cast<double>(values.size()) * 0.01)));
    const double slowFrameAverageMs = std::accumulate(values.end() - slowFrameCount,
                                                       values.end(), 0.0) /
                                      static_cast<double>(slowFrameCount);
    statistics.onePercentLowFps = slowFrameAverageMs > 0.0 ? 1000.0 / slowFrameAverageMs : 0.0;
    return statistics;
}

ShowcaseBenchmarkStatistics ShowcaseBenchmarkRun::OverallStatistics() const
{
    return StatisticsFor(ShowcaseZone::Outside, false);
}

ShowcaseBenchmarkStatistics ShowcaseBenchmarkRun::ZoneStatistics(const ShowcaseZone zone) const
{
    return StatisticsFor(zone, true);
}

std::string ShowcaseBenchmarkRun::ProgressText() const
{
    std::ostringstream out;
    out << "BENCHMARK " << std::max(1u, currentLap_) << '/' << totalLaps_
        << "  |  WAYPOINT " << replay_.Snapshot().reachedWaypoints << '/'
        << kShowcaseReplayPath.size()
        << "  |  " << ShowcaseZoneName(replay_.Snapshot().zone);
    return out.str();
}

std::string ShowcaseBenchmarkRun::BuildTextReport(const ShowcaseBenchmarkMetadata& metadata) const
{
    const ShowcaseBenchmarkStatistics overall = OverallStatistics();
    std::ostringstream out;
    out << "HORDE LANTERN RT - IN-APP BENCHMARK\n"
        << "====================================\n"
        << "Integrity: " << (Passed() ? "COMPLETE" : "INVALID") << '\n'
        << "Status: " << ShowcaseBenchmarkStatusName(status_) << '\n'
        << "Timestamp (UTC): " << metadata.timestampUtc << '\n'
        << "Build: " << metadata.buildIdentity << '\n'
        << "Shader: " << metadata.shaderIdentity << '\n'
        << "GPU: " << metadata.gpuName << '\n'
        << "Vulkan API: " << metadata.vulkanApi << '\n'
        << "RT mode: " << metadata.rtMode << '\n'
        << "Swapchain present mode: " << metadata.presentMode << '\n'
        << "RT presented every measured frame: " << (presentedEveryFrame_ ? "yes" : "no") << '\n'
        << "Material route: " << metadata.materialEncoding << '\n'
        << "Render scale: " << metadata.renderScalePercent << "%\n"
        << "Internal RT extent: " << metadata.internalWidth << 'x' << metadata.internalHeight << '\n'
        << "Presentation extent: " << metadata.presentationWidth << 'x' << metadata.presentationHeight << "\n\n"
        << "COURSE\n"
        << "Preset: deterministic 13-waypoint complete showcase route\n"
        << "Pass policy: lap 1 warm-up, final lap measured\n"
        << "Simulation: fixed 0.032 world units/frame and 1/60 second gameplay step\n"
        << "Laps completed: " << completedLaps_ << '/' << totalLaps_ << '\n'
        << "Waypoints reached: " << reachedWaypoints_ << '/'
        << static_cast<std::size_t>(totalLaps_) * kShowcaseReplayPath.size() << '\n'
        << "Measured frames: " << overall.frames << "\n\n"
        << std::fixed << std::setprecision(3)
        << "OVERALL FRAME TIME\n"
        << "Average: " << overall.averageMs << " ms\n"
        << "Median: " << overall.medianMs << " ms ("
        << (overall.medianMs > 0.0 ? 1000.0 / overall.medianMs : 0.0) << " FPS)\n"
        << "P95: " << overall.p95Ms << " ms\n"
        << "1% low: " << overall.onePercentLowFps << " FPS\n\n"
        << "PER-ZONE FRAME TIME\n"
        << "zone,frames,average_ms,median_ms,p95_ms,one_percent_low_fps\n";
    for (const ShowcaseZone zone : kReportedZones)
    {
        const ShowcaseBenchmarkStatistics statistics = ZoneStatistics(zone);
        out << ShowcaseZoneName(zone) << ',' << statistics.frames << ','
            << statistics.averageMs << ',' << statistics.medianMs << ','
            << statistics.p95Ms << ',' << statistics.onePercentLowFps << '\n';
    }
    out << "\nThis deterministic in-app course is intended for comparing Horde Lantern RT settings and builds.\n";
    return out.str();
}

std::string ShowcaseBenchmarkRun::BuildJsonReport(const ShowcaseBenchmarkMetadata& metadata) const
{
    const ShowcaseBenchmarkStatistics overall = OverallStatistics();
    std::ostringstream out;
    out << std::fixed << std::setprecision(4)
        << "{\n"
        << "  \"schema\": 1,\n"
        << "  \"result\": \"" << (Passed() ? "complete" : "invalid") << "\",\n"
        << "  \"status\": \"" << ShowcaseBenchmarkStatusName(status_) << "\",\n"
        << "  \"timestampUtc\": \"" << JsonEscape(metadata.timestampUtc) << "\",\n"
        << "  \"build\": \"" << JsonEscape(metadata.buildIdentity) << "\",\n"
        << "  \"shader\": \"" << JsonEscape(metadata.shaderIdentity) << "\",\n"
        << "  \"gpu\": \"" << JsonEscape(metadata.gpuName) << "\",\n"
        << "  \"vulkanApi\": \"" << JsonEscape(metadata.vulkanApi) << "\",\n"
        << "  \"rtMode\": \"" << JsonEscape(metadata.rtMode) << "\",\n"
        << "  \"presentMode\": \"" << JsonEscape(metadata.presentMode) << "\",\n"
        << "  \"materialEncoding\": \"" << JsonEscape(metadata.materialEncoding) << "\",\n"
        << "  \"presentedEveryFrame\": " << (presentedEveryFrame_ ? "true" : "false") << ",\n"
        << "  \"renderScalePercent\": " << metadata.renderScalePercent << ",\n"
        << "  \"internalExtent\": {\"width\": " << metadata.internalWidth
        << ", \"height\": " << metadata.internalHeight << "},\n"
        << "  \"presentationExtent\": {\"width\": " << metadata.presentationWidth
        << ", \"height\": " << metadata.presentationHeight << "},\n"
        << "  \"lapsCompleted\": " << completedLaps_ << ",\n"
        << "  \"lapsRequested\": " << totalLaps_ << ",\n"
        << "  \"waypointsReached\": " << reachedWaypoints_ << ",\n"
        << "  \"measuredFrames\": " << overall.frames << ",\n"
        << "  \"overall\": {\"averageMs\": " << overall.averageMs
        << ", \"medianMs\": " << overall.medianMs
        << ", \"p95Ms\": " << overall.p95Ms
        << ", \"onePercentLowFps\": " << overall.onePercentLowFps << "},\n"
        << "  \"zones\": [\n";
    for (std::size_t index = 0; index < kReportedZones.size(); ++index)
    {
        const ShowcaseZone zone = kReportedZones[index];
        const ShowcaseBenchmarkStatistics statistics = ZoneStatistics(zone);
        out << "    {\"name\": \"" << ShowcaseZoneName(zone) << "\", \"frames\": "
            << statistics.frames << ", \"averageMs\": " << statistics.averageMs
            << ", \"medianMs\": " << statistics.medianMs
            << ", \"p95Ms\": " << statistics.p95Ms
            << ", \"onePercentLowFps\": " << statistics.onePercentLowFps << "}"
            << (index + 1u < kReportedZones.size() ? "," : "") << '\n';
    }
    out << "  ]\n}\n";
    return out.str();
}

} // namespace horde::gameplay
