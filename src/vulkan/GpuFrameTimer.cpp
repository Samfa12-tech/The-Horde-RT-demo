#include "vulkan/GpuFrameTimer.h"

#include <array>
#include <cmath>
#include <limits>
#include <sstream>
#include <utility>

#include "vulkan/GpuTimestampMath.h"

namespace horde::vulkan
{
namespace
{

struct TimestampQueryResult
{
    std::uint64_t value = 0u;
    std::uint64_t available = 0u;
};

static_assert(sizeof(TimestampQueryResult) == 2u * sizeof(std::uint64_t));

std::string VkResultDiagnostic(const char* operation, const VkResult result)
{
    std::ostringstream out;
    out << operation << " failed with VkResult(" << static_cast<int>(result) << ").";
    return out.str();
}

} // namespace

const char* GpuFrameTimerStatusName(const GpuFrameTimerStatus status)
{
    switch (status)
    {
    case GpuFrameTimerStatus::Uninitialised: return "uninitialised";
    case GpuFrameTimerStatus::UnsupportedQueue: return "unsupported-queue";
    case GpuFrameTimerStatus::InitialisationFailed: return "initialisation-failed";
    case GpuFrameTimerStatus::AwaitingSubmission: return "awaiting-submission";
    case GpuFrameTimerStatus::AwaitingResult: return "awaiting-result";
    case GpuFrameTimerStatus::Available: return "available";
    case GpuFrameTimerStatus::ResultUnavailable: return "result-unavailable";
    case GpuFrameTimerStatus::QueryError: return "query-error";
    default: return "unknown";
    }
}

GpuFrameTimer::~GpuFrameTimer()
{
    Destroy();
}

GpuFrameTimer::GpuFrameTimer(GpuFrameTimer&& other) noexcept
{
    *this = std::move(other);
}

GpuFrameTimer& GpuFrameTimer::operator=(GpuFrameTimer&& other) noexcept
{
    if (this == &other)
    {
        return *this;
    }

    Destroy();
    device_ = std::exchange(other.device_, VkDevice{});
    queryPool_ = std::exchange(other.queryPool_, VkQueryPool{});
    slots_ = std::move(other.slots_);
    telemetry_ = std::move(other.telemetry_);
    other.slots_.clear();
    other.telemetry_ = {};
    return *this;
}

GpuFrameTimerStatus GpuFrameTimer::Initialise(const VkPhysicalDevice physicalDevice,
                                               const VkDevice device,
                                               const std::uint32_t graphicsQueueFamilyIndex,
                                               const std::uint32_t frameSlotCount)
{
    Destroy();
    telemetry_ = {};

    if (physicalDevice == VK_NULL_HANDLE || device == VK_NULL_HANDLE || frameSlotCount == 0u ||
        frameSlotCount > std::numeric_limits<std::uint32_t>::max() / 2u)
    {
        telemetry_.status = GpuFrameTimerStatus::InitialisationFailed;
        telemetry_.diagnostic = "Invalid Vulkan handles or frame-slot count supplied for GPU frame timing.";
        ++telemetry_.errorCount;
        return telemetry_.status;
    }

    std::uint32_t queueFamilyCount = 0u;
    vkGetPhysicalDeviceQueueFamilyProperties(physicalDevice, &queueFamilyCount, nullptr);
    if (graphicsQueueFamilyIndex >= queueFamilyCount)
    {
        telemetry_.status = GpuFrameTimerStatus::InitialisationFailed;
        telemetry_.diagnostic = "GPU frame timing received an out-of-range graphics queue-family index.";
        ++telemetry_.errorCount;
        return telemetry_.status;
    }

    std::vector<VkQueueFamilyProperties> queueFamilies(queueFamilyCount);
    vkGetPhysicalDeviceQueueFamilyProperties(physicalDevice, &queueFamilyCount, queueFamilies.data());
    telemetry_.timestampValidBits = queueFamilies[graphicsQueueFamilyIndex].timestampValidBits;

    VkPhysicalDeviceProperties physicalDeviceProperties{};
    vkGetPhysicalDeviceProperties(physicalDevice, &physicalDeviceProperties);
    telemetry_.timestampPeriodNanoseconds = physicalDeviceProperties.limits.timestampPeriod;

    if (telemetry_.timestampValidBits == 0u)
    {
        telemetry_.status = GpuFrameTimerStatus::UnsupportedQueue;
        telemetry_.diagnostic = "The selected graphics queue family does not support timestamp queries.";
        return telemetry_.status;
    }
    if (telemetry_.timestampValidBits > 64u ||
        !std::isfinite(telemetry_.timestampPeriodNanoseconds) ||
        telemetry_.timestampPeriodNanoseconds <= 0.0f)
    {
        telemetry_.status = GpuFrameTimerStatus::InitialisationFailed;
        telemetry_.diagnostic = "The Vulkan device reported invalid timestamp counter properties.";
        ++telemetry_.errorCount;
        return telemetry_.status;
    }

    const VkQueryPoolCreateInfo createInfo{
        VK_STRUCTURE_TYPE_QUERY_POOL_CREATE_INFO,
        nullptr,
        0u,
        VK_QUERY_TYPE_TIMESTAMP,
        frameSlotCount * 2u,
        0u};
    const VkResult createResult = vkCreateQueryPool(device, &createInfo, nullptr, &queryPool_);
    telemetry_.lastVkResult = createResult;
    if (createResult != VK_SUCCESS)
    {
        queryPool_ = VK_NULL_HANDLE;
        telemetry_.status = GpuFrameTimerStatus::InitialisationFailed;
        telemetry_.diagnostic = VkResultDiagnostic("vkCreateQueryPool", createResult);
        ++telemetry_.errorCount;
        return telemetry_.status;
    }

    device_ = device;
    slots_.resize(frameSlotCount);
    telemetry_.status = GpuFrameTimerStatus::AwaitingSubmission;
    telemetry_.diagnostic = "GPU timestamp queries are ready; awaiting the first RT submission.";
    return telemetry_.status;
}

void GpuFrameTimer::Destroy()
{
    if (device_ != VK_NULL_HANDLE && queryPool_ != VK_NULL_HANDLE)
    {
        vkDestroyQueryPool(device_, queryPool_, nullptr);
    }
    ResetMembers();
}

void GpuFrameTimer::ResetMembers()
{
    device_ = VK_NULL_HANDLE;
    queryPool_ = VK_NULL_HANDLE;
    slots_.clear();
    telemetry_ = {};
}

void GpuFrameTimer::ResetAfterDeviceIdle()
{
    if (!Supported())
    {
        return;
    }
    for (FrameSlotState& slot : slots_)
    {
        slot = {};
    }
    telemetry_.latestMilliseconds = 0.0;
    telemetry_.status = GpuFrameTimerStatus::AwaitingSubmission;
    telemetry_.diagnostic = "GPU timing state reset after the device became idle.";
}

bool GpuFrameTimer::ValidateOperationalSlot(const std::uint32_t frameSlot, const char* operation)
{
    if (!Supported())
    {
        // Preserve the more useful unsupported/initialisation-failure status.
        // Callers can unconditionally probe Supported() without turning an
        // optional telemetry feature into a runtime error.
        return false;
    }
    if (frameSlot >= slots_.size())
    {
        SetUsageError(std::string(operation) + " received an out-of-range frame slot.");
        return false;
    }
    return true;
}

void GpuFrameTimer::SetUsageError(const std::string& diagnostic)
{
    telemetry_.status = GpuFrameTimerStatus::QueryError;
    telemetry_.diagnostic = diagnostic;
    telemetry_.lastVkResult = VK_ERROR_UNKNOWN;
    ++telemetry_.errorCount;
}

void GpuFrameTimer::SetQueryError(const VkResult result, const std::string& diagnostic)
{
    telemetry_.status = GpuFrameTimerStatus::QueryError;
    telemetry_.diagnostic = diagnostic;
    telemetry_.lastVkResult = result;
    ++telemetry_.errorCount;
}

bool GpuFrameTimer::RecordBegin(const VkCommandBuffer commandBuffer, const std::uint32_t frameSlot)
{
    if (!ValidateOperationalSlot(frameSlot, "RecordBegin"))
    {
        return false;
    }
    if (commandBuffer == VK_NULL_HANDLE)
    {
        SetUsageError("RecordBegin received a null command buffer.");
        return false;
    }

    FrameSlotState& slot = slots_[frameSlot];
    if (slot.recording || slot.recorded || slot.submitted)
    {
        SetUsageError("RecordBegin attempted to overwrite a frame slot that has not been collected or cancelled.");
        return false;
    }

    const std::uint32_t firstQuery = frameSlot * 2u;
    vkCmdResetQueryPool(commandBuffer, queryPool_, firstQuery, 2u);
    vkCmdWriteTimestamp(commandBuffer, VK_PIPELINE_STAGE_TOP_OF_PIPE_BIT, queryPool_, firstQuery);
    slot.recordingCommandBuffer = commandBuffer;
    slot.recording = true;
    return true;
}

bool GpuFrameTimer::RecordEnd(const VkCommandBuffer commandBuffer, const std::uint32_t frameSlot)
{
    if (!ValidateOperationalSlot(frameSlot, "RecordEnd"))
    {
        return false;
    }
    if (commandBuffer == VK_NULL_HANDLE)
    {
        SetUsageError("RecordEnd received a null command buffer.");
        return false;
    }

    FrameSlotState& slot = slots_[frameSlot];
    if (!slot.recording || slot.recordingCommandBuffer != commandBuffer)
    {
        SetUsageError("RecordEnd did not match an active RecordBegin command buffer and frame slot.");
        return false;
    }

    vkCmdWriteTimestamp(commandBuffer,
                        VK_PIPELINE_STAGE_BOTTOM_OF_PIPE_BIT,
                        queryPool_,
                        frameSlot * 2u + 1u);
    slot.recordingCommandBuffer = VK_NULL_HANDLE;
    slot.recording = false;
    slot.recorded = true;
    return true;
}

void GpuFrameTimer::CancelRecording(const std::uint32_t frameSlot)
{
    if (frameSlot >= slots_.size())
    {
        return;
    }
    FrameSlotState& slot = slots_[frameSlot];
    if (!slot.submitted)
    {
        slot = {};
    }
}

bool GpuFrameTimer::MarkSubmitted(const std::uint32_t frameSlot,
                                  const std::uint64_t submissionSequence)
{
    if (!ValidateOperationalSlot(frameSlot, "MarkSubmitted"))
    {
        return false;
    }
    FrameSlotState& slot = slots_[frameSlot];
    if (!slot.recorded || slot.recording || slot.submitted)
    {
        SetUsageError("MarkSubmitted requires a completed timestamp pair that has not already been submitted.");
        return false;
    }

    slot.recorded = false;
    slot.submitted = true;
    slot.submissionSequence = submissionSequence;
    telemetry_.status = GpuFrameTimerStatus::AwaitingResult;
    telemetry_.diagnostic = "GPU timestamp result is awaiting its owning frame fence.";
    return true;
}

std::optional<GpuFrameTimingSample> GpuFrameTimer::CollectCompleted(const std::uint32_t frameSlot)
{
    if (!ValidateOperationalSlot(frameSlot, "CollectCompleted"))
    {
        return std::nullopt;
    }

    FrameSlotState& slot = slots_[frameSlot];
    if (!slot.submitted)
    {
        return std::nullopt;
    }

    std::array<TimestampQueryResult, 2u> results{};
    const VkResult queryResult = vkGetQueryPoolResults(
        device_,
        queryPool_,
        frameSlot * 2u,
        2u,
        sizeof(results),
        results.data(),
        sizeof(TimestampQueryResult),
        VK_QUERY_RESULT_64_BIT | VK_QUERY_RESULT_WITH_AVAILABILITY_BIT);
    telemetry_.lastVkResult = queryResult;
    const std::uint64_t submissionSequence = slot.submissionSequence;
    slot = {};

    if (queryResult != VK_SUCCESS && queryResult != VK_NOT_READY)
    {
        SetQueryError(queryResult, VkResultDiagnostic("vkGetQueryPoolResults", queryResult));
        return std::nullopt;
    }
    if (queryResult == VK_NOT_READY || results[0].available == 0u || results[1].available == 0u)
    {
        telemetry_.status = GpuFrameTimerStatus::ResultUnavailable;
        telemetry_.diagnostic = "A completed frame did not expose both GPU timestamp results.";
        ++telemetry_.unavailableResultCount;
        return std::nullopt;
    }

    const std::optional<GpuTimestampDuration> duration = ComputeGpuTimestampDuration(
        results[0].value,
        results[1].value,
        telemetry_.timestampValidBits,
        static_cast<double>(telemetry_.timestampPeriodNanoseconds));
    if (!duration.has_value())
    {
        SetQueryError(VK_ERROR_UNKNOWN, "GPU timestamp results could not be converted to a finite duration.");
        return std::nullopt;
    }

    telemetry_.status = GpuFrameTimerStatus::Available;
    telemetry_.latestMilliseconds = duration->milliseconds;
    telemetry_.diagnostic = "GPU RT command-buffer timing is available.";
    ++telemetry_.sampleCount;
    return GpuFrameTimingSample{
        duration->milliseconds,
        duration->elapsedTicks,
        submissionSequence,
        frameSlot};
}

} // namespace horde::vulkan
