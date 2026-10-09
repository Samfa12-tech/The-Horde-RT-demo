#include "vulkan/PresentTimingEvidence.h"

#include <array>
#include <cstdint>
#include <iostream>
#include <limits>
#include <sstream>
#include <string>
#include <type_traits>

namespace
{
using namespace horde::vulkan;

bool passed = true;
void Check(bool value, const char* message)
{
    if (!value)
    {
        passed = false;
        std::cerr << message << '\n';
    }
}

template<class Handle>
Handle FakeHandle(std::uintptr_t value)
{
    if constexpr (std::is_pointer_v<Handle>) return reinterpret_cast<Handle>(value);
    else return static_cast<Handle>(value);
}

struct Reply
{
    VkResult result = VK_SUCCESS;
    std::uint32_t count = 0u;
    std::array<VkPastPresentationTimingGOOGLE, PresentTimingEvidence::kTimingBatchCapacity> timings{};
};

struct FakeDriver
{
    std::array<Reply, 8u> replies{};
    std::size_t replyCount = 0u;
    std::size_t calls = 0u;
    std::size_t frameCalls = 0u;
    bool badHandles = false;
    VkDevice expectedDevice = VK_NULL_HANDLE;
    VkSwapchainKHR expectedSwapchain = VK_NULL_HANDLE;
} fake;

void AddReply(VkResult result, std::uint32_t count,
              const std::array<VkPastPresentationTimingGOOGLE, PresentTimingEvidence::kTimingBatchCapacity>& timings = {})
{
    auto& reply = fake.replies[fake.replyCount++];
    reply.result = result;
    reply.count = count;
    reply.timings = timings;
}

VKAPI_ATTR VkResult VKAPI_CALL GetPast(VkDevice device, VkSwapchainKHR swapchain,
                                       std::uint32_t* count,
                                       VkPastPresentationTimingGOOGLE* timings)
{
    if (device != fake.expectedDevice || swapchain != fake.expectedSwapchain) fake.badHandles = true;
    const auto index = fake.calls++;
    if (index >= fake.replyCount) return VK_ERROR_UNKNOWN;
    const auto& reply = fake.replies[index];
    if (timings == nullptr)
    {
        *count = reply.count;
        return reply.result;
    }
    ++fake.frameCalls;
    const auto written = reply.count < *count ? reply.count : *count;
    for (std::uint32_t item = 0u; item < written; ++item) timings[item] = reply.timings[item];
    *count = written;
    return reply.result;
}

VkDevice Device(std::uintptr_t value = 1u) { return FakeHandle<VkDevice>(value); }
VkSwapchainKHR Swapchain(std::uintptr_t value = 2u) { return FakeHandle<VkSwapchainKHR>(value); }

PresentTimingFrameMetadata Metadata(std::uint64_t record = 10u)
{
    return {9u, 4u, record, record + 100u, 77u, 123456789u, 50u, 720u, 1490u,
            PresentTimingBackend::RayTracingPipeline};
}

bool PrepareAndRegister(PresentTimingEvidence& evidence, const std::uint64_t record,
                        const VkResult result = VK_SUCCESS)
{
    VkPresentTimeGOOGLE time{};
    if (!evidence.PrepareNext(time)) return false;
    Check(time.presentID != 0u && time.desiredPresentTime == 0u,
          "Prepared timing uses a nonzero unique ID and leaves presentation ASAP");
    return evidence.RegisterPresent(time.presentID, result, Metadata(record));
}

void DisabledAndAdmission()
{
    fake = {};
    PresentTimingEvidence disabled;
    VkPresentTimeGOOGLE disabledTime{};
    Check(!disabled.HasStorage() && !disabled.Initialize(nullptr) && !disabled.HasStorage(),
          "Unsupported timing leaves the collector allocation-free");
    Check(!disabled.PrepareNext(disabledTime) && fake.calls == 0u,
          "Disabled collector neither prepares IDs nor queries the driver");

    PresentTimingEvidence evidence;
    Check(evidence.Initialize(GetPast) && evidence.HasStorage(), "Explicit resolved dispatch initializes bounded storage");
    fake.expectedDevice = Device(); fake.expectedSwapchain = Swapchain();
    Check(evidence.BindSwapchain(fake.expectedDevice, fake.expectedSwapchain, 9u), "Bind non-owning swapchain identity");
    VkPresentTimeGOOGLE first{}, second{};
    Check(evidence.PrepareNext(first) && evidence.RegisterPresent(first.presentID, VK_SUCCESS, Metadata()),
          "A successful admitted RT present becomes pending");
    Check(evidence.PrepareNext(second) && second.presentID > first.presentID &&
          evidence.RegisterPresent(second.presentID, VK_ERROR_OUT_OF_DATE_KHR, Metadata(11u)) == false,
          "Rejected present is not measured");
    Check(evidence.RowCount() == 0u && evidence.PendingCount() == 1u &&
          evidence.GetCounters().acceptedPresents == 1u && evidence.GetCounters().rejectedPresents == 1u,
          "Only successful present enters pending rows");
    VkPresentTimeGOOGLE invalidMetadataTime{};
    auto invalidMetadata = Metadata(12u);
    invalidMetadata.width = 0u;
    Check(evidence.PrepareNext(invalidMetadataTime) &&
          !evidence.RegisterPresent(invalidMetadataTime.presentID, VK_SUCCESS, invalidMetadata) &&
          evidence.GetCounters().invalidMetadata == 1u && evidence.PendingCount() == 1u,
          "Successful presents with incomplete owning-frame identity are counted but never joined");
    Check(!fake.badHandles, "Queries use the currently bound borrowed handles");
}

void OutOfOrderDuplicatesUnknownAndZero()
{
    fake = {};
    PresentTimingEvidence evidence;
    Check(evidence.Initialize(GetPast), "Initialize timing collector for delayed data");
    fake.expectedDevice = Device(); fake.expectedSwapchain = Swapchain();
    Check(evidence.BindSwapchain(fake.expectedDevice, fake.expectedSwapchain, 2u), "Bind timing test swapchain");
    VkPresentTimeGOOGLE first{}, second{};
    Check(evidence.PrepareNext(first) && evidence.RegisterPresent(first.presentID, VK_SUCCESS, Metadata(20u)), "Register first frame");
    Check(evidence.PrepareNext(second) && evidence.RegisterPresent(second.presentID, VK_SUCCESS, Metadata(21u)), "Register second frame");

    std::array<VkPastPresentationTimingGOOGLE, PresentTimingEvidence::kTimingBatchCapacity> batch{};
    batch[0].presentID = second.presentID; batch[0].actualPresentTime = 500u; batch[0].earliestPresentTime = 480u;
    batch[0].presentMargin = std::numeric_limits<std::uint64_t>::max();
    batch[1].presentID = second.presentID; batch[1].actualPresentTime = 500u;
    batch[2].presentID = 999u; batch[2].actualPresentTime = 510u;
    batch[3].presentID = first.presentID; batch[3].actualPresentTime = 0u;
    batch[4].presentID = 0u; batch[4].actualPresentTime = 520u;
    AddReply(VK_SUCCESS, 5u);
    AddReply(VK_SUCCESS, 5u, batch);
    Check(evidence.Poll() == VK_SUCCESS && evidence.RowCount() == 1u && evidence.PendingCount() == 1u,
          "Out-of-order timing admits only its matching frame and retains no-time frames as unresolved");
    const auto* row = evidence.ObservedRow(0u);
    Check(row != nullptr && row->presentID == second.presentID && row->measurementGeneration == 9u &&
          row->recordSerial == 21u &&
          row->surfaceGeneration == 2u && row->swapchainSerial == 1u && row->actualPresentTime == 500u &&
          row->earliestPresentTime == 480u &&
          row->presentMargin == std::numeric_limits<std::uint64_t>::max(),
          "Timing joins to exact frame metadata without reordering or synthesizing intervals");
    Check(evidence.GetCounters().duplicateTimings == 1u && evidence.GetCounters().unknownPresentIds == 1u &&
          evidence.GetCounters().zeroPresentTimestamps == 1u && evidence.GetCounters().zeroPresentIds == 1u,
          "Duplicate, unknown, zero-time and zero-ID records remain explicit counters");

    batch = {};
    batch[0].presentID = first.presentID; batch[0].actualPresentTime = 530u;
    AddReply(VK_SUCCESS, 1u);
    AddReply(VK_SUCCESS, 1u, batch);
    Check(evidence.Poll() == VK_SUCCESS && evidence.RowCount() == 2u && evidence.PendingCount() == 0u,
          "Late valid timing completes its earlier pending frame");

    batch = {};
    batch[0].presentID = second.presentID; batch[0].actualPresentTime = 540u;
    AddReply(VK_SUCCESS, 1u);
    AddReply(VK_SUCCESS, 1u, batch);
    Check(evidence.Poll() == VK_SUCCESS && evidence.GetCounters().duplicateTimings == 2u,
          "A duplicate arriving in a later poll cannot create another row");
}

void IncompleteErrorsAndRebind()
{
    fake = {};
    PresentTimingEvidence evidence;
    Check(evidence.Initialize(GetPast), "Initialize bounded poll fixture");
    fake.expectedDevice = Device(); fake.expectedSwapchain = Swapchain();
    Check(evidence.BindSwapchain(fake.expectedDevice, fake.expectedSwapchain, 7u), "Bind first generation");
    Check(PrepareAndRegister(evidence, 30u), "Register frame before incomplete query");

    std::array<VkPastPresentationTimingGOOGLE, PresentTimingEvidence::kTimingBatchCapacity> batch{};
    batch[0].presentID = 1u; batch[0].actualPresentTime = 700u;
    AddReply(VK_SUCCESS, 1u);
    AddReply(VK_INCOMPLETE, 1u, batch);
    Check(evidence.Poll() == VK_INCOMPLETE && fake.calls == 2u && fake.frameCalls == 1u &&
          evidence.RowCount() == 1u && evidence.GetCounters().incompleteQueries == 1u,
          "VK_INCOMPLETE returns after the count and single bounded fetch call");
    fake.replyCount = 0u; fake.calls = 0u; fake.frameCalls = 0u;

    Check(PrepareAndRegister(evidence, 31u), "Register frame for rebind accounting");
    fake.expectedSwapchain = Swapchain(3u);
    Check(evidence.BindSwapchain(fake.expectedDevice, fake.expectedSwapchain, 8u) &&
          evidence.PendingCount() == 0u && evidence.GetCounters().missingOnRebind == 1u,
          "Rebind counts unresolved prior-swapchain frames without waiting or joining generations");
    VkPresentTimeGOOGLE newGenerationTime{};
    Check(evidence.PrepareNext(newGenerationTime) && newGenerationTime.presentID == 3u &&
          evidence.RegisterPresent(newGenerationTime.presentID, VK_SUCCESS, Metadata(32u)),
          "Present IDs remain unique across swapchain generations");
    Check(evidence.PendingCount() == 1u, "New swapchain rows are independently pending");

    batch = {};
    batch[0].presentID = 2u; batch[0].actualPresentTime = 800u;
    AddReply(VK_SUCCESS, 1u);
    AddReply(VK_SUCCESS, 1u, batch);
    Check(evidence.Poll() == VK_SUCCESS && evidence.PendingCount() == 1u &&
          evidence.GetCounters().unknownPresentIds == 1u,
          "A delayed prior-generation timing cannot attach to the new swapchain row");
    batch[0].presentID = newGenerationTime.presentID; batch[0].actualPresentTime = 810u;
    AddReply(VK_SUCCESS, 1u);
    AddReply(VK_SUCCESS, 1u, batch);
    Check(evidence.Poll() == VK_SUCCESS && evidence.PendingCount() == 0u && evidence.RowCount() == 2u &&
          evidence.ObservedRow(1u)->surfaceGeneration == 8u && evidence.ObservedRow(1u)->swapchainSerial == 2u,
          "Only the new generation's exact timing joins its own completed row");

    AddReply(VK_SUCCESS, 0u);
    const auto priorCalls = fake.calls;
    Check(evidence.Poll() == VK_SUCCESS && fake.calls == priorCalls + 1u,
          "An empty asynchronous query performs only the bounded count call");
    AddReply(VK_ERROR_DEVICE_LOST, 0u);
    Check(evidence.Poll() == VK_ERROR_DEVICE_LOST && evidence.GetCounters().queryErrors == 1u,
          "Query failure is reported and counted without fabricating timings");

    VkPresentTimeGOOGLE unresolved{}, abandoned{};
    Check(evidence.PrepareNext(unresolved) && evidence.RegisterPresent(unresolved.presentID, VK_SUCCESS, Metadata(33u)) &&
          evidence.PrepareNext(abandoned), "Prepare one unresolved and one not-yet-registered present");

    std::ostringstream json;
    evidence.WriteJson(json);
    const auto text = json.str();
    Check(text.find("\"rows\":[{") != std::string::npos &&
          text.find("\"unresolved\":[{") != std::string::npos &&
          text.find("\"measurementGeneration\":9") != std::string::npos &&
          text.find("actual image presentation timestamp ns") != std::string::npos,
          "JSON keeps observed rows distinct from unresolved presents and labels the measured quantity");
    const auto callsBeforeUnbind = fake.calls;
    evidence.UnbindSwapchain();
    Check(!evidence.Bound() && evidence.PendingCount() == 0u && evidence.RowCount() == 2u &&
          evidence.GetCounters().missingOnUnbind == 1u &&
          evidence.GetCounters().abandonedPreparedPresents == 1u &&
          evidence.Poll() == VK_ERROR_INITIALIZATION_FAILED && fake.calls == callsBeforeUnbind,
          "Unbind counts unresolved and abandoned work, preserves rows, and clears borrowed swapchain access");
    Check(evidence.BindSwapchain(fake.expectedDevice, fake.expectedSwapchain, 9u),
          "Collector can bind a later surface generation after explicit unbind");
}

void Capacity()
{
    fake = {};
    PresentTimingEvidence evidence;
    Check(evidence.Initialize(GetPast), "Initialize full bounded row store");
    fake.expectedDevice = Device(); fake.expectedSwapchain = Swapchain();
    Check(evidence.BindSwapchain(fake.expectedDevice, fake.expectedSwapchain, 1u), "Bind capacity fixture");

    std::array<VkPastPresentationTimingGOOGLE, PresentTimingEvidence::kTimingBatchCapacity> batch{};
    std::uint64_t record = 1000u;
    for (std::size_t emitted = 0u; emitted < PresentTimingEvidence::kMaximumRows;
         emitted += PresentTimingEvidence::kTimingBatchCapacity)
    {
        const auto number = static_cast<std::uint32_t>(PresentTimingEvidence::kTimingBatchCapacity);
        for (std::uint32_t index = 0u; index < number; ++index)
        {
            VkPresentTimeGOOGLE time{};
            Check(evidence.PrepareNext(time), "Prepare within fixed row capacity");
            Check(evidence.RegisterPresent(time.presentID, VK_SUCCESS, Metadata(record++)),
                  "Register within fixed pending capacity");
            batch[index] = {};
            batch[index].presentID = time.presentID;
            batch[index].actualPresentTime = 1000000u + time.presentID;
        }
        AddReply(VK_SUCCESS, number);
        AddReply(VK_SUCCESS, number, batch);
        Check(evidence.Poll() == VK_SUCCESS, "Drain one fixed timing batch");
        fake.replyCount = 0u;
        fake.calls = 0u;
        fake.frameCalls = 0u;
    }
    Check(evidence.RowCount() == PresentTimingEvidence::kMaximumRows && evidence.PendingCount() == 0u,
          "Completed rows stop exactly at fixed capacity");

    VkPresentTimeGOOGLE overflow{};
    Check(evidence.PrepareNext(overflow) && evidence.RegisterPresent(overflow.presentID, VK_SUCCESS, Metadata(record)),
          "Pending capacity remains independent of completed-row capacity");
    batch = {};
    batch[0].presentID = overflow.presentID; batch[0].actualPresentTime = 2000000u;
    AddReply(VK_SUCCESS, 1u);
    AddReply(VK_SUCCESS, 1u, batch);
    Check(evidence.Poll() == VK_SUCCESS && evidence.RowCount() == PresentTimingEvidence::kMaximumRows &&
          evidence.PendingCount() == 0u && evidence.GetCounters().rowCapacityExhausted == 1u,
          "Row overflow consumes and counts the timing without exceeding storage");
    batch[0].presentID = overflow.presentID; batch[0].actualPresentTime = 2000001u;
    AddReply(VK_SUCCESS, 1u);
    AddReply(VK_SUCCESS, 1u, batch);
    Check(evidence.Poll() == VK_SUCCESS && evidence.GetCounters().duplicateTimings == 1u,
          "A timing dropped at capacity is still recognized as duplicate");
}

void PendingCapacity()
{
    fake = {};
    PresentTimingEvidence evidence;
    Check(evidence.Initialize(GetPast), "Initialize pending-capacity fixture");
    fake.expectedDevice = Device(); fake.expectedSwapchain = Swapchain();
    Check(evidence.BindSwapchain(fake.expectedDevice, fake.expectedSwapchain, 1u), "Bind pending-capacity fixture");
    for (std::size_t index = 0u; index < PresentTimingEvidence::kPendingCapacity + 1u; ++index)
        PrepareAndRegister(evidence, 4000u + index);
    Check(evidence.PendingCount() == PresentTimingEvidence::kPendingCapacity &&
          evidence.GetCounters().pendingCapacityExhausted == 1u && evidence.RowCount() == 0u,
          "Accepted presents beyond fixed pending capacity are counted without growing storage");
}
}

int main()
{
    DisabledAndAdmission();
    OutOfOrderDuplicatesUnknownAndZero();
    IncompleteErrorsAndRebind();
    Capacity();
    PendingCapacity();
    return passed ? 0 : 1;
}
