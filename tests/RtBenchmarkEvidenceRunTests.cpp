#include "telemetry/RtBenchmarkEvidenceRun.h"

#include <algorithm>
#include <array>
#include <cmath>
#include <cstddef>
#include <cstdint>
#include <cstdlib>
#include <iostream>
#include <limits>
#include <new>
#include <optional>
#include <span>
#include <string_view>
#include <type_traits>
#include <utility>
#include <vector>

#include "gameplay/ShowcaseBenchmark.h"
#include "telemetry/RtPerformanceEvidence.h"

namespace
{

std::size_t gNothrowArrayAllocationCall = 0u;
std::size_t gFailNothrowArrayAllocationCall = 0u;
bool gTrackNothrowArrayAllocations = false;

void* AllocateArray(const std::size_t size)
{
    if (void* value = std::malloc(std::max<std::size_t>(size, 1u)))
    {
        return value;
    }
    throw std::bad_alloc{};
}

void* AllocateArrayNoThrow(const std::size_t size) noexcept
{
    if (gTrackNothrowArrayAllocations)
    {
        ++gNothrowArrayAllocationCall;
        if (gNothrowArrayAllocationCall == gFailNothrowArrayAllocationCall)
        {
            return nullptr;
        }
    }
    return std::malloc(std::max<std::size_t>(size, 1u));
}

void BeginNothrowArrayFailure(const std::size_t call) noexcept
{
    gNothrowArrayAllocationCall = 0u;
    gFailNothrowArrayAllocationCall = call;
    gTrackNothrowArrayAllocations = true;
}

void EndNothrowArrayFailure() noexcept
{
    gTrackNothrowArrayAllocations = false;
    gFailNothrowArrayAllocationCall = 0u;
}

} // namespace

void* operator new[](const std::size_t size) { return AllocateArray(size); }
void* operator new[](const std::size_t size, const std::nothrow_t&) noexcept
{
    return AllocateArrayNoThrow(size);
}
void operator delete[](void* value) noexcept { std::free(value); }
void operator delete[](void* value, const std::size_t) noexcept { std::free(value); }
void operator delete[](void* value, const std::nothrow_t&) noexcept { std::free(value); }

namespace
{

using namespace horde::gameplay;
using namespace horde::telemetry;

static_assert(!std::is_copy_constructible_v<RtBenchmarkEvidenceRun>);
static_assert(!std::is_copy_assignable_v<RtBenchmarkEvidenceRun>);
static_assert(std::is_nothrow_move_constructible_v<RtBenchmarkEvidenceRun>);
static_assert(std::is_nothrow_move_assignable_v<RtBenchmarkEvidenceRun>);

struct TestContext
{
    int failures = 0;

    void Check(const bool condition, const std::string_view message)
    {
        if (!condition)
        {
            std::cerr << "FAIL: " << message << '\n';
            ++failures;
        }
    }
};

template <std::size_t Capacity>
void SetText(RtFixedText<Capacity>& output, const std::string_view value)
{
    if (!AssignRtFixedText(output, value))
    {
        std::abort();
    }
}

RtPipelineEvidenceIdentity MakePipelineIdentity()
{
    RtPipelineEvidenceIdentity pipeline{};
    pipeline.executionMode = RtExecutionMode::RayTracingPipeline;
    pipeline.instrumentation = RtInstrumentationMode::Shipping;
    pipeline.dielectricQuality = RtDielectricQuality::Mobile;
    pipeline.activeStrategy = RtMaterialStrategy::OpaqueFast;
    pipeline.waterQuality = RtWaterQuality::Mobile;
    SetText(pipeline.bundleKey, "shipping_mobile_pair");
    SetText(pipeline.opaqueFast.key, "shipping_mobile_opaque");
    SetText(pipeline.opaqueFast.sha256,
            "aaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaa");
    SetText(pipeline.genericDielectric.key, "shipping_mobile_generic");
    SetText(pipeline.genericDielectric.sha256,
            "bbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbb");
    pipeline.active = pipeline.opaqueFast;
    return pipeline;
}

RtStageFrameSample MakeStages(const std::uint64_t wholeFrameNanoseconds)
{
    RtStageFrameSample stages{};
    stages.status = RtSampleStatus::Valid;
    for (std::size_t index = 0u; index < stages.values.size(); ++index)
    {
        stages.values[index].durationNanoseconds =
            wholeFrameNanoseconds / 10u + static_cast<std::uint64_t>(index);
        stages.values[index].operationCount = 1u;
    }
    stages.values[RtStageIndex(RtStage::WholeFrameCycle)].durationNanoseconds =
        wholeFrameNanoseconds;
    const RtStageValue& player = stages.values[RtStageIndex(RtStage::PlayerSkin)];
    const RtStageValue& character = stages.values[RtStageIndex(RtStage::CharacterSkin)];
    RtStageValue& skin = stages.values[RtStageIndex(RtStage::Skin)];
    skin.durationNanoseconds = player.durationNanoseconds + character.durationNanoseconds;
    skin.workInvocationCount = player.workInvocationCount + character.workInvocationCount;
    skin.byteCount = player.byteCount + character.byteCount;
    skin.operationCount = player.operationCount + character.operationCount;
    return stages;
}

RtPerformanceEvidenceSnapshot MakeSnapshot(const std::uint64_t serial,
                                           const std::uint64_t sceneEpoch,
                                           const std::uint64_t measurementGeneration,
                                           const std::uint32_t frameSlot,
                                           const std::uint64_t cpuNanoseconds,
                                           const RtSampleStatus gpuStatus,
                                           const std::uint64_t gpuNanoseconds,
                                           const bool cpuEligible = true)
{
    RtPerformanceEvidenceSnapshot snapshot{};
    snapshot.identity.submitted.frame.sceneEpoch = sceneEpoch;
    snapshot.identity.submitted.frame.measurementGeneration = measurementGeneration;
    snapshot.identity.submitted.frame.recordAttemptSerial = serial;
    snapshot.identity.submitted.frame.recordSerial = serial;
    snapshot.identity.submitted.frame.simulationTick = 1'000u + serial;
    snapshot.identity.submitted.frame.frameSlot = frameSlot;
    snapshot.identity.submitted.submissionSerial = serial;
    snapshot.identity.completionSerial = serial;
    snapshot.scene.pipeline = MakePipelineIdentity();
    snapshot.scene.stages = MakeStages(cpuNanoseconds);
    snapshot.scene.dispatch.sceneReady = true;
    snapshot.scene.dispatch.rtDispatchRecorded = true;
    snapshot.scene.dispatch.swapchainCopyRecorded = true;
    snapshot.dielectric.status = RtSampleStatus::CompiledOut;
    snapshot.gpu.status = gpuStatus;
    if (gpuStatus == RtSampleStatus::Valid)
    {
        snapshot.gpu.hasDuration = true;
        snapshot.gpu.durationNanoseconds = gpuNanoseconds;
        snapshot.gpu.completedSubmissionSerial = serial;
        snapshot.gpu.timestampValidBits = 64u;
        snapshot.gpu.timestampPeriodPicoseconds = 1'000u;
        snapshot.gpu.sampleCount = 1u;
    }
    else if (gpuStatus == RtSampleStatus::Error)
    {
        snapshot.gpu.completedSubmissionSerial = serial;
        snapshot.gpu.errorCount = 1u;
        SetText(snapshot.gpu.detail, "query-read-error");
    }
    snapshot.presentation.outcome = RtPresentationOutcome::Presented;
    snapshot.presentation.lastSuccessfulPresentSubmissionSerial = serial;
    snapshot.cpuBenchmarkEligible = cpuEligible;
    snapshot.benchmarkEligible = cpuEligible &&
        (gpuStatus == RtSampleStatus::Valid || gpuStatus == RtSampleStatus::Disabled ||
         gpuStatus == RtSampleStatus::Unsupported);
    return snapshot;
}

void CheckCanonical(TestContext& context,
                    const RtPerformanceEvidenceSnapshot& snapshot,
                    const std::string_view message)
{
    RtEvidenceValidationError error = RtEvidenceValidationError::None;
    context.Check(ValidateRtPerformanceEvidence(snapshot, error), message);
}

bool SameStatistics(const RtStageStatistics& left, const RtStageStatistics& right)
{
    return left.valid == right.valid && left.sampleCount == right.sampleCount &&
           left.meanMilliseconds == right.meanMilliseconds &&
           left.medianMilliseconds == right.medianMilliseconds &&
           left.p90Milliseconds == right.p90Milliseconds &&
           left.p95Milliseconds == right.p95Milliseconds &&
           left.slowestOnePercentMeanMilliseconds ==
               right.slowestOnePercentMeanMilliseconds &&
           left.onePercentLowFps == right.onePercentLowFps;
}

std::optional<std::size_t> ExpectAndBind(TestContext& context,
                                         RtBenchmarkEvidenceRun& run,
                                         const RtBenchmarkFrameTag tag,
                                         const RtPerformanceEvidenceSnapshot& snapshot)
{
    const std::optional<std::size_t> index = run.ExpectFrame(tag);
    context.Check(index.has_value(), "intended frame must enter the expected ledger");
    if (index.has_value())
    {
        context.Check(run.BindSubmitted(*index, snapshot.identity.submitted),
                      "actual committed identity must bind to its expected row");
    }
    return index;
}

void TestStartCapacityAndAllocationFailure(TestContext& context)
{
    RtBenchmarkEvidenceRun run;
    context.Check(!run.Start(0u) && run.Status() == RtBenchmarkRunStatus::Invalid &&
                      run.LastFailureReason() == RtBenchmarkFailureReason::InvalidCapacity &&
                      run.Capacity() == 0u,
                  "zero capacity must fail explicitly without a configured candidate");

    BeginNothrowArrayFailure(0u);
    const bool overflowStarted = run.Start(std::numeric_limits<std::size_t>::max());
    const std::size_t overflowAllocationCalls = gNothrowArrayAllocationCall;
    EndNothrowArrayFailure();
    context.Check(!overflowStarted && overflowAllocationCalls == 0u &&
                      run.Status() == RtBenchmarkRunStatus::Invalid &&
                      run.LastFailureReason() == RtBenchmarkFailureReason::CapacityOverflow,
                  "impossible capacity must fail before any array allocation");

    for (std::size_t failedAllocation = 1u; failedAllocation <= 3u; ++failedAllocation)
    {
        RtBenchmarkEvidenceRun candidate;
        context.Check(candidate.Start(4u) && candidate.ArmMeasurement(10u, 20u) &&
                          candidate.ExpectFrame({1u, 2u}).has_value(),
                      "allocation-failure fixture must begin from a configured old run");
        BeginNothrowArrayFailure(failedAllocation);
        const bool started = candidate.Start(8u);
        const std::size_t observedCalls = gNothrowArrayAllocationCall;
        EndNothrowArrayFailure();
        context.Check(!started && observedCalls == failedAllocation &&
                          candidate.Status() == RtBenchmarkRunStatus::Invalid &&
                          candidate.LastFailureReason() ==
                              RtBenchmarkFailureReason::AllocationFailed &&
                          candidate.Capacity() == 0u && candidate.ExpectedCount() == 0u &&
                          candidate.CpuAcceptedCount() == 0u,
                      "each typed allocation failure must release the old run and leave no half-configured candidate");
    }

    context.Check(run.Start(ShowcaseBenchmarkRun::kMaximumFramesPerLap) &&
                      run.ArmMeasurement(11u, 21u),
                  "authoritative maximum route capacity must allocate and arm");
    bool retainedExactCapacity = true;
    for (std::uint32_t frame = 0u;
         frame < ShowcaseBenchmarkRun::kMaximumFramesPerLap;
         ++frame)
    {
        retainedExactCapacity = run.ExpectFrame({frame % 10u, 2u}).has_value() &&
                                retainedExactCapacity;
    }
    context.Check(retainedExactCapacity &&
                      run.ExpectedCount() == ShowcaseBenchmarkRun::kMaximumFramesPerLap,
                  "the exact 4000-frame capacity must retain every intended row");
    context.Check(!run.ExpectFrame({0u, 2u}).has_value() && run.InvalidRun() &&
                      run.FailureCount(RtBenchmarkFailureReason::CapacityExceeded) == 1u &&
                      run.ExpectedCount() == ShowcaseBenchmarkRun::kMaximumFramesPerLap,
                  "one further intended frame must invalidate rather than truncate the denominator");
}

void TestStatisticsUseSharedFullRouteMath(TestContext& context)
{
    constexpr std::size_t kSampleCount = 101u;
    RtBenchmarkEvidenceRun run;
    context.Check(run.Start(kSampleCount) && run.ArmMeasurement(30u, 40u),
                  "statistics fixture must start and arm");
    std::vector<std::uint64_t> expectedCpu;
    std::vector<std::uint64_t> expectedGpu;
    std::vector<std::uint64_t> expectedEvenZoneCpu;
    expectedCpu.reserve(kSampleCount);
    expectedGpu.reserve(kSampleCount);
    expectedEvenZoneCpu.reserve(kSampleCount / 2u);
    for (std::size_t index = 0u; index < kSampleCount; ++index)
    {
        const std::uint64_t serial = static_cast<std::uint64_t>(index + 1u);
        const std::uint64_t cpu = (serial == kSampleCount ? 90u : serial) * 1'000'000u;
        const std::uint64_t gpu = (serial == kSampleCount ? 70u : serial + 3u) * 1'000'000u;
        const std::uint32_t zone = (index % 2u) == 0u ? 7u : 9u;
        const RtPerformanceEvidenceSnapshot snapshot =
            MakeSnapshot(serial, 30u, 40u, static_cast<std::uint32_t>(index % 4u),
                         cpu, RtSampleStatus::Valid, gpu);
        CheckCanonical(context, snapshot, "statistics fixture must use canonical completions");
        ExpectAndBind(context, run, {zone, 2u}, snapshot);
        context.Check(run.Complete(snapshot), "canonical completion must resolve its exact slot");
        expectedCpu.push_back(cpu);
        expectedGpu.push_back(gpu);
        if (zone == 9u)
        {
            expectedEvenZoneCpu.push_back(cpu);
        }
    }
    context.Check(run.RecordOwnerDrainResult(true) && run.Finalize() &&
                      run.Status() == RtBenchmarkRunStatus::Complete,
                  "fully accounted CPU/GPU evidence must finalize complete");

    RtStageStatistics expectedCpuStatistics{};
    RtStageStatistics expectedGpuStatistics{};
    RtStageStatistics expectedEvenZoneStatistics{};
    context.Check(ComputeRtDurationStatistics(expectedCpu, expectedCpuStatistics) &&
                      ComputeRtDurationStatistics(expectedGpu, expectedGpuStatistics) &&
                      ComputeRtDurationStatistics(expectedEvenZoneCpu,
                                                  expectedEvenZoneStatistics),
                  "shared statistics helper must accept the comparison vectors");
    RtStageStatistics actualCpu{};
    RtStageStatistics actualGpu{};
    RtStageStatistics actualEvenZone{};
    context.Check(run.CpuStatistics(RtStage::WholeFrameCycle, actualCpu) &&
                      SameStatistics(actualCpu, expectedCpuStatistics),
                  "overall CPU distribution must be byte-for-byte shared-helper math");
    context.Check(run.GpuStatistics(actualGpu) &&
                      SameStatistics(actualGpu, expectedGpuStatistics),
                  "overall GPU distribution must be byte-for-byte shared-helper math");
    context.Check(run.CpuZoneStatistics(9u, RtStage::WholeFrameCycle, actualEvenZone) &&
                      SameStatistics(actualEvenZone, expectedEvenZoneStatistics) &&
                      actualEvenZone.sampleCount == 50u,
                  "per-zone even median must retain the submitted tag and shared math");
    context.Check(actualCpu.sampleCount == 101u && actualCpu.p90Milliseconds == 90.0 &&
                      actualCpu.p95Milliseconds == 95.0 &&
                      actualCpu.slowestOnePercentMeanMilliseconds == 99.5 &&
                      actualCpu.onePercentLowFps == 1'000.0 / 99.5,
                  "nearest-rank percentiles and raw slowest-one-percent mean must remain distinct");
}

void TestOneFrameLateAssociationAndFinalDrain(TestContext& context)
{
    RtBenchmarkEvidenceRun run;
    context.Check(run.Start(4u) && run.ArmMeasurement(50u, 60u),
                  "association fixture must start and arm");
    const RtPerformanceEvidenceSnapshot frameA =
        MakeSnapshot(1u, 50u, 60u, 0u, 10'000'000u, RtSampleStatus::Valid, 8'000'000u);
    const RtPerformanceEvidenceSnapshot frameB =
        MakeSnapshot(2u, 50u, 60u, 1u, 20'000'000u, RtSampleStatus::Valid, 9'000'000u);
    const auto a = ExpectAndBind(context, run, {3u, 2u}, frameA);
    const auto b = ExpectAndBind(context, run, {8u, 2u}, frameB);
    context.Check(run.Complete(frameA),
                  "frame A must complete after the current replay tag has advanced to B");
    RtExpectedFrameRecord row{};
    context.Check(a.has_value() && run.TryGetExpectedFrame(*a, row) && row.tag.zone == 3u &&
                      row.disposition == RtExpectedFrameDisposition::Completed,
                  "completion must retain frame A's saved submitted zone");
    context.Check(b.has_value() && run.TryGetExpectedFrame(*b, row) && row.tag.zone == 8u &&
                      row.disposition == RtExpectedFrameDisposition::PendingCompletion,
                  "the newer current frame must remain a distinct pending row");
    context.Check(!run.Finalize() && run.Status() == RtBenchmarkRunStatus::Measuring &&
                      run.PendingCompletionCount() == 1u,
                  "the last pending frame must prevent publication before an explicit drain");
    context.Check(run.Complete(frameB) && run.RecordOwnerDrainResult(true) && run.Finalize() &&
                      run.CompletedCount() == 2u && run.OutstandingCount() == 0u,
                  "the exact final token plus successful drain must complete the full ledger");
}

void TestDrainSealsSubmissionButStillAcceptsItsExistingCompletion(TestContext& context)
{
    RtBenchmarkEvidenceRun run;
    context.Check(run.Start(2u) && run.ArmMeasurement(51u, 52u),
                  "drain sealing fixture must start and arm");
    const auto pending = MakeSnapshot(
        1u, 51u, 52u, 0u, 1'000'000u, RtSampleStatus::Disabled, 0u);
    ExpectAndBind(context, run, {1u, 2u}, pending);
    context.Check(run.RecordOwnerDrainResult(true),
                  "owner drain outcome must be recorded exactly once");
    context.Check(run.Complete(pending) && run.Finalize(),
                  "a completion already pending when the drain began must still be accepted");

    context.Check(!run.ExpectFrame({2u, 2u}).has_value() && run.InvalidRun() &&
                      run.Status() == RtBenchmarkRunStatus::Invalid && !run.Finalize(),
                  "a post-drain intended frame must invalidate and cannot preserve a complete claim");

    RtBenchmarkEvidenceRun unbound;
    context.Check(unbound.Start(2u) && unbound.ArmMeasurement(53u, 54u),
                  "post-drain bind fixture must start and arm");
    const auto index = unbound.ExpectFrame({1u, 2u});
    const auto snapshot = MakeSnapshot(
        1u, 53u, 54u, 0u, 1'000'000u, RtSampleStatus::Disabled, 0u);
    context.Check(index.has_value() && unbound.RecordOwnerDrainResult(true) &&
                      !unbound.ExpectFrame({2u, 2u}).has_value() &&
                      !unbound.BindSubmitted(*index, snapshot.identity.submitted),
                  "the final drain seal must prohibit new expected rows and newly bound identities");
}

void TestCommittedBindingIsMonotonicWithoutInventingIdentity(TestContext& context)
{
    {
        RtBenchmarkEvidenceRun run;
        context.Check(run.Start(3u) && run.ArmMeasurement(61u, 62u),
                      "monotonic binding fixture must start and arm");
        for (std::uint64_t serial = 1u; serial <= 3u; ++serial)
        {
            const auto snapshot = MakeSnapshot(
                serial,
                61u,
                62u,
                static_cast<std::uint32_t>(serial % kRtMaximumFrameSlots),
                serial * 1'000'000u,
                RtSampleStatus::Disabled,
                0u);
            ExpectAndBind(context, run, {1u, 2u}, snapshot);
            context.Check(run.Complete(snapshot),
                          "strictly increasing actual committed identities must remain valid");
        }
        context.Check(run.RecordOwnerDrainResult(true) && run.Finalize(),
                      "normal multi-row committed binding must finalize complete");
    }

    {
        RtBenchmarkEvidenceRun run;
        context.Check(run.Start(2u) && run.ArmMeasurement(63u, 64u),
                      "reused submission serial fixture must start and arm");
        const auto first = MakeSnapshot(
            1u, 63u, 64u, 0u, 1'000'000u, RtSampleStatus::Disabled, 0u);
        ExpectAndBind(context, run, {1u, 2u}, first);
        context.Check(run.Complete(first), "first committed identity must complete");

        auto alteredToken = MakeSnapshot(
            2u, 63u, 64u, 1u, 2'000'000u, RtSampleStatus::Disabled, 0u);
        alteredToken.identity.submitted.submissionSerial =
            first.identity.submitted.submissionSerial;
        CheckCanonical(context, alteredToken,
                       "reused submission serial with a changed token remains canonical in isolation");
        const auto second = run.ExpectFrame({2u, 2u});
        context.Check(second.has_value() &&
                          !run.BindSubmitted(*second, alteredToken.identity.submitted) &&
                          run.FailureCount(
                              RtBenchmarkFailureReason::InvalidSubmittedIdentity) == 1u,
                      "a changed token must not hide reuse of an observed committed submission serial");
    }

    {
        RtBenchmarkEvidenceRun run;
        context.Check(run.Start(2u) && run.ArmMeasurement(65u, 66u),
                      "reused record serial fixture must start and arm");
        const auto first = MakeSnapshot(
            1u, 65u, 66u, 0u, 1'000'000u, RtSampleStatus::Disabled, 0u);
        ExpectAndBind(context, run, {1u, 2u}, first);
        context.Check(run.Complete(first), "first record serial must complete");
        auto reusedRecord = MakeSnapshot(
            2u, 65u, 66u, 1u, 2'000'000u, RtSampleStatus::Disabled, 0u);
        reusedRecord.identity.submitted.frame.recordSerial =
            first.identity.submitted.frame.recordSerial;
        const auto second = run.ExpectFrame({2u, 2u});
        context.Check(second.has_value() &&
                          !run.BindSubmitted(*second, reusedRecord.identity.submitted),
                      "committed record and attempt serials must also advance monotonically");
    }
}

void TestCompletionIdentityIsMonotonicIndependentOfCpuAdmission(TestContext& context)
{
    {
        RtBenchmarkEvidenceRun run;
        context.Check(run.Start(2u) && run.ArmMeasurement(67u, 68u),
                      "all-CPU-ineligible completion identity fixture must start");
        const auto first = MakeSnapshot(
            1u, 67u, 68u, 0u, 1'000'000u, RtSampleStatus::Valid, 2'000'000u, false);
        auto reusedCompletionSerial = MakeSnapshot(
            2u, 67u, 68u, 1u, 1'000'000u, RtSampleStatus::Valid, 2'000'000u, false);
        reusedCompletionSerial.identity.completionSerial = first.identity.completionSerial;
        ExpectAndBind(context, run, {1u, 2u}, first);
        context.Check(run.Complete(first),
                      "first CPU-ineligible completion must establish the observed serial floor");
        ExpectAndBind(context, run, {2u, 2u}, reusedCompletionSerial);
        context.Check(!run.Complete(reusedCompletionSerial) && run.InvalidRun() &&
                          run.PendingCompletionCount() == 1u &&
                          run.FailureCount(
                              RtBenchmarkFailureReason::InvalidCompletionIdentity) == 1u,
                      "a distinct submission must not reuse a completion serial when CPU is ineligible");
    }

    {
        RtBenchmarkEvidenceRun run;
        context.Check(run.Start(2u) && run.ArmMeasurement(69u, 70u),
                      "mixed CPU-admission completion identity fixture must start");
        auto cpuIneligible = MakeSnapshot(
            1u, 69u, 70u, 0u, 1'000'000u, RtSampleStatus::Valid, 2'000'000u, false);
        cpuIneligible.identity.completionSerial = 5u;
        auto cpuEligible = MakeSnapshot(
            2u, 69u, 70u, 1u, 1'000'000u, RtSampleStatus::Valid, 2'000'000u, true);
        cpuEligible.identity.completionSerial = 4u;
        ExpectAndBind(context, run, {1u, 2u}, cpuIneligible);
        context.Check(run.Complete(cpuIneligible),
                      "CPU-ineligible completion must still advance the owner identity floor");
        ExpectAndBind(context, run, {2u, 2u}, cpuEligible);
        context.Check(!run.Complete(cpuEligible) && run.CpuAcceptedCount() == 0u &&
                          run.PendingCompletionCount() == 1u && run.InvalidRun(),
                      "later CPU eligibility must not allow a decreasing completion serial past the owner");
    }
}

void TestRealTwoLapRouteRetainsEveryMeasuredFrame(TestContext& context)
{
    constexpr std::uint64_t kSceneEpoch = 70u;
    constexpr std::uint64_t kMeasurementGeneration = 80u;
    ShowcaseBenchmarkRun benchmark;
    benchmark.Start(2u);
    RtBenchmarkEvidenceRun run;
    context.Check(run.Start(ShowcaseBenchmarkRun::kMaximumFramesPerLap),
                  "full route owner must allocate from the authoritative route limit");

    bool armed = false;
    std::uint64_t serial = 0u;
    std::optional<RtPerformanceEvidenceSnapshot> delayedCompletion;
    std::size_t loopGuard = 0u;
    while (benchmark.IsRunning() && loopGuard < 8'000u)
    {
        const ShowcaseBenchmarkAdvance advance = benchmark.Advance();
        benchmark.RecordFrame(12.5, true);
        ++loopGuard;
        if (benchmark.CurrentLap() != benchmark.TotalLaps())
        {
            continue;
        }
        if (!armed)
        {
            context.Check(advance.lapStarted && run.ArmMeasurement(
                              kSceneEpoch, kMeasurementGeneration),
                          "measurement must arm only after the warmup-to-measured transition");
            armed = true;
        }
        if (delayedCompletion.has_value())
        {
            context.Check(run.Complete(*delayedCompletion),
                          "one-frame-late route completion must resolve its prior saved row");
        }

        ++serial;
        RtPerformanceEvidenceSnapshot snapshot = MakeSnapshot(
            serial,
            kSceneEpoch,
            kMeasurementGeneration,
            static_cast<std::uint32_t>(serial % kRtMaximumFrameSlots),
            12'000'000u + serial,
            RtSampleStatus::Valid,
            9'000'000u + serial);
        const std::uint32_t zone = static_cast<std::uint32_t>(advance.replay.zone);
        ExpectAndBind(context, run, {zone, benchmark.CurrentLap()}, snapshot);
        delayedCompletion = snapshot;
    }

    context.Check(benchmark.Status() == ShowcaseBenchmarkStatus::Complete && benchmark.Passed(),
                  "unchanged production two-lap replay must finish before evidence assertions");
    context.Check(armed && serial == 1'838u && benchmark.Frames().size() == 1'838u &&
                      run.ExpectedCount() == 1'838u && run.CompletedCount() == 1'837u &&
                      run.PendingCompletionCount() == 1u,
                  "production traversal must retain 1838 intended frames including the final pending row");
    context.Check(!run.Finalize(),
                  "route completion alone must not certify its one-frame-late evidence ledger");
    context.Check(delayedCompletion.has_value() && run.Complete(*delayedCompletion) &&
                      run.RecordOwnerDrainResult(true) && run.Finalize(),
                  "successful final-idle delivery of the exact last snapshot must permit publication");

    std::array<std::size_t, 10u> expectedZoneCounts{{160u, 98u, 599u, 189u, 157u,
                                                     157u, 157u, 157u, 63u, 101u}};
    const std::array<ShowcaseZone, 10u> zones{{
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
    std::size_t zoneSum = 0u;
    for (std::size_t zoneIndex = 0u; zoneIndex < zones.size(); ++zoneIndex)
    {
        std::size_t ownerCount = 0u;
        for (std::size_t rowIndex = 0u; rowIndex < run.ExpectedCount(); ++rowIndex)
        {
            RtExpectedFrameRecord row{};
            if (run.TryGetExpectedFrame(rowIndex, row) &&
                row.tag.zone == static_cast<std::uint32_t>(zones[zoneIndex]))
            {
                ++ownerCount;
            }
        }
        context.Check(ownerCount == expectedZoneCounts[zoneIndex],
                      "full route expected ledger must preserve the production zone count");
        context.Check(ownerCount == benchmark.ZoneStatistics(zones[zoneIndex]).frames,
                      "evidence zones must match the unchanged production benchmark route");
        zoneSum += ownerCount;
    }
    context.Check(zoneSum == 1'838u &&
                      expectedZoneCounts[2u] == 599u && run.CpuAcceptedCount() == 1'838u,
                  "all measured zones must sum to 1838 with 599 shadow-corridor frames");
}

void TestGpuAvailabilityDoesNotEraseCpuEvidence(TestContext& context)
{
    constexpr std::array<RtSampleStatus, 4u> kStatuses{{
        RtSampleStatus::Disabled,
        RtSampleStatus::Unsupported,
        RtSampleStatus::Pending,
        RtSampleStatus::Error,
    }};
    RtBenchmarkEvidenceRun run;
    context.Check(run.Start(kStatuses.size()) && run.ArmMeasurement(90u, 100u),
                  "CPU/GPU split fixture must start and arm");
    for (std::size_t index = 0u; index < kStatuses.size(); ++index)
    {
        const auto snapshot = MakeSnapshot(
            index + 1u, 90u, 100u, static_cast<std::uint32_t>(index),
            (index + 1u) * 1'000'000u, kStatuses[index], 0u, true);
        CheckCanonical(context, snapshot,
                       "optional GPU failure must remain a canonical CPU-eligible completion");
        ExpectAndBind(context, run, {1u, 2u}, snapshot);
        context.Check(run.Complete(snapshot),
                      "optional GPU status must still account its exact CPU completion");
    }
    context.Check(run.RecordOwnerDrainResult(true) && run.Finalize() &&
                      run.CpuAcceptedCount() == kStatuses.size(),
                  "all CPU-admitted rows must complete independently of GPU availability");
    RtStageStatistics cpu{};
    RtStageStatistics gpu{};
    context.Check(run.CpuStatistics(RtStage::WholeFrameCycle, cpu) && cpu.valid &&
                      cpu.sampleCount == kStatuses.size(),
                  "CPU statistics must remain available when every GPU sample is non-Valid");
    context.Check(!run.GpuStatistics(gpu) && !gpu.valid && gpu.sampleCount == 0u,
                  "non-Valid GPU statuses must project as no distribution, never measured zero");
    const RtGpuSampleStatusCounts counts = run.GpuStatusCounts();
    context.Check(counts.valid == 0u && counts.disabled == 1u &&
                      counts.unsupported == 1u && counts.pending == 1u &&
                      counts.error == 1u && counts.notReady == 0u &&
                      counts.compiledOut == 0u && counts.unknown == 0u &&
                      counts.missing == 0u,
                  "GPU status counts must preserve every unavailable reason separately");
}

void TestCompletedFailuresRetainTheirCanonicalCause(TestContext& context)
{
    {
        RtBenchmarkEvidenceRun run;
        context.Check(run.Start(3u) && run.ArmMeasurement(101u, 102u),
                      "presentation failure metadata fixture must start");
        auto failed = MakeSnapshot(
            1u, 101u, 102u, 0u, 1'000'000u, RtSampleStatus::Valid, 2'000'000u, false);
        failed.presentation.outcome = RtPresentationOutcome::Failed;
        failed.presentation.lastSuccessfulPresentSubmissionSerial = 0u;
        auto needsRecreate = MakeSnapshot(
            2u, 101u, 102u, 1u, 1'000'000u, RtSampleStatus::Valid, 3'000'000u, false);
        needsRecreate.presentation.outcome =
            RtPresentationOutcome::NotPresentedNeedsRecreate;
        needsRecreate.presentation.lastSuccessfulPresentSubmissionSerial = 0u;
        auto presentedNeedsRecreate = MakeSnapshot(
            3u, 101u, 102u, 2u, 1'000'000u, RtSampleStatus::Valid, 4'000'000u, false);
        presentedNeedsRecreate.presentation.outcome =
            RtPresentationOutcome::PresentedNeedsRecreate;
        CheckCanonical(context, failed,
                       "failed-present completion must remain canonical CPU-ineligible evidence");
        CheckCanonical(context, needsRecreate,
                       "out-of-date completion must remain canonical CPU-ineligible evidence");
        CheckCanonical(context, presentedNeedsRecreate,
                       "successful SUBOPTIMAL presentation must remain canonical CPU-ineligible evidence");
        const auto failedIndex = ExpectAndBind(context, run, {1u, 2u}, failed);
        context.Check(run.Complete(failed),
                      "bound failed-present submission must stay pending until exact completion");
        const auto recreateIndex = ExpectAndBind(context, run, {2u, 2u}, needsRecreate);
        context.Check(run.Complete(needsRecreate),
                      "non-presented recreate completion must retain its exact row");
        const auto presentedRecreateIndex =
            ExpectAndBind(context, run, {3u, 2u}, presentedNeedsRecreate);
        context.Check(run.Complete(presentedNeedsRecreate) &&
                          run.RecordOwnerDrainResult(true) &&
                          !run.Finalize(),
                      "completed presentation failures must finalize CPU-incomplete, not disappear");
        RtExpectedFrameRecord failedRow{};
        RtExpectedFrameRecord recreateRow{};
        RtExpectedFrameRecord presentedRecreateRow{};
        context.Check(failedIndex.has_value() && recreateIndex.has_value() &&
                          presentedRecreateIndex.has_value() &&
                          run.TryGetExpectedFrame(*failedIndex, failedRow) &&
                          run.TryGetExpectedFrame(*recreateIndex, recreateRow) &&
                          run.TryGetExpectedFrame(
                              *presentedRecreateIndex, presentedRecreateRow) &&
                          failedRow.presentationOutcome == RtPresentationOutcome::Failed &&
                          recreateRow.presentationOutcome ==
                              RtPresentationOutcome::NotPresentedNeedsRecreate &&
                          presentedRecreateRow.presentationOutcome ==
                              RtPresentationOutcome::PresentedNeedsRecreate &&
                          failedRow.rejectionReason ==
                              RtBenchmarkFailureReason::PresentationFailed &&
                          recreateRow.rejectionReason ==
                              RtBenchmarkFailureReason::PresentationFailed &&
                          presentedRecreateRow.rejectionReason ==
                              RtBenchmarkFailureReason::PresentedNeedsRecreate &&
                          presentedRecreateRow.rejectionReason !=
                              RtBenchmarkFailureReason::PresentationFailed &&
                          failedRow.hasCompletionIdentity && failedRow.hasGpuStatus &&
                          failedRow.gpuStatus == RtSampleStatus::Valid &&
                          presentedRecreateRow.hasCompletionIdentity &&
                          presentedRecreateRow.hasGpuStatus &&
                          presentedRecreateRow.gpuStatus == RtSampleStatus::Valid,
                      "failed, out-of-date and SUBOPTIMAL rows must retain precise presentation causes");
        RtStageStatistics gpu{};
        context.Check(run.GpuStatistics(gpu) && gpu.sampleCount == 3u,
                      "presentation failure must not erase independently completed GPU timing evidence");
    }

    {
        RtBenchmarkEvidenceRun run;
        context.Check(run.Start(2u) && run.ArmMeasurement(103u, 104u),
                      "stage and Diagnostic failure metadata fixture must start");
        auto stageError = MakeSnapshot(
            1u, 103u, 104u, 0u, 1'000'000u, RtSampleStatus::Valid, 2'000'000u, false);
        stageError.scene.stages = {};
        stageError.scene.stages.status = RtSampleStatus::Error;
        auto diagnosticError = MakeSnapshot(
            2u, 103u, 104u, 1u, 1'000'000u, RtSampleStatus::Valid, 3'000'000u, false);
        diagnosticError.scene.pipeline.instrumentation = RtInstrumentationMode::Diagnostic;
        diagnosticError.dielectric.status = RtSampleStatus::Error;
        diagnosticError.dielectric.compiled = true;
        diagnosticError.dielectric.completedSubmissionSerial =
            diagnosticError.identity.submitted.submissionSerial;
        SetText(diagnosticError.dielectric.detail, "counter-read-error");
        CheckCanonical(context, stageError,
                       "stage Error completion must remain canonical with CPU admission false");
        CheckCanonical(context, diagnosticError,
                       "Diagnostic Error completion must remain canonical with CPU admission false");
        const auto stageIndex = ExpectAndBind(context, run, {3u, 2u}, stageError);
        context.Check(run.Complete(stageError), "stage Error must retain its exact completed row");
        const auto diagnosticIndex =
            ExpectAndBind(context, run, {4u, 2u}, diagnosticError);
        context.Check(run.Complete(diagnosticError) && run.RecordOwnerDrainResult(true) &&
                          !run.Finalize(),
                      "stage and Diagnostic errors must account the ledger but fail CPU completeness");
        RtExpectedFrameRecord stageRow{};
        RtExpectedFrameRecord diagnosticRow{};
        context.Check(stageIndex.has_value() && diagnosticIndex.has_value() &&
                          run.TryGetExpectedFrame(*stageIndex, stageRow) &&
                          run.TryGetExpectedFrame(*diagnosticIndex, diagnosticRow) &&
                          stageRow.cpuStageStatus == RtSampleStatus::Error &&
                          stageRow.diagnosticStatus == RtSampleStatus::CompiledOut &&
                          stageRow.rejectionReason == RtBenchmarkFailureReason::CpuStageError &&
                          diagnosticRow.cpuStageStatus == RtSampleStatus::Valid &&
                          diagnosticRow.diagnosticStatus == RtSampleStatus::Error &&
                          diagnosticRow.rejectionReason ==
                              RtBenchmarkFailureReason::DiagnosticFailed &&
                          stageRow.hasGpuStatus && diagnosticRow.hasGpuStatus,
                      "completed rows must distinguish stage, Diagnostic and GPU dispositions");
    }
}

void TestRejectionsAndIdentityFailures(TestContext& context)
{
    {
        RtBenchmarkEvidenceRun run;
        context.Check(run.Start(2u) && run.ArmMeasurement(110u, 120u),
                      "explicit rejection fixture must start");
        const auto first = run.ExpectFrame({1u, 2u});
        const auto second = run.ExpectFrame({2u, 2u});
        const auto submitted = MakeSnapshot(
            1u, 110u, 120u, 0u, 1'000'000u, RtSampleStatus::Disabled, 0u);
        context.Check(first.has_value() && second.has_value() &&
                          run.RejectExpected(*first, RtBenchmarkFailureReason::SubmissionFailed) &&
                          run.BindSubmitted(*second, submitted.identity.submitted) &&
                          run.RejectExpected(*second,
                                             RtBenchmarkFailureReason::PresentationFailed),
                      "failed submit and present must remain explicit expected-row dispositions");
        context.Check(run.RecordOwnerDrainResult(true) && !run.Finalize() &&
                          run.Status() == RtBenchmarkRunStatus::Incomplete &&
                          run.RejectedCount() == 2u && run.AccountedCount() == 2u &&
                          run.GpuStatusCounts().missing == 2u,
                      "explicit rejected rows must preserve the denominator and finalize incomplete");
    }

    {
        RtBenchmarkEvidenceRun run;
        context.Check(run.Start(1u) && run.ArmMeasurement(121u, 122u),
                      "duplicate disposition fixture must start");
        const auto index = run.ExpectFrame({1u, 2u});
        context.Check(index.has_value() &&
                          run.RejectExpected(
                              *index, RtBenchmarkFailureReason::SubmissionFailed) &&
                          !run.RejectExpected(
                              *index, RtBenchmarkFailureReason::SubmissionFailed) &&
                          run.RejectedCount() == 1u &&
                          run.FailureCount(
                              RtBenchmarkFailureReason::DuplicateDisposition) == 1u,
                      "an expected row must accept exactly one terminal rejection disposition");
    }

    {
        RtBenchmarkEvidenceRun run;
        context.Check(run.Start(1u) && run.ArmMeasurement(123u, 124u),
                      "failed drain fixture must start");
        const auto snapshot = MakeSnapshot(
            1u, 123u, 124u, 0u, 1'000'000u, RtSampleStatus::Disabled, 0u);
        ExpectAndBind(context, run, {1u, 2u}, snapshot);
        context.Check(run.RecordOwnerDrainResult(false) && !run.Finalize() &&
                          run.Status() == RtBenchmarkRunStatus::Incomplete &&
                          run.FailureCount(RtBenchmarkFailureReason::DrainFailed) == 1u &&
                          run.FailureCount(RtBenchmarkFailureReason::MissingCompletion) == 1u,
                      "failed final idle must remain explicit and cannot certify pending evidence");
    }

    {
        RtBenchmarkEvidenceRun run;
        context.Check(run.Start(1u) && run.ArmMeasurement(130u, 140u),
                      "missing completion fixture must start");
        const auto snapshot = MakeSnapshot(
            1u, 130u, 140u, 0u, 1'000'000u, RtSampleStatus::Valid, 2'000'000u);
        const auto index = ExpectAndBind(context, run, {1u, 2u}, snapshot);
        context.Check(run.RecordOwnerDrainResult(true) && !run.Finalize() &&
                          run.Status() == RtBenchmarkRunStatus::Incomplete &&
                          run.PendingCompletionCount() == 0u && run.RejectedCount() == 1u &&
                          run.FailureCount(RtBenchmarkFailureReason::MissingCompletion) == 1u,
                      "successful graphics idle with a missing exact completion must remain incomplete");
        RtExpectedFrameRecord row{};
        context.Check(index.has_value() && run.TryGetExpectedFrame(*index, row) &&
                          row.disposition == RtExpectedFrameDisposition::Rejected &&
                          row.rejectionReason == RtBenchmarkFailureReason::MissingCompletion,
                      "finalization must retain the missing completion on its intended row");
    }

    {
        RtBenchmarkEvidenceRun run;
        context.Check(run.Start(1u) && run.ArmMeasurement(150u, 160u),
                      "tokenless fixture must start");
        const auto exact = MakeSnapshot(
            1u, 150u, 160u, 0u, 1'000'000u, RtSampleStatus::Disabled, 0u);
        ExpectAndBind(context, run, {1u, 2u}, exact);
        context.Check(!run.Complete({}) && run.InvalidRun() &&
                          run.FailureCount(RtBenchmarkFailureReason::TokenlessCompletion) == 1u,
                      "tokenless completion must be counted and invalidate without consuming the row");
        context.Check(run.Complete(exact) && run.RecordOwnerDrainResult(true) &&
                          !run.Finalize() && run.Status() == RtBenchmarkRunStatus::Invalid,
                      "later exact accounting must not erase a prior tokenless protocol fault");
    }

    {
        RtBenchmarkEvidenceRun run;
        context.Check(run.Start(1u) && run.ArmMeasurement(170u, 180u),
                      "duplicate fixture must start");
        const auto exact = MakeSnapshot(
            1u, 170u, 180u, 0u, 1'000'000u, RtSampleStatus::Disabled, 0u);
        ExpectAndBind(context, run, {1u, 2u}, exact);
        context.Check(run.Complete(exact) && !run.Complete(exact) && run.InvalidRun() &&
                          run.FailureCount(RtBenchmarkFailureReason::DuplicateCompletion) == 1u,
                      "duplicate completion must not append or silently alter the accepted count");
        context.Check(run.CpuAcceptedCount() == 1u && run.CompletedCount() == 1u,
                      "duplicate completion must retain exactly one accepted sample");
    }

    {
        RtBenchmarkEvidenceRun run;
        context.Check(run.Start(1u) && run.ArmMeasurement(190u, 200u),
                      "mismatch fixture must start");
        const auto exact = MakeSnapshot(
            1u, 190u, 200u, 0u, 1'000'000u, RtSampleStatus::Disabled, 0u);
        const auto mismatched = MakeSnapshot(
            2u, 190u, 200u, 0u, 1'000'000u, RtSampleStatus::Disabled, 0u);
        ExpectAndBind(context, run, {1u, 2u}, exact);
        context.Check(!run.Complete(mismatched) && run.InvalidRun() &&
                          run.FailureCount(RtBenchmarkFailureReason::MismatchedCompletion) == 1u &&
                          run.PendingCompletionCount() == 1u,
                      "same-generation wrong identity must leave the exact pending row intact");
        context.Check(run.Complete(exact), "the exact token must still account after a mismatch");
    }

    {
        RtBenchmarkEvidenceRun run;
        context.Check(run.Start(1u) && run.ArmMeasurement(210u, 220u),
                      "stale fixture must start");
        const auto exact = MakeSnapshot(
            1u, 210u, 220u, 0u, 1'000'000u, RtSampleStatus::Disabled, 0u);
        const auto stale = MakeSnapshot(
            2u, 211u, 220u, 0u, 1'000'000u, RtSampleStatus::Disabled, 0u);
        ExpectAndBind(context, run, {1u, 2u}, exact);
        context.Check(!run.Complete(stale) && run.InvalidRun() &&
                          run.FailureCount(RtBenchmarkFailureReason::StaleCompletion) == 1u &&
                          run.PendingCompletionCount() == 1u,
                      "different-epoch completion must be counted as stale without consuming current evidence");
    }

    {
        RtBenchmarkEvidenceRun run;
        context.Check(run.Start(1u) && run.ArmMeasurement(230u, 240u),
                      "malformed fixture must start");
        auto malformed = MakeSnapshot(
            1u, 230u, 240u, 0u, 1'000'000u, RtSampleStatus::Disabled, 0u);
        ExpectAndBind(context, run, {1u, 2u}, malformed);
        malformed.schema = 99u;
        context.Check(!run.Complete(malformed) && run.InvalidRun() &&
                          run.FailureCount(RtBenchmarkFailureReason::InvalidCompletion) == 1u,
                      "malformed canonical evidence must be rejected before token association");
    }
}

void TestCpuIneligibleAndCancellation(TestContext& context)
{
    {
        RtBenchmarkEvidenceRun run;
        context.Check(run.Start(2u) && run.ArmMeasurement(250u, 260u),
                      "CPU rejection fixture must start");
        const auto snapshot = MakeSnapshot(
            1u, 250u, 260u, 0u, 1'000'000u, RtSampleStatus::Valid, 2'000'000u, false);
        CheckCanonical(context, snapshot,
                       "CPU-ineligible fixture must still be a canonical completion");
        const auto index = ExpectAndBind(context, run, {4u, 2u}, snapshot);
        context.Check(run.Complete(snapshot),
                      "CPU-ineligible completion must account before the next exact row");
        const auto cpuEligible = MakeSnapshot(
            2u, 250u, 260u, 1u, 3'000'000u, RtSampleStatus::Valid, 3'000'000u, true);
        ExpectAndBind(context, run, {5u, 2u}, cpuEligible);
        context.Check(run.Complete(cpuEligible) && run.RecordOwnerDrainResult(true) &&
                          !run.Finalize() && run.Status() == RtBenchmarkRunStatus::Incomplete &&
                          run.CompletedCount() == 2u && run.CpuAcceptedCount() == 1u &&
                          run.CpuRejectedCount() == 1u,
                      "CPU-ineligible history must not prevent later CPU accounting but keeps CPU incomplete");
        RtExpectedFrameRecord row{};
        context.Check(index.has_value() && run.TryGetExpectedFrame(*index, row) &&
                          row.disposition == RtExpectedFrameDisposition::Completed &&
                          row.hasGpuStatus && row.gpuStatus == RtSampleStatus::Valid &&
                          !row.cpuAccepted &&
                          row.rejectionReason == RtBenchmarkFailureReason::CpuIneligible,
                      "CPU rejection must not discard independently valid GPU disposition metadata");
        RtStageStatistics gpu{};
        context.Check(run.GpuStatistics(gpu) && gpu.valid && gpu.sampleCount == 2u &&
                          gpu.meanMilliseconds == 2.5,
                      "all valid GPU rows must remain reportable across an independently CPU-ineligible row");
    }

    {
        RtBenchmarkEvidenceRun run;
        context.Check(run.Start(3u) && run.ArmMeasurement(270u, 280u),
                      "cancellation fixture must start");
        const auto complete = MakeSnapshot(
            1u, 270u, 280u, 0u, 1'000'000u, RtSampleStatus::Disabled, 0u);
        const auto pending = MakeSnapshot(
            2u, 270u, 280u, 1u, 1'000'000u, RtSampleStatus::Disabled, 0u);
        ExpectAndBind(context, run, {1u, 2u}, complete);
        context.Check(run.Complete(complete), "cancellation fixture first row must complete");
        ExpectAndBind(context, run, {2u, 2u}, pending);
        context.Check(run.ExpectFrame({3u, 2u}).has_value(),
                      "cancellation fixture must retain an unsubmitted intended row");
        run.Cancel();
        context.Check(run.Status() == RtBenchmarkRunStatus::Cancelled && run.InvalidRun() &&
                          run.CompletedCount() == 1u && run.CancelledCount() == 2u &&
                          run.AccountedCount() == 3u && run.OutstandingCount() == 0u &&
                          !run.Finalize(),
                      "cancellation must account every outstanding row without erasing prior completion");
    }
}

void TestMoveTransfersOwnershipWithoutAliasing(TestContext& context)
{
    RtBenchmarkEvidenceRun source;
    context.Check(source.Start(2u) && source.ArmMeasurement(290u, 300u),
                  "move fixture source must start");
    const auto first = MakeSnapshot(
        1u, 290u, 300u, 0u, 4'000'000u, RtSampleStatus::Valid, 3'000'000u);
    ExpectAndBind(context, source, {5u, 2u}, first);

    RtBenchmarkEvidenceRun moved(std::move(source));
    context.Check(source.Status() == RtBenchmarkRunStatus::Empty && source.Capacity() == 0u &&
                      source.ExpectedCount() == 0u && moved.Capacity() == 2u &&
                      moved.PendingCompletionCount() == 1u,
                  "move construction must transfer storage/scalars and empty the source");
    context.Check(source.Start(1u) && source.ArmMeasurement(310u, 320u),
                  "moved-from owner must be independently reusable");

    RtBenchmarkEvidenceRun assigned;
    context.Check(assigned.Start(3u), "move-assignment destination must own old storage first");
    assigned = std::move(moved);
    context.Check(moved.Status() == RtBenchmarkRunStatus::Empty && moved.Capacity() == 0u &&
                      assigned.Capacity() == 2u && assigned.Complete(first) &&
                      assigned.RecordOwnerDrainResult(true) && assigned.Finalize(),
                  "move assignment must release destination storage and preserve pending associations");
    context.Check(source.ExpectedCount() == 0u && assigned.CpuAcceptedCount() == 1u,
                  "reusing the moved-from owner must not alias the transferred sample arrays");
}

} // namespace

int main()
{
    TestContext context;
    TestStartCapacityAndAllocationFailure(context);
    TestStatisticsUseSharedFullRouteMath(context);
    TestOneFrameLateAssociationAndFinalDrain(context);
    TestDrainSealsSubmissionButStillAcceptsItsExistingCompletion(context);
    TestCommittedBindingIsMonotonicWithoutInventingIdentity(context);
    TestCompletionIdentityIsMonotonicIndependentOfCpuAdmission(context);
    TestRealTwoLapRouteRetainsEveryMeasuredFrame(context);
    TestGpuAvailabilityDoesNotEraseCpuEvidence(context);
    TestCompletedFailuresRetainTheirCanonicalCause(context);
    TestRejectionsAndIdentityFailures(context);
    TestCpuIneligibleAndCancellation(context);
    TestMoveTransfersOwnershipWithoutAliasing(context);

    if (context.failures == 0)
    {
        std::cout << "Full-route RT benchmark evidence owner tests passed.\n";
    }
    return context.failures == 0 ? 0 : 1;
}
