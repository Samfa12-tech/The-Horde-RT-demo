#include "vulkan/raytracing/experimental/StagedPrimaryTiming.h"
#include "vulkan/raytracing/experimental/StagedPrimaryProfile.h"

#include <algorithm>
#include <array>
#include <cmath>
#include <cstdint>
#include <iostream>
#include <limits>
#include <string>
#include <type_traits>
#include <utility>
#include <vector>

using namespace horde::vulkan::raytracing::experimental;

namespace {
template <typename Handle>
Handle FakeHandle(std::uintptr_t value)
{
    if constexpr (std::is_pointer_v<Handle>) return reinterpret_cast<Handle>(value);
    else return static_cast<Handle>(value);
}

struct Write {
    VkCommandBuffer command = VK_NULL_HANDLE;
    VkPipelineStageFlagBits stage{};
    VkQueryPool pool = VK_NULL_HANDLE;
    std::uint32_t index = 0u;
};
struct State {
    std::uintptr_t next = 10u;
    std::uint32_t timestampBits = 8u;
    float periodNanoseconds = 1'000'000.0f;
    std::uint32_t queueCount = 1u;
    VkResult createResult = VK_SUCCESS;
    bool partialPoolOnCreateFailure = false;
    int createCalls = 0;
    int destroyCalls = 0;
    std::uint32_t createdQueryCount = 0u;
    int resetCalls = 0;
    VkCommandBuffer resetCommand = VK_NULL_HANDLE;
    std::uint32_t resetFirst = 0u;
    std::uint32_t resetCount = 0u;
    std::vector<Write> writes;
    int resultCalls = 0;
    VkQueryPool resultPool = VK_NULL_HANDLE;
    std::uint32_t resultFirst = 0u;
    std::uint32_t resultCount = 0u;
    std::size_t resultDataSize = 0u;
    VkDeviceSize resultStride = 0u;
    VkQueryResultFlags resultFlags = 0u;
    VkResult resultCode = VK_SUCCESS;
    std::array<std::uint64_t, 3u> values{250u, 2u, 7u};
    std::array<std::uint64_t, 3u> availability{1u, 1u, 1u};
};
State g;

struct Check {
    int failures = 0;
    void That(bool condition, const char* message) {
        if (!condition) { std::cerr << "FAIL: " << message << '\n'; ++failures; }
    }
};

void ResetState()
{
    g = {};
    g.timestampBits = 8u;
    g.periodNanoseconds = 1'000'000.0f;
    g.queueCount = 1u;
    g.createResult = VK_SUCCESS;
    g.values = {250u, 2u, 7u};
    g.availability = {1u, 1u, 1u};
    g.resultCode = VK_SUCCESS;
}

bool BeginAndRecord(StagedPrimaryTiming& timer, std::uint32_t slot, std::uintptr_t cmd)
{
    const auto command = FakeHandle<VkCommandBuffer>(cmd);
    return timer.RecordBegin(command, slot) && timer.RecordPrimaryEnd(command, slot) &&
           timer.RecordEnd(command, slot);
}
bool Submit(StagedPrimaryTiming& timer, std::uint32_t slot, std::uint64_t sequence,
            std::uintptr_t cmd = 100u)
{
    return BeginAndRecord(timer, slot, cmd) && timer.MarkSubmitted(slot, sequence);
}
}

extern "C" {
VKAPI_ATTR void VKAPI_CALL vkGetPhysicalDeviceQueueFamilyProperties(
    VkPhysicalDevice, std::uint32_t* count, VkQueueFamilyProperties* properties)
{
    if (properties == nullptr) { *count = g.queueCount; return; }
    const auto fill = std::min(*count, g.queueCount);
    for (std::uint32_t i = 0u; i < fill; ++i) {
        properties[i] = {};
        properties[i].timestampValidBits = g.timestampBits;
    }
    *count = fill;
}
VKAPI_ATTR void VKAPI_CALL vkGetPhysicalDeviceProperties(VkPhysicalDevice, VkPhysicalDeviceProperties* properties)
{
    *properties = {};
    properties->limits.timestampPeriod = g.periodNanoseconds;
}
VKAPI_ATTR VkResult VKAPI_CALL vkCreateQueryPool(VkDevice, const VkQueryPoolCreateInfo* info,
    const VkAllocationCallbacks*, VkQueryPool* pool)
{
    ++g.createCalls;
    g.createdQueryCount = info->queryCount;
    *pool = g.createResult == VK_SUCCESS || g.partialPoolOnCreateFailure
        ? FakeHandle<VkQueryPool>(++g.next) : VK_NULL_HANDLE;
    return g.createResult;
}
VKAPI_ATTR void VKAPI_CALL vkDestroyQueryPool(VkDevice, VkQueryPool, const VkAllocationCallbacks*)
{ ++g.destroyCalls; }
VKAPI_ATTR void VKAPI_CALL vkCmdResetQueryPool(VkCommandBuffer command, VkQueryPool,
    std::uint32_t first, std::uint32_t count)
{
    ++g.resetCalls; g.resetCommand = command; g.resetFirst = first; g.resetCount = count;
}
VKAPI_ATTR void VKAPI_CALL vkCmdWriteTimestamp(VkCommandBuffer command, VkPipelineStageFlagBits stage,
    VkQueryPool pool, std::uint32_t index)
{ g.writes.push_back({command, stage, pool, index}); }
VKAPI_ATTR VkResult VKAPI_CALL vkGetQueryPoolResults(VkDevice, VkQueryPool pool,
    std::uint32_t first, std::uint32_t count, std::size_t size, void* data,
    VkDeviceSize stride, VkQueryResultFlags flags)
{
    ++g.resultCalls; g.resultPool = pool; g.resultFirst = first; g.resultCount = count;
    g.resultDataSize = size; g.resultStride = stride; g.resultFlags = flags;
    struct QueryWord { std::uint64_t value; std::uint64_t available; };
    auto* words = static_cast<QueryWord*>(data);
    for (std::uint32_t i = 0u; i < count && i < g.values.size(); ++i)
        words[i] = {g.values[i], g.availability[i]};
    return g.resultCode;
}
}

int main()
{
    Check check;
    ResetState();
    {
        StagedPrimaryTiming timer;
        g.timestampBits = 0u;
        check.That(timer.Initialise(FakeHandle<VkPhysicalDevice>(1u), FakeHandle<VkDevice>(2u), 0u, 2u) ==
                       StagedPrimaryTimingInitStatus::UnsupportedQueue && !timer.Supported() && g.createCalls == 0,
                   "timestamp-unsupported queue is an optional unavailable feature");
    }
    ResetState();
    {
        StagedPrimaryTiming timer;
        g.timestampBits = 65u;
        check.That(timer.Initialise(FakeHandle<VkPhysicalDevice>(1u), FakeHandle<VkDevice>(2u), 0u, 1u) ==
                       StagedPrimaryTimingInitStatus::InitialisationFailed && g.createCalls == 0,
                   "counter width above 64 is rejected before pool creation");
    }
    ResetState();
    {
        StagedPrimaryTiming timer;
        g.periodNanoseconds = 0.0f;
        check.That(timer.Initialise(FakeHandle<VkPhysicalDevice>(1u), FakeHandle<VkDevice>(2u), 0u, 1u) ==
                       StagedPrimaryTimingInitStatus::InitialisationFailed && g.createCalls == 0,
                   "zero timestamp period is rejected");
        check.That(timer.Initialise(FakeHandle<VkPhysicalDevice>(1u), FakeHandle<VkDevice>(2u), 0u, 17u) ==
                       StagedPrimaryTimingInitStatus::InitialisationFailed && g.createCalls == 0,
                   "frame-slot count above bounded maximum is rejected");
    }
    ResetState();
    {
        StagedPrimaryTiming timer;
        g.queueCount = 1u;
        check.That(timer.Initialise(FakeHandle<VkPhysicalDevice>(1u), FakeHandle<VkDevice>(2u), 1u, 1u) ==
                       StagedPrimaryTimingInitStatus::InitialisationFailed && g.createCalls == 0,
                   "out-of-range queue family rejects without query pool");
    }
    ResetState();
    {
        StagedPrimaryTiming timer;
        g.createResult = VK_ERROR_OUT_OF_HOST_MEMORY;
        g.partialPoolOnCreateFailure = true;
        check.That(timer.Initialise(FakeHandle<VkPhysicalDevice>(1u), FakeHandle<VkDevice>(2u), 0u, 2u) ==
                       StagedPrimaryTimingInitStatus::InitialisationFailed && !timer.Supported() &&
                       g.destroyCalls == 1,
                   "pool creation failure remains optional and destroys partial pool output exactly once");
    }

    ResetState();
    {
        StagedPrimaryTiming timer;
        check.That(timer.Initialise(FakeHandle<VkPhysicalDevice>(1u), FakeHandle<VkDevice>(2u), 0u, 2u) ==
                       StagedPrimaryTimingInitStatus::Ready && timer.Supported() &&
                       timer.FrameSlotCount() == 2u && g.createdQueryCount == 6u,
                   "two slots allocate exactly three timestamp queries per slot");
        const auto cmd0 = FakeHandle<VkCommandBuffer>(100u);
        const auto cmdOther = FakeHandle<VkCommandBuffer>(101u);
        check.That(!timer.RecordPrimaryEnd(cmd0, 0u) && !timer.RecordEnd(cmd0, 0u),
                   "timestamp stages reject out-of-order calls");
        check.That(timer.RecordBegin(cmd0, 0u) && g.resetCalls == 1 && g.resetCommand == cmd0 &&
                       g.resetFirst == 0u && g.resetCount == 3u && g.writes.size() == 1u &&
                       g.writes[0].stage == VK_PIPELINE_STAGE_TOP_OF_PIPE_BIT && g.writes[0].index == 0u,
                   "begin resets three queries and writes the first timestamp at TOP_OF_PIPE");
        check.That(!timer.RecordBegin(cmd0, 0u) && !timer.RecordPrimaryEnd(cmdOther, 0u),
                   "active slot cannot be overwritten or ended on another command buffer");
        check.That(timer.RecordPrimaryEnd(cmd0, 0u) && g.writes.size() == 2u &&
                       g.writes[1].stage == VK_PIPELINE_STAGE_RAY_TRACING_SHADER_BIT_KHR &&
                       g.writes[1].index == 1u && !timer.RecordEnd(cmdOther, 0u),
                   "primary boundary writes RT timestamp at query one and rejects wrong command");
        check.That(timer.RecordEnd(cmd0, 0u) && g.writes.size() == 3u &&
                       g.writes[2].stage == VK_PIPELINE_STAGE_RAY_TRACING_SHADER_BIT_KHR &&
                       g.writes[2].index == 2u,
                   "shade boundary writes RT timestamp at query two");
        check.That(!timer.MarkSubmitted(0u, 0u) && timer.MarkSubmitted(0u, 77u),
                   "submission requires a nonzero exact identity");
        timer.CancelRecording(0u);
        check.That(!timer.RecordBegin(cmd0, 0u), "cancel must not discard submitted query ownership");
        const auto valid = timer.CollectCompleted(0u);
        check.That(valid.status == StagedPrimaryTimingCollectionStatus::Valid && valid.consumed &&
                       valid.hasSamples && valid.frameSlot == 0u && valid.submissionSequence == 77u &&
                       valid.primaryIncludingPrerequisiteWait.elapsedTicks == 8u &&
                       valid.primaryIncludingPrerequisiteWait.milliseconds == 8.0 &&
                       valid.shadeIncludingBarrier.elapsedTicks == 5u &&
                       valid.shadeIncludingBarrier.milliseconds == 5.0,
                   "collection preserves identity and computes separate wrap-safe intervals");
        check.That(g.resultFirst == 0u && g.resultCount == 3u &&
                       g.resultDataSize == 3u * 2u * sizeof(std::uint64_t) &&
                       g.resultStride == 2u * sizeof(std::uint64_t) &&
                       (g.resultFlags & VK_QUERY_RESULT_64_BIT) != 0u &&
                       (g.resultFlags & VK_QUERY_RESULT_WITH_AVAILABILITY_BIT) != 0u &&
                       (g.resultFlags & VK_QUERY_RESULT_WAIT_BIT) == 0u,
                   "query read requests values plus availability without WAIT");
        check.That(timer.CollectCompleted(0u).status == StagedPrimaryTimingCollectionStatus::NoSubmittedWork,
                   "a consumed slot has no second collection");

        // Slot one has a different query triplet and can be cancelled after
        // recording when the graphics submission fails.
        check.That(timer.RecordBegin(cmd0, 1u) && timer.RecordPrimaryEnd(cmd0, 1u) &&
                       timer.RecordEnd(cmd0, 1u) && !timer.MarkSubmitted(1u, 0u),
                   "failed submission leaves a recorded but unsubmitted slot");
        timer.CancelRecording(1u);
        check.That(timer.RecordBegin(cmd0, 1u) && g.resetFirst == 3u &&
                       g.writes.back().index == 3u,
                   "cancelled unsubmitted slot can be reused at its own query base");
        timer.ResetAfterDeviceIdle();
        check.That(timer.RecordBegin(cmd0, 1u), "device-idle reset clears in-progress slot state");
    }

    for (const auto mode : {VK_NOT_READY, VK_SUCCESS, VK_ERROR_DEVICE_LOST}) {
        ResetState();
        StagedPrimaryTiming timer;
        (void)timer.Initialise(FakeHandle<VkPhysicalDevice>(1u), FakeHandle<VkDevice>(2u), 0u, 1u);
        check.That(Submit(timer, 0u, 900u), "result-status fixture submits an identified triplet");
        g.resultCode = mode;
        if (mode == VK_SUCCESS) g.availability[1] = 0u;
        const auto result = timer.CollectCompleted(0u);
        const auto expected = mode == VK_ERROR_DEVICE_LOST ? StagedPrimaryTimingCollectionStatus::Error
            : StagedPrimaryTimingCollectionStatus::Unavailable;
        check.That(result.status == expected && result.consumed && !result.hasSamples &&
                       result.frameSlot == 0u && result.submissionSequence == 900u &&
                       result.result == mode,
                   "unavailable and query-error results preserve exact identity and never report zero samples");
        check.That(timer.CollectCompleted(0u).status == StagedPrimaryTimingCollectionStatus::NoSubmittedWork,
                   "unavailable/error outcome consumes submitted slot exactly once");
    }

    ResetState();
    {
        StagedPrimaryTiming timer;
        check.That(timer.Initialise(FakeHandle<VkPhysicalDevice>(1u), FakeHandle<VkDevice>(2u), 0u, 1u) ==
                       StagedPrimaryTimingInitStatus::Ready, "move-lifetime owner initializes");
        StagedPrimaryTiming moved(std::move(timer));
        check.That(!timer.Supported() && moved.Supported() && g.destroyCalls == 0,
                   "move transfers unique query-pool ownership without early destruction");
        StagedPrimaryTiming assigned;
        assigned = std::move(moved);
        check.That(!moved.Supported() && assigned.Supported() && g.destroyCalls == 0,
                   "move assignment preserves unique query-pool ownership");
        assigned.Destroy();
        assigned.Destroy();
        check.That(g.destroyCalls == 1 && !assigned.Supported(),
                   "explicit and destructor cleanup destroy query pool only once");
    }

    ResetState();
    {
        StagedPrimaryTiming timer;
        (void)timer.Initialise(FakeHandle<VkPhysicalDevice>(1u), FakeHandle<VkDevice>(2u), 0u, 1u);
        {
            StagedPrimaryRecordingGuard guard(timer, 0u);
            check.That(BeginAndRecord(timer, 0u, 200u), "guard fixture records");
        }
        check.That(Submit(timer, 0u, 10u), "unsubmitted early-return guard permits next recording");
        {
            StagedPrimaryRecordingGuard guard(timer, 0u);
        }
        check.That(timer.CollectCompleted(0u).submissionSequence == 10u,
                   "early-return guard preserves successfully submitted fence ownership");

        StagedPrimaryProfile profile;
        check.That(profile.Start(3u), "profile preallocates bounded rows outside frame collection");
        horde::telemetry::RtPerformanceEvidenceSnapshot owner{};
        owner.identity.submitted.frame.sceneEpoch = 2u;
        owner.identity.submitted.frame.measurementGeneration = 3u;
        owner.cpuBenchmarkEligible = true;
        owner.gpu.status = horde::telemetry::RtSampleStatus::Valid;
        owner.gpu.hasDuration = true;
        owner.gpu.durationNanoseconds = 13'000'000u;
        StagedPrimaryTimingCollection result{};
        result.status = StagedPrimaryTimingCollectionStatus::Valid;
        result.hasSamples = result.consumed = true;
        result.primaryIncludingPrerequisiteWait = {8u, 8.0};
        result.shadeIncludingBarrier = {5u, 5.0};
        owner.identity.submitted.submissionSerial = owner.gpu.completedSubmissionSerial = result.submissionSequence = 100u;
        check.That(profile.Append(result, owner), "profile retains exact owner and sample");
        result.status = StagedPrimaryTimingCollectionStatus::Unavailable; result.hasSamples = false;
        owner.identity.submitted.submissionSerial = owner.gpu.completedSubmissionSerial = result.submissionSequence = 110u;
        check.That(profile.Append(result, owner), "unavailable query is retained as gap");
        result.status = StagedPrimaryTimingCollectionStatus::Valid; result.hasSamples = true;
        owner.identity.submitted.submissionSerial = owner.gpu.completedSubmissionSerial = 120u;
        result.submissionSequence = 119u;
        check.That(profile.Append(result, owner), "mismatched query identity is retained without duration admission");
        check.That(!profile.Append(result, owner), "duplicate/capacity cannot overwrite existing rows");
        const auto json = profile.Json(4u, timer);
        check.That(json.find("\"retainedCount\":3") != std::string::npos &&
                   json.find("\"missingCompletionCount\":1") != std::string::npos &&
                   json.find("\"rejectedCount\":1") != std::string::npos &&
                   json.find("\"primaryMilliseconds\":8") != std::string::npos &&
                   json.find("\"primaryMilliseconds\":null") != std::string::npos &&
                   json.find("\"identityMatches\":false") != std::string::npos &&
                   json.find("\"dramBandwidthMeasured\":false") != std::string::npos,
                   "JSON retains missing/rejected/identity gaps and does not infer DRAM traffic");
        const auto attached = AttachStagedPrimaryProfile("{\"ordinary\":1}\n", json);
        check.That(attached.find("\"ordinary\":1,\n\"stagedPassProfiling\":") != std::string::npos &&
                   AttachStagedPrimaryProfile("{}", "{}").find("{,\n") == std::string::npos &&
                   AttachStagedPrimaryProfile("malformed", json) == "malformed",
                   "profile attachment preserves ordinary report and handles empty/rejected input");
        check.That(!profile.Start(StagedPrimaryProfile::kMaximumRows + 1u) && profile.Count() == 0u,
                   "failed profile restart discards old run, rather than publishing stale rows");
        check.That(profile.Start(2u), "profile can restart after invalid capacity");
        result.submissionSequence = 120u;
        result.primaryIncludingPrerequisiteWait.milliseconds = std::numeric_limits<double>::quiet_NaN();
        check.That(profile.Append(result, owner) && profile.Json(1u, timer).find("\"primaryMilliseconds\":null") != std::string::npos,
                   "nonfinite duration cannot produce invalid numeric JSON");
        owner.identity.submitted.frame.frameSlot = 1u;
        check.That(!profile.Append(result, owner), "one-frame-in-flight profile rejects unsupported slot");
    }
    if (check.failures == 0) std::cout << "PASS staged-primary timestamp/profile ordering/identity/failure tests\n";
    return check.failures == 0 ? 0 : 1;
}
