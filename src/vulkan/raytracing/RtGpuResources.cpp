#include "vulkan/raytracing/RtGpuResources.h"
#include "vulkan/raytracing/RtDeviceAddressLayout.h"
#include "vulkan/raytracing/RtSceneRecordObservation.h"

#include <array>
#include <algorithm>
#include <cstring>
#include <utility>

namespace horde::vulkan::raytracing
{

RtGpuBufferPreparation::~RtGpuBufferPreparation() { Cancel(); }
RtGpuBufferPreparation::RtGpuBufferPreparation(RtGpuBufferPreparation&& other) noexcept
    : resources_(other.resources_) { *this = std::move(other); }
RtGpuBufferPreparation& RtGpuBufferPreparation::operator=(RtGpuBufferPreparation&& other) noexcept
{
    if (this == &other) return *this;
    Cancel(); resources_ = other.resources_;
    uploads_ = std::exchange(other.uploads_, {});
    buffers_ = std::exchange(other.buffers_, {});
    state_ = std::exchange(other.state_, State::Idle);
    count_ = std::exchange(other.count_, 0u); row_ = std::exchange(other.row_, 0u);
    offset_ = std::exchange(other.offset_, 0u);
    uploadedBytes_ = std::exchange(other.uploadedBytes_, 0u);
    lastCopyBytes_ = std::exchange(other.lastCopyBytes_, 0u);
    return *this;
}
void RtGpuBufferPreparation::Cancel() noexcept
{
    for (auto& buffer : buffers_) resources_.DestroyBuffer(buffer);
    uploads_ = {}; state_ = State::Idle; count_ = row_ = 0u;
    offset_ = uploadedBytes_ = lastCopyBytes_ = 0u;
}
void RtGpuBufferPreparation::AccumulateResourceInventory(
    horde::telemetry::RtResourceInventory& inventory) const noexcept
{
    for (const auto& buffer : buffers_) AccumulateRtGpuBuffer(inventory, buffer);
}
bool RtGpuBufferPreparation::Begin(const std::span<const RtGpuBufferUpload> uploads,
    std::string& diagnostic)
{
    if (state_ == State::Preparing || state_ == State::Ready ||
        resources_.device_ == VK_NULL_HANDLE || resources_.physicalDevice_ == VK_NULL_HANDLE ||
        uploads.empty() || uploads.size() > uploads_.size())
    {
        diagnostic = "Invalid Vulkan context, batch capacity or already-owned preparation.";
        return false;
    }
    VkDeviceSize total = 0u;
    for (const auto& upload : uploads)
    {
        if (!upload.data || upload.size == 0u || upload.size > std::numeric_limits<std::size_t>::max() ||
            upload.usage == 0u || !upload.label || upload.label[0] == '\0' ||
            upload.size > std::numeric_limits<VkDeviceSize>::max() - total)
        {
            diagnostic = "GPU preparation needs valid bounded payloads and labels.";
            return false;
        }
        total += upload.size;
    }
    Cancel();
    std::copy(uploads.begin(), uploads.end(), uploads_.begin());
    count_ = uploads.size(); state_ = State::Preparing;
    diagnostic.clear(); return true;
}
bool RtGpuBufferPreparation::Advance(const VkDeviceSize maxCopyBytes, std::string& diagnostic)
{
    lastCopyBytes_ = 0u;
    if (state_ != State::Preparing || maxCopyBytes == 0u)
    {
        diagnostic = "GPU preparation advance needs an active owner and positive copy budget.";
        return false;
    }
    const auto fail = [&] { Cancel(); state_ = State::Failed; return false; };
    const auto& upload = uploads_[row_];
    auto& buffer = buffers_[row_];
    if (buffer.Empty())
    {
        constexpr auto flags = VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT;
        if (!resources_.CreateBuffer(upload.size, upload.usage, flags, upload.deviceAddress,
                buffer, diagnostic, upload.preferredMemoryFlags)) return fail();
        if (upload.deviceAddress && buffer.address == 0u)
        {
            diagnostic = std::string("Zero device address for ") + upload.label + ".";
            return fail();
        }
        if (!resources_.MapBufferForHostWrites(buffer, diagnostic)) return fail();
    }
    const auto bytes = std::min(maxCopyBytes, upload.size - offset_);
    if (!resources_.WriteBufferRange(buffer, offset_,
            static_cast<const std::byte*>(upload.data) + static_cast<std::size_t>(offset_),
            bytes, upload.label, diagnostic)) return fail();
    offset_ += bytes; uploadedBytes_ += bytes; lastCopyBytes_ = bytes;
    if (offset_ == upload.size)
    {
        if (!upload.retainMapping)
        {
            vkUnmapMemory(resources_.device_, buffer.memory);
            buffer.mappedWriteData = nullptr;
        }
        offset_ = 0u;
        if (++row_ == count_) state_ = State::Ready;
    }
    diagnostic.clear(); return true;
}
bool RtGpuBufferPreparation::Take(const std::span<RtGpuBuffer> destination, std::string& diagnostic)
{
    if (state_ != State::Ready || destination.size() != count_ ||
        std::any_of(destination.begin(), destination.end(), [](const auto& buffer) { return !buffer.Empty(); }))
    {
        diagnostic = "GPU preparation take needs a complete candidate and exact empty destination.";
        return false;
    }
    for (std::size_t i = 0u; i < count_; ++i)
        destination[i] = std::exchange(buffers_[i], {});
    uploads_ = {}; state_ = State::Idle;
    diagnostic.clear(); return true;
}

bool RtGpuResources::PrepareBufferUploads(
    const std::span<const RtGpuBufferUpload> uploads, const std::span<RtGpuBuffer> destination,
    std::string& diagnostic) const
{
    if (device_ == VK_NULL_HANDLE || physicalDevice_ == VK_NULL_HANDLE ||
        uploads.empty() || uploads.size() > kBufferUploadBatchCapacity ||
        uploads.size() != destination.size())
    {
        diagnostic = "Invalid Vulkan context or bounded GPU upload batch.";
        return false;
    }
    // Validate the entire destination and payload before any Vulkan call. An
    // admitted buffer (including stray allocation facts) is never overwritten.
    for (std::size_t i = 0u; i < uploads.size(); ++i)
    {
        const auto& upload = uploads[i];
        const auto& buffer = destination[i];
        if (upload.data == nullptr || upload.size == 0u ||
            upload.size > std::numeric_limits<std::size_t>::max() ||
            upload.usage == 0u || upload.label == nullptr || upload.label[0] == '\0' ||
            !buffer.Empty())
        {
            diagnostic = "GPU upload batch needs valid payloads and empty candidate destinations.";
            return false;
        }
    }

    std::array<RtGpuBuffer, kBufferUploadBatchCapacity> candidate{};
    const auto rollback = [this, &candidate, &uploads]() {
        for (std::size_t i = 0u; i < uploads.size(); ++i) DestroyBuffer(candidate[i]);
    };
    constexpr VkMemoryPropertyFlags uploadMemory =
        VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT;
    for (std::size_t i = 0u; i < uploads.size(); ++i)
    {
        const auto& upload = uploads[i];
        auto& buffer = candidate[i];
        if (!CreateBuffer(upload.size, upload.usage, uploadMemory, upload.deviceAddress,
                          buffer, diagnostic, upload.preferredMemoryFlags))
        {
            rollback();
            return false;
        }
        if (upload.deviceAddress && buffer.address == 0u)
        {
            diagnostic = std::string("Zero device address for ") + upload.label + ".";
            rollback();
            return false;
        }
        // One map/copy per row. Keep mappings only for the dynamic resources;
        // the same cleanup path owns success-null-map and ordinary failures.
        if (!MapBufferForHostWrites(buffer, diagnostic) ||
            !WriteBuffer(buffer, upload.data, upload.size, upload.label, diagnostic))
        {
            rollback();
            return false;
        }
        if (!upload.retainMapping)
        {
            vkUnmapMemory(device_, buffer.memory);
            buffer.mappedWriteData = nullptr;
        }
    }
    for (std::size_t i = 0u; i < uploads.size(); ++i)
        destination[i] = std::exchange(candidate[i], RtGpuBuffer{});
    diagnostic.clear();
    return true;
}

void RtGpuResources::Bind(const VkPhysicalDevice physicalDevice,
                          const VkDevice device,
                          const PFN_vkDestroyAccelerationStructureKHR destroyAccelerationStructure,
                          const PFN_vkGetBufferDeviceAddressKHR getBufferDeviceAddress)
{
    physicalDevice_ = physicalDevice;
    device_ = device;
    destroyAccelerationStructure_ = destroyAccelerationStructure;
    getBufferDeviceAddress_ = getBufferDeviceAddress;
}

void RtGpuResources::Reset()
{
    physicalDevice_ = VK_NULL_HANDLE;
    device_ = VK_NULL_HANDLE;
    destroyAccelerationStructure_ = nullptr;
    getBufferDeviceAddress_ = nullptr;
}

std::uint32_t RtGpuResources::FindMemoryType(const std::uint32_t typeBits,
                                             const VkMemoryPropertyFlags flags,
                                             VkMemoryPropertyFlags* selectedFlags) const
{
    if (selectedFlags != nullptr)
    {
        *selectedFlags = 0u;
    }
    VkPhysicalDeviceMemoryProperties memoryProperties{};
    vkGetPhysicalDeviceMemoryProperties(physicalDevice_, &memoryProperties);
    for (std::uint32_t i = 0; i < memoryProperties.memoryTypeCount; ++i)
    {
        if ((typeBits & (1u << i)) != 0u &&
            (memoryProperties.memoryTypes[i].propertyFlags & flags) == flags)
        {
            if (selectedFlags != nullptr)
            {
                *selectedFlags = memoryProperties.memoryTypes[i].propertyFlags;
            }
            return i;
        }
    }
    return UINT32_MAX;
}

bool RtGpuResources::CreateBuffer(VkDeviceSize size,
                                  VkBufferUsageFlags usage,
                                  const VkMemoryPropertyFlags memoryFlags,
                                  const bool deviceAddress,
                                  RtGpuBuffer& out,
                                  std::string& diagnostic,
                                  const VkMemoryPropertyFlags preferredMemoryFlags) const
{
    out = {};
    if (physicalDevice_ == VK_NULL_HANDLE || device_ == VK_NULL_HANDLE || size == 0u)
    {
        diagnostic = "Invalid Vulkan resource context or zero-sized RT buffer.";
        return false;
    }
    if (deviceAddress)
    {
        usage |= VK_BUFFER_USAGE_SHADER_DEVICE_ADDRESS_BIT;
    }

    const VkBufferCreateInfo bufferInfo{
        VK_STRUCTURE_TYPE_BUFFER_CREATE_INFO,
        nullptr,
        0u,
        size,
        usage,
        VK_SHARING_MODE_EXCLUSIVE,
        0u,
        nullptr};
    if (vkCreateBuffer(device_, &bufferInfo, nullptr, &out.buffer) != VK_SUCCESS)
    {
        diagnostic = "Failed to create RT buffer.";
        return false;
    }

    VkMemoryRequirements requirements{};
    vkGetBufferMemoryRequirements(device_, out.buffer, &requirements);
    VkMemoryPropertyFlags selectedMemoryFlags = 0u;
    // Relax placement only when a compatible preferred type is unavailable.
    // Allocation/binding failures never retry on another heap.
    std::uint32_t memoryType = FindMemoryType(
        requirements.memoryTypeBits, memoryFlags | preferredMemoryFlags, &selectedMemoryFlags);
    if (memoryType == UINT32_MAX && preferredMemoryFlags != 0u)
    {
        memoryType = FindMemoryType(requirements.memoryTypeBits, memoryFlags, &selectedMemoryFlags);
    }
    if (memoryType == UINT32_MAX)
    {
        diagnostic = "No compatible memory type for RT buffer.";
        DestroyBuffer(out);
        return false;
    }

    VkMemoryAllocateFlagsInfo flagsInfo{VK_STRUCTURE_TYPE_MEMORY_ALLOCATE_FLAGS_INFO};
    flagsInfo.flags = VK_MEMORY_ALLOCATE_DEVICE_ADDRESS_BIT;
    const VkMemoryAllocateInfo allocateInfo{
        VK_STRUCTURE_TYPE_MEMORY_ALLOCATE_INFO,
        deviceAddress ? &flagsInfo : nullptr,
        requirements.size,
        memoryType};
    if (vkAllocateMemory(device_, &allocateInfo, nullptr, &out.memory) != VK_SUCCESS ||
        vkBindBufferMemory(device_, out.buffer, out.memory, 0u) != VK_SUCCESS)
    {
        diagnostic = "Failed to allocate RT buffer memory.";
        DestroyBuffer(out);
        return false;
    }

    out.size = size;
    out.allocationSize = requirements.size;
    out.memoryPropertyFlags = selectedMemoryFlags;
    out.address = deviceAddress ? BufferAddress(out.buffer) : 0u;
    diagnostic.clear();
    return true;
}

bool RtGpuResources::CreateAlignedBuffer(const VkDeviceSize usableSize,
                                         const VkDeviceSize addressAlignment,
                                         const VkBufferUsageFlags usage,
                                         const VkMemoryPropertyFlags memoryFlags,
                                         RtGpuBuffer& out,
                                         std::string& diagnostic) const
{
    out = {};
    std::uint64_t paddedSize = 0u;
    if (!TryPaddedDeviceAddressSize(usableSize, addressAlignment, paddedSize))
    {
        diagnostic = "Invalid or overflowing aligned RT buffer size/alignment.";
        return false;
    }
    if (!CreateBuffer(paddedSize, usage, memoryFlags, true, out, diagnostic))
        return false;
    std::uint64_t offset = 0u;
    if (!TryAlignDeviceAddressRange(out.address, out.size, usableSize,
                                    addressAlignment, offset))
    {
        diagnostic = "RT device address is zero, overflowing or outside the aligned buffer range.";
        DestroyBuffer(out);
        return false;
    }
    out.deviceAddressOffset = offset;
    diagnostic.clear();
    return true;
}

bool RtGpuResources::MapBufferForHostWrites(RtGpuBuffer& buffer,
                                           std::string& diagnostic) const
{
    constexpr VkMemoryPropertyFlags required =
        VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT;
    if (device_ == VK_NULL_HANDLE || buffer.memory == VK_NULL_HANDLE ||
        buffer.buffer == VK_NULL_HANDLE || buffer.size == 0u ||
        buffer.size > std::numeric_limits<std::size_t>::max() ||
        (buffer.memoryPropertyFlags & required) != required)
    {
        diagnostic = "Persistent RT uploads require a valid host-visible coherent buffer.";
        return false;
    }
    if (buffer.mappedWriteData != nullptr)
    {
        diagnostic.clear();
        return true;
    }
    void* mapped = nullptr;
    const VkResult result = vkMapMemory(device_, buffer.memory, 0u, buffer.size, 0u, &mapped);
    if (result != VK_SUCCESS || mapped == nullptr)
    {
        if (result == VK_SUCCESS) vkUnmapMemory(device_, buffer.memory);
        diagnostic = "Failed to persistently map RT upload memory.";
        return false;
    }
    buffer.mappedWriteData = mapped;
    diagnostic.clear();
    return true;
}

bool RtGpuResources::WriteBuffer(const RtGpuBuffer& buffer,
                                 const void* data,
                                 const VkDeviceSize size,
                                 const char* label,
                                 std::string& diagnostic,
                                 RtSceneRecordObservation* observation) const
{
    return WriteBufferRange(buffer, 0u, data, size, label, diagnostic, observation);
}

bool RtGpuResources::WriteBufferRange(const RtGpuBuffer& buffer,
                                      const VkDeviceSize offset,
                                      const void* data,
                                      const VkDeviceSize size,
                                      const char* label,
                                      std::string& diagnostic,
                                      RtSceneRecordObservation* observation) const
{
    if (buffer.memory == VK_NULL_HANDLE || data == nullptr || size == 0u ||
        offset > buffer.size || size > buffer.size - offset ||
        size > std::numeric_limits<std::size_t>::max() ||
        offset > std::numeric_limits<std::size_t>::max())
    {
        diagnostic = std::string("Invalid ") + label + " upload.";
        return false;
    }

    RtSceneStageScope uploadScope(observation, horde::telemetry::RtStage::DynamicUpload);
    if (buffer.mappedWriteData != nullptr)
    {
        std::memcpy(static_cast<std::uint8_t*>(buffer.mappedWriteData) +
                        static_cast<std::size_t>(offset),
                    data, static_cast<std::size_t>(size));
        // Still one copy/upload operation. There is no per-write map/unmap.
        uploadScope.Complete(1u, size, 1u);
        diagnostic.clear();
        return true;
    }
    void* mapped = nullptr;
    if (vkMapMemory(device_, buffer.memory, offset, size, 0u, &mapped) != VK_SUCCESS || mapped == nullptr)
    {
        uploadScope.Cancel();
        diagnostic = std::string("Failed to map ") + label + " memory.";
        return false;
    }
    std::memcpy(mapped, data, static_cast<std::size_t>(size));
    vkUnmapMemory(device_, buffer.memory);
    uploadScope.Complete(1u, size, 1u);
    diagnostic.clear();
    return true;
}

void RtGpuResources::DestroyBuffer(RtGpuBuffer& buffer) const
{
    if (device_ != VK_NULL_HANDLE)
    {
        if (buffer.mappedWriteData != nullptr && buffer.memory != VK_NULL_HANDLE)
        {
            vkUnmapMemory(device_, buffer.memory);
        }
        if (buffer.buffer != VK_NULL_HANDLE)
        {
            vkDestroyBuffer(device_, buffer.buffer, nullptr);
        }
        if (buffer.memory != VK_NULL_HANDLE)
        {
            vkFreeMemory(device_, buffer.memory, nullptr);
        }
    }
    buffer = {};
}

void RtGpuResources::DestroyAccelerationStructure(RtAccelerationStructure& accelerationStructure) const
{
    if (device_ != VK_NULL_HANDLE && accelerationStructure.handle != VK_NULL_HANDLE &&
        destroyAccelerationStructure_ != nullptr)
    {
        destroyAccelerationStructure_(device_, accelerationStructure.handle, nullptr);
    }
    DestroyBuffer(accelerationStructure.backing);
    accelerationStructure = {};
}

VkDeviceAddress RtGpuResources::BufferAddress(const VkBuffer buffer) const
{
    if (getBufferDeviceAddress_ == nullptr)
    {
        return 0u;
    }
    VkBufferDeviceAddressInfo addressInfo{VK_STRUCTURE_TYPE_BUFFER_DEVICE_ADDRESS_INFO};
    addressInfo.buffer = buffer;
    return getBufferDeviceAddress_(device_, &addressInfo);
}

} // namespace horde::vulkan::raytracing
