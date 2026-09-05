#include "telemetry/RtPerformanceEvidence.h"

#include <algorithm>
#include <array>
#include <cmath>
#include <cstddef>
#include <cstdint>
#include <cstdlib>
#include <cstring>
#include <filesystem>
#include <fstream>
#include <iostream>
#include <limits>
#include <new>
#include <sstream>
#include <string>
#include <string_view>
#include <type_traits>

#if defined(_MSC_VER)
#include <malloc.h>
#endif

namespace
{

std::uint64_t gAllocationCount = 0u;
bool gCountAllocations = false;

void CountAllocation()
{
    if (gCountAllocations)
    {
        ++gAllocationCount;
    }
}

void* Allocate(const std::size_t size)
{
    CountAllocation();
    if (void* value = std::malloc(std::max<std::size_t>(size, 1u)))
    {
        return value;
    }
    throw std::bad_alloc{};
}

void* AllocateNoThrow(const std::size_t size) noexcept
{
    try
    {
        return Allocate(size);
    }
    catch (...)
    {
        return nullptr;
    }
}

void* AllocateAligned(const std::size_t size, const std::size_t alignment)
{
    CountAllocation();
#if defined(_MSC_VER)
    if (void* value = _aligned_malloc(std::max<std::size_t>(size, 1u), alignment))
    {
        return value;
    }
#else
    void* value = nullptr;
    if (posix_memalign(&value, alignment, std::max<std::size_t>(size, 1u)) == 0)
    {
        return value;
    }
#endif
    throw std::bad_alloc{};
}

void* AllocateAlignedNoThrow(const std::size_t size, const std::size_t alignment) noexcept
{
    try
    {
        return AllocateAligned(size, alignment);
    }
    catch (...)
    {
        return nullptr;
    }
}

void FreeAligned(void* value) noexcept
{
#if defined(_MSC_VER)
    _aligned_free(value);
#else
    std::free(value);
#endif
}

} // namespace

void* operator new(const std::size_t size) { return Allocate(size); }
void* operator new[](const std::size_t size) { return Allocate(size); }
void* operator new(const std::size_t size, const std::nothrow_t&) noexcept
{
    return AllocateNoThrow(size);
}
void* operator new[](const std::size_t size, const std::nothrow_t&) noexcept
{
    return AllocateNoThrow(size);
}
void* operator new(const std::size_t size, const std::align_val_t alignment)
{
    return AllocateAligned(size, static_cast<std::size_t>(alignment));
}
void* operator new[](const std::size_t size, const std::align_val_t alignment)
{
    return AllocateAligned(size, static_cast<std::size_t>(alignment));
}
void* operator new(const std::size_t size,
                   const std::align_val_t alignment,
                   const std::nothrow_t&) noexcept
{
    return AllocateAlignedNoThrow(size, static_cast<std::size_t>(alignment));
}
void* operator new[](const std::size_t size,
                     const std::align_val_t alignment,
                     const std::nothrow_t&) noexcept
{
    return AllocateAlignedNoThrow(size, static_cast<std::size_t>(alignment));
}
void operator delete(void* value) noexcept { std::free(value); }
void operator delete[](void* value) noexcept { std::free(value); }
void operator delete(void* value, const std::size_t) noexcept { std::free(value); }
void operator delete[](void* value, const std::size_t) noexcept { std::free(value); }
void operator delete(void* value, const std::nothrow_t&) noexcept { std::free(value); }
void operator delete[](void* value, const std::nothrow_t&) noexcept { std::free(value); }
void operator delete(void* value, const std::align_val_t) noexcept { FreeAligned(value); }
void operator delete[](void* value, const std::align_val_t) noexcept { FreeAligned(value); }
void operator delete(void* value, const std::size_t, const std::align_val_t) noexcept
{
    FreeAligned(value);
}
void operator delete[](void* value, const std::size_t, const std::align_val_t) noexcept
{
    FreeAligned(value);
}
void operator delete(void* value, const std::align_val_t, const std::nothrow_t&) noexcept
{
    FreeAligned(value);
}
void operator delete[](void* value, const std::align_val_t, const std::nothrow_t&) noexcept
{
    FreeAligned(value);
}

namespace
{

using namespace horde::telemetry;

static_assert(kRtEvidenceSampleCapacity == 128u);
static_assert(kRtDielectricCounterCount == 41u);
static_assert(std::is_trivially_copyable_v<RtFixedText<65u>>);
static_assert(std::is_standard_layout_v<RtFixedText<65u>>);
static_assert(std::is_trivially_copyable_v<RtFrameToken>);
static_assert(std::is_standard_layout_v<RtFrameToken>);
static_assert(std::is_trivially_copyable_v<RtSubmittedFrameIdentity>);
static_assert(std::is_standard_layout_v<RtSubmittedFrameIdentity>);
static_assert(std::is_trivially_copyable_v<RtCompletedFrameIdentity>);
static_assert(std::is_standard_layout_v<RtCompletedFrameIdentity>);
static_assert(std::is_trivially_copyable_v<RtPipelineEvidenceIdentity>);
static_assert(std::is_standard_layout_v<RtPipelineEvidenceIdentity>);
static_assert(std::is_trivially_copyable_v<RtResourceInventory>);
static_assert(std::is_standard_layout_v<RtResourceInventory>);
static_assert(std::is_trivially_copyable_v<RtPlayerDiagnostics>);
static_assert(std::is_standard_layout_v<RtPlayerDiagnostics>);
static_assert(std::is_trivially_copyable_v<RtStageFrameSample>);
static_assert(std::is_standard_layout_v<RtStageFrameSample>);
static_assert(std::is_trivially_copyable_v<RtDiagnosticEvidence>);
static_assert(std::is_standard_layout_v<RtDiagnosticEvidence>);
static_assert(std::is_trivially_copyable_v<RtGpuTimingEvidence>);
static_assert(std::is_standard_layout_v<RtGpuTimingEvidence>);
static_assert(std::is_trivially_copyable_v<RtSceneFrameEvidence>);
static_assert(std::is_standard_layout_v<RtSceneFrameEvidence>);
static_assert(std::is_trivially_copyable_v<RtPerformanceEvidenceSnapshot>);
static_assert(std::is_standard_layout_v<RtPerformanceEvidenceSnapshot>);
static_assert(std::is_trivially_copyable_v<RtLifecyclePublishedState>);
static_assert(std::is_standard_layout_v<RtLifecyclePublishedState>);
static_assert(std::is_trivially_copyable_v<RtRecordedSceneEvidence>);
static_assert(std::is_standard_layout_v<RtRecordedSceneEvidence>);
static_assert(std::is_trivially_copyable_v<RtSubmittedStageSample>);
static_assert(std::is_standard_layout_v<RtSubmittedStageSample>);
static_assert(std::is_trivially_copyable_v<RtStageAccumulator>);
static_assert(std::is_standard_layout_v<RtStageAccumulator>);
static_assert(std::is_trivially_copyable_v<RtStageSampleCollector>);
static_assert(std::is_standard_layout_v<RtStageSampleCollector>);
static_assert(std::is_trivially_copyable_v<RtEvidenceLifecycle>);
static_assert(std::is_standard_layout_v<RtEvidenceLifecycle>);
static_assert(sizeof(RtFrameToken) <= 64u);
static_assert(sizeof(RtPerformanceEvidenceSnapshot) < 4096u);

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
void SetText(TestContext& context,
             RtFixedText<Capacity>& destination,
             const std::string_view value,
             const std::string_view label)
{
    context.Check(AssignRtFixedText(destination, value), label);
}

RtStageFrameSample MakeStageFrame(const std::uint64_t baseNanoseconds = 0u)
{
    RtStageFrameSample frame{};
    frame.status = RtSampleStatus::Valid;
    for (std::size_t index = 0u; index < kRtStageCount; ++index)
    {
        frame.values[index].durationNanoseconds = baseNanoseconds + index;
        frame.values[index].workInvocationCount = index == 0u ? 1u : 0u;
        frame.values[index].byteCount = index == RtStageIndex(RtStage::DynamicUpload) ? 64u : 0u;
        frame.values[index].operationCount = index == RtStageIndex(RtStage::DynamicUpload) ? 2u : 0u;
    }
    const RtStageValue& player = frame.values[RtStageIndex(RtStage::PlayerSkin)];
    const RtStageValue& character = frame.values[RtStageIndex(RtStage::CharacterSkin)];
    RtStageValue& total = frame.values[RtStageIndex(RtStage::Skin)];
    total.durationNanoseconds = player.durationNanoseconds + character.durationNanoseconds;
    total.workInvocationCount = player.workInvocationCount + character.workInvocationCount;
    total.byteCount = player.byteCount + character.byteCount;
    total.operationCount = player.operationCount + character.operationCount;
    return frame;
}

RtPipelineEvidenceIdentity MakePipelineIdentity(TestContext& context,
                                                const RtInstrumentationMode instrumentation,
                                                const RtMaterialStrategy strategy,
                                                const char sentinel)
{
    RtPipelineEvidenceIdentity identity{};
    identity.instrumentation = instrumentation;
    identity.dielectricQuality = RtDielectricQuality::High;
    identity.activeStrategy = strategy;
    identity.waterQuality = RtWaterQuality::High;
    SetText(context,
            identity.bundleKey,
            instrumentation == RtInstrumentationMode::Shipping
                ? "shipping_high_pair"
                : "diagnostic_high_pair",
            "bundle key must fit");
    SetText(context, identity.opaqueFast.key, "opaque_fast", "opaque key must fit");
    SetText(context, identity.genericDielectric.key, "generic_dielectric", "generic key must fit");
    const std::string opaqueHash(64u, sentinel);
    const std::string genericHash(64u, sentinel == 'a' ? 'b' : 'd');
    SetText(context, identity.opaqueFast.sha256, opaqueHash, "opaque hash must fit");
    SetText(context, identity.genericDielectric.sha256, genericHash, "generic hash must fit");
    identity.active = strategy == RtMaterialStrategy::OpaqueFast
        ? identity.opaqueFast
        : identity.genericDielectric;
    return identity;
}

RtSceneFrameEvidence MakeSceneEvidence(TestContext& context,
                                       const RtInstrumentationMode instrumentation,
                                       const RtMaterialStrategy strategy,
                                       const char sentinel,
                                       const std::uint64_t durationSentinel)
{
    RtSceneFrameEvidence evidence{};
    evidence.pipeline = MakePipelineIdentity(context, instrumentation, strategy, sentinel);
    evidence.resources.bufferCount = static_cast<std::uint32_t>(sentinel);
    evidence.resources.memoryAllocationCount = 3u;
    evidence.resources.bottomLevelAccelerationStructureCount = 9u;
    evidence.resources.topLevelAccelerationStructureCount = 1u;
    evidence.resources.tlasInstanceCount = 20u;
    evidence.resources.pipelineCount = 2u;
    evidence.resources.shaderBindingTableCount = 2u;
    evidence.resources.descriptorSetCount = 1u;
    evidence.resources.hostVisibleBytes = 4096u;
    evidence.resources.deviceLocalBytes = 8192u;
    evidence.player.skinCadenceHz = 60u;
    evidence.player.skinUpdateCount = durationSentinel;
    evidence.player.maximumSocketErrorMicrometres = durationSentinel + 10u;
    evidence.player.primaryPixelCount = static_cast<std::uint32_t>(sentinel);
    evidence.player.primaryVisible = sentinel == 'c';
    evidence.stages = MakeStageFrame(durationSentinel);
    evidence.dispatch.sceneReady = true;
    evidence.dispatch.rtDispatchRecorded = true;
    evidence.dispatch.swapchainCopyRecorded = true;
    return evidence;
}

RtRecordedSceneEvidence MakeRecordedScene(const RtSceneFrameEvidence& evidence)
{
    RtRecordedSceneEvidence recorded{};
    recorded.pipeline = evidence.pipeline;
    recorded.resources = evidence.resources;
    recorded.player = evidence.player;
    recorded.dispatch = evidence.dispatch;
    return recorded;
}

RtSubmittedStageSample MakeSubmittedStages(const RtSubmittedFrameIdentity& submitted,
                                           const RtStageFrameSample& stages)
{
    RtSubmittedStageSample sample{};
    sample.identity = submitted;
    sample.stages = stages;
    return sample;
}

RtDiagnosticEvidence MakeDiagnostic(const RtInstrumentationMode instrumentation,
                                    const RtSampleStatus status,
                                    const std::uint64_t submissionSerial,
                                    const std::uint64_t sentinel)
{
    RtDiagnosticEvidence diagnostic{};
    diagnostic.status = status;
    diagnostic.compiled = instrumentation == RtInstrumentationMode::Diagnostic;
    if (status == RtSampleStatus::Valid)
    {
        diagnostic.hasCounters = true;
        diagnostic.completedSubmissionSerial = submissionSerial;
        diagnostic.readCount = 1u;
        diagnostic.resetCount = 1u;
        for (std::size_t index = 0u; index < diagnostic.counters.size(); ++index)
        {
            diagnostic.counters[index] =
                static_cast<std::uint32_t>(sentinel + static_cast<std::uint64_t>(index));
        }
    }
    else if (status == RtSampleStatus::Error)
    {
        diagnostic.completedSubmissionSerial = submissionSerial;
        diagnostic.readCount = 1u;
        diagnostic.resetCount = 1u;
        (void)AssignRtFixedText(diagnostic.detail, "diagnostic-read-error");
    }
    return diagnostic;
}

RtGpuTimingEvidence MakeGpu(const RtSampleStatus status,
                            const std::uint64_t submissionSerial,
                            const std::uint64_t durationNanoseconds)
{
    RtGpuTimingEvidence gpu{};
    gpu.status = status;
    gpu.timestampValidBits = 64u;
    gpu.timestampPeriodPicoseconds = 1000u;
    if (status == RtSampleStatus::Valid)
    {
        gpu.hasDuration = true;
        gpu.durationNanoseconds = durationNanoseconds;
        gpu.completedSubmissionSerial = submissionSerial;
        gpu.sampleCount = 1u;
    }
    else if (status == RtSampleStatus::Error)
    {
        gpu.completedSubmissionSerial = submissionSerial;
        gpu.errorCount = 1u;
        (void)AssignRtFixedText(gpu.detail, "gpu-query-error");
    }
    return gpu;
}

RtPerformanceEvidenceSnapshot MakeSnapshot(TestContext& context,
                                           const RtInstrumentationMode instrumentation,
                                           const RtSampleStatus gpuStatus = RtSampleStatus::Disabled)
{
    RtPerformanceEvidenceSnapshot snapshot{};
    snapshot.schema = kRtPerformanceEvidenceSchema;
    snapshot.identity.submitted.frame.sceneEpoch = 11u;
    snapshot.identity.submitted.frame.measurementGeneration = 20u;
    snapshot.identity.submitted.frame.recordAttemptSerial = 1u;
    snapshot.identity.submitted.frame.recordSerial = 1u;
    snapshot.identity.submitted.frame.simulationTick = 101u;
    snapshot.identity.submitted.frame.frameSlot = 0u;
    snapshot.identity.submitted.submissionSerial = 1u;
    snapshot.identity.completionSerial = 1u;
    snapshot.scene = MakeSceneEvidence(
        context, instrumentation, RtMaterialStrategy::OpaqueFast, 'a', 0u);
    snapshot.dielectric = instrumentation == RtInstrumentationMode::Shipping
        ? MakeDiagnostic(instrumentation, RtSampleStatus::CompiledOut, 0u, 0u)
        : MakeDiagnostic(instrumentation, RtSampleStatus::Valid, 1u, 1u);
    snapshot.gpu = MakeGpu(gpuStatus,
                           gpuStatus == RtSampleStatus::Valid || gpuStatus == RtSampleStatus::Error
                               ? 1u
                               : 0u,
                           0u);
    snapshot.presentation.outcome = RtPresentationOutcome::Presented;
    snapshot.presentation.lastSuccessfulPresentSubmissionSerial = 1u;
    snapshot.benchmarkEligible = true;
    return snapshot;
}

bool SameBytes(const RtEvidenceLifecycle& left, const RtEvidenceLifecycle& right)
{
    return std::memcmp(&left, &right, sizeof(left)) == 0;
}

std::string ReadFile(const std::filesystem::path& path)
{
    std::ifstream input(path, std::ios::binary);
    std::ostringstream contents;
    contents << input.rdbuf();
    return contents.str();
}

void TestFixedTextAndEnums(TestContext& context)
{
    RtFixedText<8u> text{};
    context.Check(AssignRtFixedText(text, "seven"), "bounded text must accept a fitting value");
    const RtFixedText<8u> before = text;
    context.Check(!AssignRtFixedText(text, "12345678"),
                  "bounded text must reject a value without room for its terminator");
    context.Check(std::memcmp(&before, &text, sizeof(text)) == 0,
                  "bounded text overflow rejection must be transactional");
    context.Check(RtFixedTextView(text) == "seven", "bounded text must retain exact content");
    const std::string embeddedNull("ab\0cd", 5u);
    context.Check(!AssignRtFixedText(text, embeddedNull) &&
                      std::memcmp(&before, &text, sizeof(text)) == 0,
                  "bounded identity text must reject embedded nulls transactionally");

    context.Check(std::string_view(RtSampleStatusName(RtSampleStatus::NotReady)) == "not-ready",
                  "NotReady must have one canonical name");
    context.Check(std::string_view(RtSampleStatusName(RtSampleStatus::CompiledOut)) == "compiled-out",
                  "CompiledOut must have one canonical name");
    context.Check(std::string_view(RtPresentationOutcomeName(
                      RtPresentationOutcome::NotPresentedNeedsRecreate)) ==
                      "not-presented-needs-recreate",
                  "presentation outcomes must have canonical Vulkan-free names");
    context.Check(RtSampleAvailable(RtSampleStatus::Valid),
                  "only Valid status must project as available");
    context.Check(!RtSampleAvailable(RtSampleStatus::Pending),
                  "Pending must not project as available");
    context.Check(RtSampleStatusName(static_cast<RtSampleStatus>(255u)) == nullptr,
                  "unknown sample status must not acquire a plausible name");
}

void TestStageAccumulatorAndConversion(TestContext& context)
{
    std::uint64_t nanoseconds = 0u;
    context.Check(CheckedMillisecondsToNanoseconds(1.25, nanoseconds) &&
                      nanoseconds == 1'250'000u,
                  "finite injected milliseconds must convert to integer nanoseconds");
    context.Check(CheckedMillisecondsToNanoseconds(0.0, nanoseconds) && nanoseconds == 0u,
                  "measured zero must remain a valid integer zero");
    context.Check(!CheckedMillisecondsToNanoseconds(-0.01, nanoseconds),
                  "negative injected durations must be rejected");
    context.Check(!CheckedMillisecondsToNanoseconds(
                      std::numeric_limits<double>::quiet_NaN(), nanoseconds),
                  "NaN injected durations must be rejected");
    context.Check(!CheckedMillisecondsToNanoseconds(
                      std::numeric_limits<double>::infinity(), nanoseconds),
                  "infinite injected durations must be rejected");
    context.Check(!CheckedMillisecondsToNanoseconds(
                      std::numeric_limits<double>::max(), nanoseconds),
                  "out-of-range injected durations must be rejected");

    RtStageAccumulator accumulator;
    context.Check(accumulator.Begin(), "first stage attempt must begin");
    context.Check(!accumulator.Begin(), "a second stage attempt cannot overwrite active scratch");
    context.Check(accumulator.Accumulate(RtStage::SimulationStep, 100u, 1u, 0u, 1u),
                  "first simulation contribution must accumulate");
    context.Check(accumulator.Accumulate(RtStage::SimulationStep, 200u, 2u, 0u, 1u),
                  "multiple simulation calls must accumulate in one submitted frame");
    context.Check(accumulator.Accumulate(RtStage::DynamicUpload, 400u, 2u, 4096u, 2u),
                  "upload timing and volume metadata must accumulate together");
    context.Check(accumulator.Accumulate(RtStage::PlayerSkin, 50u, 1u, 0u, 1u) &&
                      accumulator.Accumulate(RtStage::CharacterSkin, 70u, 2u, 0u, 2u),
                  "player and character skin contributions must accumulate independently");
    context.Check(!accumulator.Accumulate(RtStage::Skin, 999u, 1u, 0u, 1u),
                  "derived total skin timing must reject direct contributions that could double count");
    RtStageFrameSample committed{};
    context.Check(accumulator.Commit(committed), "finite stage scratch must commit");
    context.Check(committed.status == RtSampleStatus::Valid,
                  "finite committed stages must be Valid");
    const RtStageValue& simulation = committed.values[RtStageIndex(RtStage::SimulationStep)];
    context.Check(simulation.durationNanoseconds == 300u &&
                      simulation.workInvocationCount == 3u &&
                      simulation.operationCount == 2u,
                  "multiple simulation contributions must retain exact totals");
    const RtStageValue& upload = committed.values[RtStageIndex(RtStage::DynamicUpload)];
    context.Check(upload.durationNanoseconds == 400u && upload.byteCount == 4096u &&
                      upload.operationCount == 2u,
                  "dynamic upload scope must retain duration, bytes and operations");
    const RtStageValue& playerSkin = committed.values[RtStageIndex(RtStage::PlayerSkin)];
    const RtStageValue& characterSkin = committed.values[RtStageIndex(RtStage::CharacterSkin)];
    const RtStageValue& totalSkin = committed.values[RtStageIndex(RtStage::Skin)];
    context.Check(playerSkin.durationNanoseconds == 50u &&
                      characterSkin.durationNanoseconds == 70u &&
                      totalSkin.durationNanoseconds == 120u &&
                      totalSkin.workInvocationCount == 3u &&
                      totalSkin.operationCount == 3u,
                  "aggregate skin stage must exactly equal its player and character components");
    const RtStageValue& optional = committed.values[RtStageIndex(RtStage::BlasRefitRecord)];
    context.Check(optional.durationNanoseconds == 0u && optional.workInvocationCount == 0u,
                  "optional no-work stages must commit legitimate zero samples");

    const RtStageAggregateSet aggregates = accumulator.AggregatesByValue();
    for (const RtStageAggregate& aggregate : aggregates.values)
    {
        context.Check(aggregate.sampleCount == 1u,
                      "every successfully committed frame must add one denominator sample per stage");
    }
    context.Check(aggregates.values[RtStageIndex(RtStage::SimulationStep)].latestNanoseconds == 300u &&
                      aggregates.values[RtStageIndex(RtStage::SimulationStep)].sumNanoseconds == 300u &&
                      aggregates.values[RtStageIndex(RtStage::SimulationStep)].maxNanoseconds == 300u,
                  "stage aggregate latest/sum/max must be exact");

    context.Check(accumulator.Begin(), "a new attempt must begin after commit");
    context.Check(accumulator.Accumulate(RtStage::TraceCopyRecord, 999u, 1u, 0u, 1u),
                  "aborted attempt may collect scratch");
    context.Check(accumulator.Abort(), "active attempt must abort");
    const RtStageAggregateSet afterAbort = accumulator.AggregatesByValue();
    context.Check(std::memcmp(&aggregates, &afterAbort, sizeof(aggregates)) == 0,
                  "abort must discard scratch without changing aggregates");
    context.Check(accumulator.ResetAggregates(),
                  "idle accumulator must reset its lifetime aggregates");
    for (const RtStageAggregate& aggregate : accumulator.AggregatesByValue().values)
    {
        context.Check(aggregate.sampleCount == 0u && aggregate.sumNanoseconds == 0u &&
                          aggregate.statisticsValid && !aggregate.overflowed,
                      "aggregate reset must restore an empty valid denominator");
    }

    RtStageAggregate saturated{};
    saturated.sumNanoseconds = std::numeric_limits<std::uint64_t>::max();
    saturated.sampleCount = std::numeric_limits<std::uint64_t>::max();
    saturated.workInvocationCount = std::numeric_limits<std::uint64_t>::max();
    saturated.byteCount = std::numeric_limits<std::uint64_t>::max();
    saturated.operationCount = std::numeric_limits<std::uint64_t>::max();
    RtStageValue one{};
    one.durationNanoseconds = 1u;
    one.workInvocationCount = 1u;
    one.byteCount = 1u;
    one.operationCount = 1u;
    context.Check(!AccumulateCommittedStage(saturated, one),
                  "aggregate overflow must return failure instead of a plausible statistic");
    context.Check(saturated.sumNanoseconds == std::numeric_limits<std::uint64_t>::max() &&
                      saturated.sampleCount == std::numeric_limits<std::uint64_t>::max() &&
                      saturated.workInvocationCount == std::numeric_limits<std::uint64_t>::max() &&
                      saturated.byteCount == std::numeric_limits<std::uint64_t>::max() &&
                      saturated.operationCount == std::numeric_limits<std::uint64_t>::max() &&
                      saturated.overflowed && !saturated.statisticsValid,
                  "all aggregate fields must saturate and invalidate statistics on overflow");

    RtStageAccumulator overflowAccumulator;
    context.Check(overflowAccumulator.Begin(), "overflow fixture must begin");
    context.Check(overflowAccumulator.Accumulate(
                      RtStage::DynamicUpload,
                      std::numeric_limits<std::uint64_t>::max(),
                      std::numeric_limits<std::uint64_t>::max(),
                      std::numeric_limits<std::uint64_t>::max(),
                      std::numeric_limits<std::uint64_t>::max()),
                  "maximum first contribution is representable");
    context.Check(!overflowAccumulator.Accumulate(RtStage::DynamicUpload, 1u, 1u, 1u, 1u),
                  "scratch overflow must be explicit");
    RtStageFrameSample overflowFrame{};
    context.Check(!overflowAccumulator.Commit(overflowFrame) &&
                      overflowFrame.status == RtSampleStatus::Error &&
                      overflowFrame.values[RtStageIndex(RtStage::DynamicUpload)].overflowed,
                  "overflowing scratch must commit only as invalid evidence");
}

RtCompletedStageSample MakeCollectedSample(const std::uint64_t completionSerial,
                                           const std::uint64_t durationNanoseconds,
                                           const std::uint64_t sceneEpoch = 11u,
                                           const std::uint64_t generation = 20u)
{
    RtCompletedStageSample sample{};
    sample.identity.submitted.frame.sceneEpoch = sceneEpoch;
    sample.identity.submitted.frame.measurementGeneration = generation;
    sample.identity.submitted.frame.recordAttemptSerial = completionSerial;
    sample.identity.submitted.frame.recordSerial = completionSerial;
    sample.identity.submitted.frame.simulationTick = 100u + completionSerial;
    sample.identity.submitted.frame.frameSlot = 0u;
    sample.identity.submitted.submissionSerial = completionSerial;
    sample.identity.completionSerial = completionSerial;
    sample.stages = MakeStageFrame(durationNanoseconds);
    sample.benchmarkEligible = true;
    return sample;
}

void TestBoundedCollector(TestContext& context)
{
    RtStageSampleCollector collector;
    context.Check(collector.Start(11u, 20u), "collector must start for a non-zero identity");
    for (std::uint64_t milliseconds = 1u; milliseconds <= 19u; ++milliseconds)
    {
        context.Check(collector.Append(MakeCollectedSample(milliseconds, milliseconds * 1'000'000u)),
                      "ordered matching stage sample must append");
    }
    context.Check(collector.Append(MakeCollectedSample(20u, 100'000'000u)),
                  "skewed twentieth sample must append");
    RtStageStatistics statistics{};
    context.Check(collector.Statistics(RtStage::SimulationStep, statistics),
                  "valid bounded samples must produce statistics");
    context.Check(statistics.valid && statistics.sampleCount == 20u,
                  "statistics must retain the exact bounded denominator");
    context.Check(std::abs(statistics.meanMilliseconds - 14.5) < 1.0e-12,
                  "mean must include every sample");
    context.Check(std::abs(statistics.medianMilliseconds - 10.5) < 1.0e-12,
                  "even median must use the midpoint of the two central samples");
    context.Check(std::abs(statistics.p90Milliseconds - 18.0) < 1.0e-12,
                  "P90 must use sorted nearest-rank ceil(p*N)-1");
    context.Check(std::abs(statistics.p95Milliseconds - 19.0) < 1.0e-12,
                  "P95 continuity must use sorted nearest rank");
    context.Check(std::abs(statistics.onePercentLowFps - 10.0) < 1.0e-12,
                  "one-percent low must average the slowest ceil(1%*N) samples");

    RtStageSampleCollector serialCollector;
    context.Check(serialCollector.Start(11u, 20u) &&
                      serialCollector.Append(MakeCollectedSample(1u, 1'000'000u)),
                  "serial-rejection collector must accept its first complete identity");
    RtCompletedStageSample duplicateSubmission = MakeCollectedSample(2u, 2'000'000u);
    duplicateSubmission.identity.submitted.submissionSerial = 1u;
    context.Check(!serialCollector.Append(duplicateSubmission) &&
                      serialCollector.Size() == 1u && serialCollector.InvalidRun(),
                  "collector must reject a duplicate submitted identity even with a new completion");

    RtStageSampleCollector malformedCollector;
    context.Check(malformedCollector.Start(11u, 20u),
                  "malformed-stage collector must start");
    RtCompletedStageSample malformedStages = MakeCollectedSample(1u, 1'000'000u);
    ++malformedStages.stages.values[RtStageIndex(RtStage::Skin)].durationNanoseconds;
    context.Check(!malformedCollector.Append(malformedStages) &&
                      malformedCollector.Size() == 0u && malformedCollector.InvalidRun(),
                  "collector must reject a stage sample that violates the shared stage contract");

    const std::size_t retainedCount = collector.Size();
    RtCompletedStageSample mismatched = MakeCollectedSample(21u, 21'000'000u, 12u, 20u);
    context.Check(!collector.Append(mismatched), "collector must reject a stale/mismatched epoch");
    context.Check(collector.Size() == retainedCount && collector.InvalidRun() &&
                      collector.IdentityRejectCount() == 1u,
                  "identity rejection must preserve samples and explicitly invalidate the run");
    context.Check(!collector.Statistics(RtStage::SimulationStep, statistics),
                  "an identity-invalid run must not emit complete statistics");

    context.Check(collector.Start(11u, 21u), "collector restart must clear prior invalid state");
    for (std::size_t index = 0u; index < kRtEvidenceSampleCapacity; ++index)
    {
        context.Check(collector.Append(MakeCollectedSample(
                          index + 1u, index, 11u, 21u)),
                      "collector must retain every sample through its fixed capacity");
    }
    const RtCompletedStageSample overflow = MakeCollectedSample(
        kRtEvidenceSampleCapacity + 1u, 999u, 11u, 21u);
    context.Check(!collector.Append(overflow), "capacity overflow must return failure");
    context.Check(collector.Size() == kRtEvidenceSampleCapacity &&
                      collector.OverflowCount() == 1u && collector.InvalidRun(),
                  "capacity overflow must retain the first set and invalidate the run");
}

void TestLifecycleAssociationAndTransactions(TestContext& context)
{
    RtEvidenceLifecycle lifecycle;
    RtLifecycleSeeds seeds{};
    seeds.sceneEpoch = 10u;
    seeds.measurementGeneration = 20u;
    RtLifecycleResetEffects effects{};
    context.Check(lifecycle.Initialise(
                      seeds, 2u, RtSampleStatus::Pending, RtSampleStatus::Disabled, effects),
                  "lifecycle must initialise from preserved non-zero seeds");
    context.Check(effects.epochChanged && effects.pendingSubmissionsInvalidated &&
                      effects.collectorInvalidated && effects.nextSampleEligible,
                  "initialise must implement the binding reset table");
    const RtLifecyclePublishedState initial = lifecycle.PublishedStateByValue();
    context.Check(initial.sceneEpoch == 11u && initial.measurementGeneration == 20u &&
                      initial.diagnosticStatus == RtSampleStatus::Pending &&
                      initial.gpuStatus == RtSampleStatus::Disabled && initial.running &&
                      !initial.presented && !initial.hasCompletedEvidence,
                  "first frame must publish Pending/no stale completed evidence");

    RtFrameToken attemptA{};
    context.Check(lifecycle.BeginRecord(0u, 101u, attemptA), "record A must begin");
    context.Check(attemptA.recordAttemptSerial == 1u && attemptA.recordSerial == 0u,
                  "Begin must allocate a unique attempt but no successful-record serial");
    const RtSceneFrameEvidence sceneA = MakeSceneEvidence(
        context, RtInstrumentationMode::Diagnostic, RtMaterialStrategy::OpaqueFast, 'a', 1'000u);
    RtFrameToken recordedA{};
    context.Check(lifecycle.FinishRecord(attemptA, MakeRecordedScene(sceneA), recordedA),
                  "record A must finish");
    context.Check(recordedA.recordSerial == 1u,
                  "successful recording must allocate a distinct monotonic identity");
    RtSubmittedFrameIdentity submittedA{};
    context.Check(lifecycle.Submit(recordedA, submittedA), "record A must submit");
    context.Check(submittedA.submissionSerial == 1u,
                  "successful submission must allocate a monotonic submission identity");
    context.Check(lifecycle.AttachPresentation(submittedA, RtPresentationOutcome::Presented),
                  "record A must attach its exact present result");

    RtFrameToken attemptB{};
    context.Check(lifecycle.BeginRecord(1u, 202u, attemptB),
                  "record B may begin in another fixed slot while A awaits completion");
    const RtSceneFrameEvidence sceneB = MakeSceneEvidence(
        context, RtInstrumentationMode::Diagnostic, RtMaterialStrategy::GenericDielectric, 'c', 2'000u);
    RtFrameToken recordedB{};
    context.Check(lifecycle.FinishRecord(attemptB, MakeRecordedScene(sceneB), recordedB),
                  "record B must finish");

    RtSubmittedStageSample mismatchedStages = MakeSubmittedStages(submittedA, sceneB.stages);
    mismatchedStages.identity.frame.recordAttemptSerial = recordedB.recordAttemptSerial;
    mismatchedStages.identity.frame.recordSerial = recordedB.recordSerial;
    const RtEvidenceLifecycle beforeMismatchedStages = lifecycle;
    RtPerformanceEvidenceSnapshot rejectedMismatchedStages{};
    context.Check(!lifecycle.CompleteFence(
                      submittedA,
                      mismatchedStages,
                      MakeDiagnostic(RtInstrumentationMode::Diagnostic,
                                     RtSampleStatus::Valid,
                                     submittedA.submissionSerial,
                                     1u),
                      MakeGpu(RtSampleStatus::Valid, submittedA.submissionSerial, 11'000'000u),
                      rejectedMismatchedStages) &&
                      SameBytes(beforeMismatchedStages, lifecycle),
                  "stage evidence tagged with record B must not join completed record A");

    RtPerformanceEvidenceSnapshot completedA{};
    context.Check(lifecycle.CompleteFence(
                      submittedA,
                      MakeSubmittedStages(submittedA, sceneA.stages),
                      MakeDiagnostic(RtInstrumentationMode::Diagnostic,
                                     RtSampleStatus::Valid,
                                     submittedA.submissionSerial,
                                     1u),
                      MakeGpu(RtSampleStatus::Valid, submittedA.submissionSerial, 11'000'000u),
                      completedA),
                  "matching completed fence must publish record A");
    context.Check(completedA.identity.submitted.frame.simulationTick == 101u &&
                      completedA.identity.submitted.frame.recordAttemptSerial == 1u &&
                      completedA.identity.submitted.submissionSerial == 1u &&
                      completedA.scene.pipeline.activeStrategy == RtMaterialStrategy::OpaqueFast &&
                      completedA.scene.resources.bufferCount == static_cast<std::uint32_t>('a') &&
                      completedA.scene.player.skinUpdateCount == 1'000u &&
                      !completedA.scene.player.primaryVisible &&
                      completedA.scene.stages.values[0].durationNanoseconds == 1'000u &&
                      completedA.dielectric.counters[0] == 1u &&
                      completedA.gpu.durationNanoseconds == 11'000'000u,
                  "completed A must contain only A identity, scene, diagnostic and GPU sentinels");
    context.Check(completedA.scene.pipeline.activeStrategy != sceneB.pipeline.activeStrategy &&
                      completedA.scene.resources.bufferCount != sceneB.resources.bufferCount &&
                      completedA.scene.player.primaryVisible != sceneB.player.primaryVisible &&
                      completedA.scene.stages.values[0].durationNanoseconds !=
                          sceneB.stages.values[0].durationNanoseconds,
                  "record B sentinels must not leak into completed A");
    const RtPerformanceEvidenceSnapshot stableA = lifecycle.CompletedEvidenceByValue();
    context.Check(std::memcmp(&completedA, &stableA, sizeof(completedA)) == 0,
                  "owner-thread publication must return a stable completed value copy");

    RtSubmittedFrameIdentity submittedB{};
    context.Check(lifecycle.Submit(recordedB, submittedB), "record B must submit after A completes");
    RtEvidenceLifecycle beforeInvalid = lifecycle;
    RtSubmittedFrameIdentity stale = submittedB;
    ++stale.frame.sceneEpoch;
    context.Check(!lifecycle.AttachPresentation(stale, RtPresentationOutcome::Presented) &&
                      SameBytes(beforeInvalid, lifecycle),
                  "stale epoch rejection must leave lifecycle bytes unchanged");
    beforeInvalid = lifecycle;
    stale = submittedB;
    stale.frame.frameSlot = 3u;
    context.Check(!lifecycle.AttachPresentation(stale, RtPresentationOutcome::Presented) &&
                      SameBytes(beforeInvalid, lifecycle),
                  "wrong-slot rejection must leave lifecycle bytes unchanged");
    context.Check(lifecycle.AttachPresentation(submittedB, RtPresentationOutcome::Failed),
                  "failed present remains an explicit outcome on its submitted token");
    beforeInvalid = lifecycle;
    context.Check(!lifecycle.AttachPresentation(submittedB, RtPresentationOutcome::Presented) &&
                      SameBytes(beforeInvalid, lifecycle),
                  "duplicate present attachment must be transactionally rejected");
    RtPerformanceEvidenceSnapshot completedB{};
    context.Check(lifecycle.CompleteFence(
                      submittedB,
                      MakeSubmittedStages(submittedB, sceneB.stages),
                      MakeDiagnostic(RtInstrumentationMode::Diagnostic,
                                     RtSampleStatus::Valid,
                                     submittedB.submissionSerial,
                                     2u),
                      MakeGpu(RtSampleStatus::Disabled, 0u, 0u),
                      completedB),
                  "failed-present submission may still publish matching completed diagnostics");
    context.Check(completedB.presentation.outcome == RtPresentationOutcome::Failed &&
                      !completedB.benchmarkEligible &&
                      completedB.presentation.lastSuccessfulPresentSubmissionSerial ==
                          submittedA.submissionSerial,
                  "failed present must not advance success identity or enter benchmark samples");
    beforeInvalid = lifecycle;
    context.Check(!lifecycle.CompleteFence(
                      submittedB,
                      MakeSubmittedStages(submittedB, sceneB.stages),
                      MakeDiagnostic(RtInstrumentationMode::Diagnostic,
                                     RtSampleStatus::Valid,
                                     submittedB.submissionSerial,
                                     2u),
                      MakeGpu(RtSampleStatus::Disabled, 0u, 0u),
                      completedB) &&
                      SameBytes(beforeInvalid, lifecycle),
                  "duplicate fence completion must be transactionally rejected");

    RtFrameToken abortedAttempt{};
    context.Check(lifecycle.BeginRecord(0u, 303u, abortedAttempt),
                  "aborted record fixture must begin");
    context.Check(lifecycle.AbortRecord(abortedAttempt), "active record must abort");
    RtFrameToken nextAttempt{};
    context.Check(lifecycle.BeginRecord(0u, 304u, nextAttempt) &&
                      nextAttempt.recordAttemptSerial > abortedAttempt.recordAttemptSerial,
                  "aborted attempt serial must be consumed and never reused");
    RtFrameToken recordedForFailedSubmit{};
    context.Check(lifecycle.FinishRecord(nextAttempt, MakeRecordedScene(sceneA),
                                         recordedForFailedSubmit),
                  "failed-submit fixture must finish recording");
    const std::uint64_t submissionBeforeFailure = lifecycle.SeedsByValue().submissionSerial;
    context.Check(lifecycle.FailSubmit(recordedForFailedSubmit),
                  "explicit failed submit must release the recorded slot");
    context.Check(lifecycle.SeedsByValue().submissionSerial == submissionBeforeFailure,
                  "failed submit must not allocate or advance a successful submission identity");

    RtFrameToken shutdownAttempt{};
    RtFrameToken shutdownRecorded{};
    RtSubmittedFrameIdentity shutdownSubmitted{};
    context.Check(lifecycle.BeginRecord(0u, 404u, shutdownAttempt) &&
                      lifecycle.FinishRecord(
                          shutdownAttempt, MakeRecordedScene(sceneA), shutdownRecorded) &&
                      lifecycle.Submit(shutdownRecorded, shutdownSubmitted),
                  "shutdown fixture must reach submitted state");
    RtPerformanceEvidenceSnapshot shutdownEvidence{};
    context.Check(lifecycle.CompleteFinalIdle(
                      shutdownSubmitted,
                      MakeSubmittedStages(shutdownSubmitted, sceneA.stages),
                      MakeDiagnostic(RtInstrumentationMode::Diagnostic,
                                     RtSampleStatus::Valid,
                                     shutdownSubmitted.submissionSerial,
                                     4u),
                      MakeGpu(RtSampleStatus::Disabled, 0u, 0u),
                      shutdownEvidence),
                  "final idle completion may explicitly use NotAttempted presentation");
    context.Check(shutdownEvidence.presentation.outcome == RtPresentationOutcome::NotAttempted &&
                      shutdownEvidence.presentation.finalIdleCompletion &&
                      !shutdownEvidence.benchmarkEligible,
                  "NotAttempted finalisation must never become benchmark eligible");
}

void TestLifecycleResetTableAndExhaustion(TestContext& context)
{
    RtEvidenceLifecycle lifecycle;
    RtLifecycleSeeds seeds{};
    seeds.sceneEpoch = 40u;
    seeds.measurementGeneration = 50u;
    RtLifecycleResetEffects effects{};
    context.Check(lifecycle.Initialise(
                      seeds, 1u, RtSampleStatus::CompiledOut, RtSampleStatus::Disabled, effects),
                  "shipping lifecycle must initialise");
    const std::uint64_t initialEpoch = lifecycle.PublishedStateByValue().sceneEpoch;

    RtFrameToken retainedAttempt{};
    RtFrameToken retainedRecord{};
    RtSubmittedFrameIdentity retainedSubmit{};
    const RtSceneFrameEvidence shippingScene = MakeSceneEvidence(
        context, RtInstrumentationMode::Shipping, RtMaterialStrategy::OpaqueFast, 'a', 10u);
    context.Check(lifecycle.BeginRecord(0u, 1u, retainedAttempt) &&
                      lifecycle.FinishRecord(
                          retainedAttempt, MakeRecordedScene(shippingScene), retainedRecord) &&
                      lifecycle.Submit(retainedRecord, retainedSubmit) &&
                      lifecycle.AttachPresentation(retainedSubmit, RtPresentationOutcome::Presented),
                  "pending general-evidence submission must be retained across generation changes");

    const std::array<RtLifecycleEvent, 5u> clearingEvents{{
        RtLifecycleEvent::BenchmarkStart,
        RtLifecycleEvent::WarmupToMeasure,
        RtLifecycleEvent::RouteReset,
        RtLifecycleEvent::Retry,
        RtLifecycleEvent::CheckpointChange,
    }};
    std::uint64_t expectedGeneration = 50u;
    for (const RtLifecycleEvent event : clearingEvents)
    {
        context.Check(lifecycle.ApplyEvent(event, effects),
                      "measurement-reset event must succeed");
        ++expectedGeneration;
        const bool nextSampleEligible = event != RtLifecycleEvent::BenchmarkStart;
        context.Check(effects.measurementGenerationChanged && effects.collectorCleared &&
                          !effects.pendingSubmissionsInvalidated &&
                          effects.nextSampleEligible == nextSampleEligible &&
                          lifecycle.PublishedStateByValue().measurementGeneration == expectedGeneration,
                      "measurement-reset event must follow the exact reset table");
    }
    RtPerformanceEvidenceSnapshot oldGenerationEvidence{};
    context.Check(lifecycle.CompleteFence(
                      retainedSubmit,
                      MakeSubmittedStages(retainedSubmit, shippingScene.stages),
                      MakeDiagnostic(RtInstrumentationMode::Shipping,
                                     RtSampleStatus::CompiledOut,
                                     0u,
                                     0u),
                      MakeGpu(RtSampleStatus::Disabled, 0u, 0u),
                      oldGenerationEvidence),
                  "old-generation submission remains valid for general evidence");
    context.Check(!oldGenerationEvidence.benchmarkEligible,
                  "old-generation completion must be benchmark-ineligible");

    context.Check(lifecycle.ApplyEvent(RtLifecycleEvent::Pause, effects), "pause must succeed");
    context.Check(!effects.epochChanged && !effects.measurementGenerationChanged &&
                      !effects.collectorCleared && !effects.nextSampleEligible &&
                      lifecycle.PublishedStateByValue().paused,
                  "pause must retain identities and suspend sample eligibility");
    RtFrameToken rejectedWhilePaused{};
    context.Check(!lifecycle.BeginRecord(0u, 2u, rejectedWhilePaused),
                  "paused lifecycle must not begin a measured render attempt");
    context.Check(lifecycle.ApplyEvent(RtLifecycleEvent::Resume, effects), "resume must succeed");
    ++expectedGeneration;
    context.Check(effects.measurementGenerationChanged && effects.collectorCleared &&
                      effects.restartFrameCycleClock && effects.nextSampleEligible &&
                      !lifecycle.PublishedStateByValue().paused,
                  "resume must change generation and tell the caller to restart frame-cycle timing");

    context.Check(lifecycle.ApplyEvent(RtLifecycleEvent::GpuTimingEnabled, effects),
                  "GPU timing enable must succeed");
    ++expectedGeneration;
    context.Check(lifecycle.PublishedStateByValue().gpuStatus == RtSampleStatus::Pending &&
                      effects.clearGpuAggregates && effects.collectorCleared,
                  "GPU timing enable must start Pending in a fresh generation");
    context.Check(lifecycle.ApplyEvent(RtLifecycleEvent::GpuTimingDisabled, effects),
                  "GPU timing disable must succeed");
    ++expectedGeneration;
    context.Check(lifecycle.PublishedStateByValue().gpuStatus == RtSampleStatus::Disabled &&
                      effects.clearGpuAggregates && effects.collectorCleared,
                  "GPU timing disable must be a non-error null state in a fresh generation");

    RtFrameToken staleAttempt{};
    RtFrameToken staleRecord{};
    RtSubmittedFrameIdentity staleSubmit{};
    context.Check(lifecycle.BeginRecord(0u, 3u, staleAttempt) &&
                      lifecycle.FinishRecord(
                          staleAttempt, MakeRecordedScene(shippingScene), staleRecord) &&
                      lifecycle.Submit(staleRecord, staleSubmit),
                  "recreate fixture must retain a pending submission");
    context.Check(lifecycle.Recreate(RtResourceResetReason::SwapchainRecreate,
                                     RtSampleStatus::CompiledOut,
                                     RtSampleStatus::Disabled,
                                     effects),
                  "resource recreation must succeed");
    context.Check(lifecycle.PublishedStateByValue().sceneEpoch == initialEpoch + 1u &&
                      lifecycle.PublishedStateByValue().measurementGeneration == expectedGeneration &&
                      effects.epochChanged && effects.pendingSubmissionsInvalidated &&
                      effects.collectorInvalidated && !lifecycle.PublishedStateByValue().presented &&
                      !lifecycle.PublishedStateByValue().hasCompletedEvidence,
                  "recreate must invalidate pending work and stale published values without changing generation");
    context.Check(lifecycle.PublishedStateByValue().lastResourceReset ==
                      RtResourceResetReason::SwapchainRecreate,
                  "recreate must retain its semantic reset reason without platform types");
    const RtEvidenceLifecycle beforeStaleCompletion = lifecycle;
    RtPerformanceEvidenceSnapshot rejected{};
    context.Check(!lifecycle.CompleteFinalIdle(
                      staleSubmit,
                      MakeSubmittedStages(staleSubmit, shippingScene.stages),
                      MakeDiagnostic(RtInstrumentationMode::Shipping,
                                     RtSampleStatus::CompiledOut,
                                     0u,
                                     0u),
                      MakeGpu(RtSampleStatus::Disabled, 0u, 0u),
                      rejected) &&
                      SameBytes(beforeStaleCompletion, lifecycle),
                  "completion from a prior epoch must be rejected transactionally");

    for (const RtResourceResetReason reason : {
             RtResourceResetReason::RenderScaleChange,
             RtResourceResetReason::SurfaceReplacement,
             RtResourceResetReason::DeviceReplacement,
             RtResourceResetReason::DiagnosticResourceReplacement,
             RtResourceResetReason::TimestampResourceReplacement})
    {
        const std::uint64_t epochBefore = lifecycle.PublishedStateByValue().sceneEpoch;
        context.Check(lifecycle.Recreate(reason,
                                         RtSampleStatus::CompiledOut,
                                         RtSampleStatus::Disabled,
                                         effects) &&
                          lifecycle.PublishedStateByValue().sceneEpoch == epochBefore + 1u &&
                          lifecycle.PublishedStateByValue().lastResourceReset == reason &&
                          effects.pendingSubmissionsInvalidated &&
                          effects.collectorInvalidated &&
                          !lifecycle.PublishedStateByValue().presented,
                      "every resource/surface/device replacement must apply the same epoch reset contract");
    }

    context.Check(lifecycle.Destroy(effects), "destroy/surface-stop must succeed");
    context.Check(effects.epochChanged && effects.pendingSubmissionsInvalidated &&
                      effects.collectorInvalidated && !effects.nextSampleEligible &&
                      !lifecycle.PublishedStateByValue().running &&
                      lifecycle.PublishedStateByValue().diagnosticStatus == RtSampleStatus::NotReady &&
                      !lifecycle.PublishedStateByValue().presented &&
                      !lifecycle.PublishedStateByValue().hasCompletedEvidence,
                  "destroy must publish an explicit not-running, no-stale-evidence state");
    context.Check(lifecycle.PublishedStateByValue().lastResourceReset ==
                      RtResourceResetReason::DestroyOrSurfaceStop,
                  "destroy must retain an explicit surface-stop reset reason");
    const RtEvidenceLifecycle stoppedLifecycle = lifecycle;
    context.Check(!lifecycle.Recreate(RtResourceResetReason::SurfaceReplacement,
                                      RtSampleStatus::CompiledOut,
                                      RtSampleStatus::Disabled,
                                      effects) &&
                      SameBytes(stoppedLifecycle, lifecycle),
                  "destroyed lifecycle must require explicit reinitialisation before new samples");
    context.Check(!lifecycle.Initialise(seeds,
                                        1u,
                                        RtSampleStatus::CompiledOut,
                                        RtSampleStatus::Disabled,
                                        effects) &&
                      SameBytes(stoppedLifecycle, lifecycle),
                  "reinitialisation must reject stale caller-owned identity seeds transactionally");
    const RtLifecycleSeeds restartSeeds = lifecycle.SeedsByValue();
    const std::uint64_t stoppedEpoch = restartSeeds.sceneEpoch;
    context.Check(lifecycle.Initialise(restartSeeds,
                                       1u,
                                       RtSampleStatus::CompiledOut,
                                       RtSampleStatus::Disabled,
                                       effects) &&
                      lifecycle.PublishedStateByValue().running &&
                      lifecycle.PublishedStateByValue().sceneEpoch == stoppedEpoch + 1u,
                  "explicit reinitialisation must preserve and advance every caller-owned identity seed");

    RtEvidenceLifecycle exhausted;
    RtLifecycleSeeds exhaustedSeeds{};
    exhaustedSeeds.sceneEpoch = std::numeric_limits<std::uint64_t>::max();
    exhaustedSeeds.measurementGeneration = 1u;
    const RtEvidenceLifecycle beforeExhausted = exhausted;
    context.Check(!exhausted.Initialise(exhaustedSeeds,
                                        1u,
                                        RtSampleStatus::Pending,
                                        RtSampleStatus::Disabled,
                                        effects) &&
                      SameBytes(beforeExhausted, exhausted),
                  "epoch exhaustion must fail closed without wrapping or mutation");

    RtEvidenceLifecycle attemptExhausted;
    exhaustedSeeds.sceneEpoch = 1u;
    exhaustedSeeds.measurementGeneration = 1u;
    exhaustedSeeds.recordAttemptSerial = std::numeric_limits<std::uint64_t>::max();
    context.Check(attemptExhausted.Initialise(exhaustedSeeds,
                                              1u,
                                              RtSampleStatus::Pending,
                                              RtSampleStatus::Disabled,
                                              effects),
                  "attempt-exhaustion fixture must initialise without consuming attempt identity");
    const RtEvidenceLifecycle beforeAttempt = attemptExhausted;
    RtFrameToken impossible{};
    context.Check(!attemptExhausted.BeginRecord(0u, 1u, impossible) &&
                      SameBytes(beforeAttempt, attemptExhausted),
                  "record-attempt exhaustion must fail closed without wrapping");

    RtEvidenceLifecycle generationExhausted;
    exhaustedSeeds = {};
    exhaustedSeeds.sceneEpoch = 1u;
    exhaustedSeeds.measurementGeneration = std::numeric_limits<std::uint64_t>::max();
    context.Check(generationExhausted.Initialise(exhaustedSeeds,
                                                 1u,
                                                 RtSampleStatus::Pending,
                                                 RtSampleStatus::Disabled,
                                                 effects),
                  "generation-exhaustion fixture must initialise");
    const RtEvidenceLifecycle beforeGeneration = generationExhausted;
    context.Check(!generationExhausted.ApplyEvent(RtLifecycleEvent::BenchmarkStart, effects) &&
                      SameBytes(beforeGeneration, generationExhausted),
                  "measurement-generation exhaustion must fail closed without wrapping");

    RtEvidenceLifecycle recordExhausted;
    exhaustedSeeds = {};
    exhaustedSeeds.sceneEpoch = 1u;
    exhaustedSeeds.measurementGeneration = 1u;
    exhaustedSeeds.successfulRecordSerial = std::numeric_limits<std::uint64_t>::max();
    context.Check(recordExhausted.Initialise(exhaustedSeeds,
                                             1u,
                                             RtSampleStatus::CompiledOut,
                                             RtSampleStatus::Disabled,
                                             effects),
                  "successful-record exhaustion fixture must initialise independent serial spaces");
    RtFrameToken recordAttempt{};
    context.Check(recordExhausted.BeginRecord(0u, 1u, recordAttempt),
                  "successful-record exhaustion fixture must begin an attempt");
    const RtEvidenceLifecycle beforeRecordFinish = recordExhausted;
    RtFrameToken impossibleRecord{};
    context.Check(!recordExhausted.FinishRecord(
                      recordAttempt, MakeRecordedScene(shippingScene), impossibleRecord) &&
                      SameBytes(beforeRecordFinish, recordExhausted),
                  "successful-record identity exhaustion must fail closed without wrapping");

    RtEvidenceLifecycle submissionExhausted;
    exhaustedSeeds = {};
    exhaustedSeeds.sceneEpoch = 1u;
    exhaustedSeeds.measurementGeneration = 1u;
    exhaustedSeeds.submissionSerial = std::numeric_limits<std::uint64_t>::max();
    context.Check(submissionExhausted.Initialise(exhaustedSeeds,
                                                 1u,
                                                 RtSampleStatus::CompiledOut,
                                                 RtSampleStatus::Disabled,
                                                 effects),
                  "submission exhaustion fixture must initialise independent serial spaces");
    RtFrameToken submissionAttempt{};
    RtFrameToken submissionRecord{};
    context.Check(submissionExhausted.BeginRecord(0u, 1u, submissionAttempt) &&
                      submissionExhausted.FinishRecord(
                          submissionAttempt, MakeRecordedScene(shippingScene), submissionRecord),
                  "submission exhaustion fixture must finish a record");
    const RtEvidenceLifecycle beforeSubmit = submissionExhausted;
    RtSubmittedFrameIdentity impossibleSubmit{};
    context.Check(!submissionExhausted.Submit(submissionRecord, impossibleSubmit) &&
                      SameBytes(beforeSubmit, submissionExhausted),
                  "submission identity exhaustion must fail closed without wrapping");

    RtEvidenceLifecycle completionExhausted;
    exhaustedSeeds = {};
    exhaustedSeeds.sceneEpoch = 1u;
    exhaustedSeeds.measurementGeneration = 1u;
    exhaustedSeeds.completionSerial = std::numeric_limits<std::uint64_t>::max();
    context.Check(completionExhausted.Initialise(exhaustedSeeds,
                                                 1u,
                                                 RtSampleStatus::CompiledOut,
                                                 RtSampleStatus::Disabled,
                                                 effects),
                  "completion exhaustion fixture must initialise independent serial spaces");
    RtFrameToken completionAttempt{};
    RtFrameToken completionRecord{};
    RtSubmittedFrameIdentity completionSubmit{};
    context.Check(completionExhausted.BeginRecord(0u, 1u, completionAttempt) &&
                      completionExhausted.FinishRecord(
                          completionAttempt, MakeRecordedScene(shippingScene), completionRecord) &&
                      completionExhausted.Submit(completionRecord, completionSubmit) &&
                      completionExhausted.AttachPresentation(
                          completionSubmit, RtPresentationOutcome::Presented),
                  "completion exhaustion fixture must reach presented submission");
    const RtEvidenceLifecycle beforeCompletion = completionExhausted;
    RtPerformanceEvidenceSnapshot impossibleCompletion{};
    context.Check(!completionExhausted.CompleteFence(
                      completionSubmit,
                      MakeSubmittedStages(completionSubmit, shippingScene.stages),
                      MakeDiagnostic(RtInstrumentationMode::Shipping,
                                     RtSampleStatus::CompiledOut,
                                     0u,
                                     0u),
                      MakeGpu(RtSampleStatus::Disabled, 0u, 0u),
                      impossibleCompletion) &&
                      SameBytes(beforeCompletion, completionExhausted),
                  "completion identity exhaustion must fail closed without wrapping");
}

void TestMeasurementEligibility(TestContext& context)
{
    RtEvidenceLifecycle lifecycle;
    RtLifecycleSeeds seeds{};
    seeds.sceneEpoch = 7u;
    seeds.measurementGeneration = 9u;
    RtLifecycleResetEffects effects{};
    context.Check(lifecycle.Initialise(
                      seeds, 1u, RtSampleStatus::CompiledOut, RtSampleStatus::Disabled, effects),
                  "measurement-eligibility lifecycle must initialise");
    const RtSceneFrameEvidence scene = MakeSceneEvidence(
        context, RtInstrumentationMode::Shipping, RtMaterialStrategy::OpaqueFast, 'a', 100u);

    const auto completePresentedFrame = [&](const std::uint64_t tick,
                                            RtPerformanceEvidenceSnapshot& completed) {
        RtFrameToken attempt{};
        RtFrameToken recorded{};
        RtSubmittedFrameIdentity submitted{};
        return lifecycle.BeginRecord(0u, tick, attempt) &&
               lifecycle.FinishRecord(attempt, MakeRecordedScene(scene), recorded) &&
               lifecycle.Submit(recorded, submitted) &&
               lifecycle.AttachPresentation(submitted, RtPresentationOutcome::Presented) &&
               lifecycle.CompleteFence(
                   submitted,
                   MakeSubmittedStages(submitted, scene.stages),
                   MakeDiagnostic(RtInstrumentationMode::Shipping,
                                  RtSampleStatus::CompiledOut,
                                  0u,
                                  0u),
                   MakeGpu(RtSampleStatus::Disabled, 0u, 0u),
                   completed);
    };

    context.Check(lifecycle.ApplyEvent(RtLifecycleEvent::BenchmarkStart, effects) &&
                      !effects.nextSampleEligible,
                  "benchmark start must enter an explicit warm-up-ineligible generation");
    RtPerformanceEvidenceSnapshot warmup{};
    context.Check(completePresentedFrame(1u, warmup) && !warmup.benchmarkEligible,
                  "a presented completion during benchmark warm-up must remain ineligible");

    context.Check(lifecycle.ApplyEvent(RtLifecycleEvent::WarmupToMeasure, effects) &&
                      effects.nextSampleEligible,
                  "warm-up transition must enable the new measurement generation");
    RtPerformanceEvidenceSnapshot measured{};
    context.Check(completePresentedFrame(2u, measured) && measured.benchmarkEligible,
                  "a matching presented completion after warm-up must become eligible");

    RtFrameToken prePauseAttempt{};
    RtFrameToken prePauseRecorded{};
    RtSubmittedFrameIdentity prePauseSubmitted{};
    context.Check(lifecycle.BeginRecord(0u, 3u, prePauseAttempt) &&
                      lifecycle.FinishRecord(
                          prePauseAttempt, MakeRecordedScene(scene), prePauseRecorded) &&
                      lifecycle.Submit(prePauseRecorded, prePauseSubmitted) &&
                      lifecycle.AttachPresentation(
                          prePauseSubmitted, RtPresentationOutcome::Presented) &&
                      lifecycle.ApplyEvent(RtLifecycleEvent::Pause, effects),
                  "pause fixture must retain one exact submitted frame");
    RtPerformanceEvidenceSnapshot pausedCompletion{};
    context.Check(lifecycle.CompleteFence(
                      prePauseSubmitted,
                      MakeSubmittedStages(prePauseSubmitted, scene.stages),
                      MakeDiagnostic(RtInstrumentationMode::Shipping,
                                     RtSampleStatus::CompiledOut,
                                     0u,
                                     0u),
                      MakeGpu(RtSampleStatus::Disabled, 0u, 0u),
                      pausedCompletion) &&
                      !pausedCompletion.benchmarkEligible,
                  "a retained submission completed while paused must not enter benchmark data");

    context.Check(lifecycle.ApplyEvent(RtLifecycleEvent::Resume, effects) &&
                      effects.nextSampleEligible && effects.restartFrameCycleClock,
                  "resume must start a fresh eligible generation and frame-cycle clock");
    RtPerformanceEvidenceSnapshot resumed{};
    context.Check(completePresentedFrame(4u, resumed) && resumed.benchmarkEligible,
                  "a matching presented completion after resume must become eligible");
}

void TestDiagnosticPendingLifecycle(TestContext& context)
{
    RtEvidenceLifecycle lifecycle;
    RtLifecycleSeeds seeds{};
    seeds.sceneEpoch = 3u;
    seeds.measurementGeneration = 5u;
    RtLifecycleResetEffects effects{};
    context.Check(lifecycle.Initialise(
                      seeds, 1u, RtSampleStatus::Pending, RtSampleStatus::Disabled, effects),
                  "Diagnostic Pending lifecycle must initialise");
    const RtSceneFrameEvidence scene = MakeSceneEvidence(
        context, RtInstrumentationMode::Diagnostic, RtMaterialStrategy::OpaqueFast, 'a', 100u);

    RtFrameToken firstAttempt{};
    RtFrameToken firstRecord{};
    RtSubmittedFrameIdentity firstSubmission{};
    context.Check(lifecycle.BeginRecord(0u, 1u, firstAttempt) &&
                      lifecycle.FinishRecord(
                          firstAttempt, MakeRecordedScene(scene), firstRecord) &&
                      lifecycle.Submit(firstRecord, firstSubmission) &&
                      lifecycle.AttachPresentation(
                          firstSubmission, RtPresentationOutcome::Presented),
                  "first Diagnostic submission must reach matching presentation");
    RtPerformanceEvidenceSnapshot pending{};
    context.Check(lifecycle.CompleteFence(
                      firstSubmission,
                      MakeSubmittedStages(firstSubmission, scene.stages),
                      MakeDiagnostic(
                          RtInstrumentationMode::Diagnostic, RtSampleStatus::Pending, 0u, 0u),
                      MakeGpu(RtSampleStatus::Disabled, 0u, 0u),
                      pending) &&
                      pending.dielectric.status == RtSampleStatus::Pending &&
                      !pending.dielectric.hasCounters && !pending.benchmarkEligible,
                  "first completed Diagnostic submission must publish Pending, not false zero counters");

    RtFrameToken secondAttempt{};
    RtFrameToken secondRecord{};
    RtSubmittedFrameIdentity secondSubmission{};
    context.Check(lifecycle.BeginRecord(0u, 2u, secondAttempt) &&
                      lifecycle.FinishRecord(
                          secondAttempt, MakeRecordedScene(scene), secondRecord) &&
                      lifecycle.Submit(secondRecord, secondSubmission) &&
                      lifecycle.AttachPresentation(
                          secondSubmission, RtPresentationOutcome::Presented),
                  "second Diagnostic submission must retain its exact identity");
    RtPerformanceEvidenceSnapshot valid{};
    context.Check(lifecycle.CompleteFence(
                      secondSubmission,
                      MakeSubmittedStages(secondSubmission, scene.stages),
                      MakeDiagnostic(RtInstrumentationMode::Diagnostic,
                                     RtSampleStatus::Valid,
                                     secondSubmission.submissionSerial,
                                     9u),
                      MakeGpu(RtSampleStatus::Disabled, 0u, 0u),
                      valid) &&
                      valid.dielectric.status == RtSampleStatus::Valid &&
                      valid.dielectric.completedSubmissionSerial ==
                          secondSubmission.submissionSerial &&
                      valid.dielectric.counters[0] == 9u && valid.benchmarkEligible,
                  "Diagnostic may become Valid only with counters joined to the completed submission");
}

void TestDiagnosticResetFailure(TestContext& context)
{
    RtEvidenceLifecycle lifecycle;
    RtLifecycleSeeds seeds{};
    seeds.sceneEpoch = 13u;
    seeds.measurementGeneration = 17u;
    RtLifecycleResetEffects effects{};
    context.Check(lifecycle.Initialise(
                      seeds, 1u, RtSampleStatus::Pending, RtSampleStatus::Disabled, effects),
                  "Diagnostic reset-failure lifecycle must initialise Pending");

    RtFrameToken ordinaryAbort{};
    context.Check(lifecycle.BeginRecord(0u, 1u, ordinaryAbort) &&
                      lifecycle.AbortRecord(ordinaryAbort) &&
                      lifecycle.PublishedStateByValue().diagnosticStatus == RtSampleStatus::Pending,
                  "generic record abort must not invent a Diagnostic failure");

    RtFrameToken resetFailure{};
    context.Check(lifecycle.BeginRecord(0u, 2u, resetFailure),
                  "Diagnostic reset-failure attempt must begin");
    context.Check(lifecycle.FailDiagnosticReset(resetFailure, effects) &&
                      lifecycle.PublishedStateByValue().diagnosticStatus == RtSampleStatus::Error &&
                      effects.collectorInvalidated && !effects.nextSampleEligible,
                  "Diagnostic reset failure must publish Error and invalidate measurement");
    const RtEvidenceLifecycle afterFailure = lifecycle;
    context.Check(!lifecycle.FailDiagnosticReset(resetFailure, effects) &&
                      SameBytes(afterFailure, lifecycle),
                  "duplicate Diagnostic reset failure must be transactionally rejected");
    RtFrameToken blocked{};
    context.Check(!lifecycle.BeginRecord(0u, 3u, blocked),
                  "Diagnostic reset Error must fail closed until resource recreation");

    context.Check(lifecycle.Recreate(RtResourceResetReason::DiagnosticResourceReplacement,
                                      RtSampleStatus::Pending,
                                      RtSampleStatus::Disabled,
                                      effects) &&
                      lifecycle.PublishedStateByValue().diagnosticStatus == RtSampleStatus::Pending &&
                      effects.epochChanged && effects.nextSampleEligible,
                  "Diagnostic resource recreation must recover into a new Pending epoch");

    const RtSceneFrameEvidence scene = MakeSceneEvidence(
        context, RtInstrumentationMode::Diagnostic, RtMaterialStrategy::OpaqueFast, 'a', 100u);
    RtFrameToken readAttempt{};
    RtFrameToken readRecord{};
    RtSubmittedFrameIdentity readSubmission{};
    context.Check(lifecycle.BeginRecord(0u, 4u, readAttempt) &&
                      lifecycle.FinishRecord(
                          readAttempt, MakeRecordedScene(scene), readRecord) &&
                      lifecycle.Submit(readRecord, readSubmission) &&
                      lifecycle.AttachPresentation(
                          readSubmission, RtPresentationOutcome::Presented),
                  "Diagnostic read-failure fixture must reach a matching submitted frame");
    RtPerformanceEvidenceSnapshot readFailure{};
    context.Check(lifecycle.CompleteFence(
                      readSubmission,
                      MakeSubmittedStages(readSubmission, scene.stages),
                      MakeDiagnostic(RtInstrumentationMode::Diagnostic,
                                     RtSampleStatus::Error,
                                     readSubmission.submissionSerial,
                                     0u),
                      MakeGpu(RtSampleStatus::Disabled, 0u, 0u),
                      readFailure) &&
                      readFailure.dielectric.status == RtSampleStatus::Error &&
                      !readFailure.benchmarkEligible &&
                      lifecycle.PublishedStateByValue().diagnosticStatus == RtSampleStatus::Error,
                  "matching Diagnostic read failure must publish Error and fail the sample closed");
    context.Check(!lifecycle.BeginRecord(0u, 5u, blocked),
                  "Diagnostic read Error must also block new recording until recreation");
}

void TestValidatorAndSerializers(TestContext& context)
{
    RtEvidenceValidationError error = RtEvidenceValidationError::None;
    RtPerformanceEvidenceSnapshot shipping = MakeSnapshot(
        context, RtInstrumentationMode::Shipping, RtSampleStatus::Disabled);
    context.Check(ValidateRtPerformanceEvidence(shipping, error) &&
                      error == RtEvidenceValidationError::None,
                  "valid Shipping evidence must pass the shared validator");
    std::string shippingJson;
    context.Check(SerializeRtPerformanceEvidenceJson(shipping, shippingJson, error),
                  "valid Shipping evidence must serialize to canonical JSON");
    const std::string expectedShippingJson =
        "{\"schema\":1,\"identity\":{\"sceneEpoch\":11,\"measurementGeneration\":20,"
        "\"recordAttemptSerial\":1,\"recordSerial\":1,\"submissionSerial\":1,"
        "\"completionSerial\":1,\"simulationTick\":101,\"frameSlot\":0},"
        "\"pipeline\":{\"instrumentation\":\"shipping\",\"dielectricQuality\":\"high\","
        "\"bundleKey\":\"shipping_high_pair\",\"opaqueFast\":{\"key\":\"opaque_fast\","
        "\"sha256\":\"aaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaa\"},"
        "\"genericDielectric\":{\"key\":\"generic_dielectric\","
        "\"sha256\":\"bbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbb\"},"
        "\"activeStrategy\":\"opaque-fast\",\"activeKey\":\"opaque_fast\","
        "\"activeSha256\":\"aaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaa\","
        "\"waterQuality\":\"high\"},"
        "\"dispatch\":{\"sceneReady\":true,\"rtDispatchRecorded\":true,"
        "\"swapchainCopyRecorded\":true},"
        "\"resources\":{\"bufferCount\":97,\"memoryAllocationCount\":3,"
        "\"bottomLevelAccelerationStructureCount\":9,\"topLevelAccelerationStructureCount\":1,"
        "\"tlasInstanceCount\":20,\"pipelineCount\":2,\"shaderBindingTableCount\":2,"
        "\"descriptorSetCount\":1,\"hostVisibleBytes\":4096,\"deviceLocalBytes\":8192},"
        "\"player\":{\"skinCadenceHz\":60,\"skinUpdateCount\":0,"
        "\"maximumSocketErrorMicrometres\":10,\"primaryPixelCount\":97,"
        "\"primaryVisible\":false},"
        "\"dielectric\":{\"status\":\"compiled-out\",\"available\":false,\"compiled\":false,"
        "\"completedSubmissionSerial\":0,\"readCount\":0,\"resetCount\":0,\"counters\":null,"
        "\"detail\":\"\"},"
        "\"gpu\":{\"status\":\"disabled\",\"available\":false,"
        "\"completedSubmissionSerial\":0,\"wholeRtGpuMs\":null,\"timestampValidBits\":64,"
        "\"timestampPeriodPicoseconds\":1000,\"sampleCount\":0,"
        "\"unavailableResultCount\":0,\"errorCount\":0,\"detail\":\"\"},"
        "\"stages\":{\"status\":\"valid\",\"simulationStepCpuMs\":0.000000,"
        "\"skinCpuMs\":0.000005,\"playerSkinCpuMs\":0.000002,"
        "\"characterSkinCpuMs\":0.000003,\"dynamicUploadCpuMs\":0.000004,"
        "\"blasRefitRecordCpuMs\":0.000005,\"tlasUpdateRecordCpuMs\":0.000006,"
        "\"traceCopyRecordCpuMs\":0.000007,\"frameFenceWaitCpuMs\":0.000008,"
        "\"imageAcquireCpuMs\":0.000009,\"queueSubmitCpuMs\":0.000010,"
        "\"presentCallCpuMs\":0.000011,\"wholeFrameCycleCpuMs\":0.000012,"
        "\"workInvocationCounts\":[1,0,0,0,0,0,0,0,0,0,0,0,0],"
        "\"byteCounts\":[0,0,0,0,64,0,0,0,0,0,0,0,0],"
        "\"operationCounts\":[0,0,0,0,2,0,0,0,0,0,0,0,0]},"
        "\"presentation\":{\"outcome\":\"presented\",\"presented\":true,"
        "\"lastSuccessfulPresentSubmissionSerial\":1,\"finalIdleCompletion\":false},"
        "\"benchmarkEligible\":true}\n";
    context.Check(shippingJson == expectedShippingJson,
                  "Shipping JSON must match the hand-derived canonical golden document");
    context.Check(shippingJson.find("\"counters\":null") != std::string::npos &&
                      shippingJson.find("\"wholeRtGpuMs\":null") != std::string::npos,
                  "non-Valid numeric diagnostics and GPU timing must serialize as JSON null");

    std::string shippingText;
    context.Check(SerializeRtPerformanceEvidenceText(shipping, shippingText, error),
                  "valid Shipping evidence must serialize to canonical text");
    const std::string expectedShippingText =
        "RT PERFORMANCE EVIDENCE schema=1\n"
        "Frame: epoch=11 generation=20 attempt=1 record=1 submission=1 completion=1 tick=101 slot=0\n"
        "Pipeline: shipping/high pair=shipping_high_pair active=opaque-fast opaque_fast@aaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaa\n"
        "Player: skin-cadence-hz=60 skin-updates=0 max-socket-error-um=10 primary-pixels=97 primary-visible=no\n"
        "Presentation: presented last-successful-submission=1 final-idle=no benchmark-eligible=yes\n"
        "Dielectric diagnostics: compiled-out counters=N/A reads=0 resets=0\n"
        "Whole RT GPU: disabled value=N/A\n"
        "Stages (CPU wall/record): simulationStepCpuMs=0.000000 skinCpuMs=0.000005 playerSkinCpuMs=0.000002 characterSkinCpuMs=0.000003 dynamicUploadCpuMs=0.000004 blasRefitRecordCpuMs=0.000005 tlasUpdateRecordCpuMs=0.000006 traceCopyRecordCpuMs=0.000007 frameFenceWaitCpuMs=0.000008 imageAcquireCpuMs=0.000009 queueSubmitCpuMs=0.000010 presentCallCpuMs=0.000011 wholeFrameCycleCpuMs=0.000012\n";
    context.Check(shippingText == expectedShippingText,
                  "Shipping text must match the hand-derived canonical golden projection");
    context.Check(shippingText.find("N/A") != std::string::npos,
                  "text projection must use N/A for unavailable numeric values");

    RtPerformanceEvidenceSnapshot diagnostic = MakeSnapshot(
        context, RtInstrumentationMode::Diagnostic, RtSampleStatus::Valid);
    context.Check(ValidateRtPerformanceEvidence(diagnostic, error),
                  "valid Diagnostic counters and GPU zero sample must pass validation");
    std::string diagnosticJson;
    context.Check(SerializeRtPerformanceEvidenceJson(diagnostic, diagnosticJson, error),
                  "valid Diagnostic evidence must serialize");
    context.Check(diagnosticJson.find("\"status\":\"valid\",\"available\":true,"
                                      "\"compiled\":true") != std::string::npos &&
                      diagnosticJson.find("\"counters\":[1,2,3,4,5,6,7,8,9,10,11,12,13,"
                                          "14,15,16,17,18,19,20,21,22,23,24,25,26,27,28,29,"
                                          "30,31,32,33,34,35,36,37,38,39,40,41]") !=
                          std::string::npos &&
                      diagnosticJson.find("\"wholeRtGpuMs\":0.000000") != std::string::npos,
                      "Diagnostic JSON must retain exact 41 counters and a genuinely measured zero");

    RtPerformanceEvidenceSnapshot firstDiagnostic = diagnostic;
    firstDiagnostic.dielectric = MakeDiagnostic(
        RtInstrumentationMode::Diagnostic, RtSampleStatus::Pending, 0u, 0u);
    firstDiagnostic.benchmarkEligible = false;
    std::string firstDiagnosticJson;
    context.Check(ValidateRtPerformanceEvidence(firstDiagnostic, error) &&
                      SerializeRtPerformanceEvidenceJson(
                          firstDiagnostic, firstDiagnosticJson, error) &&
                      firstDiagnosticJson.find(
                          "\"dielectric\":{\"status\":\"pending\",\"available\":false,") !=
                          std::string::npos &&
                      firstDiagnosticJson.find("\"counters\":null") != std::string::npos,
                  "first completed Diagnostic frame must remain Pending with null counters");

    RtPerformanceEvidenceSnapshot invalidStages = shipping;
    invalidStages.scene.stages.status = RtSampleStatus::Error;
    invalidStages.scene.stages.values[RtStageIndex(RtStage::DynamicUpload)].overflowed = true;
    invalidStages.benchmarkEligible = false;
    std::string invalidStagesJson;
    context.Check(SerializeRtPerformanceEvidenceJson(
                      invalidStages, invalidStagesJson, error) &&
                      invalidStagesJson.find("\"simulationStepCpuMs\":null") !=
                          std::string::npos &&
                      invalidStagesJson.find(
                          "\"workInvocationCounts\":[null,null,null,null,null") !=
                          std::string::npos,
                  "non-Valid stage durations and counts must serialize as numeric nulls");

    RtPerformanceEvidenceSnapshot invalidMeasured = diagnostic;
    invalidMeasured.dielectric.readCount = 0u;
    context.Check(!ValidateRtPerformanceEvidence(invalidMeasured, error),
                  "Valid Diagnostic counters require an actual completed read");
    invalidMeasured = diagnostic;
    invalidMeasured.dielectric.resetCount = 0u;
    context.Check(!ValidateRtPerformanceEvidence(invalidMeasured, error),
                  "Valid Diagnostic counters require a matching recorded reset");
    invalidMeasured = diagnostic;
    invalidMeasured.gpu.sampleCount = 0u;
    context.Check(!ValidateRtPerformanceEvidence(invalidMeasured, error),
                  "Valid GPU timing requires a completed timestamp sample");
    invalidMeasured = diagnostic;
    invalidMeasured.gpu.timestampValidBits = 0u;
    context.Check(!ValidateRtPerformanceEvidence(invalidMeasured, error),
                  "Valid GPU timing requires non-zero timestamp-valid bits");
    invalidMeasured = diagnostic;
    invalidMeasured.gpu.timestampPeriodPicoseconds = 0u;
    context.Check(!ValidateRtPerformanceEvidence(invalidMeasured, error),
                  "Valid GPU timing requires a non-zero timestamp period");

    for (const RtSampleStatus status : {RtSampleStatus::Disabled,
                                        RtSampleStatus::Unsupported,
                                        RtSampleStatus::Pending,
                                        RtSampleStatus::Error})
    {
        RtPerformanceEvidenceSnapshot fixture = MakeSnapshot(
            context, RtInstrumentationMode::Shipping, status);
        if (status == RtSampleStatus::Error)
        {
            fixture.gpu.completedSubmissionSerial = fixture.identity.submitted.submissionSerial;
        }
        if (status == RtSampleStatus::Pending || status == RtSampleStatus::Error)
        {
            fixture.benchmarkEligible = false;
        }
        context.Check(ValidateRtPerformanceEvidence(fixture, error),
                      "non-Valid GPU status fixture must remain valid without numeric duration");
        std::string json;
        context.Check(SerializeRtPerformanceEvidenceJson(fixture, json, error) &&
                          json.find("\"wholeRtGpuMs\":null") != std::string::npos,
                      "Disabled/Unsupported/Pending/Error GPU status must serialize numeric null");
    }

    RtPerformanceEvidenceSnapshot invalid = shipping;
    invalid.schema = kRtPerformanceEvidenceSchema + 1u;
    std::string rejectedJson = "preexisting";
    context.Check(!SerializeRtPerformanceEvidenceJson(invalid, rejectedJson, error) &&
                      rejectedJson.empty() &&
                      error == RtEvidenceValidationError::UnknownSchema,
                  "unknown schema must fail with no partial JSON document");
    std::string rejectedText = "preexisting";
    context.Check(!SerializeRtPerformanceEvidenceText(invalid, rejectedText, error) &&
                      rejectedText.empty(),
                  "unknown schema must fail with no partial text document");

    invalid = shipping;
    invalid.identity.submitted.submissionSerial = 2u;
    context.Check(!ValidateRtPerformanceEvidence(invalid, error) &&
                      error == RtEvidenceValidationError::InconsistentIdentity,
                  "joined serial mismatch must be rejected");
    invalid = shipping;
    invalid.gpu.hasDuration = true;
    invalid.gpu.durationNanoseconds = 1u;
    context.Check(!ValidateRtPerformanceEvidence(invalid, error) &&
                      error == RtEvidenceValidationError::NonValidNumericSample,
                  "non-Valid GPU state carrying a number must be rejected");
    invalid = shipping;
    invalid.dielectric.hasCounters = true;
    context.Check(!ValidateRtPerformanceEvidence(invalid, error) &&
                      error == RtEvidenceValidationError::NonValidNumericSample,
                  "CompiledOut diagnostics carrying numeric counters must be rejected");
    invalid = shipping;
    invalid.dielectric.counters[0] = 1u;
    context.Check(!ValidateRtPerformanceEvidence(invalid, error) &&
                      error == RtEvidenceValidationError::NonValidNumericSample,
                  "non-Valid diagnostic state must reject hidden non-zero counter payloads");
    invalid = shipping;
    invalid.scene.pipeline.opaqueFast.sha256.value[0] = 'z';
    context.Check(!ValidateRtPerformanceEvidence(invalid, error) &&
                      error == RtEvidenceValidationError::MalformedHash,
                  "malformed SHA-256 identity must be rejected");
    invalid = shipping;
    invalid.scene.pipeline.instrumentation = static_cast<RtInstrumentationMode>(255u);
    context.Check(!ValidateRtPerformanceEvidence(invalid, error) &&
                      error == RtEvidenceValidationError::UnknownEnum,
                  "unknown enum value must be rejected");
    invalid = shipping;
    invalid.scene.pipeline.bundleKey.value.fill('x');
    context.Check(!ValidateRtPerformanceEvidence(invalid, error) &&
                      error == RtEvidenceValidationError::UnterminatedText,
                  "unterminated fixed identity text must be rejected distinctly");
    invalid = shipping;
    invalid.scene.dispatch.rtDispatchRecorded = false;
    context.Check(!ValidateRtPerformanceEvidence(invalid, error),
                  "submitted evidence without the native RT dispatch must be rejected");
    invalid = shipping;
    invalid.presentation.outcome = RtPresentationOutcome::NotAttempted;
    invalid.benchmarkEligible = false;
    context.Check(!ValidateRtPerformanceEvidence(invalid, error),
                  "NotAttempted presentation must require explicit final-idle provenance");

    RtPerformanceEvidenceSnapshot diagnosticError = MakeSnapshot(
        context, RtInstrumentationMode::Diagnostic, RtSampleStatus::Disabled);
    diagnosticError.dielectric = MakeDiagnostic(
        RtInstrumentationMode::Diagnostic,
        RtSampleStatus::Error,
        diagnosticError.identity.submitted.submissionSerial,
        0u);
    diagnosticError.benchmarkEligible = false;
    std::string diagnosticErrorJson;
    context.Check(ValidateRtPerformanceEvidence(diagnosticError, error) &&
                      SerializeRtPerformanceEvidenceJson(
                          diagnosticError, diagnosticErrorJson, error) &&
                      diagnosticErrorJson.find("\"dielectric\":{\"status\":\"error\"") !=
                          std::string::npos &&
                      diagnosticErrorJson.find("\"counters\":null") != std::string::npos,
                  "Diagnostic read/reset Error must publish explicitly with null counters");
    diagnosticError.benchmarkEligible = true;
    context.Check(!ValidateRtPerformanceEvidence(diagnosticError, error) &&
                      error == RtEvidenceValidationError::InvalidDiagnosticState,
                  "Diagnostic Error must invalidate the benchmark sample");

    invalid = shipping;
    invalid.gpu.status = RtSampleStatus::Pending;
    invalid.benchmarkEligible = true;
    context.Check(!ValidateRtPerformanceEvidence(invalid, error) &&
                      error == RtEvidenceValidationError::InvalidGpuState,
                  "enabled-but-Pending GPU evidence must not enter a complete benchmark sample");
}

void TestAllocationHookCoverage(TestContext& context)
{
    gAllocationCount = 0u;
    gCountAllocations = true;
    void* scalar = ::operator new(8u);
    void* array = ::operator new[](8u);
    void* nothrowScalar = ::operator new(8u, std::nothrow);
    void* nothrowArray = ::operator new[](8u, std::nothrow);
    void* aligned = ::operator new(64u, std::align_val_t{64u});
    void* alignedArray = ::operator new[](64u, std::align_val_t{64u});
    void* alignedNothrow = ::operator new(64u, std::align_val_t{64u}, std::nothrow);
    void* alignedArrayNothrow = ::operator new[](64u, std::align_val_t{64u}, std::nothrow);
    gCountAllocations = false;
    ::operator delete(scalar);
    ::operator delete[](array);
    ::operator delete(nothrowScalar, std::nothrow);
    ::operator delete[](nothrowArray, std::nothrow);
    ::operator delete(aligned, std::align_val_t{64u});
    ::operator delete[](alignedArray, std::align_val_t{64u});
    ::operator delete(alignedNothrow, std::align_val_t{64u}, std::nothrow);
    ::operator delete[](alignedArrayNothrow, std::align_val_t{64u}, std::nothrow);
    context.Check(gAllocationCount == 8u,
                  "allocation hook must observe scalar, array, aligned and nothrow forms");
}

void TestSourceDependencyFence(TestContext& context)
{
    const std::filesystem::path root = HORDE_RT_SOURCE_DIR;
    const std::filesystem::path header = root / "src/telemetry/RtPerformanceEvidence.h";
    const std::filesystem::path source = root / "src/telemetry/RtPerformanceEvidence.cpp";
    const std::string module = ReadFile(header) + ReadFile(source);
    context.Check(!module.empty(), "pure evidence source fence must read the shared module");
    for (const std::string_view forbidden : {"<vulkan/",
                                             "\"vulkan/",
                                             "\"platform/",
                                             "PresentableTinyRtScene",
                                             "GpuFrameTimer",
                                             "VkResult",
                                             "VkDevice",
                                             "VkCommand"})
    {
        context.Check(module.find(forbidden) == std::string::npos,
                      "pure evidence module must not depend on Vulkan, scene or platform types");
    }

    const std::string sourceList = ReadFile(root / "cmake/HordeRtSources.cmake");
    context.Check(sourceList.find("telemetry/RtPerformanceEvidence.cpp") != std::string::npos,
                  "Windows and Android shared source list must consume the pure implementation");

    const std::array<std::string_view, 4u> canonicalFields{{
        "rtPerformanceEvidence",
        "simulationStepCpuMs",
        "blasRefitRecordCpuMs",
        "traceCopyRecordCpuMs",
    }};
    for (const std::filesystem::directory_entry& entry :
         std::filesystem::recursive_directory_iterator(root / "src/platform"))
    {
        if (!entry.is_regular_file() ||
            (entry.path().extension() != ".cpp" && entry.path().extension() != ".h"))
        {
            continue;
        }
        const std::string platformSource = ReadFile(entry.path());
        for (const std::string_view field : canonicalFields)
        {
            context.Check(platformSource.find(field) == std::string::npos,
                          "platform shells must not duplicate canonical RT evidence field literals");
        }
    }
}

void TestNoAllocationAndOneWayIsolation(TestContext& context)
{
    RtPipelineEvidenceIdentity pipeline = MakePipelineIdentity(
        context, RtInstrumentationMode::Shipping, RtMaterialStrategy::OpaqueFast, 'a');
    RtSceneFrameEvidence scene = MakeSceneEvidence(
        context, RtInstrumentationMode::Shipping, RtMaterialStrategy::OpaqueFast, 'a', 1u);
    RtDiagnosticEvidence diagnostic = MakeDiagnostic(
        RtInstrumentationMode::Shipping, RtSampleStatus::CompiledOut, 0u, 0u);
    RtGpuTimingEvidence gpu = MakeGpu(RtSampleStatus::Disabled, 0u, 0u);
    RtEvidenceValidationError validationError = RtEvidenceValidationError::None;

    struct DeterministicSimulationSentinel
    {
        std::uint64_t tick = 777u;
        std::array<std::uint32_t, 8u> gameplay{{1u, 2u, 3u, 4u, 5u, 6u, 7u, 8u}};
    };
    struct DeterministicRenderInputSentinel
    {
        std::array<std::uint64_t, 8u> shaderAndTransformBits{{
            0x10u, 0x20u, 0x30u, 0x40u, 0x50u, 0x60u, 0x70u, 0x80u}};
    };
    const DeterministicSimulationSentinel simulation{};
    const DeterministicRenderInputSentinel renderInputs{};
    const DeterministicSimulationSentinel simulationBefore = simulation;
    const DeterministicRenderInputSentinel renderBefore = renderInputs;

    RtStageAccumulator accumulator;
    RtStageSampleCollector collector;
    RtEvidenceLifecycle lifecycle;
    RtLifecycleSeeds seeds{};
    seeds.sceneEpoch = 100u;
    seeds.measurementGeneration = 200u;
    RtLifecycleResetEffects effects{};
    context.Check(lifecycle.Initialise(
                      seeds, 1u, RtSampleStatus::CompiledOut, RtSampleStatus::Disabled, effects),
                  "allocation fixture lifecycle must initialise");
    context.Check(collector.Start(101u, 200u), "allocation fixture collector must start");

    gAllocationCount = 0u;
    gCountAllocations = true;
    std::uint64_t checksum = 0u;
    bool operationsSucceeded = true;
    for (std::uint64_t index = 1u; index <= 100'000u; ++index)
    {
        operationsSucceeded = accumulator.Begin() && operationsSucceeded;
        operationsSucceeded =
            accumulator.Accumulate(RtStage::SimulationStep, index, 1u, 0u, 1u) &&
            operationsSucceeded;
        RtStageFrameSample stages{};
        operationsSucceeded = accumulator.Commit(stages) && operationsSucceeded;
        scene.stages = stages;

        RtFrameToken attempt{};
        RtFrameToken recorded{};
        RtSubmittedFrameIdentity submitted{};
        operationsSucceeded =
            lifecycle.BeginRecord(0u, simulation.tick + index, attempt) && operationsSucceeded;
        operationsSucceeded =
            lifecycle.FinishRecord(attempt, MakeRecordedScene(scene), recorded) &&
            operationsSucceeded;
        operationsSucceeded = lifecycle.Submit(recorded, submitted) && operationsSucceeded;
        operationsSucceeded =
            lifecycle.AttachPresentation(submitted, RtPresentationOutcome::Presented) &&
            operationsSucceeded;
        RtPerformanceEvidenceSnapshot completed{};
        operationsSucceeded =
            lifecycle.CompleteFence(
                submitted, MakeSubmittedStages(submitted, scene.stages), diagnostic, gpu, completed) &&
            operationsSucceeded;
        RtPerformanceEvidenceSnapshot byValue = lifecycle.CompletedEvidenceByValue();
        checksum ^= byValue.identity.completionSerial;
        operationsSucceeded =
            ValidateRtPerformanceEvidence(byValue, validationError) && operationsSucceeded;

        if (index <= kRtEvidenceSampleCapacity)
        {
            RtCompletedStageSample collected{};
            collected.identity = byValue.identity;
            collected.stages = byValue.scene.stages;
            collected.benchmarkEligible = true;
            operationsSucceeded = collector.Append(collected) && operationsSucceeded;
        }
        else if (index == kRtEvidenceSampleCapacity + 1u)
        {
            RtStageStatistics statistics{};
            operationsSucceeded =
                collector.Statistics(RtStage::SimulationStep, statistics) && operationsSucceeded;
            checksum ^= statistics.sampleCount;
            operationsSucceeded = collector.Start(101u, 200u) && operationsSucceeded;
        }

        pipeline = byValue.scene.pipeline;
    }
    gCountAllocations = false;
    context.Check(operationsSucceeded,
                  "all operations inside the no-allocation fixture must satisfy their contracts");
    context.Check(gAllocationCount == 0u,
                  "100,000 accumulator/lifecycle/copy/validator/collector operations must allocate zero bytes");
    context.Check(checksum != std::numeric_limits<std::uint64_t>::max() &&
                      pipeline.activeStrategy == RtMaterialStrategy::OpaqueFast,
                  "allocation-free operations must remain observable to the test");
    context.Check(std::memcmp(&simulation, &simulationBefore, sizeof(simulation)) == 0 &&
                      std::memcmp(&renderInputs, &renderBefore, sizeof(renderInputs)) == 0,
                  "radically changing timing observations must not mutate simulation or render inputs");

    RtPerformanceEvidenceSnapshot serializerFailure = lifecycle.CompletedEvidenceByValue();
    serializerFailure.schema = 999u;
    std::string rejected;
    context.Check(!SerializeRtPerformanceEvidenceJson(serializerFailure, rejected, validationError) &&
                      rejected.empty() &&
                      std::memcmp(&simulation, &simulationBefore, sizeof(simulation)) == 0 &&
                      std::memcmp(&renderInputs, &renderBefore, sizeof(renderInputs)) == 0,
                  "serializer failure must remain a one-way observer with no deterministic side effect");
}

} // namespace

int main()
{
    TestContext context;
    TestFixedTextAndEnums(context);
    TestStageAccumulatorAndConversion(context);
    TestBoundedCollector(context);
    TestLifecycleAssociationAndTransactions(context);
    TestLifecycleResetTableAndExhaustion(context);
    TestMeasurementEligibility(context);
    TestDiagnosticPendingLifecycle(context);
    TestDiagnosticResetFailure(context);
    TestValidatorAndSerializers(context);
    TestSourceDependencyFence(context);
    TestAllocationHookCoverage(context);
    TestNoAllocationAndOneWayIsolation(context);

    if (context.failures == 0)
    {
        std::cout << "Immutable RT performance evidence tests passed.\n";
    }
    return context.failures == 0 ? 0 : 1;
}
