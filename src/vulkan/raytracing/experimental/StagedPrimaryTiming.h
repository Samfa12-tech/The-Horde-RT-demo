#pragma once

#include <cstdint>
#include <string>
#include <vector>

#include <vulkan/vulkan.h>

namespace horde::vulkan::raytracing::experimental {

enum class StagedPrimaryTimingInitStatus {
    Uninitialised,
    Ready,
    UnsupportedQueue,
    InitialisationFailed,
};

enum class StagedPrimaryTimingCollectionStatus {
    NoSubmittedWork,
    Valid,
    Unavailable,
    Error,
};

struct StagedPrimaryStageTiming {
    std::uint64_t elapsedTicks = 0u;
    double milliseconds = 0.0;
};

// A result remains bound to its submitting slot/sequence even for an
// unavailable timestamp or query IO failure. Collection is only legal after
// the caller has observed that slot's own graphics fence complete.
struct StagedPrimaryTimingCollection {
    StagedPrimaryTimingCollectionStatus status =
        StagedPrimaryTimingCollectionStatus::NoSubmittedWork;
    bool consumed = false;
    bool hasSamples = false;
    std::uint32_t frameSlot = 0u;
    std::uint64_t submissionSequence = 0u;
    VkResult result = VK_SUCCESS;
    StagedPrimaryStageTiming primaryIncludingPrerequisiteWait{};
    StagedPrimaryStageTiming shadeIncludingBarrier{};
};

// Investigation-only 3-timestamp helper. Queue/fence ownership is external;
// no renderer failure follows from unsupported queries or missing samples.
class StagedPrimaryTiming final {
public:
    static constexpr std::uint32_t kMaximumFrameSlots = 16u;

    StagedPrimaryTiming() = default;
    ~StagedPrimaryTiming();
    StagedPrimaryTiming(const StagedPrimaryTiming&) = delete;
    StagedPrimaryTiming& operator=(const StagedPrimaryTiming&) = delete;
    StagedPrimaryTiming(StagedPrimaryTiming&& other) noexcept;
    StagedPrimaryTiming& operator=(StagedPrimaryTiming&& other) noexcept;

    StagedPrimaryTimingInitStatus Initialise(VkPhysicalDevice physicalDevice,
        VkDevice device, std::uint32_t graphicsQueueFamilyIndex,
        std::uint32_t frameSlotCount);
    void Destroy() noexcept;
    void ResetAfterDeviceIdle() noexcept;

    bool RecordBegin(VkCommandBuffer commandBuffer, std::uint32_t frameSlot) noexcept;
    bool RecordPrimaryEnd(VkCommandBuffer commandBuffer, std::uint32_t frameSlot) noexcept;
    bool RecordEnd(VkCommandBuffer commandBuffer, std::uint32_t frameSlot) noexcept;
    void CancelRecording(std::uint32_t frameSlot) noexcept;
    bool MarkSubmitted(std::uint32_t frameSlot, std::uint64_t submissionSequence) noexcept;
    StagedPrimaryTimingCollection CollectCompleted(std::uint32_t frameSlot) noexcept;

    bool Supported() const noexcept { return queryPool_ != VK_NULL_HANDLE; }
    std::uint32_t FrameSlotCount() const noexcept { return static_cast<std::uint32_t>(slots_.size()); }
    std::uint32_t TimestampValidBits() const noexcept { return timestampValidBits_; }
    double TimestampPeriodNanoseconds() const noexcept { return timestampPeriodNanoseconds_; }
    StagedPrimaryTimingInitStatus Status() const noexcept { return status_; }
    const std::string& Diagnostic() const noexcept { return diagnostic_; }

private:
    enum class SlotPhase : std::uint8_t { Empty, Started, PrimaryEnded, Recorded, Submitted };
    struct FrameSlotState {
        VkCommandBuffer commandBuffer = VK_NULL_HANDLE;
        std::uint64_t submissionSequence = 0u;
        SlotPhase phase = SlotPhase::Empty;
    };

    bool OperationalSlot(std::uint32_t frameSlot) const noexcept;
    void MoveFrom(StagedPrimaryTiming&& other) noexcept;

    VkDevice device_ = VK_NULL_HANDLE;
    VkQueryPool queryPool_ = VK_NULL_HANDLE;
    std::uint32_t timestampValidBits_ = 0u;
    double timestampPeriodNanoseconds_ = 0.0;
    std::vector<FrameSlotState> slots_;
    StagedPrimaryTimingInitStatus status_ = StagedPrimaryTimingInitStatus::Uninitialised;
    std::string diagnostic_ = "Staged-primary timestamps are not initialised.";
};

} // namespace horde::vulkan::raytracing::experimental
