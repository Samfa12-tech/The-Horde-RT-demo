#pragma once

#include <array>
#include <cstddef>
#include <cstdint>
#include <memory>
#include <optional>

#include "telemetry/RtPerformanceEvidence.h"

namespace horde::telemetry
{

enum class RtBenchmarkRunStatus : std::uint8_t
{
    Empty,
    Allocated,
    Measuring,
    Complete,
    Incomplete,
    Cancelled,
    Invalid,
};

enum class RtBenchmarkFailureReason : std::uint8_t
{
    None,
    InvalidCapacity,
    CapacityOverflow,
    AllocationFailed,
    InvalidState,
    InvalidMeasurementIdentity,
    CapacityExceeded,
    InvalidFrameTag,
    InvalidExpectedIndex,
    DuplicateDisposition,
    InvalidSubmittedIdentity,
    PendingSlotOccupied,
    SubmissionFailed,
    PresentationFailed,
    PresentedNeedsRecreate,
    TokenlessCompletion,
    InvalidCompletion,
    InvalidCompletionIdentity,
    DuplicateCompletion,
    StaleCompletion,
    MismatchedCompletion,
    CpuStageError,
    DiagnosticFailed,
    CpuIneligible,
    CpuCollectorRejected,
    MissingCompletion,
    DrainFailed,
    Cancelled,
    Count,
};

enum class RtExpectedFrameDisposition : std::uint8_t
{
    AwaitingSubmission,
    PendingCompletion,
    Completed,
    Rejected,
    Cancelled,
};

struct RtBenchmarkFrameTag
{
    std::uint32_t zone = 0u;
    std::uint32_t lap = 0u;
};

struct RtExpectedFrameRecord
{
    RtBenchmarkFrameTag tag{};
    RtExpectedFrameDisposition disposition = RtExpectedFrameDisposition::AwaitingSubmission;
    RtBenchmarkFailureReason rejectionReason = RtBenchmarkFailureReason::None;
    bool hasSubmittedIdentity = false;
    RtSubmittedFrameIdentity submittedIdentity{};
    bool hasCompletionIdentity = false;
    RtCompletedFrameIdentity completionIdentity{};
    bool hasGpuStatus = false;
    RtSampleStatus gpuStatus = RtSampleStatus::NotReady;
    RtPresentationOutcome presentationOutcome = RtPresentationOutcome::NotAttempted;
    RtSampleStatus cpuStageStatus = RtSampleStatus::NotReady;
    RtSampleStatus diagnosticStatus = RtSampleStatus::NotReady;
    bool hasGpuDuration = false;
    std::uint64_t gpuDurationNanoseconds = 0u;
    bool cpuAccepted = false;
    std::size_t cpuSampleIndex = 0u;
};

struct RtGpuSampleStatusCounts
{
    std::size_t notReady = 0u;
    std::size_t compiledOut = 0u;
    std::size_t unknown = 0u;
    std::size_t valid = 0u;
    std::size_t disabled = 0u;
    std::size_t unsupported = 0u;
    std::size_t pending = 0u;
    std::size_t error = 0u;
    std::size_t missing = 0u;
};

// Owns one bounded measurement run. Allocation and measurement identity arming
// are deliberately separate so warm-up lifecycle events cannot contaminate the
// final generation. Frame-time operations never grow storage or retain spans.
class RtBenchmarkEvidenceRun
{
public:
    RtBenchmarkEvidenceRun() noexcept = default;
    ~RtBenchmarkEvidenceRun() = default;

    RtBenchmarkEvidenceRun(const RtBenchmarkEvidenceRun&) = delete;
    RtBenchmarkEvidenceRun& operator=(const RtBenchmarkEvidenceRun&) = delete;
    RtBenchmarkEvidenceRun(RtBenchmarkEvidenceRun&& other) noexcept;
    RtBenchmarkEvidenceRun& operator=(RtBenchmarkEvidenceRun&& other) noexcept;

    [[nodiscard]] bool Start(std::size_t capacity) noexcept;
    [[nodiscard]] bool ArmMeasurement(std::uint64_t sceneEpoch,
                                      std::uint64_t measurementGeneration) noexcept;
    [[nodiscard]] std::optional<std::size_t> ExpectFrame(RtBenchmarkFrameTag tag) noexcept;
    [[nodiscard]] bool BindSubmitted(
        std::size_t expectedIndex,
        const RtSubmittedFrameIdentity& committedIdentity) noexcept;
    [[nodiscard]] bool RejectExpected(std::size_t expectedIndex,
                                      RtBenchmarkFailureReason reason) noexcept;

    // True means the canonical completion was associated with its exact row.
    // CPU admission is reported separately and can be false while GPU metadata
    // remains retained on that completed row.
    [[nodiscard]] bool Complete(const RtPerformanceEvidenceSnapshot& snapshot) noexcept;

    // Records the external owner's final drain result. This flag never certifies
    // completeness by itself; Finalize also checks the full ledger and slots.
    [[nodiscard]] bool RecordOwnerDrainResult(bool succeeded) noexcept;
    [[nodiscard]] bool Finalize() noexcept;
    void Cancel() noexcept;

    [[nodiscard]] bool CpuStatistics(RtStage stage, RtStageStatistics& output) const noexcept;
    [[nodiscard]] bool CpuZoneStatistics(std::uint32_t zone,
                                         RtStage stage,
                                         RtStageStatistics& output) const noexcept;
    [[nodiscard]] bool GpuStatistics(RtStageStatistics& output) const noexcept;
    [[nodiscard]] bool GpuZoneStatistics(std::uint32_t zone,
                                         RtStageStatistics& output) const noexcept;
    [[nodiscard]] bool TryGetExpectedFrame(std::size_t index,
                                           RtExpectedFrameRecord& output) const noexcept;
    [[nodiscard]] RtGpuSampleStatusCounts GpuStatusCounts() const noexcept;

    [[nodiscard]] RtBenchmarkRunStatus Status() const noexcept { return status_; }
    [[nodiscard]] RtBenchmarkFailureReason LastFailureReason() const noexcept
    {
        return lastFailureReason_;
    }
    [[nodiscard]] std::uint64_t FailureCount(RtBenchmarkFailureReason reason) const noexcept;
    [[nodiscard]] std::size_t Capacity() const noexcept { return capacity_; }
    [[nodiscard]] std::size_t ExpectedCount() const noexcept { return expectedCount_; }
    [[nodiscard]] std::size_t CompletedCount() const noexcept { return completedCount_; }
    [[nodiscard]] std::size_t RejectedCount() const noexcept { return rejectedCount_; }
    [[nodiscard]] std::size_t CancelledCount() const noexcept { return cancelledCount_; }
    [[nodiscard]] std::size_t CpuAcceptedCount() const noexcept { return cpuAcceptedCount_; }
    [[nodiscard]] std::size_t CpuRejectedCount() const noexcept { return cpuRejectedCount_; }
    [[nodiscard]] std::size_t PendingCompletionCount() const noexcept;
    [[nodiscard]] std::size_t AccountedCount() const noexcept;
    [[nodiscard]] std::size_t OutstandingCount() const noexcept;
    [[nodiscard]] std::uint64_t SceneEpoch() const noexcept { return sceneEpoch_; }
    [[nodiscard]] std::uint64_t MeasurementGeneration() const noexcept
    {
        return measurementGeneration_;
    }
    [[nodiscard]] bool InvalidRun() const noexcept { return invalidRun_; }

private:
    struct PendingSlot
    {
        bool occupied = false;
        std::size_t expectedIndex = 0u;
    };

    void Reset() noexcept;
    void MoveFrom(RtBenchmarkEvidenceRun&& other) noexcept;
    void RecordFailure(RtBenchmarkFailureReason reason, bool protocolInvalid) noexcept;
    void RejectOutstanding(RtBenchmarkFailureReason reason) noexcept;
    [[nodiscard]] bool CollectCpuSample(RtExpectedFrameRecord& row,
                                        const RtPerformanceEvidenceSnapshot& snapshot) noexcept;
    [[nodiscard]] bool StatisticsForCpu(std::uint32_t zone,
                                        bool filterZone,
                                        RtStage stage,
                                        RtStageStatistics& output) const noexcept;
    [[nodiscard]] bool StatisticsForGpu(std::uint32_t zone,
                                        bool filterZone,
                                        RtStageStatistics& output) const noexcept;

    std::unique_ptr<RtExpectedFrameRecord[]> expectedFrames_{};
    std::unique_ptr<RtCompletedStageSample[]> cpuSamples_{};
    std::unique_ptr<std::uint64_t[]> scratch_{};
    RtStageSampleCollectionCore cpuCore_{};
    std::array<PendingSlot, kRtMaximumFrameSlots> pendingSlots_{};
    RtSubmittedFrameIdentity lastBoundIdentity_{};
    RtCompletedFrameIdentity lastCompletedIdentity_{};
    std::array<std::uint64_t,
               static_cast<std::size_t>(RtBenchmarkFailureReason::Count)> failureCounts_{};
    std::size_t capacity_ = 0u;
    std::size_t expectedCount_ = 0u;
    std::size_t completedCount_ = 0u;
    std::size_t rejectedCount_ = 0u;
    std::size_t cancelledCount_ = 0u;
    std::size_t cpuAcceptedCount_ = 0u;
    std::size_t cpuRejectedCount_ = 0u;
    std::uint64_t sceneEpoch_ = 0u;
    std::uint64_t measurementGeneration_ = 0u;
    RtBenchmarkRunStatus status_ = RtBenchmarkRunStatus::Empty;
    RtBenchmarkFailureReason lastFailureReason_ = RtBenchmarkFailureReason::None;
    bool invalidRun_ = false;
    bool hasLastBoundIdentity_ = false;
    bool hasLastCompletedIdentity_ = false;
    bool drainResultRecorded_ = false;
    bool drainSucceeded_ = false;
};

} // namespace horde::telemetry
