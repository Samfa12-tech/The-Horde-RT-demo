#pragma once

#include <vulkan/vulkan.h>

#include <array>
#include <cstddef>
#include <cstdint>
#include <limits>
#include <memory>
#include <ostream>

namespace horde::vulkan
{
enum class PresentTimingBackend : std::uint32_t
{
    RayTracingPipeline = 0,
    RayQueryCompute = 1
};

struct PresentTimingFrameMetadata
{
    std::uint64_t measurementGeneration = 0u;
    std::uint64_t sceneEpoch = 0u;
    std::uint64_t recordSerial = 0u;
    std::uint64_t submissionSerial = 0u;
    std::uint64_t simulationTick = 0u;
    std::uint64_t queuedSteadyNs = 0u;
    std::uint32_t scalePercent = 0u;
    std::uint32_t width = 0u;
    std::uint32_t height = 0u;
    PresentTimingBackend backend = PresentTimingBackend::RayTracingPipeline;
};

// Bounded, owner-thread evidence collection for VK_GOOGLE_display_timing.
// It borrows the device/swapchain and dispatch function; it owns no Vulkan
// objects, waits for nothing, and performs no per-frame dynamic allocation.
// Constructing or leaving the collector disabled allocates no memory.
class PresentTimingEvidence
{
public:
    static constexpr std::size_t kMaximumRows = 32768u;
    static constexpr std::size_t kPendingCapacity = 256u;
    static constexpr std::size_t kTimingBatchCapacity = 32u;
    static constexpr std::size_t kMaximumCallsPerPoll = 2u;
    static constexpr std::size_t kSeenIdCapacity = 65536u;

    struct Row
    {
        std::uint64_t surfaceGeneration = 0u;
        std::uint64_t swapchainSerial = 0u;
        std::uint64_t measurementGeneration = 0u;
        std::uint64_t sceneEpoch = 0u;
        std::uint64_t recordSerial = 0u;
        std::uint64_t submissionSerial = 0u;
        std::uint64_t simulationTick = 0u;
        std::uint64_t queuedSteadyNs = 0u;
        std::uint64_t actualPresentTime = 0u;
        std::uint64_t earliestPresentTime = 0u;
        std::uint64_t presentMargin = 0u;
        std::uint32_t presentID = 0u;
        std::uint32_t scalePercent = 0u;
        std::uint32_t width = 0u;
        std::uint32_t height = 0u;
        PresentTimingBackend backend = PresentTimingBackend::RayTracingPipeline;
    };

    struct Counters
    {
        std::uint64_t preparedPresents = 0u;
        std::uint64_t acceptedPresents = 0u;
        std::uint64_t rejectedPresents = 0u;
        std::uint64_t invalidRegistrations = 0u;
        std::uint64_t invalidMetadata = 0u;
        std::uint64_t pendingCapacityExhausted = 0u;
        std::uint64_t queryCalls = 0u;
        std::uint64_t queryErrors = 0u;
        std::uint64_t incompleteQueries = 0u;
        std::uint64_t zeroPresentTimestamps = 0u;
        std::uint64_t zeroPresentIds = 0u;
        std::uint64_t unknownPresentIds = 0u;
        std::uint64_t duplicateTimings = 0u;
        std::uint64_t rowCapacityExhausted = 0u;
        std::uint64_t missingOnRebind = 0u;
        std::uint64_t missingOnUnbind = 0u;
        std::uint64_t abandonedPreparedPresents = 0u;
    };

    PresentTimingEvidence() = default;
    PresentTimingEvidence(const PresentTimingEvidence&) = delete;
    PresentTimingEvidence& operator=(const PresentTimingEvidence&) = delete;

    // Explicit opt-in. A null resolved PFN means unsupported and leaves the
    // collector allocation-free. Allocation happens here, never during a frame.
    bool Initialize(PFN_vkGetPastPresentationTimingGOOGLE getPastTiming) noexcept
    {
        if (initialized_ || getPastTiming == nullptr) return false;
        try
        {
            storage_ = std::make_unique<Storage>();
        }
        catch (...)
        {
            storage_.reset();
            return false;
        }
        getPastTiming_ = getPastTiming;
        initialized_ = true;
        return true;
    }

    // Bind after the app creates its swapchain. A rebind never waits: unresolved
    // accepted presents are counted as missing and discarded before the new
    // swapchain serial begins. IDs remain unique for this collector's lifetime.
    bool BindSwapchain(VkDevice device, VkSwapchainKHR swapchain,
                       std::uint64_t surfaceGeneration) noexcept
    {
        if (!initialized_ || device == VK_NULL_HANDLE || swapchain == VK_NULL_HANDLE ||
            surfaceGeneration == 0u || nextSwapchainSerial_ == 0u)
            return false;

        if (bound_) UnbindSwapchain(true);

        device_ = device;
        swapchain_ = swapchain;
        surfaceGeneration_ = surfaceGeneration;
        swapchainSerial_ = nextSwapchainSerial_++;
        bound_ = true;
        return true;
    }

    // Release borrowed handles before the application destroys its swapchain.
    // This never waits and preserves completed rows and the lifetime ID sequence.
    void UnbindSwapchain() noexcept { UnbindSwapchain(false); }

    // Call only for an admitted RT frame immediately before vkQueuePresentKHR.
    // The caller attaches this value to VkPresentTimesInfoGOOGLE. desired time
    // zero preserves the existing ASAP presentation behavior.
    bool PrepareNext(VkPresentTimeGOOGLE& presentTime) noexcept
    {
        if (!initialized_ || !bound_ || preparedPresentID_ != 0u || nextPresentID_ == 0u)
            return false;

        presentTime = {};
        presentTime.presentID = nextPresentID_;
        presentTime.desiredPresentTime = 0u;
        preparedPresentID_ = nextPresentID_;
        if (nextPresentID_ == std::numeric_limits<std::uint32_t>::max()) nextPresentID_ = 0u;
        else ++nextPresentID_;
        ++counters_.preparedPresents;
        return true;
    }

    // Register after the present call. Only VK_SUCCESS creates a pending row;
    // SUBOPTIMAL and every error remain rejected by this evidence policy.
    bool RegisterPresent(std::uint32_t presentID, VkResult result,
                         const PresentTimingFrameMetadata& metadata) noexcept
    {
        if (preparedPresentID_ == 0u || presentID != preparedPresentID_ || !bound_)
        {
            ++counters_.invalidRegistrations;
            return false;
        }
        preparedPresentID_ = 0u;
        if (result != VK_SUCCESS)
        {
            ++counters_.rejectedPresents;
            return false;
        }

        ++counters_.acceptedPresents;
        if (!ValidMetadata(metadata))
        {
            ++counters_.invalidMetadata;
            return false;
        }
        if (pendingCount_ == pending_.size())
        {
            ++counters_.pendingCapacityExhausted;
            return false;
        }

        Row row;
        CopyMetadata(row, metadata);
        row.surfaceGeneration = surfaceGeneration_;
        row.swapchainSerial = swapchainSerial_;
        row.presentID = presentID;
        pending_[pendingCount_++] = row;
        return true;
    }

    // Query only asynchronous history for this borrowed swapchain. A count
    // query and at most one fixed-batch fetch bound this invocation to two
    // driver calls. An incomplete fetch is reported for a later Poll.
    // Invoke on the same owner thread that serializes swapchain host access.
    VkResult Poll() noexcept
    {
        if (!initialized_) return VK_ERROR_EXTENSION_NOT_PRESENT;
        if (!bound_) return VK_ERROR_INITIALIZATION_FAILED;

        std::uint32_t available = 0u;
        VkResult result = Query(&available, nullptr);
        if (result != VK_SUCCESS)
        {
            ++counters_.queryErrors;
            lastQueryResult_ = result;
            return result;
        }
        lastQueryResult_ = VK_SUCCESS;
        if (available == 0u) return VK_SUCCESS;

        const auto request = static_cast<std::uint32_t>(
            available < kTimingBatchCapacity ? available : kTimingBatchCapacity);
        if (request == 0u) return VK_SUCCESS;

        std::array<VkPastPresentationTimingGOOGLE, kTimingBatchCapacity> timings{};
        std::uint32_t written = request;
        result = Query(&written, timings.data());
        if (result != VK_SUCCESS && result != VK_INCOMPLETE)
        {
            ++counters_.queryErrors;
            lastQueryResult_ = result;
            return result;
        }
        if (result == VK_INCOMPLETE) ++counters_.incompleteQueries;
        const auto safeWritten = written < kTimingBatchCapacity ? written :
            static_cast<std::uint32_t>(kTimingBatchCapacity);
        for (std::uint32_t index = 0u; index < safeWritten; ++index)
            AdmitTiming(timings[index]);

        lastQueryResult_ = result;
        return result;
    }

    std::size_t RowCount() const noexcept { return rowCount_; }
    const Row* ObservedRow(std::size_t index) const noexcept
    {
        return storage_ != nullptr && index < rowCount_ ? &storage_->rows[index] : nullptr;
    }
    const Counters& GetCounters() const noexcept { return counters_; }
    std::size_t PendingCount() const noexcept { return pendingCount_; }
    bool Enabled() const noexcept { return initialized_; }
    bool Bound() const noexcept { return bound_; }
    bool HasStorage() const noexcept { return storage_ != nullptr; }
    VkResult LastQueryResult() const noexcept { return lastQueryResult_; }

    // Writes raw evidence plus unresolved IDs as separate arrays. No FPS or
    // scanout claim is computed from these image-presentation timestamps.
    void WriteJson(std::ostream& output) const
    {
        output << "{\"schemaVersion\":1,\"status\":{\"enabled\":"
               << (initialized_ ? "true" : "false") << ",\"bound\":"
               << (bound_ ? "true" : "false") << ",\"surfaceGeneration\":"
               << surfaceGeneration_ << ",\"swapchainSerial\":" << swapchainSerial_
               << ",\"pendingCount\":" << pendingCount_ << ",\"lastQueryResult\":"
               << static_cast<std::int32_t>(lastQueryResult_) << "},\"allocatedStorageBytes\":" << (storage_ ? sizeof(Storage) : 0u)
               << ",\"metric\":\"actual image presentation timestamp ns from VK_GOOGLE_display_timing; not CPU/GPU reciprocal, scanout, or photons\",\"counters\":{"
               << "\"preparedPresents\":" << counters_.preparedPresents
               << ",\"acceptedPresents\":" << counters_.acceptedPresents
               << ",\"rejectedPresents\":" << counters_.rejectedPresents
               << ",\"invalidRegistrations\":" << counters_.invalidRegistrations
               << ",\"invalidMetadata\":" << counters_.invalidMetadata
               << ",\"pendingCapacityExhausted\":" << counters_.pendingCapacityExhausted
               << ",\"queryCalls\":" << counters_.queryCalls
               << ",\"queryErrors\":" << counters_.queryErrors
               << ",\"incompleteQueries\":" << counters_.incompleteQueries
               << ",\"zeroPresentTimestamps\":" << counters_.zeroPresentTimestamps
               << ",\"zeroPresentIds\":" << counters_.zeroPresentIds
               << ",\"unknownPresentIds\":" << counters_.unknownPresentIds
               << ",\"duplicateTimings\":" << counters_.duplicateTimings
               << ",\"rowCapacityExhausted\":" << counters_.rowCapacityExhausted
               << ",\"missingOnRebind\":" << counters_.missingOnRebind
               << ",\"missingOnUnbind\":" << counters_.missingOnUnbind
               << ",\"abandonedPreparedPresents\":" << counters_.abandonedPreparedPresents
               << "},\"rows\":[";
        for (std::size_t index = 0u; index < rowCount_; ++index)
        {
            if (index != 0u) output << ',';
            WriteRow(output, storage_->rows[index]);
        }
        output << "],\"unresolved\":[";
        for (std::size_t index = 0u; index < pendingCount_; ++index)
        {
            if (index != 0u) output << ',';
            WriteRow(output, pending_[index]);
        }
        output << "]}";
    }

private:
    struct Storage
    {
        std::array<Row, kMaximumRows> rows{};
        std::array<std::uint32_t, kSeenIdCapacity> seenIds{};
    };

    VkResult Query(std::uint32_t* count, VkPastPresentationTimingGOOGLE* timings) noexcept
    {
        ++counters_.queryCalls;
        return getPastTiming_(device_, swapchain_, count, timings);
    }

    void UnbindSwapchain(bool forRebind) noexcept
    {
        if (!bound_) return;
        if (forRebind) counters_.missingOnRebind += pendingCount_;
        else counters_.missingOnUnbind += pendingCount_;
        pendingCount_ = 0u;
        if (preparedPresentID_ != 0u)
        {
            ++counters_.abandonedPreparedPresents;
            preparedPresentID_ = 0u;
        }
        device_ = VK_NULL_HANDLE;
        swapchain_ = VK_NULL_HANDLE;
        bound_ = false;
    }

    static bool ValidMetadata(const PresentTimingFrameMetadata& metadata) noexcept
    {
        return metadata.measurementGeneration != 0u && metadata.sceneEpoch != 0u &&
               metadata.recordSerial != 0u &&
               metadata.submissionSerial != 0u && metadata.scalePercent > 0u &&
               metadata.scalePercent <= 100u && metadata.width > 0u && metadata.height > 0u &&
               (metadata.backend == PresentTimingBackend::RayTracingPipeline ||
                metadata.backend == PresentTimingBackend::RayQueryCompute);
    }

    static void CopyMetadata(Row& row, const PresentTimingFrameMetadata& metadata) noexcept
    {
        row.measurementGeneration = metadata.measurementGeneration;
        row.sceneEpoch = metadata.sceneEpoch;
        row.recordSerial = metadata.recordSerial;
        row.submissionSerial = metadata.submissionSerial;
        row.simulationTick = metadata.simulationTick;
        row.queuedSteadyNs = metadata.queuedSteadyNs;
        row.scalePercent = metadata.scalePercent;
        row.width = metadata.width;
        row.height = metadata.height;
        row.backend = metadata.backend;
    }

    std::size_t SeenSlot(std::uint32_t presentID) const noexcept
    {
        std::size_t index = (static_cast<std::uint64_t>(presentID) * 2654435761ull) % kSeenIdCapacity;
        for (std::size_t probe = 0u; probe < kSeenIdCapacity; ++probe)
        {
            const auto value = storage_->seenIds[index];
            if (value == 0u || value == presentID) return index;
            index = (index + 1u) % kSeenIdCapacity;
        }
        return kSeenIdCapacity;
    }

    bool RememberSeen(std::uint32_t presentID) noexcept
    {
        const auto slot = SeenSlot(presentID);
        if (slot == kSeenIdCapacity) return false;
        if (storage_->seenIds[slot] == presentID) return false;
        storage_->seenIds[slot] = presentID;
        return true;
    }

    void AdmitTiming(const VkPastPresentationTimingGOOGLE& timing) noexcept
    {
        if (timing.presentID == 0u)
        {
            ++counters_.zeroPresentIds;
            return;
        }
        const auto seenSlot = SeenSlot(timing.presentID);
        if (seenSlot < kSeenIdCapacity && storage_->seenIds[seenSlot] == timing.presentID)
        {
            ++counters_.duplicateTimings;
            return;
        }
        if (timing.actualPresentTime == 0u)
        {
            ++counters_.zeroPresentTimestamps;
            return;
        }

        std::size_t pendingIndex = pendingCount_;
        for (std::size_t index = 0u; index < pendingCount_; ++index)
            if (pending_[index].presentID == timing.presentID)
            {
                pendingIndex = index;
                break;
            }
        if (pendingIndex == pendingCount_)
        {
            ++counters_.unknownPresentIds;
            return;
        }

        Row row = pending_[pendingIndex];
        pending_[pendingIndex] = pending_[pendingCount_ - 1u];
        --pendingCount_;
        row.actualPresentTime = timing.actualPresentTime;
        row.earliestPresentTime = timing.earliestPresentTime;
        row.presentMargin = timing.presentMargin;
        if (rowCount_ == kMaximumRows)
        {
            ++counters_.rowCapacityExhausted;
            RememberSeen(timing.presentID);
            return;
        }
        if (!RememberSeen(timing.presentID))
        {
            ++counters_.duplicateTimings;
            return;
        }
        storage_->rows[rowCount_++] = row;
    }

    static const char* BackendName(PresentTimingBackend backend) noexcept
    {
        return backend == PresentTimingBackend::RayQueryCompute ? "RayQueryCompute" : "RayTracingPipeline";
    }

    static void WriteRow(std::ostream& output, const Row& row)
    {
        output << "{\"surfaceGeneration\":" << row.surfaceGeneration
               << ",\"swapchainSerial\":" << row.swapchainSerial
               << ",\"measurementGeneration\":" << row.measurementGeneration
               << ",\"sceneEpoch\":" << row.sceneEpoch
               << ",\"recordSerial\":" << row.recordSerial
               << ",\"submissionSerial\":" << row.submissionSerial
               << ",\"simulationTick\":" << row.simulationTick
               << ",\"queuedSteadyNs\":" << row.queuedSteadyNs
               << ",\"presentID\":" << row.presentID
               << ",\"actualPresentTime\":" << row.actualPresentTime
               << ",\"earliestPresentTime\":" << row.earliestPresentTime
               << ",\"presentMargin\":" << row.presentMargin
               << ",\"scalePercent\":" << row.scalePercent
               << ",\"width\":" << row.width << ",\"height\":" << row.height
               << ",\"backend\":\"" << BackendName(row.backend) << "\"}";
    }

    std::unique_ptr<Storage> storage_;
    PFN_vkGetPastPresentationTimingGOOGLE getPastTiming_ = nullptr;
    VkDevice device_ = VK_NULL_HANDLE;
    VkSwapchainKHR swapchain_ = VK_NULL_HANDLE;
    std::array<Row, kPendingCapacity> pending_{};
    std::size_t pendingCount_ = 0u;
    std::size_t rowCount_ = 0u;
    std::uint64_t surfaceGeneration_ = 0u;
    std::uint64_t swapchainSerial_ = 0u;
    std::uint64_t nextSwapchainSerial_ = 1u;
    std::uint32_t nextPresentID_ = 1u;
    std::uint32_t preparedPresentID_ = 0u;
    VkResult lastQueryResult_ = VK_SUCCESS;
    Counters counters_{};
    bool initialized_ = false;
    bool bound_ = false;
};
} // namespace horde::vulkan
