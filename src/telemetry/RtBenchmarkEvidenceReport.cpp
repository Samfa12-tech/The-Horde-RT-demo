#include "telemetry/RtBenchmarkEvidenceReport.h"

#include <algorithm>
#include <array>
#include <cstddef>
#include <cstdint>
#include <iomanip>
#include <locale>
#include <sstream>
#include <string>
#include <string_view>
#include <vector>

#include "gameplay/ShowcaseRoute.h"
#include "telemetry/RtPerformanceEvidence.h"

namespace horde::telemetry
{
namespace
{

const char* RunStatusName(const RtBenchmarkRunStatus status) noexcept
{
    switch (status)
    {
    case RtBenchmarkRunStatus::Empty: return "empty";
    case RtBenchmarkRunStatus::Allocated: return "allocated";
    case RtBenchmarkRunStatus::Measuring: return "measuring";
    case RtBenchmarkRunStatus::Complete: return "complete";
    case RtBenchmarkRunStatus::Incomplete: return "incomplete";
    case RtBenchmarkRunStatus::Cancelled: return "cancelled";
    case RtBenchmarkRunStatus::Invalid: return "invalid";
    default: return "unknown";
    }
}

const char* FailureReasonName(const RtBenchmarkFailureReason reason) noexcept
{
    switch (reason)
    {
    case RtBenchmarkFailureReason::None: return "none";
    case RtBenchmarkFailureReason::InvalidCapacity: return "invalid-capacity";
    case RtBenchmarkFailureReason::CapacityOverflow: return "capacity-overflow";
    case RtBenchmarkFailureReason::AllocationFailed: return "allocation-failed";
    case RtBenchmarkFailureReason::InvalidState: return "invalid-state";
    case RtBenchmarkFailureReason::InvalidMeasurementIdentity:
        return "invalid-measurement-identity";
    case RtBenchmarkFailureReason::CapacityExceeded: return "capacity-exceeded";
    case RtBenchmarkFailureReason::InvalidFrameTag: return "invalid-frame-tag";
    case RtBenchmarkFailureReason::InvalidExpectedIndex: return "invalid-expected-index";
    case RtBenchmarkFailureReason::DuplicateDisposition: return "duplicate-disposition";
    case RtBenchmarkFailureReason::InvalidSubmittedIdentity:
        return "invalid-submitted-identity";
    case RtBenchmarkFailureReason::PendingSlotOccupied: return "pending-slot-occupied";
    case RtBenchmarkFailureReason::SubmissionFailed: return "submission-failed";
    case RtBenchmarkFailureReason::PresentationFailed: return "presentation-failed";
    case RtBenchmarkFailureReason::PresentedNeedsRecreate:
        return "presented-needs-recreate";
    case RtBenchmarkFailureReason::TokenlessCompletion: return "tokenless-completion";
    case RtBenchmarkFailureReason::InvalidCompletion: return "invalid-completion";
    case RtBenchmarkFailureReason::InvalidCompletionIdentity:
        return "invalid-completion-identity";
    case RtBenchmarkFailureReason::DuplicateCompletion: return "duplicate-completion";
    case RtBenchmarkFailureReason::StaleCompletion: return "stale-completion";
    case RtBenchmarkFailureReason::MismatchedCompletion: return "mismatched-completion";
    case RtBenchmarkFailureReason::CpuStageError: return "cpu-stage-error";
    case RtBenchmarkFailureReason::DiagnosticFailed: return "diagnostic-failed";
    case RtBenchmarkFailureReason::CpuIneligible: return "cpu-ineligible";
    case RtBenchmarkFailureReason::CpuCollectorRejected: return "cpu-collector-rejected";
    case RtBenchmarkFailureReason::MissingCompletion: return "missing-completion";
    case RtBenchmarkFailureReason::DrainFailed: return "drain-failed";
    case RtBenchmarkFailureReason::Cancelled: return "cancelled";
    case RtBenchmarkFailureReason::Count: break;
    }
    return "unknown";
}

const char* DispositionName(const RtExpectedFrameDisposition disposition) noexcept
{
    switch (disposition)
    {
    case RtExpectedFrameDisposition::AwaitingSubmission: return "awaiting-submission";
    case RtExpectedFrameDisposition::PendingCompletion: return "pending-completion";
    case RtExpectedFrameDisposition::Completed: return "completed";
    case RtExpectedFrameDisposition::Rejected: return "rejected";
    case RtExpectedFrameDisposition::Cancelled: return "cancelled";
    default: return "unknown";
    }
}

std::string ZoneName(const std::uint32_t zone)
{
    using horde::gameplay::ShowcaseZone;
    switch (static_cast<ShowcaseZone>(zone))
    {
    case ShowcaseZone::Outside:
    case ShowcaseZone::Opening:
    case ShowcaseZone::SkeletonRoom:
    case ShowcaseZone::ShadowCorridor:
    case ShowcaseZone::SkylightChamber:
    case ShowcaseZone::YellowTorchBay:
    case ShowcaseZone::BlueTorchBay:
    case ShowcaseZone::RedTorchBay:
    case ShowcaseZone::GreenTorchBay:
    case ShowcaseZone::TransmissionThreshold:
    case ShowcaseZone::Finale:
        return horde::gameplay::ShowcaseZoneName(static_cast<ShowcaseZone>(zone));
    default:
        return "unknown-" + std::to_string(zone);
    }
}

const char* SampleStatusName(const RtSampleStatus status) noexcept
{
    const char* name = RtSampleStatusName(status);
    return name != nullptr ? name : "unknown";
}

const char* PresentationName(const RtPresentationOutcome outcome) noexcept
{
    const char* name = RtPresentationOutcomeName(outcome);
    return name != nullptr ? name : "unknown";
}

const char* StageName(const RtStage stage) noexcept
{
    const char* name = RtStageMetricName(stage);
    return name != nullptr ? name : "unknown";
}

void WriteJsonString(std::ostream& out, const std::string_view value)
{
    constexpr char hex[] = "0123456789abcdef";
    out << '"';
    for (const unsigned char character : value)
    {
        switch (character)
        {
        case '\\': out << "\\\\"; break;
        case '"': out << "\\\""; break;
        case '\b': out << "\\b"; break;
        case '\f': out << "\\f"; break;
        case '\n': out << "\\n"; break;
        case '\r': out << "\\r"; break;
        case '\t': out << "\\t"; break;
        default:
            if (character < 0x20u)
            {
                out << "\\u00" << hex[character >> 4u] << hex[character & 0x0fu];
            }
            else
            {
                out << static_cast<char>(character);
            }
            break;
        }
    }
    out << '"';
}

void WriteJsonStatistics(std::ostream& out,
                         const bool available,
                         const RtStageStatistics& statistics,
                         const bool includeFrameRate)
{
    if (!available || !statistics.valid || statistics.sampleCount == 0u)
    {
        out << "null";
        return;
    }
    out << "{\"sampleCount\": " << statistics.sampleCount
        << ", \"meanMilliseconds\": " << statistics.meanMilliseconds
        << ", \"medianMilliseconds\": " << statistics.medianMilliseconds
        << ", \"p90Milliseconds\": " << statistics.p90Milliseconds
        << ", \"p95Milliseconds\": " << statistics.p95Milliseconds
        << ", \"slowestOnePercentMeanMilliseconds\": "
        << statistics.slowestOnePercentMeanMilliseconds;
    if (includeFrameRate)
    {
        out << ", \"onePercentLowFps\": " << statistics.onePercentLowFps;
    }
    out << '}';
}

void WriteTextStatistics(std::ostream& out,
                         const bool available,
                         const RtStageStatistics& statistics,
                         const bool includeFrameRate)
{
    if (!available || !statistics.valid || statistics.sampleCount == 0u)
    {
        out << "N/A";
        return;
    }
    out << "samples=" << statistics.sampleCount
        << " mean=" << statistics.meanMilliseconds << " ms"
        << " median=" << statistics.medianMilliseconds << " ms"
        << " p90=" << statistics.p90Milliseconds << " ms"
        << " p95=" << statistics.p95Milliseconds << " ms"
        << " slowest1%Mean=" << statistics.slowestOnePercentMeanMilliseconds << " ms";
    if (includeFrameRate)
    {
        out << " onePercentLow=" << statistics.onePercentLowFps << " FPS";
    }
}

void WriteJsonGpuCounts(std::ostream& out,
                        const RtGpuSampleStatusCounts& counts,
                        const std::size_t denominator)
{
    out << "{\"denominator\": " << denominator
        << ", \"not-ready\": " << counts.notReady
        << ", \"compiled-out\": " << counts.compiledOut
        << ", \"unknown\": " << counts.unknown
        << ", \"valid\": " << counts.valid
        << ", \"disabled\": " << counts.disabled
        << ", \"unsupported\": " << counts.unsupported
        << ", \"pending\": " << counts.pending
        << ", \"error\": " << counts.error
        << ", \"missing\": " << counts.missing << '}';
}

void IncrementGpuCount(RtGpuSampleStatusCounts& counts,
                       const RtExpectedFrameRecord& row) noexcept
{
    if (!row.hasGpuStatus)
    {
        ++counts.missing;
        return;
    }
    switch (row.gpuStatus)
    {
    case RtSampleStatus::NotReady: ++counts.notReady; break;
    case RtSampleStatus::CompiledOut: ++counts.compiledOut; break;
    case RtSampleStatus::Disabled: ++counts.disabled; break;
    case RtSampleStatus::Unsupported: ++counts.unsupported; break;
    case RtSampleStatus::Pending: ++counts.pending; break;
    case RtSampleStatus::Valid: ++counts.valid; break;
    case RtSampleStatus::Error: ++counts.error; break;
    default: ++counts.unknown; break;
    }
}

struct ZoneProjection
{
    std::uint32_t zone = 0u;
    std::size_t intended = 0u;
    std::size_t completed = 0u;
    std::size_t rejected = 0u;
    std::size_t cancelled = 0u;
    std::size_t cpuAccepted = 0u;
    std::size_t cpuRejected = 0u;
    std::size_t outstanding = 0u;
    RtGpuSampleStatusCounts gpu{};
};

std::vector<ZoneProjection> ProjectZones(const RtBenchmarkEvidenceRun& run)
{
    std::vector<ZoneProjection> zones;
    zones.reserve(run.ExpectedCount());
    for (std::size_t index = 0u; index < run.ExpectedCount(); ++index)
    {
        RtExpectedFrameRecord row{};
        if (!run.TryGetExpectedFrame(index, row))
        {
            continue;
        }
        auto found = std::find_if(zones.begin(), zones.end(), [&](const ZoneProjection& zone) {
            return zone.zone == row.tag.zone;
        });
        if (found == zones.end())
        {
            zones.push_back({});
            found = zones.end() - 1;
            found->zone = row.tag.zone;
        }
        ZoneProjection& zone = *found;
        ++zone.intended;
        switch (row.disposition)
        {
        case RtExpectedFrameDisposition::Completed: ++zone.completed; break;
        case RtExpectedFrameDisposition::Rejected: ++zone.rejected; break;
        case RtExpectedFrameDisposition::Cancelled: ++zone.cancelled; break;
        default: ++zone.outstanding; break;
        }
        if (row.cpuAccepted)
        {
            ++zone.cpuAccepted;
        }
        else if (row.disposition == RtExpectedFrameDisposition::Completed)
        {
            ++zone.cpuRejected;
        }
        IncrementGpuCount(zone.gpu, row);
    }
    std::sort(zones.begin(), zones.end(), [](const ZoneProjection& left,
                                             const ZoneProjection& right) {
        return left.zone < right.zone;
    });
    return zones;
}

void WriteJsonIdentity(std::ostream& out, const RtSubmittedFrameIdentity& identity)
{
    out << "{\"sceneEpoch\": " << identity.frame.sceneEpoch
        << ", \"measurementGeneration\": " << identity.frame.measurementGeneration
        << ", \"recordAttemptSerial\": " << identity.frame.recordAttemptSerial
        << ", \"recordSerial\": " << identity.frame.recordSerial
        << ", \"simulationTick\": " << identity.frame.simulationTick
        << ", \"frameSlot\": " << identity.frame.frameSlot
        << ", \"submissionSerial\": " << identity.submissionSerial << '}';
}

void WriteJsonIdentity(std::ostream& out, const RtCompletedFrameIdentity& identity)
{
    out << "{\"sceneEpoch\": " << identity.submitted.frame.sceneEpoch
        << ", \"measurementGeneration\": "
        << identity.submitted.frame.measurementGeneration
        << ", \"recordAttemptSerial\": "
        << identity.submitted.frame.recordAttemptSerial
        << ", \"recordSerial\": " << identity.submitted.frame.recordSerial
        << ", \"simulationTick\": " << identity.submitted.frame.simulationTick
        << ", \"frameSlot\": " << identity.submitted.frame.frameSlot
        << ", \"submissionSerial\": " << identity.submitted.submissionSerial
        << ", \"completionSerial\": " << identity.completionSerial << '}';
}

void WriteJsonCpuStages(std::ostream& out,
                        const RtBenchmarkEvidenceRun& run,
                        const std::uint32_t zone,
                        const bool filterZone)
{
    out << '{';
    for (std::size_t index = 0u; index < kRtStageCount; ++index)
    {
        const RtStage stage = static_cast<RtStage>(index);
        RtStageStatistics statistics{};
        const bool available = filterZone
            ? run.CpuZoneStatistics(zone, stage, statistics)
            : run.CpuStatistics(stage, statistics);
        if (index != 0u)
        {
            out << ", ";
        }
        WriteJsonString(out, StageName(stage));
        out << ": ";
        WriteJsonStatistics(out, available, statistics, stage == RtStage::WholeFrameCycle);
    }
    out << '}';
}

} // namespace

std::string BuildRtBenchmarkEvidenceJson(const RtBenchmarkEvidenceRun& run)
{
    std::ostringstream out;
    out.imbue(std::locale::classic());
    out << std::fixed << std::setprecision(4);
    const std::vector<ZoneProjection> zones = ProjectZones(run);

    out << "{\n"
        << "  \"schema\": 1,\n"
        << "  \"status\": ";
    WriteJsonString(out, RunStatusName(run.Status()));
    out << ",\n  \"lastFailure\": ";
    WriteJsonString(out, FailureReasonName(run.LastFailureReason()));
    out << ",\n"
        << "  \"invalidRun\": " << (run.InvalidRun() ? "true" : "false") << ",\n"
        << "  \"sceneEpoch\": " << run.SceneEpoch() << ",\n"
        << "  \"measurementGeneration\": " << run.MeasurementGeneration() << ",\n"
        << "  \"capacity\": " << run.Capacity() << ",\n"
        << "  \"counts\": {\"expected\": " << run.ExpectedCount()
        << ", \"completed\": " << run.CompletedCount()
        << ", \"rejected\": " << run.RejectedCount()
        << ", \"cancelled\": " << run.CancelledCount()
        << ", \"cpuAccepted\": " << run.CpuAcceptedCount()
        << ", \"cpuRejected\": " << run.CpuRejectedCount()
        << ", \"outstanding\": " << run.OutstandingCount() << "},\n"
        << "  \"gpuStatusCounts\": ";
    WriteJsonGpuCounts(out, run.GpuStatusCounts(), run.ExpectedCount());

    out << ",\n  \"failureReasonCounts\": {";
    bool firstFailure = true;
    for (std::size_t index = 1u;
         index < static_cast<std::size_t>(RtBenchmarkFailureReason::Count);
         ++index)
    {
        if (!firstFailure)
        {
            out << ", ";
        }
        firstFailure = false;
        const RtBenchmarkFailureReason reason =
            static_cast<RtBenchmarkFailureReason>(index);
        WriteJsonString(out, FailureReasonName(reason));
        out << ": " << run.FailureCount(reason);
    }
    out << "},\n  \"cpuStages\": ";
    WriteJsonCpuStages(out, run, 0u, false);
    RtStageStatistics gpuStatistics{};
    const bool gpuAvailable = run.GpuStatistics(gpuStatistics);
    out << ",\n  \"gpuRtDurationMs\": ";
    WriteJsonStatistics(out, gpuAvailable, gpuStatistics, false);

    out << ",\n  \"zones\": [";
    for (std::size_t index = 0u; index < zones.size(); ++index)
    {
        const ZoneProjection& zone = zones[index];
        out << (index == 0u ? "\n" : ",\n")
            << "    {\"zone\": " << zone.zone << ", \"name\": ";
        WriteJsonString(out, ZoneName(zone.zone));
        out << ", \"counts\": {\"intended\": " << zone.intended
            << ", \"completed\": " << zone.completed
            << ", \"rejected\": " << zone.rejected
            << ", \"cancelled\": " << zone.cancelled
            << ", \"cpuAccepted\": " << zone.cpuAccepted
            << ", \"cpuRejected\": " << zone.cpuRejected
            << ", \"outstanding\": " << zone.outstanding << "}, \"gpuStatusCounts\": ";
        WriteJsonGpuCounts(out, zone.gpu, zone.intended);
        out << ", \"cpuStages\": ";
        WriteJsonCpuStages(out, run, zone.zone, true);
        RtStageStatistics zoneGpuStatistics{};
        const bool zoneGpuAvailable = run.GpuZoneStatistics(zone.zone, zoneGpuStatistics);
        out << ", \"gpuRtDurationMs\": ";
        WriteJsonStatistics(out, zoneGpuAvailable, zoneGpuStatistics, false);
        out << '}';
    }
    if (!zones.empty())
    {
        out << '\n';
    }
    out << "  ],\n  \"rows\": [";
    bool firstRow = true;
    for (std::size_t index = 0u; index < run.ExpectedCount(); ++index)
    {
        RtExpectedFrameRecord row{};
        if (!run.TryGetExpectedFrame(index, row))
        {
            continue;
        }
        out << (firstRow ? "\n" : ",\n") << "    {\"index\": " << index
            << ", \"lap\": " << row.tag.lap
            << ", \"zone\": " << row.tag.zone << ", \"zoneName\": ";
        firstRow = false;
        WriteJsonString(out, ZoneName(row.tag.zone));
        out << ", \"disposition\": ";
        WriteJsonString(out, DispositionName(row.disposition));
        out << ", \"failure\": ";
        WriteJsonString(out, FailureReasonName(row.rejectionReason));
        out << ", \"submittedIdentity\": ";
        if (row.hasSubmittedIdentity)
        {
            WriteJsonIdentity(out, row.submittedIdentity);
        }
        else
        {
            out << "null";
        }
        out << ", \"completionIdentity\": ";
        if (row.hasCompletionIdentity)
        {
            WriteJsonIdentity(out, row.completionIdentity);
        }
        else
        {
            out << "null";
        }
        out << ", \"presentationOutcome\": ";
        WriteJsonString(out, PresentationName(row.presentationOutcome));
        out << ", \"cpuStageStatus\": ";
        WriteJsonString(out, SampleStatusName(row.cpuStageStatus));
        out << ", \"diagnosticStatus\": ";
        WriteJsonString(out, SampleStatusName(row.diagnosticStatus));
        out << ", \"gpuStatus\": ";
        if (row.hasGpuStatus)
        {
            WriteJsonString(out, SampleStatusName(row.gpuStatus));
        }
        else
        {
            out << "null";
        }
        out << ", \"gpuDurationNanoseconds\": ";
        if (row.hasGpuDuration)
        {
            out << row.gpuDurationNanoseconds;
        }
        else
        {
            out << "null";
        }
        out << ", \"cpuAccepted\": " << (row.cpuAccepted ? "true" : "false")
            << ", \"cpuSampleIndex\": ";
        if (row.cpuAccepted)
        {
            out << row.cpuSampleIndex;
        }
        else
        {
            out << "null";
        }
        out << '}';
    }
    if (!firstRow)
    {
        out << '\n';
    }
    out << "  ]\n}\n";
    return out.str();
}

std::string BuildRtBenchmarkEvidenceText(const RtBenchmarkEvidenceRun& run)
{
    std::ostringstream out;
    out.imbue(std::locale::classic());
    out << std::fixed << std::setprecision(3)
        << "HORDE LANTERN RT - BENCHMARK EVIDENCE\n"
        << "Schema: 1\n"
        << "Whole-frame FPS is an inverse-duration proxy, not achieved presentation rate.\n"
        << "Status: " << RunStatusName(run.Status()) << '\n'
        << "Identity: epoch " << run.SceneEpoch()
        << ", generation " << run.MeasurementGeneration() << '\n'
        << "Capacity: " << run.Capacity() << '\n'
        << "Rows: expected " << run.ExpectedCount()
        << ", completed " << run.CompletedCount()
        << ", rejected " << run.RejectedCount()
        << ", cancelled " << run.CancelledCount()
        << ", outstanding " << run.OutstandingCount() << '\n'
        << "CPU: accepted " << run.CpuAcceptedCount()
        << ", rejected " << run.CpuRejectedCount() << '\n';
    const RtGpuSampleStatusCounts gpuCounts = run.GpuStatusCounts();
    out << "GPU statuses (denominator " << run.ExpectedCount() << "): valid "
        << gpuCounts.valid << ", pending " << gpuCounts.pending
        << ", error " << gpuCounts.error << ", disabled " << gpuCounts.disabled
        << ", unsupported " << gpuCounts.unsupported << ", compiled-out "
        << gpuCounts.compiledOut << ", not-ready " << gpuCounts.notReady
        << ", unknown " << gpuCounts.unknown << ", missing " << gpuCounts.missing << '\n';

    out << "Failures:";
    bool anyFailure = false;
    for (std::size_t index = 1u;
         index < static_cast<std::size_t>(RtBenchmarkFailureReason::Count);
         ++index)
    {
        const RtBenchmarkFailureReason reason =
            static_cast<RtBenchmarkFailureReason>(index);
        const std::uint64_t count = run.FailureCount(reason);
        if (count == 0u)
        {
            continue;
        }
        out << (anyFailure ? ", " : " ") << FailureReasonName(reason) << '=' << count;
        anyFailure = true;
    }
    if (!anyFailure)
    {
        out << " none";
    }
    out << "\n\nCPU STAGES\n";
    for (std::size_t index = 0u; index < kRtStageCount; ++index)
    {
        const RtStage stage = static_cast<RtStage>(index);
        RtStageStatistics statistics{};
        const bool available = run.CpuStatistics(stage, statistics);
        out << StageName(stage) << ": ";
        WriteTextStatistics(out, available, statistics, stage == RtStage::WholeFrameCycle);
        out << '\n';
    }
    RtStageStatistics gpuStatistics{};
    const bool gpuAvailable = run.GpuStatistics(gpuStatistics);
    out << "gpuRtDurationMs: ";
    WriteTextStatistics(out, gpuAvailable, gpuStatistics, false);

    out << "\n\nZONES\n";
    for (const ZoneProjection& zone : ProjectZones(run))
    {
        out << ZoneName(zone.zone) << " (" << zone.zone << "): intended=" << zone.intended
            << " completed=" << zone.completed << " rejected=" << zone.rejected
            << " cancelled=" << zone.cancelled << " outstanding=" << zone.outstanding
            << " cpuAccepted=" << zone.cpuAccepted << " cpuRejected=" << zone.cpuRejected
            << " gpuValid=" << zone.gpu.valid << '/' << zone.intended
            << " cpuWholeFrame=";
        RtStageStatistics cpuZoneStatistics{};
        const bool cpuZoneAvailable = run.CpuZoneStatistics(
            zone.zone, RtStage::WholeFrameCycle, cpuZoneStatistics);
        WriteTextStatistics(out, cpuZoneAvailable, cpuZoneStatistics, true);
        out << " gpuRt=";
        RtStageStatistics gpuZoneStatistics{};
        const bool gpuZoneAvailable = run.GpuZoneStatistics(zone.zone, gpuZoneStatistics);
        WriteTextStatistics(out, gpuZoneAvailable, gpuZoneStatistics, false);
        out << '\n';
    }
    return out.str();
}

} // namespace horde::telemetry
