#pragma once

#include <cstdint>
#include <optional>
#include <string>
#include <vector>

#include <vulkan/vulkan.h>

namespace horde::vulkan
{

enum class GpuFrameTimerStatus
{
    Uninitialised,
    UnsupportedQueue,
    InitialisationFailed,
    AwaitingSubmission,
    AwaitingResult,
    Available,
    ResultUnavailable,
    QueryError,
};

struct GpuFrameTimingSample
{
    double milliseconds = 0.0;
    std::uint64_t elapsedTicks = 0u;
    std::uint64_t submissionSequence = 0u;
    std::uint32_t frameSlot = 0u;
};

struct GpuFrameTimerTelemetry
{
    GpuFrameTimerStatus status = GpuFrameTimerStatus::Uninitialised;
    double latestMilliseconds = 0.0;
    float timestampPeriodNanoseconds = 0.0f;
    std::uint32_t timestampValidBits = 0u;
    std::uint64_t sampleCount = 0u;
    std::uint64_t unavailableResultCount = 0u;
    std::uint64_t errorCount = 0u;
    VkResult lastVkResult = VK_SUCCESS;
    std::string diagnostic = "GPU frame timing has not been initialised.";
};

// Device-level timestamp-query helper. The caller owns queue/fence ordering:
// CollectCompleted must only be called after the fence for that frame slot has
// completed successfully. This class is intended for the owning render thread
// and performs no internal locking.
class GpuFrameTimer
{
public:
    GpuFrameTimer() = default;
    ~GpuFrameTimer();

    GpuFrameTimer(const GpuFrameTimer&) = delete;
    GpuFrameTimer& operator=(const GpuFrameTimer&) = delete;
    GpuFrameTimer(GpuFrameTimer&& other) noexcept;
    GpuFrameTimer& operator=(GpuFrameTimer&& other) noexcept;

    // Unsupported timestamp queues and query-pool creation failures are
    // represented by the returned status and telemetry. They are not renderer
    // startup failures; callers should continue without GPU timing.
    GpuFrameTimerStatus Initialise(VkPhysicalDevice physicalDevice,
                                   VkDevice device,
                                   std::uint32_t graphicsQueueFamilyIndex,
                                   std::uint32_t frameSlotCount);
    void Destroy();

    // Discards recorded/submitted state after the caller has made the device
    // idle, for example during a render-scale or swapchain transition.
    void ResetAfterDeviceIdle();

    bool RecordBegin(VkCommandBuffer commandBuffer, std::uint32_t frameSlot);
    bool RecordEnd(VkCommandBuffer commandBuffer, std::uint32_t frameSlot);
    void CancelRecording(std::uint32_t frameSlot);
    bool MarkSubmitted(std::uint32_t frameSlot, std::uint64_t submissionSequence);
    std::optional<GpuFrameTimingSample> CollectCompleted(std::uint32_t frameSlot);

    bool Supported() const { return queryPool_ != VK_NULL_HANDLE; }
    std::uint32_t FrameSlotCount() const { return static_cast<std::uint32_t>(slots_.size()); }
    const GpuFrameTimerTelemetry& Telemetry() const { return telemetry_; }

private:
    struct FrameSlotState
    {
        VkCommandBuffer recordingCommandBuffer = VK_NULL_HANDLE;
        std::uint64_t submissionSequence = 0u;
        bool recording = false;
        bool recorded = false;
        bool submitted = false;
    };

    bool ValidateOperationalSlot(std::uint32_t frameSlot, const char* operation);
    void SetUsageError(const std::string& diagnostic);
    void SetQueryError(VkResult result, const std::string& diagnostic);
    void ResetMembers();

    VkDevice device_ = VK_NULL_HANDLE;
    VkQueryPool queryPool_ = VK_NULL_HANDLE;
    std::vector<FrameSlotState> slots_;
    GpuFrameTimerTelemetry telemetry_;
};

const char* GpuFrameTimerStatusName(GpuFrameTimerStatus status);

constexpr bool GpuFrameTimerHasCurrentSample(const GpuFrameTimerStatus status)
{
    return status == GpuFrameTimerStatus::Available;
}

} // namespace horde::vulkan
