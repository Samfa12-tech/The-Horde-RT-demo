#include "telemetry/RtBenchmarkEvidenceRun.h"

#include <algorithm>
#include <limits>
#include <new>
#include <span>
#include <utility>

namespace horde::telemetry
{
namespace
{

template <typename Value>
bool ArrayCapacityOverflows(const std::size_t capacity) noexcept
{
    const std::size_t maximumDifference =
        static_cast<std::size_t>(std::numeric_limits<std::ptrdiff_t>::max());
    return capacity > std::numeric_limits<std::size_t>::max() / sizeof(Value) ||
           capacity > maximumDifference / sizeof(Value);
}

bool SameFrameToken(const RtFrameToken& left, const RtFrameToken& right) noexcept
{
    return left.sceneEpoch == right.sceneEpoch &&
           left.measurementGeneration == right.measurementGeneration &&
           left.recordAttemptSerial == right.recordAttemptSerial &&
           left.recordSerial == right.recordSerial &&
           left.simulationTick == right.simulationTick &&
           left.frameSlot == right.frameSlot;
}

bool SameSubmittedIdentity(const RtSubmittedFrameIdentity& left,
                           const RtSubmittedFrameIdentity& right) noexcept
{
    return SameFrameToken(left.frame, right.frame) &&
           left.submissionSerial == right.submissionSerial;
}

bool SubmittedIdentityPopulated(const RtSubmittedFrameIdentity& identity) noexcept
{
    const RtFrameToken& frame = identity.frame;
    return frame.sceneEpoch != 0u && frame.measurementGeneration != 0u &&
           frame.recordAttemptSerial != 0u && frame.recordSerial != 0u &&
           frame.frameSlot < kRtMaximumFrameSlots && identity.submissionSerial != 0u;
}

bool CompletionIdentityPopulated(const RtCompletedFrameIdentity& identity) noexcept
{
    return SubmittedIdentityPopulated(identity.submitted) && identity.completionSerial != 0u;
}

bool ValidFailureReason(const RtBenchmarkFailureReason reason) noexcept
{
    return reason > RtBenchmarkFailureReason::None &&
           reason < RtBenchmarkFailureReason::Count;
}

void IncrementSaturating(std::uint64_t& value) noexcept
{
    if (value != std::numeric_limits<std::uint64_t>::max())
    {
        ++value;
    }
}

} // namespace

RtBenchmarkEvidenceRun::RtBenchmarkEvidenceRun(RtBenchmarkEvidenceRun&& other) noexcept
{
    MoveFrom(std::move(other));
}

RtBenchmarkEvidenceRun& RtBenchmarkEvidenceRun::operator=(
    RtBenchmarkEvidenceRun&& other) noexcept
{
    if (this != &other)
    {
        Reset();
        MoveFrom(std::move(other));
    }
    return *this;
}

void RtBenchmarkEvidenceRun::Reset() noexcept
{
    expectedFrames_.reset();
    cpuSamples_.reset();
    scratch_.reset();
    cpuCore_ = {};
    pendingSlots_ = {};
    lastBoundIdentity_ = {};
    lastCompletedIdentity_ = {};
    failureCounts_ = {};
    capacity_ = 0u;
    expectedCount_ = 0u;
    completedCount_ = 0u;
    rejectedCount_ = 0u;
    cancelledCount_ = 0u;
    cpuAcceptedCount_ = 0u;
    cpuRejectedCount_ = 0u;
    sceneEpoch_ = 0u;
    measurementGeneration_ = 0u;
    status_ = RtBenchmarkRunStatus::Empty;
    lastFailureReason_ = RtBenchmarkFailureReason::None;
    invalidRun_ = false;
    hasLastBoundIdentity_ = false;
    hasLastCompletedIdentity_ = false;
    drainResultRecorded_ = false;
    drainSucceeded_ = false;
}

void RtBenchmarkEvidenceRun::MoveFrom(RtBenchmarkEvidenceRun&& other) noexcept
{
    expectedFrames_ = std::move(other.expectedFrames_);
    cpuSamples_ = std::move(other.cpuSamples_);
    scratch_ = std::move(other.scratch_);
    cpuCore_ = other.cpuCore_;
    pendingSlots_ = other.pendingSlots_;
    lastBoundIdentity_ = other.lastBoundIdentity_;
    lastCompletedIdentity_ = other.lastCompletedIdentity_;
    failureCounts_ = other.failureCounts_;
    capacity_ = other.capacity_;
    expectedCount_ = other.expectedCount_;
    completedCount_ = other.completedCount_;
    rejectedCount_ = other.rejectedCount_;
    cancelledCount_ = other.cancelledCount_;
    cpuAcceptedCount_ = other.cpuAcceptedCount_;
    cpuRejectedCount_ = other.cpuRejectedCount_;
    sceneEpoch_ = other.sceneEpoch_;
    measurementGeneration_ = other.measurementGeneration_;
    status_ = other.status_;
    lastFailureReason_ = other.lastFailureReason_;
    invalidRun_ = other.invalidRun_;
    hasLastBoundIdentity_ = other.hasLastBoundIdentity_;
    hasLastCompletedIdentity_ = other.hasLastCompletedIdentity_;
    drainResultRecorded_ = other.drainResultRecorded_;
    drainSucceeded_ = other.drainSucceeded_;
    other.Reset();
}

void RtBenchmarkEvidenceRun::RecordFailure(const RtBenchmarkFailureReason reason,
                                           const bool protocolInvalid) noexcept
{
    if (!ValidFailureReason(reason))
    {
        return;
    }
    lastFailureReason_ = reason;
    IncrementSaturating(failureCounts_[static_cast<std::size_t>(reason)]);
    if (protocolInvalid)
    {
        invalidRun_ = true;
        cpuCore_.Invalidate();
        if (status_ == RtBenchmarkRunStatus::Complete)
        {
            status_ = RtBenchmarkRunStatus::Invalid;
        }
    }
}

bool RtBenchmarkEvidenceRun::Start(const std::size_t capacity) noexcept
{
    // A failed restart must not leave an older run eligible for publication.
    Reset();
    if (capacity == 0u)
    {
        status_ = RtBenchmarkRunStatus::Invalid;
        RecordFailure(RtBenchmarkFailureReason::InvalidCapacity, true);
        return false;
    }
    if (ArrayCapacityOverflows<RtExpectedFrameRecord>(capacity) ||
        ArrayCapacityOverflows<RtCompletedStageSample>(capacity) ||
        ArrayCapacityOverflows<std::uint64_t>(capacity))
    {
        status_ = RtBenchmarkRunStatus::Invalid;
        RecordFailure(RtBenchmarkFailureReason::CapacityOverflow, true);
        return false;
    }

    std::unique_ptr<RtExpectedFrameRecord[]> expectedFrames;
    std::unique_ptr<RtCompletedStageSample[]> cpuSamples;
    std::unique_ptr<std::uint64_t[]> scratch;
    try
    {
        expectedFrames.reset(new (std::nothrow) RtExpectedFrameRecord[capacity]);
        if (expectedFrames == nullptr)
        {
            status_ = RtBenchmarkRunStatus::Invalid;
            RecordFailure(RtBenchmarkFailureReason::AllocationFailed, true);
            return false;
        }
        cpuSamples.reset(new (std::nothrow) RtCompletedStageSample[capacity]);
        if (cpuSamples == nullptr)
        {
            status_ = RtBenchmarkRunStatus::Invalid;
            RecordFailure(RtBenchmarkFailureReason::AllocationFailed, true);
            return false;
        }
        scratch.reset(new (std::nothrow) std::uint64_t[capacity]);
        if (scratch == nullptr)
        {
            status_ = RtBenchmarkRunStatus::Invalid;
            RecordFailure(RtBenchmarkFailureReason::AllocationFailed, true);
            return false;
        }
    }
    catch (...)
    {
        status_ = RtBenchmarkRunStatus::Invalid;
        RecordFailure(RtBenchmarkFailureReason::AllocationFailed, true);
        return false;
    }

    expectedFrames_ = std::move(expectedFrames);
    cpuSamples_ = std::move(cpuSamples);
    scratch_ = std::move(scratch);
    capacity_ = capacity;
    status_ = RtBenchmarkRunStatus::Allocated;
    return true;
}

bool RtBenchmarkEvidenceRun::ArmMeasurement(const std::uint64_t sceneEpoch,
                                            const std::uint64_t measurementGeneration) noexcept
{
    if (status_ != RtBenchmarkRunStatus::Allocated)
    {
        RecordFailure(RtBenchmarkFailureReason::InvalidState, true);
        return false;
    }
    if (sceneEpoch == 0u || measurementGeneration == 0u)
    {
        status_ = RtBenchmarkRunStatus::Invalid;
        RecordFailure(RtBenchmarkFailureReason::InvalidMeasurementIdentity, true);
        return false;
    }
    if (!cpuCore_.Start(std::span<RtCompletedStageSample>(cpuSamples_.get(), capacity_),
                        sceneEpoch,
                        measurementGeneration))
    {
        status_ = RtBenchmarkRunStatus::Invalid;
        RecordFailure(RtBenchmarkFailureReason::InvalidMeasurementIdentity, true);
        return false;
    }
    sceneEpoch_ = sceneEpoch;
    measurementGeneration_ = measurementGeneration;
    status_ = RtBenchmarkRunStatus::Measuring;
    return true;
}

std::optional<std::size_t> RtBenchmarkEvidenceRun::ExpectFrame(
    const RtBenchmarkFrameTag tag) noexcept
{
    if (status_ != RtBenchmarkRunStatus::Measuring || drainResultRecorded_)
    {
        RecordFailure(RtBenchmarkFailureReason::InvalidState, true);
        return std::nullopt;
    }
    if (tag.lap == 0u)
    {
        RecordFailure(RtBenchmarkFailureReason::InvalidFrameTag, true);
        return std::nullopt;
    }
    if (expectedCount_ == capacity_)
    {
        RecordFailure(RtBenchmarkFailureReason::CapacityExceeded, true);
        return std::nullopt;
    }

    const std::size_t index = expectedCount_++;
    expectedFrames_[index] = {};
    expectedFrames_[index].tag = tag;
    return index;
}

bool RtBenchmarkEvidenceRun::BindSubmitted(
    const std::size_t expectedIndex,
    const RtSubmittedFrameIdentity& committedIdentity) noexcept
{
    if (status_ != RtBenchmarkRunStatus::Measuring || drainResultRecorded_)
    {
        RecordFailure(RtBenchmarkFailureReason::InvalidState, true);
        return false;
    }
    if (expectedIndex >= expectedCount_)
    {
        RecordFailure(RtBenchmarkFailureReason::InvalidExpectedIndex, true);
        return false;
    }
    RtExpectedFrameRecord& row = expectedFrames_[expectedIndex];
    if (row.disposition != RtExpectedFrameDisposition::AwaitingSubmission ||
        row.hasSubmittedIdentity)
    {
        RecordFailure(RtBenchmarkFailureReason::DuplicateDisposition, true);
        return false;
    }
    const RtFrameToken& frame = committedIdentity.frame;
    if (!SubmittedIdentityPopulated(committedIdentity) ||
        frame.sceneEpoch != sceneEpoch_ ||
        frame.measurementGeneration != measurementGeneration_)
    {
        RecordFailure(RtBenchmarkFailureReason::InvalidSubmittedIdentity, true);
        return false;
    }
    PendingSlot& slot = pendingSlots_[frame.frameSlot];
    if (slot.occupied)
    {
        RecordFailure(RtBenchmarkFailureReason::PendingSlotOccupied, true);
        return false;
    }
    if (hasLastBoundIdentity_ &&
        (frame.recordAttemptSerial <= lastBoundIdentity_.frame.recordAttemptSerial ||
         frame.recordSerial <= lastBoundIdentity_.frame.recordSerial ||
         committedIdentity.submissionSerial <= lastBoundIdentity_.submissionSerial))
    {
        RecordFailure(RtBenchmarkFailureReason::InvalidSubmittedIdentity, true);
        return false;
    }

    row.hasSubmittedIdentity = true;
    row.submittedIdentity = committedIdentity;
    row.disposition = RtExpectedFrameDisposition::PendingCompletion;
    slot.occupied = true;
    slot.expectedIndex = expectedIndex;
    lastBoundIdentity_ = committedIdentity;
    hasLastBoundIdentity_ = true;
    return true;
}

bool RtBenchmarkEvidenceRun::RejectExpected(const std::size_t expectedIndex,
                                            const RtBenchmarkFailureReason reason) noexcept
{
    if (status_ != RtBenchmarkRunStatus::Measuring)
    {
        RecordFailure(RtBenchmarkFailureReason::InvalidState, true);
        return false;
    }
    if (expectedIndex >= expectedCount_)
    {
        RecordFailure(RtBenchmarkFailureReason::InvalidExpectedIndex, true);
        return false;
    }
    if (!ValidFailureReason(reason))
    {
        RecordFailure(RtBenchmarkFailureReason::DuplicateDisposition, true);
        return false;
    }
    RtExpectedFrameRecord& row = expectedFrames_[expectedIndex];
    if (row.disposition != RtExpectedFrameDisposition::AwaitingSubmission &&
        row.disposition != RtExpectedFrameDisposition::PendingCompletion)
    {
        RecordFailure(RtBenchmarkFailureReason::DuplicateDisposition, true);
        return false;
    }
    if (row.disposition == RtExpectedFrameDisposition::PendingCompletion)
    {
        const std::uint32_t frameSlot = row.submittedIdentity.frame.frameSlot;
        if (frameSlot < pendingSlots_.size() &&
            pendingSlots_[frameSlot].occupied &&
            pendingSlots_[frameSlot].expectedIndex == expectedIndex)
        {
            pendingSlots_[frameSlot] = {};
        }
    }
    row.disposition = RtExpectedFrameDisposition::Rejected;
    row.rejectionReason = reason;
    ++rejectedCount_;
    RecordFailure(reason, false);
    cpuCore_.Invalidate();
    return true;
}

bool RtBenchmarkEvidenceRun::CollectCpuSample(
    RtExpectedFrameRecord& row,
    const RtPerformanceEvidenceSnapshot& snapshot) noexcept
{
    if (!snapshot.cpuBenchmarkEligible)
    {
        if (snapshot.presentation.outcome ==
            RtPresentationOutcome::PresentedNeedsRecreate)
        {
            row.rejectionReason = RtBenchmarkFailureReason::PresentedNeedsRecreate;
        }
        else if (snapshot.presentation.outcome != RtPresentationOutcome::Presented)
        {
            row.rejectionReason = RtBenchmarkFailureReason::PresentationFailed;
        }
        else if (snapshot.scene.stages.status != RtSampleStatus::Valid)
        {
            row.rejectionReason = RtBenchmarkFailureReason::CpuStageError;
        }
        else if (snapshot.dielectric.status != RtSampleStatus::CompiledOut &&
                 snapshot.dielectric.status != RtSampleStatus::Valid)
        {
            row.rejectionReason = RtBenchmarkFailureReason::DiagnosticFailed;
        }
        else
        {
            row.rejectionReason = RtBenchmarkFailureReason::CpuIneligible;
        }
        ++cpuRejectedCount_;
        RecordFailure(row.rejectionReason, false);
        cpuCore_.Invalidate();
        return false;
    }

    RtCompletedStageSample sample{};
    sample.identity = snapshot.identity;
    sample.stages = snapshot.scene.stages;
    sample.benchmarkEligible = snapshot.cpuBenchmarkEligible;
    const std::size_t sampleIndex = cpuCore_.Size();
    if (!cpuCore_.Append(std::span<RtCompletedStageSample>(cpuSamples_.get(), capacity_), sample))
    {
        row.rejectionReason = RtBenchmarkFailureReason::CpuCollectorRejected;
        ++cpuRejectedCount_;
        RecordFailure(RtBenchmarkFailureReason::CpuCollectorRejected, true);
        return false;
    }
    row.cpuAccepted = true;
    row.cpuSampleIndex = sampleIndex;
    ++cpuAcceptedCount_;
    return true;
}

bool RtBenchmarkEvidenceRun::Complete(const RtPerformanceEvidenceSnapshot& snapshot) noexcept
{
    if (status_ != RtBenchmarkRunStatus::Measuring)
    {
        RecordFailure(RtBenchmarkFailureReason::InvalidState, true);
        return false;
    }
    if (!CompletionIdentityPopulated(snapshot.identity))
    {
        RecordFailure(RtBenchmarkFailureReason::TokenlessCompletion, true);
        return false;
    }
    RtEvidenceValidationError validationError = RtEvidenceValidationError::None;
    if (!ValidateRtPerformanceEvidence(snapshot, validationError))
    {
        RecordFailure(RtBenchmarkFailureReason::InvalidCompletion, true);
        return false;
    }

    const RtSubmittedFrameIdentity& submitted = snapshot.identity.submitted;
    const RtFrameToken& frame = submitted.frame;
    if (frame.sceneEpoch != sceneEpoch_ ||
        frame.measurementGeneration != measurementGeneration_)
    {
        RecordFailure(RtBenchmarkFailureReason::StaleCompletion, true);
        return false;
    }
    PendingSlot& slot = pendingSlots_[frame.frameSlot];
    if (!slot.occupied)
    {
        for (std::size_t index = 0u; index < expectedCount_; ++index)
        {
            const RtExpectedFrameRecord& row = expectedFrames_[index];
            if (row.hasCompletionIdentity &&
                SameSubmittedIdentity(row.completionIdentity.submitted, submitted))
            {
                RecordFailure(RtBenchmarkFailureReason::DuplicateCompletion, true);
                return false;
            }
        }
        RecordFailure(RtBenchmarkFailureReason::MismatchedCompletion, true);
        return false;
    }
    if (slot.expectedIndex >= expectedCount_)
    {
        RecordFailure(RtBenchmarkFailureReason::MismatchedCompletion, true);
        return false;
    }
    RtExpectedFrameRecord& row = expectedFrames_[slot.expectedIndex];
    if (row.disposition != RtExpectedFrameDisposition::PendingCompletion ||
        !row.hasSubmittedIdentity ||
        !SameSubmittedIdentity(row.submittedIdentity, submitted))
    {
        RecordFailure(RtBenchmarkFailureReason::MismatchedCompletion, true);
        return false;
    }
    if (hasLastCompletedIdentity_ &&
        snapshot.identity.completionSerial <= lastCompletedIdentity_.completionSerial)
    {
        RecordFailure(RtBenchmarkFailureReason::InvalidCompletionIdentity, true);
        return false;
    }

    lastCompletedIdentity_ = snapshot.identity;
    hasLastCompletedIdentity_ = true;
    slot = {};
    row.disposition = RtExpectedFrameDisposition::Completed;
    row.hasCompletionIdentity = true;
    row.completionIdentity = snapshot.identity;
    row.hasGpuStatus = true;
    row.gpuStatus = snapshot.gpu.status;
    row.presentationOutcome = snapshot.presentation.outcome;
    row.cpuStageStatus = snapshot.scene.stages.status;
    row.diagnosticStatus = snapshot.dielectric.status;
    row.hasGpuDuration = snapshot.gpu.status == RtSampleStatus::Valid;
    row.gpuDurationNanoseconds = row.hasGpuDuration ? snapshot.gpu.durationNanoseconds : 0u;
    ++completedCount_;
    (void)CollectCpuSample(row, snapshot);
    return true;
}

bool RtBenchmarkEvidenceRun::RecordOwnerDrainResult(const bool succeeded) noexcept
{
    if (status_ != RtBenchmarkRunStatus::Measuring || drainResultRecorded_)
    {
        RecordFailure(RtBenchmarkFailureReason::InvalidState, true);
        return false;
    }
    drainResultRecorded_ = true;
    drainSucceeded_ = succeeded;
    if (!succeeded)
    {
        RecordFailure(RtBenchmarkFailureReason::DrainFailed, false);
        cpuCore_.Invalidate();
    }
    return true;
}

void RtBenchmarkEvidenceRun::RejectOutstanding(const RtBenchmarkFailureReason reason) noexcept
{
    for (std::size_t index = 0u; index < expectedCount_; ++index)
    {
        RtExpectedFrameRecord& row = expectedFrames_[index];
        if (row.disposition != RtExpectedFrameDisposition::AwaitingSubmission &&
            row.disposition != RtExpectedFrameDisposition::PendingCompletion)
        {
            continue;
        }
        if (row.disposition == RtExpectedFrameDisposition::PendingCompletion)
        {
            const std::uint32_t frameSlot = row.submittedIdentity.frame.frameSlot;
            if (frameSlot < pendingSlots_.size() &&
                pendingSlots_[frameSlot].occupied &&
                pendingSlots_[frameSlot].expectedIndex == index)
            {
                pendingSlots_[frameSlot] = {};
            }
        }
        row.disposition = RtExpectedFrameDisposition::Rejected;
        row.rejectionReason = reason;
        ++rejectedCount_;
        RecordFailure(reason, false);
    }
    cpuCore_.Invalidate();
}

bool RtBenchmarkEvidenceRun::Finalize() noexcept
{
    if (invalidRun_ && status_ == RtBenchmarkRunStatus::Complete)
    {
        status_ = RtBenchmarkRunStatus::Invalid;
        return false;
    }
    if (status_ == RtBenchmarkRunStatus::Complete)
    {
        return true;
    }
    if (status_ != RtBenchmarkRunStatus::Measuring)
    {
        return false;
    }
    if (!drainResultRecorded_)
    {
        return false;
    }
    if (OutstandingCount() != 0u)
    {
        RejectOutstanding(RtBenchmarkFailureReason::MissingCompletion);
    }
    if (!drainSucceeded_)
    {
        status_ = invalidRun_ ? RtBenchmarkRunStatus::Invalid
                              : RtBenchmarkRunStatus::Incomplete;
        return false;
    }
    if (expectedCount_ == 0u || AccountedCount() != expectedCount_)
    {
        RecordFailure(RtBenchmarkFailureReason::InvalidState, true);
        status_ = RtBenchmarkRunStatus::Invalid;
        return false;
    }
    if (invalidRun_)
    {
        status_ = RtBenchmarkRunStatus::Invalid;
        return false;
    }
    if (rejectedCount_ != 0u || cancelledCount_ != 0u || cpuRejectedCount_ != 0u ||
        completedCount_ != expectedCount_ || cpuAcceptedCount_ != expectedCount_ ||
        PendingCompletionCount() != 0u || !cpuCore_.CompleteReportEligible())
    {
        status_ = RtBenchmarkRunStatus::Incomplete;
        return false;
    }
    status_ = RtBenchmarkRunStatus::Complete;
    return true;
}

void RtBenchmarkEvidenceRun::Cancel() noexcept
{
    if (status_ != RtBenchmarkRunStatus::Allocated &&
        status_ != RtBenchmarkRunStatus::Measuring)
    {
        return;
    }
    for (std::size_t index = 0u; index < expectedCount_; ++index)
    {
        RtExpectedFrameRecord& row = expectedFrames_[index];
        if (row.disposition != RtExpectedFrameDisposition::AwaitingSubmission &&
            row.disposition != RtExpectedFrameDisposition::PendingCompletion)
        {
            continue;
        }
        if (row.disposition == RtExpectedFrameDisposition::PendingCompletion)
        {
            const std::uint32_t frameSlot = row.submittedIdentity.frame.frameSlot;
            if (frameSlot < pendingSlots_.size() &&
                pendingSlots_[frameSlot].occupied &&
                pendingSlots_[frameSlot].expectedIndex == index)
            {
                pendingSlots_[frameSlot] = {};
            }
        }
        row.disposition = RtExpectedFrameDisposition::Cancelled;
        row.rejectionReason = RtBenchmarkFailureReason::Cancelled;
        ++cancelledCount_;
        RecordFailure(RtBenchmarkFailureReason::Cancelled, false);
    }
    invalidRun_ = true;
    cpuCore_.Invalidate();
    status_ = RtBenchmarkRunStatus::Cancelled;
}

bool RtBenchmarkEvidenceRun::StatisticsForCpu(const std::uint32_t zone,
                                              const bool filterZone,
                                              const RtStage stage,
                                              RtStageStatistics& output) const noexcept
{
    output = {};
    const std::size_t stageIndex = RtStageIndex(stage);
    if (status_ != RtBenchmarkRunStatus::Complete || invalidRun_ ||
        stageIndex >= kRtStageCount || expectedCount_ == 0u ||
        cpuAcceptedCount_ != expectedCount_)
    {
        return false;
    }
    if (!filterZone)
    {
        return cpuCore_.Statistics(
            std::span<const RtCompletedStageSample>(cpuSamples_.get(), capacity_),
            stage,
            std::span<std::uint64_t>(scratch_.get(), capacity_),
            output);
    }

    std::size_t sampleCount = 0u;
    for (std::size_t index = 0u; index < expectedCount_; ++index)
    {
        const RtExpectedFrameRecord& row = expectedFrames_[index];
        if (row.tag.zone != zone)
        {
            continue;
        }
        if (!row.cpuAccepted || row.cpuSampleIndex >= cpuCore_.Size())
        {
            return false;
        }
        scratch_[sampleCount++] =
            cpuSamples_[row.cpuSampleIndex].stages.values[stageIndex].durationNanoseconds;
    }
    return ComputeRtDurationStatistics(
        std::span<std::uint64_t>(scratch_.get(), sampleCount), output);
}

bool RtBenchmarkEvidenceRun::StatisticsForGpu(const std::uint32_t zone,
                                              const bool filterZone,
                                              RtStageStatistics& output) const noexcept
{
    output = {};
    const bool finalized = status_ == RtBenchmarkRunStatus::Complete ||
                           status_ == RtBenchmarkRunStatus::Incomplete;
    if (!finalized || invalidRun_ || !drainResultRecorded_ || !drainSucceeded_ ||
        expectedCount_ == 0u || AccountedCount() != expectedCount_)
    {
        return false;
    }
    std::size_t sampleCount = 0u;
    for (std::size_t index = 0u; index < expectedCount_; ++index)
    {
        const RtExpectedFrameRecord& row = expectedFrames_[index];
        if (filterZone && row.tag.zone != zone)
        {
            continue;
        }
        if (row.disposition != RtExpectedFrameDisposition::Completed ||
            !row.hasGpuStatus || row.gpuStatus != RtSampleStatus::Valid ||
            !row.hasGpuDuration)
        {
            return false;
        }
        scratch_[sampleCount++] = row.gpuDurationNanoseconds;
    }
    return ComputeRtDurationStatistics(
        std::span<std::uint64_t>(scratch_.get(), sampleCount), output);
}

bool RtBenchmarkEvidenceRun::CpuStatistics(const RtStage stage,
                                           RtStageStatistics& output) const noexcept
{
    return StatisticsForCpu(0u, false, stage, output);
}

bool RtBenchmarkEvidenceRun::CpuZoneStatistics(const std::uint32_t zone,
                                               const RtStage stage,
                                               RtStageStatistics& output) const noexcept
{
    return StatisticsForCpu(zone, true, stage, output);
}

bool RtBenchmarkEvidenceRun::GpuStatistics(RtStageStatistics& output) const noexcept
{
    return StatisticsForGpu(0u, false, output);
}

bool RtBenchmarkEvidenceRun::GpuZoneStatistics(const std::uint32_t zone,
                                               RtStageStatistics& output) const noexcept
{
    return StatisticsForGpu(zone, true, output);
}

bool RtBenchmarkEvidenceRun::TryGetExpectedFrame(
    const std::size_t index,
    RtExpectedFrameRecord& output) const noexcept
{
    if (index >= expectedCount_ || expectedFrames_ == nullptr)
    {
        output = {};
        return false;
    }
    output = expectedFrames_[index];
    return true;
}

RtGpuSampleStatusCounts RtBenchmarkEvidenceRun::GpuStatusCounts() const noexcept
{
    RtGpuSampleStatusCounts counts{};
    for (std::size_t index = 0u; index < expectedCount_; ++index)
    {
        const RtExpectedFrameRecord& row = expectedFrames_[index];
        if (!row.hasGpuStatus)
        {
            ++counts.missing;
            continue;
        }
        switch (row.gpuStatus)
        {
        case RtSampleStatus::NotReady: ++counts.notReady; break;
        case RtSampleStatus::CompiledOut: ++counts.compiledOut; break;
        case RtSampleStatus::Valid: ++counts.valid; break;
        case RtSampleStatus::Disabled: ++counts.disabled; break;
        case RtSampleStatus::Unsupported: ++counts.unsupported; break;
        case RtSampleStatus::Pending: ++counts.pending; break;
        case RtSampleStatus::Error: ++counts.error; break;
        default: ++counts.unknown; break;
        }
    }
    return counts;
}

std::uint64_t RtBenchmarkEvidenceRun::FailureCount(
    const RtBenchmarkFailureReason reason) const noexcept
{
    const std::size_t index = static_cast<std::size_t>(reason);
    return index < failureCounts_.size() ? failureCounts_[index] : 0u;
}

std::size_t RtBenchmarkEvidenceRun::PendingCompletionCount() const noexcept
{
    std::size_t count = 0u;
    for (const PendingSlot& slot : pendingSlots_)
    {
        count += slot.occupied ? 1u : 0u;
    }
    return count;
}

std::size_t RtBenchmarkEvidenceRun::AccountedCount() const noexcept
{
    return completedCount_ + rejectedCount_ + cancelledCount_;
}

std::size_t RtBenchmarkEvidenceRun::OutstandingCount() const noexcept
{
    const std::size_t accounted = AccountedCount();
    return accounted <= expectedCount_ ? expectedCount_ - accounted : 0u;
}

} // namespace horde::telemetry
