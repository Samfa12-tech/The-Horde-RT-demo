#include "vulkan/raytracing/experimental/StagedPrimaryTiming.h"

#include "vulkan/GpuTimestampMath.h"

#include <array>
#include <cmath>
#include <new>
#include <utility>

namespace horde::vulkan::raytracing::experimental {
namespace {
struct QueryWord {
    std::uint64_t value = 0u;
    std::uint64_t available = 0u;
};
static_assert(sizeof(QueryWord) == 2u * sizeof(std::uint64_t));
}

StagedPrimaryTiming::~StagedPrimaryTiming() { Destroy(); }

StagedPrimaryTiming::StagedPrimaryTiming(StagedPrimaryTiming&& other) noexcept
{
    MoveFrom(std::move(other));
}

StagedPrimaryTiming& StagedPrimaryTiming::operator=(StagedPrimaryTiming&& other) noexcept
{
    if (this != &other) {
        Destroy();
        MoveFrom(std::move(other));
    }
    return *this;
}

void StagedPrimaryTiming::MoveFrom(StagedPrimaryTiming&& other) noexcept
{
    device_ = std::exchange(other.device_, VkDevice{});
    queryPool_ = std::exchange(other.queryPool_, VkQueryPool{});
    timestampValidBits_ = std::exchange(other.timestampValidBits_, 0u);
    timestampPeriodNanoseconds_ = std::exchange(other.timestampPeriodNanoseconds_, 0.0);
    slots_ = std::move(other.slots_);
    status_ = std::exchange(other.status_, StagedPrimaryTimingInitStatus::Uninitialised);
    diagnostic_ = std::move(other.diagnostic_);
    other.slots_.clear();
    other.diagnostic_.clear();
}

StagedPrimaryTimingInitStatus StagedPrimaryTiming::Initialise(
    VkPhysicalDevice physicalDevice, VkDevice device,
    std::uint32_t graphicsQueueFamilyIndex, std::uint32_t frameSlotCount)
{
    Destroy();
    if (physicalDevice == VK_NULL_HANDLE || device == VK_NULL_HANDLE ||
        frameSlotCount == 0u || frameSlotCount > kMaximumFrameSlots) {
        status_ = StagedPrimaryTimingInitStatus::InitialisationFailed;
        diagnostic_ = "Invalid handles or frame-slot count for staged-primary timing.";
        return status_;
    }

    std::uint32_t familyCount = 0u;
    vkGetPhysicalDeviceQueueFamilyProperties(physicalDevice, &familyCount, nullptr);
    if (familyCount == 0u || graphicsQueueFamilyIndex >= familyCount) {
        status_ = StagedPrimaryTimingInitStatus::InitialisationFailed;
        diagnostic_ = "Graphics queue-family index is invalid for staged-primary timing.";
        return status_;
    }

    std::vector<VkQueueFamilyProperties> families;
    try {
        families.resize(familyCount);
    } catch (const std::bad_alloc&) {
        status_ = StagedPrimaryTimingInitStatus::InitialisationFailed;
        diagnostic_ = "Could not allocate queue-family properties for staged-primary timing.";
        return status_;
    }
    vkGetPhysicalDeviceQueueFamilyProperties(physicalDevice, &familyCount, families.data());
    if (graphicsQueueFamilyIndex >= familyCount || graphicsQueueFamilyIndex >= families.size()) {
        status_ = StagedPrimaryTimingInitStatus::InitialisationFailed;
        diagnostic_ = "Queue-family enumeration changed during staged-primary timing setup.";
        return status_;
    }

    VkPhysicalDeviceProperties properties{};
    vkGetPhysicalDeviceProperties(physicalDevice, &properties);
    const double period = static_cast<double>(properties.limits.timestampPeriod);
    const std::uint32_t validBits = families[graphicsQueueFamilyIndex].timestampValidBits;
    if (!std::isfinite(period) || period <= 0.0) {
        status_ = StagedPrimaryTimingInitStatus::InitialisationFailed;
        diagnostic_ = "Device reported an invalid timestamp period.";
        return status_;
    }
    if (validBits == 0u) {
        status_ = StagedPrimaryTimingInitStatus::UnsupportedQueue;
        diagnostic_ = "Graphics queue does not support timestamp queries; staged timing is unavailable.";
        return status_;
    }
    if (validBits > 64u) {
        status_ = StagedPrimaryTimingInitStatus::InitialisationFailed;
        diagnostic_ = "Device reported timestamp width greater than 64 bits.";
        return status_;
    }

    VkQueryPool candidatePool = VK_NULL_HANDLE;
    const VkQueryPoolCreateInfo createInfo{
        VK_STRUCTURE_TYPE_QUERY_POOL_CREATE_INFO, nullptr, 0u, VK_QUERY_TYPE_TIMESTAMP,
        frameSlotCount * 3u, 0u};
    const VkResult result = vkCreateQueryPool(device, &createInfo, nullptr, &candidatePool);
    if (result != VK_SUCCESS || candidatePool == VK_NULL_HANDLE) {
        if (candidatePool != VK_NULL_HANDLE) vkDestroyQueryPool(device, candidatePool, nullptr);
        status_ = StagedPrimaryTimingInitStatus::InitialisationFailed;
        diagnostic_ = "vkCreateQueryPool failed for staged-primary timestamp triplets.";
        return status_;
    }

    try {
        slots_.assign(frameSlotCount, FrameSlotState{});
    } catch (const std::bad_alloc&) {
        vkDestroyQueryPool(device, candidatePool, nullptr);
        status_ = StagedPrimaryTimingInitStatus::InitialisationFailed;
        diagnostic_ = "Could not allocate staged-primary timestamp slot state.";
        return status_;
    }
    device_ = device;
    queryPool_ = candidatePool;
    timestampValidBits_ = validBits;
    timestampPeriodNanoseconds_ = period;
    status_ = StagedPrimaryTimingInitStatus::Ready;
    diagnostic_ = "Staged-primary timestamp triplets are ready.";
    return status_;
}

void StagedPrimaryTiming::Destroy() noexcept
{
    if (device_ != VK_NULL_HANDLE && queryPool_ != VK_NULL_HANDLE)
        vkDestroyQueryPool(device_, queryPool_, nullptr);
    device_ = VK_NULL_HANDLE;
    queryPool_ = VK_NULL_HANDLE;
    timestampValidBits_ = 0u;
    timestampPeriodNanoseconds_ = 0.0;
    slots_.clear();
    status_ = StagedPrimaryTimingInitStatus::Uninitialised;
    diagnostic_.clear();
}

void StagedPrimaryTiming::ResetAfterDeviceIdle() noexcept
{
    if (!Supported()) return;
    for (auto& slot : slots_) slot = {};
}

bool StagedPrimaryTiming::OperationalSlot(std::uint32_t frameSlot) const noexcept
{
    return Supported() && frameSlot < slots_.size();
}

bool StagedPrimaryTiming::RecordBegin(VkCommandBuffer commandBuffer, std::uint32_t frameSlot) noexcept
{
    if (!OperationalSlot(frameSlot) || commandBuffer == VK_NULL_HANDLE) return false;
    auto& slot = slots_[frameSlot];
    if (slot.phase != SlotPhase::Empty) return false;
    const std::uint32_t firstQuery = frameSlot * 3u;
    vkCmdResetQueryPool(commandBuffer, queryPool_, firstQuery, 3u);
    vkCmdWriteTimestamp(commandBuffer, VK_PIPELINE_STAGE_TOP_OF_PIPE_BIT, queryPool_, firstQuery);
    slot.commandBuffer = commandBuffer;
    slot.phase = SlotPhase::Started;
    return true;
}

bool StagedPrimaryTiming::RecordPrimaryEnd(VkCommandBuffer commandBuffer,
                                            std::uint32_t frameSlot) noexcept
{
    if (!OperationalSlot(frameSlot) || commandBuffer == VK_NULL_HANDLE) return false;
    auto& slot = slots_[frameSlot];
    if (slot.phase != SlotPhase::Started || slot.commandBuffer != commandBuffer) return false;
    vkCmdWriteTimestamp(commandBuffer, VK_PIPELINE_STAGE_RAY_TRACING_SHADER_BIT_KHR,
                        queryPool_, frameSlot * 3u + 1u);
    slot.phase = SlotPhase::PrimaryEnded;
    return true;
}

bool StagedPrimaryTiming::RecordEnd(VkCommandBuffer commandBuffer,
                                     std::uint32_t frameSlot) noexcept
{
    if (!OperationalSlot(frameSlot) || commandBuffer == VK_NULL_HANDLE) return false;
    auto& slot = slots_[frameSlot];
    if (slot.phase != SlotPhase::PrimaryEnded || slot.commandBuffer != commandBuffer) return false;
    vkCmdWriteTimestamp(commandBuffer, VK_PIPELINE_STAGE_RAY_TRACING_SHADER_BIT_KHR,
                        queryPool_, frameSlot * 3u + 2u);
    slot.commandBuffer = VK_NULL_HANDLE;
    slot.phase = SlotPhase::Recorded;
    return true;
}

void StagedPrimaryTiming::CancelRecording(std::uint32_t frameSlot) noexcept
{
    if (frameSlot >= slots_.size() || slots_[frameSlot].phase == SlotPhase::Submitted) return;
    slots_[frameSlot] = {};
}

bool StagedPrimaryTiming::MarkSubmitted(std::uint32_t frameSlot,
                                         std::uint64_t submissionSequence) noexcept
{
    if (!OperationalSlot(frameSlot) || submissionSequence == 0u) return false;
    auto& slot = slots_[frameSlot];
    if (slot.phase != SlotPhase::Recorded) return false;
    slot.phase = SlotPhase::Submitted;
    slot.submissionSequence = submissionSequence;
    return true;
}

StagedPrimaryTimingCollection StagedPrimaryTiming::CollectCompleted(std::uint32_t frameSlot) noexcept
{
    StagedPrimaryTimingCollection collection{};
    collection.frameSlot = frameSlot;
    if (!OperationalSlot(frameSlot)) {
        collection.status = StagedPrimaryTimingCollectionStatus::Error;
        collection.result = VK_ERROR_UNKNOWN;
        return collection;
    }
    auto& slot = slots_[frameSlot];
    if (slot.phase != SlotPhase::Submitted) return collection;

    collection.consumed = true;
    collection.submissionSequence = slot.submissionSequence;
    std::array<QueryWord, 3u> words{};
    const VkResult result = vkGetQueryPoolResults(device_, queryPool_, frameSlot * 3u, 3u,
        sizeof(words), words.data(), sizeof(QueryWord),
        VK_QUERY_RESULT_64_BIT | VK_QUERY_RESULT_WITH_AVAILABILITY_BIT);
    slot = {}; // The owning fence is already complete; every outcome consumes this submission once.
    collection.result = result;
    if (result != VK_SUCCESS && result != VK_NOT_READY) {
        collection.status = StagedPrimaryTimingCollectionStatus::Error;
        return collection;
    }
    if (result == VK_NOT_READY || words[0].available == 0u ||
        words[1].available == 0u || words[2].available == 0u) {
        collection.status = StagedPrimaryTimingCollectionStatus::Unavailable;
        return collection;
    }

    const auto primary = horde::vulkan::ComputeGpuTimestampDuration(
        words[0].value, words[1].value, timestampValidBits_, timestampPeriodNanoseconds_);
    const auto shade = horde::vulkan::ComputeGpuTimestampDuration(
        words[1].value, words[2].value, timestampValidBits_, timestampPeriodNanoseconds_);
    if (!primary || !shade) {
        collection.status = StagedPrimaryTimingCollectionStatus::Error;
        collection.result = VK_ERROR_UNKNOWN;
        return collection;
    }

    collection.status = StagedPrimaryTimingCollectionStatus::Valid;
    collection.hasSamples = true;
    collection.primaryIncludingPrerequisiteWait = {primary->elapsedTicks, primary->milliseconds};
    collection.shadeIncludingBarrier = {shade->elapsedTicks, shade->milliseconds};
    return collection;
}

} // namespace horde::vulkan::raytracing::experimental
