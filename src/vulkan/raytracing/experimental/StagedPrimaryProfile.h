#pragma once

#include "vulkan/raytracing/experimental/StagedPrimaryTiming.h"
#include "telemetry/RtPerformanceEvidence.h"

#include <iomanip>
#include <cmath>
#include <locale>
#include <memory>
#include <new>
#include <sstream>

namespace horde::vulkan::raytracing::experimental {

// Every early return cancels only unsubmitted recording. MarkSubmitted retains
// ownership through presentation failure; its fence still owns collection.
class StagedPrimaryRecordingGuard final {
public:
    StagedPrimaryRecordingGuard(StagedPrimaryTiming& timer, std::uint32_t slot) noexcept
        : timer_(timer), slot_(slot) {}
    ~StagedPrimaryRecordingGuard() { timer_.CancelRecording(slot_); }
    StagedPrimaryRecordingGuard(const StagedPrimaryRecordingGuard&) = delete;
    StagedPrimaryRecordingGuard& operator=(const StagedPrimaryRecordingGuard&) = delete;
private:
    StagedPrimaryTiming& timer_;
    std::uint32_t slot_;
};

// One bounded investigation report, separate from the accepted benchmark ledger.
// No allocation, JSON work or file IO occurs while retaining a frame. Both
// current platforms have one frame in flight; enforce monotonic completions.
class StagedPrimaryProfile final {
public:
    static constexpr std::size_t kMaximumRows = 4000u;
    bool Start(std::size_t capacity) noexcept {
        rows_.reset(); count_ = 0u; capacity_ = 0u; rejected_ = 0u;
        if (capacity == 0u || capacity > kMaximumRows) return false;
        rows_.reset(new (std::nothrow) Row[capacity]);
        if (!rows_) return false;
        capacity_ = capacity; return true;
    }
    bool Append(const StagedPrimaryTimingCollection& timing,
                const horde::telemetry::RtPerformanceEvidenceSnapshot& snapshot) noexcept {
        const auto& owner = snapshot.identity.submitted;
        if (!rows_ || count_ >= capacity_ || owner.submissionSerial == 0u ||
            owner.frame.frameSlot != 0u || owner.frame.sceneEpoch == 0u ||
            owner.frame.measurementGeneration == 0u ||
            (count_ != 0u && owner.submissionSerial <= rows_[count_ - 1u].owner.submissionSerial)) {
            ++rejected_; return false;
        }
        auto& row = rows_[count_++]; row.owner = owner;
        row.cpuEligible = snapshot.cpuBenchmarkEligible;
        row.totalGpuAvailable = snapshot.gpu.hasDuration &&
            snapshot.gpu.status == horde::telemetry::RtSampleStatus::Valid &&
            snapshot.gpu.completedSubmissionSerial == owner.submissionSerial;
        row.totalGpuNanoseconds = row.totalGpuAvailable ? snapshot.gpu.durationNanoseconds : 0u;
        row.timing = timing;
        row.identityMatches = timing.consumed && timing.frameSlot == owner.frame.frameSlot &&
            timing.submissionSequence == owner.submissionSerial;
        return true; // Retaining an unavailable/mismatched row is not a pass.
    }
    std::size_t Count() const noexcept { return count_; }
    std::string Json(std::size_t expectedCount, const StagedPrimaryTiming& timer,
                     const std::string& intermediate = "null") const {
        std::ostringstream out; out.imbue(std::locale::classic()); out << std::setprecision(12);
        out << "{\"schema\":1,\"investigationOnly\":true,\"profiledRun\":true,"
            << "\"supported\":" << (timer.Supported() ? "true" : "false")
            << ",\"initialisationStatus\":" << static_cast<int>(timer.Status())
            << ",\"timestampValidBits\":" << timer.TimestampValidBits()
            << ",\"timestampPeriodNanoseconds\":";
        if (std::isfinite(timer.TimestampPeriodNanoseconds()) && timer.TimestampPeriodNanoseconds() > 0.0)
            out << timer.TimestampPeriodNanoseconds();
        else out << "null";
        out << ",\"configuredAdditionalQueriesPerSubmission\":3,\"capacity\":" << capacity_
            << ",\"logicalRowStorageBytes\":" << capacity_ * sizeof(Row)
            << ",\"expectedCount\":" << expectedCount << ",\"retainedCount\":" << count_
            << ",\"rejectedCount\":" << rejected_
            << ",\"missingCompletionCount\":" << (expectedCount > count_ ? expectedCount - count_ : 0u)
            << ",\"primaryScope\":\"TOP-to-primary-RT-complete; includes prerequisite waits\","
            << "\"shadeScope\":\"primary-RT-complete-to-shade-RT-complete; includes barrier\","
            << "\"dramBandwidthMeasured\":false,\"rows\":[";
        for (std::size_t i = 0u; i < count_; ++i) {
            const auto& row = rows_[i]; const auto& timing = row.timing;
            const bool available = row.identityMatches && timing.hasSamples &&
                timing.status == StagedPrimaryTimingCollectionStatus::Valid &&
                std::isfinite(timing.primaryIncludingPrerequisiteWait.milliseconds) &&
                std::isfinite(timing.shadeIncludingBarrier.milliseconds) &&
                timing.primaryIncludingPrerequisiteWait.milliseconds >= 0.0 &&
                timing.shadeIncludingBarrier.milliseconds >= 0.0;
            if (i != 0u) out << ',';
            out << "{\"submissionSerial\":" << row.owner.submissionSerial
                << ",\"frameSlot\":" << row.owner.frame.frameSlot
                << ",\"sceneEpoch\":" << row.owner.frame.sceneEpoch
                << ",\"measurementGeneration\":" << row.owner.frame.measurementGeneration
                << ",\"simulationTick\":" << row.owner.frame.simulationTick
                << ",\"cpuEligible\":" << (row.cpuEligible ? "true" : "false")
                << ",\"queryConsumed\":" << (timing.consumed ? "true" : "false")
                << ",\"querySubmissionSerial\":" << timing.submissionSequence
                << ",\"queryFrameSlot\":" << timing.frameSlot
                << ",\"identityMatches\":" << (row.identityMatches ? "true" : "false")
                << ",\"queryStatus\":\"" << StatusName(timing.status)
                << "\",\"vkResult\":" << static_cast<int>(timing.result)
                << ",\"totalGpuNanoseconds\":";
            if (row.totalGpuAvailable) out << row.totalGpuNanoseconds; else out << "null";
            out << ",\"primaryMilliseconds\":";
            if (available) out << timing.primaryIncludingPrerequisiteWait.milliseconds; else out << "null";
            out << ",\"shadeIncludingBarrierMilliseconds\":";
            if (available) out << timing.shadeIncludingBarrier.milliseconds; else out << "null";
            out << ",\"primaryTicks\":";
            if (available) out << timing.primaryIncludingPrerequisiteWait.elapsedTicks; else out << "null";
            out << ",\"shadeTicks\":";
            if (available) out << timing.shadeIncludingBarrier.elapsedTicks; else out << "null";
            out << '}';
        }
        out << "],\"intermediate\":" << intermediate << '}'; return out.str();
    }
private:
    static const char* StatusName(StagedPrimaryTimingCollectionStatus status) noexcept {
        switch (status) {
        case StagedPrimaryTimingCollectionStatus::Valid: return "valid";
        case StagedPrimaryTimingCollectionStatus::Unavailable: return "unavailable";
        case StagedPrimaryTimingCollectionStatus::Error: return "error";
        default: return "no-submitted-work";
        }
    }
    struct Row {
        horde::telemetry::RtSubmittedFrameIdentity owner{};
        StagedPrimaryTimingCollection timing{};
        bool identityMatches = false, cpuEligible = false, totalGpuAvailable = false;
        std::uint64_t totalGpuNanoseconds = 0u;
    };
    std::unique_ptr<Row[]> rows_;
    std::size_t count_ = 0u, capacity_ = 0u, rejected_ = 0u;
};

inline std::string AttachStagedPrimaryProfile(std::string json, const std::string& profile) {
    const auto end = json.find_last_not_of(" \t\r\n");
    if (end == std::string::npos || json[end] != '}') return json;
    const auto previous = end == 0u ? std::string::npos : json.find_last_not_of(" \t\r\n", end - 1u);
    const bool empty = previous != std::string::npos && json[previous] == '{';
    json.insert(end, (empty ? "\n" : ",\n") + std::string("\"stagedPassProfiling\":") + profile + "\n");
    return json;
}
} // namespace horde::vulkan::raytracing::experimental
