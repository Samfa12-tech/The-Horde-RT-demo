#include "vulkan/GpuFrameTimer.h"
#include "vulkan/raytracing/RtSceneGpuStageProbe.h"
#include "vulkan/raytracing/RtDispatchClockProbe.h"

#include <array>
#include <cstdint>
#include <iostream>
#include <limits>
#include <type_traits>

namespace
{

enum class QueryMode
{
    Valid,
    NotReady,
    Unavailable,
    Error,
};

QueryMode gQueryMode = QueryMode::Valid;
std::uint64_t gQueryReadCount = 0u;
VkPipelineStageFlagBits gBeginStage{},gEndStage{};
bool gQueryResultWaitRequested = false;

template <typename Handle>
Handle FakeHandle(const std::uintptr_t value)
{
    if constexpr (std::is_pointer_v<Handle>)
    {
        return reinterpret_cast<Handle>(value);
    }
    else
    {
        return static_cast<Handle>(value);
    }
}

struct TestContext
{
    int failures = 0;

    void Check(const bool condition, const char* message)
    {
        if (!condition)
        {
            std::cerr << "FAIL: " << message << '\n';
            ++failures;
        }
    }
};

} // namespace

extern "C"
{

VKAPI_ATTR void VKAPI_CALL vkGetPhysicalDeviceQueueFamilyProperties(
    VkPhysicalDevice,
    std::uint32_t* count,
    VkQueueFamilyProperties* properties)
{
    if (properties == nullptr)
    {
        *count = 1u;
        return;
    }
    properties[0] = {};
    properties[0].timestampValidBits = 48u;
}

VKAPI_ATTR void VKAPI_CALL vkGetPhysicalDeviceProperties(
    VkPhysicalDevice,
    VkPhysicalDeviceProperties* properties)
{
    *properties = {};
    properties->limits.timestampPeriod = 2.0f;
}

VKAPI_ATTR VkResult VKAPI_CALL vkCreateQueryPool(
    VkDevice,
    const VkQueryPoolCreateInfo*,
    const VkAllocationCallbacks*,
    VkQueryPool* pool)
{
    *pool = FakeHandle<VkQueryPool>(0x44u);
    return VK_SUCCESS;
}

VKAPI_ATTR void VKAPI_CALL vkDestroyQueryPool(
    VkDevice,
    VkQueryPool,
    const VkAllocationCallbacks*)
{
}

VKAPI_ATTR void VKAPI_CALL vkCmdResetQueryPool(
    VkCommandBuffer,
    VkQueryPool,
    std::uint32_t,
    std::uint32_t)
{
}

VKAPI_ATTR void VKAPI_CALL vkCmdWriteTimestamp(
    VkCommandBuffer,
    VkPipelineStageFlagBits stage,
    VkQueryPool,
    std::uint32_t query)
{
    (query%2u?gEndStage:gBeginStage)=stage;
}

VKAPI_ATTR VkResult VKAPI_CALL vkGetQueryPoolResults(
    VkDevice,
    VkQueryPool,
    std::uint32_t,
    std::uint32_t,
    std::size_t,
    void* data,
    VkDeviceSize,
    VkQueryResultFlags flags)
{
    ++gQueryReadCount;
    gQueryResultWaitRequested = gQueryResultWaitRequested || (flags & VK_QUERY_RESULT_WAIT_BIT) != 0;
    if (gQueryMode == QueryMode::Error)
    {
        return VK_ERROR_DEVICE_LOST;
    }
    if (gQueryMode == QueryMode::NotReady)
    {
        return VK_NOT_READY;
    }
    auto* values = static_cast<std::uint64_t*>(data);
    values[0] = 100u;
    values[1] = gQueryMode == QueryMode::Unavailable ? 0u : 1u;
    values[2] = 160u;
    values[3] = 1u;
    return VK_SUCCESS;
}

} // extern "C"

int main()
{
    using namespace horde::vulkan;

    TestContext context;
    GpuFrameTimer timer;
    context.Check(timer.Initialise(FakeHandle<VkPhysicalDevice>(0x11u),
                                   FakeHandle<VkDevice>(0x22u),
                                   0u,
                                   1u) == GpuFrameTimerStatus::AwaitingSubmission,
                  "timer fixture must initialise");

    const VkCommandBuffer commandBuffer = FakeHandle<VkCommandBuffer>(0x33u);
    const auto submitAndCollect = [&](const std::uint64_t serial, const QueryMode mode) {
        gQueryMode = mode;
        context.Check(timer.RecordBegin(commandBuffer, 0u) &&
                          timer.RecordEnd(commandBuffer, 0u) &&
                          timer.MarkSubmitted(0u, serial),
                      "timer fixture must record and submit a timestamp pair");
        return timer.CollectCompleted(0u);
    };

    const auto checkRepeatDrainWithoutQueryIo = [&](const char* message) {
        const std::uint64_t readsBeforeDrain = gQueryReadCount;
        const GpuFrameTimingCollection drained = timer.CollectCompleted(0u);
        context.Check(drained.status == GpuFrameTimingCollectionStatus::NoSubmittedWork &&
                          !drained.consumed && !drained.hasSample &&
                          drained.frameSlot == 0u && drained.submissionSequence == 0u &&
                          gQueryReadCount == readsBeforeDrain,
                      message);
    };

    const GpuFrameTimingCollection notReady = submitAndCollect(40u, QueryMode::NotReady);
    context.Check(gBeginStage==VK_PIPELINE_STAGE_TOP_OF_PIPE_BIT && gEndStage==VK_PIPELINE_STAGE_BOTTOM_OF_PIPE_BIT,
                  "ordinary whole-frame timestamp endpoints must remain unchanged");
    context.Check(notReady.status == GpuFrameTimingCollectionStatus::Unavailable &&
                      notReady.consumed && notReady.frameSlot == 0u &&
                      notReady.submissionSequence == 40u && !notReady.hasSample &&
                      notReady.result == VK_NOT_READY,
                  "VK_NOT_READY must consume and identify the submitted query pair");
    checkRepeatDrainWithoutQueryIo(
        "a consumed VK_NOT_READY result must leave no query work for a repeat drain");

    const GpuFrameTimingCollection unavailable =
        submitAndCollect(41u, QueryMode::Unavailable);
    context.Check(unavailable.status == GpuFrameTimingCollectionStatus::Unavailable &&
                      unavailable.consumed && unavailable.frameSlot == 0u &&
                      unavailable.submissionSequence == 41u && !unavailable.hasSample,
                  "availability-zero must consume and identify the submitted query pair");

    const GpuFrameTimingCollection queryError =
        submitAndCollect(42u, QueryMode::Error);
    context.Check(queryError.status == GpuFrameTimingCollectionStatus::Error &&
                      queryError.consumed && queryError.frameSlot == 0u &&
                      queryError.submissionSequence == 42u && !queryError.hasSample &&
                      queryError.result == VK_ERROR_DEVICE_LOST,
                  "query failure must consume and identify the submitted query pair");

    const GpuFrameTimingCollection valid = submitAndCollect(43u, QueryMode::Valid);
    context.Check(valid.status == GpuFrameTimingCollectionStatus::Valid && valid.consumed &&
                      valid.hasSample && valid.frameSlot == 0u &&
                      valid.submissionSequence == 43u && valid.sample.elapsedTicks == 60u &&
                      valid.sample.milliseconds == 0.00012,
                  "valid collection must retain exact identity and duration");

    const std::uint64_t readsBeforeEmpty = gQueryReadCount;
    const GpuFrameTimingCollection empty = timer.CollectCompleted(0u);
    context.Check(empty.status == GpuFrameTimingCollectionStatus::NoSubmittedWork &&
                      !empty.consumed && !empty.hasSample &&
                      gQueryReadCount == readsBeforeEmpty,
                  "an empty slot must be distinct and perform no query IO");

    // Initialise rejects an invalid timestamp period, so this corruption-only
    // fixture exercises the defensive conversion guard without adding a
    // production test API. The timer itself is non-const, making this
    // const_cast a controlled mutation of the underlying telemetry object.
    auto& mutableTelemetry = const_cast<GpuFrameTimerTelemetry&>(timer.Telemetry());
    const float validTimestampPeriod = mutableTelemetry.timestampPeriodNanoseconds;
    mutableTelemetry.timestampPeriodNanoseconds = std::numeric_limits<float>::infinity();
    const GpuFrameTimingCollection invalidDuration =
        submitAndCollect(44u, QueryMode::Valid);
    mutableTelemetry.timestampPeriodNanoseconds = validTimestampPeriod;
    context.Check(invalidDuration.status == GpuFrameTimingCollectionStatus::Error &&
                      invalidDuration.consumed && invalidDuration.frameSlot == 0u &&
                      invalidDuration.submissionSequence == 44u &&
                      !invalidDuration.hasSample && invalidDuration.result == VK_ERROR_UNKNOWN,
                  "duration conversion failure must consume and identify the submitted query pair");
    checkRepeatDrainWithoutQueryIo(
        "a consumed duration conversion failure must leave no query work for a repeat drain");

    for(const auto stage: {VK_PIPELINE_STAGE_ACCELERATION_STRUCTURE_BUILD_BIT_KHR,
        VK_PIPELINE_STAGE_RAY_TRACING_SHADER_BIT_KHR,VK_PIPELINE_STAGE_COMPUTE_SHADER_BIT,VK_PIPELINE_STAGE_TRANSFER_BIT}) {
        context.Check(timer.RecordBegin(commandBuffer,0,stage) && timer.RecordEnd(commandBuffer,0,stage) &&
            gBeginStage==stage && gEndStage==stage && timer.MarkSubmitted(0,100+static_cast<unsigned>(stage)),
            "profile timers must use the selected AS/shader/transfer execution endpoints");
        context.Check(timer.CollectCompleted(0).hasSample,"stage-specific query must retain normal collection validity");
    }
    {
        horde::vulkan::raytracing::RtSceneGpuStageProbe guard({&timer,nullptr,nullptr},0);
        context.Check(guard.Begin(commandBuffer,0,VK_PIPELINE_STAGE_TRANSFER_BIT),"probe begins its borrowed timer");
        guard.End(commandBuffer,0,VK_PIPELINE_STAGE_TRANSFER_BIT);
    }
    context.Check(timer.RecordBegin(commandBuffer,0) && timer.RecordEnd(commandBuffer,0),
                  "aborted/unsubmitted probe scope must release its recording");
    timer.CancelRecording(0);
    {
        horde::vulkan::raytracing::RtSceneGpuStageProbe guard({&timer,nullptr,nullptr},0);
        context.Check(guard.Begin(commandBuffer,0,VK_PIPELINE_STAGE_TRANSFER_BIT),"probe records a submitted timer");
        guard.End(commandBuffer,0,VK_PIPELINE_STAGE_TRANSFER_BIT);
        context.Check(guard.recorded[0] && timer.MarkSubmitted(0,900),"submitted probe retains exact identity");
    }
    const auto retained=timer.CollectCompleted(0);
    context.Check(retained.hasSample && retained.submissionSequence==900,
                  "recording guard must not cancel queries owned by submitted GPU work");

    {
        using namespace horde::vulkan::raytracing;
        context.Check(DispatchClockAxisSamples(16)==0 && DispatchClockAxisSamples(17)==1 &&
            DispatchClockAxisSamples(576)==18 && DispatchClockAxisSamples(1188)==37,
            "sparse clock grid must match exact selected pixel centers");
        std::array<RtDispatchClockSample,2> samples{};
        samples[0].tag={7,16,16,0};samples[1].tag={7,48,16,1};
        samples[0].ticks[1]={4,1};samples[1].ticks[1]={8,0};
        std::array<std::uint64_t,8> totals{};
        context.Check(SummarizeDispatchClock(samples,7,64,32,totals) && totals[1]==(1ull<<32)+12,
            "clock sums must preserve both halves, instance miss and exact pixels");
        samples[1].tag[0]=6;
        context.Check(!SummarizeDispatchClock(samples,7,64,32,totals),"stale recording ticket rejected");
        samples[1].tag[0]=7;samples[1].tag[1]=16;
        context.Check(!SummarizeDispatchClock(samples,7,64,32,totals),"duplicate/wrong sample pixel rejected");
        samples[1].tag[1]=48;
        context.Check(!SummarizeDispatchClock(std::span(samples).first(1),7,64,32,totals),"missing clock sample rejected");
        samples[0].ticks[1]={0xffffffffu,0xffffffffu};
        context.Check(!SummarizeDispatchClock(samples,7,64,32,totals),"aggregate overflow rejected");
        samples[0].ticks[1]={};samples[1].ticks[1]={};
        context.Check(!SummarizeDispatchClock(samples,7,64,32,totals),"all-zero unavailable clock rejected");
    }

    context.Check(!gQueryResultWaitRequested,
                  "owning-fence timestamp collection must never request a query-result wait");

    if (context.failures == 0)
    {
        std::cout << "GPU frame timer collection identity tests passed.\n";
    }
    return context.failures == 0 ? 0 : 1;
}
