#include "telemetry/BenchmarkSummary.h"

#include <algorithm>
#include <cmath>
#include <utility>

namespace horde::telemetry
{
namespace
{
constexpr std::array<horde::gameplay::ShowcaseZone, kBenchmarkSummaryZoneCount> kZones{{
    horde::gameplay::ShowcaseZone::Opening, horde::gameplay::ShowcaseZone::SkeletonRoom,
    horde::gameplay::ShowcaseZone::ShadowCorridor, horde::gameplay::ShowcaseZone::SkylightChamber,
    horde::gameplay::ShowcaseZone::YellowTorchBay, horde::gameplay::ShowcaseZone::BlueTorchBay,
    horde::gameplay::ShowcaseZone::RedTorchBay, horde::gameplay::ShowcaseZone::GreenTorchBay,
    horde::gameplay::ShowcaseZone::TransmissionThreshold, horde::gameplay::ShowcaseZone::Finale}};

bool SameConfiguration(const BenchmarkSummaryConfiguration& a, const BenchmarkSummaryConfiguration& b)
{
    const auto& x = a.metadata;
    const auto& y = b.metadata;
    return a.platform == b.platform && a.water == b.water && a.dielectric == b.dielectric &&
        a.fire == b.fire && a.glassEnabled == b.glassEnabled && a.sceneEpoch == b.sceneEpoch &&
        a.measurementGeneration == b.measurementGeneration && x.buildIdentity == y.buildIdentity &&
        x.shaderIdentity == y.shaderIdentity && x.executionBackend == y.executionBackend &&
        x.gpuName == y.gpuName && x.vulkanApi == y.vulkanApi &&
        x.presentMode == y.presentMode && x.materialEncoding == y.materialEncoding &&
        x.legacyFrameTimingScope == y.legacyFrameTimingScope &&
        x.renderScalePercent == y.renderScalePercent && x.internalWidth == y.internalWidth &&
        x.internalHeight == y.internalHeight && x.presentationWidth == y.presentationWidth &&
        x.presentationHeight == y.presentationHeight;
}

bool ValidConfiguration(const BenchmarkSummaryConfiguration& c)
{
    const auto& m = c.metadata;
    const bool platform = c.platform == BenchmarkSummaryPlatform::Windows || c.platform == BenchmarkSummaryPlatform::Android;
    const auto expectedScope = c.platform == BenchmarkSummaryPlatform::Windows ?
        "windows-render-plus-rtlab-telemetry" : "android-render-entry-through-present";
    // Neutral positive percentage rounding, independent of a consuming build's
    // ordinary50/isolated33 scale admission macro.
    const bool scale = (m.renderScalePercent >= 50u && m.renderScalePercent <= 100u) ||
        m.renderScalePercent == 33u || m.renderScalePercent == 40u;
    const auto scaled = [&m](const std::uint32_t dimension) {
        return std::max(1u, static_cast<std::uint32_t>((std::uint64_t{dimension} * m.renderScalePercent + 50u) / 100u));
    };
    return platform && m.legacyFrameTimingScope == expectedScope && c.sceneEpoch != 0u &&
        !m.buildIdentity.empty() && m.buildIdentity.size() <= 128u &&
        !m.shaderIdentity.empty() && m.shaderIdentity.size() <= 512u &&
        !m.materialEncoding.empty() && m.materialEncoding.size() <= 128u &&
        m.gpuName.size() <= 128u && m.vulkanApi.size() <= 32u &&
        c.measurementGeneration != 0u && c.water <= RtWaterQuality::High &&
        c.dielectric <= RtDielectricQuality::High && c.fire <= BenchmarkSummaryFireQuality::High &&
        (m.executionBackend == "RayTracingPipeline" || m.executionBackend == "RayQueryCompute") &&
        (m.presentMode == "FIFO" || m.presentMode == "MAILBOX" || m.presentMode == "IMMEDIATE" ||
         m.presentMode == "FIFO_RELAXED") && scale &&
        m.internalWidth > 0u && m.internalHeight > 0u && m.internalWidth <= m.presentationWidth &&
        m.internalHeight <= m.presentationHeight && m.presentationWidth <= 16384u && m.presentationHeight <= 16384u &&
        m.internalWidth == scaled(m.presentationWidth) && m.internalHeight == scaled(m.presentationHeight);
}

bool ValidStatistics(const RtStageStatistics& s, const std::size_t expected)
{
    if (expected == 0u) return !s.valid && s.sampleCount == 0u;
    return s.valid && s.sampleCount == expected && std::isfinite(s.meanMilliseconds) &&
        std::isfinite(s.medianMilliseconds) && std::isfinite(s.p90Milliseconds) &&
        std::isfinite(s.p95Milliseconds) && std::isfinite(s.slowestOnePercentMeanMilliseconds) &&
        std::isfinite(s.onePercentLowFps) && s.meanMilliseconds >= 0.0 && s.medianMilliseconds >= 0.0 &&
        s.p90Milliseconds >= s.medianMilliseconds && s.p95Milliseconds >= s.p90Milliseconds &&
        s.slowestOnePercentMeanMilliseconds >= 0.0 && s.onePercentLowFps >= 0.0;
}

bool CollectPopulation(const horde::gameplay::ShowcaseBenchmarkRun& benchmark,
    const RtBenchmarkEvidenceRun& evidence, const bool zoneOnly, const std::uint32_t zone,
    BenchmarkSummaryPopulation& population)
{
    population.legacy = zoneOnly ? benchmark.ZoneStatistics(static_cast<horde::gameplay::ShowcaseZone>(zone)) :
        benchmark.OverallStatistics();
    population.frames = population.legacy.frames;
    const auto& legacy = population.legacy;
    if (!std::isfinite(legacy.averageMs) || !std::isfinite(legacy.medianMs) || !std::isfinite(legacy.p95Ms) ||
        !std::isfinite(legacy.onePercentLowFps) || legacy.averageMs < 0.0 || legacy.medianMs < 0.0 ||
        legacy.p95Ms < legacy.medianMs || legacy.onePercentLowFps < 0.0) return false;
    for (std::size_t i = 0u; i < kRtStageCount; ++i)
    {
        const auto stage = static_cast<RtStage>(i);
        const bool available = zoneOnly ? evidence.CpuZoneStatistics(zone, stage, population.cpu[i]) :
            evidence.CpuStatistics(stage, population.cpu[i]);
        if (available != (population.frames > 0u) || !ValidStatistics(population.cpu[i], population.frames)) return false;
    }
    std::size_t counted = 0u;
    std::array<std::uint64_t, horde::gameplay::ShowcaseBenchmarkRun::kMaximumFramesPerLap> gpuDurations{};
    for (std::size_t i = 0u; i < evidence.ExpectedCount(); ++i)
    {
        RtExpectedFrameRecord row{};
        if (!evidence.TryGetExpectedFrame(i, row)) return false;
        if (zoneOnly && row.tag.zone != zone) continue;
        ++counted;
        switch (row.gpuStatus)
        {
        case RtSampleStatus::Valid:
            if (!row.hasGpuDuration || population.gpuStatuses.valid >= gpuDurations.size()) return false;
            gpuDurations[population.gpuStatuses.valid++] = row.gpuDurationNanoseconds;
            break;
        case RtSampleStatus::Disabled: ++population.gpuStatuses.disabled; break;
        case RtSampleStatus::Unsupported: ++population.gpuStatuses.unsupported; break;
        case RtSampleStatus::CompiledOut: ++population.gpuStatuses.compiledOut; break;
        // No pending/error/stale GPU population may masquerade as completed statistics.
        default: return false;
        }
    }
    if (counted != population.frames) return false;
    // Existing full-evidence GpuStatistics deliberately requires every scoped
    // row to have valid timing. This bounded summary instead names its available
    // population explicitly and projects ONLY those exact completed-row durations;
    // unavailable rows remain in the denominator/status counts, never as zero.
    const bool available = ComputeRtDurationStatistics(
        std::span<std::uint64_t>(gpuDurations.data(), population.gpuStatuses.valid), population.gpu);
    return available == (population.gpuStatuses.valid > 0u) && ValidStatistics(population.gpu, population.gpuStatuses.valid);
}
} // namespace

bool IsBenchmarkSummaryUuid(const std::string_view uuid) noexcept
{
    if (uuid.size() != 36u || uuid[14] != '4' ||
        (uuid[19] != '8' && uuid[19] != '9' && uuid[19] != 'a' && uuid[19] != 'b')) return false;
    for (std::size_t i = 0u; i < uuid.size(); ++i)
    {
        if (i == 8u || i == 13u || i == 18u || i == 23u) { if (uuid[i] != '-') return false; }
        else if (!((uuid[i] >= '0' && uuid[i] <= '9') || (uuid[i] >= 'a' && uuid[i] <= 'f'))) return false;
    }
    return true;
}

FrozenBenchmarkSummary CaptureBenchmarkSummary(const horde::gameplay::ShowcaseBenchmarkRun& benchmark,
    const RtBenchmarkEvidenceRun& evidence, const BenchmarkSummaryConfiguration& measurementStart,
    const BenchmarkSummaryConfiguration& completion, const std::string_view runUuid,
    const std::string_view rawModel, const BenchmarkSummaryCooling cooling)
{
    FrozenBenchmarkSummary result;
    if (!IsBenchmarkSummaryUuid(runUuid)) { result.status_ = BenchmarkSummaryStatus::InvalidIdentity; return result; }
    if (!ValidConfiguration(measurementStart) || !ValidConfiguration(completion) ||
        !SameConfiguration(measurementStart, completion) || cooling > BenchmarkSummaryCooling::ExternalDeclared ||
        rawModel.size() > 128u || benchmark.TotalLaps() != horde::gameplay::ShowcaseBenchmarkRun::kDefaultLaps)
    { result.status_ = BenchmarkSummaryStatus::InvalidConfiguration; return result; }
    if (!benchmark.Passed() || !benchmark.PresentedEveryFrame() ||
        evidence.Status() != RtBenchmarkRunStatus::Complete || evidence.InvalidRun() ||
        evidence.SceneEpoch() != completion.sceneEpoch || evidence.MeasurementGeneration() != completion.measurementGeneration ||
        evidence.ExpectedCount() == 0u || evidence.ExpectedCount() > horde::gameplay::ShowcaseBenchmarkRun::kMaximumFramesPerLap ||
        evidence.CompletedCount() != evidence.ExpectedCount() || evidence.CpuAcceptedCount() != evidence.ExpectedCount() ||
        evidence.RejectedCount() != 0u || evidence.CancelledCount() != 0u || evidence.CpuRejectedCount() != 0u ||
        evidence.OutstandingCount() != 0u || evidence.PendingCompletionCount() != 0u)
    { result.status_ = BenchmarkSummaryStatus::Incomplete; return result; }
    if (benchmark.Frames().size() != evidence.ExpectedCount())
    { result.status_ = BenchmarkSummaryStatus::MismatchedPopulation; return result; }
    for (std::size_t i = 0u; i < benchmark.Frames().size(); ++i)
    {
        const auto& frame = benchmark.Frames()[i];
        RtExpectedFrameRecord row{};
        if (!evidence.TryGetExpectedFrame(i, row) || row.tag.lap != benchmark.TotalLaps() || row.tag.lap != frame.lap ||
            row.tag.zone != static_cast<std::uint32_t>(frame.zone) ||
            std::find(kZones.begin(), kZones.end(), frame.zone) == kZones.end() ||
            row.disposition != RtExpectedFrameDisposition::Completed || !row.cpuAccepted || !row.hasGpuStatus ||
            row.presentationOutcome != RtPresentationOutcome::Presented ||
            !std::isfinite(frame.frameTimeMs) || frame.frameTimeMs <= 0.0)
        { result.status_ = BenchmarkSummaryStatus::MismatchedPopulation; return result; }
    }
    BenchmarkSummaryData data;
    data.runUuid = runUuid;
    data.configuration.platform = completion.platform;
    data.configuration.water = completion.water;
    data.configuration.dielectric = completion.dielectric;
    data.configuration.fire = completion.fire;
    data.configuration.glassEnabled = completion.glassEnabled;
    data.configuration.sceneEpoch = completion.sceneEpoch;
    data.configuration.measurementGeneration = completion.measurementGeneration;
    const auto& m = completion.metadata;
    auto& copied = data.configuration.metadata;
    copied.buildIdentity = m.buildIdentity;
    copied.shaderIdentity = m.shaderIdentity;
    copied.gpuName = m.gpuName;
    copied.vulkanApi = m.vulkanApi;
    copied.executionBackend = m.executionBackend;
    copied.presentMode = m.presentMode;
    copied.legacyFrameTimingScope = m.legacyFrameTimingScope;
    copied.materialEncoding = m.materialEncoding;
    copied.renderScalePercent = m.renderScalePercent;
    copied.internalWidth = m.internalWidth;
    copied.internalHeight = m.internalHeight;
    copied.presentationWidth = m.presentationWidth;
    copied.presentationHeight = m.presentationHeight;
    // Never carry caller-supplied report paths, timestamp/run text or unrelated RT
    // diagnostics into this typed record. Selected hardware labels remain private.
    data.rawModel = rawModel;
    data.cooling = cooling;
    data.workload = benchmark.Workload();
    data.laps = benchmark.CompletedLaps();
    if (!CollectPopulation(benchmark, evidence, false, 0u, data.overall))
    { result.status_ = BenchmarkSummaryStatus::InvalidTiming; return result; }
    for (std::size_t i = 0u; i < kZones.size(); ++i)
    {
        data.zones[i].zone = kZones[i];
        if (!CollectPopulation(benchmark, evidence, true, static_cast<std::uint32_t>(kZones[i]), data.zones[i].population))
        { result.status_ = BenchmarkSummaryStatus::InvalidTiming; return result; }
    }
    result.data_ = std::move(data);
    result.status_ = BenchmarkSummaryStatus::Ready;
    return result;
}
} // namespace horde::telemetry
