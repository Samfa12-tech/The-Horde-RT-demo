#pragma once

#include "telemetry/RtPerformanceEvidence.h"

#include <cstdint>
#include <limits>
#include <string>

#include <vulkan/vulkan.h>

namespace horde::vulkan::raytracing
{

struct RtSceneRecordObservation;

struct RtGpuBuffer
{
    VkBuffer buffer = VK_NULL_HANDLE;
    VkDeviceMemory memory = VK_NULL_HANDLE;
    VkDeviceAddress address = 0;
    VkDeviceSize size = 0;
    VkDeviceSize allocationSize = 0;
    VkMemoryPropertyFlags memoryPropertyFlags = 0u;
};

struct RtAccelerationStructure
{
    RtGpuBuffer backing;
    VkAccelerationStructureKHR handle = VK_NULL_HANDLE;
    VkDeviceAddress address = 0;
};

struct RtUpdatableTriangleBlas
{
    RtGpuBuffer vertices;
    RtAccelerationStructure accelerationStructure;
    RtGpuBuffer updateScratch;
    VkDeviceSize vertexStride = 0u;
    std::uint32_t vertexCount = 0u;
};

inline void AccumulateRtResourceCount(std::uint32_t& value) noexcept
{
    if (value != std::numeric_limits<std::uint32_t>::max())
    {
        ++value;
    }
}

inline void AccumulateRtAllocationBytes(std::uint64_t& value,
                                        const VkDeviceSize allocationSize) noexcept
{
    const std::uint64_t size = static_cast<std::uint64_t>(allocationSize);
    if (size > std::numeric_limits<std::uint64_t>::max() - value)
    {
        value = std::numeric_limits<std::uint64_t>::max();
        return;
    }
    value += size;
}

inline void AccumulateRtMemoryAllocation(
    horde::telemetry::RtResourceInventory& inventory,
    const VkDeviceMemory memory,
    const VkDeviceSize allocationSize,
    const VkMemoryPropertyFlags memoryPropertyFlags) noexcept
{
    if (memory == VK_NULL_HANDLE)
    {
        return;
    }
    AccumulateRtResourceCount(inventory.memoryAllocationCount);
    if ((memoryPropertyFlags & VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT) != 0u)
    {
        AccumulateRtAllocationBytes(inventory.hostVisibleBytes, allocationSize);
    }
    if ((memoryPropertyFlags & VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT) != 0u)
    {
        AccumulateRtAllocationBytes(inventory.deviceLocalBytes, allocationSize);
    }
}

inline void AccumulateRtGpuBuffer(
    horde::telemetry::RtResourceInventory& inventory,
    const RtGpuBuffer& buffer) noexcept
{
    if (buffer.buffer != VK_NULL_HANDLE)
    {
        AccumulateRtResourceCount(inventory.bufferCount);
    }
    AccumulateRtMemoryAllocation(
        inventory, buffer.memory, buffer.allocationSize, buffer.memoryPropertyFlags);
}

// Non-owning Vulkan device seam used by scene resource owners. Resource
// lifetime remains explicit because the platform owns the VkDevice.
class RtGpuResources
{
public:
    void Bind(VkPhysicalDevice physicalDevice,
              VkDevice device,
              PFN_vkDestroyAccelerationStructureKHR destroyAccelerationStructure,
              PFN_vkGetBufferDeviceAddressKHR getBufferDeviceAddress);
    void Reset();

    bool CreateBuffer(VkDeviceSize size,
                      VkBufferUsageFlags usage,
                      VkMemoryPropertyFlags memoryFlags,
                      bool deviceAddress,
                      RtGpuBuffer& out,
                      std::string& diagnostic) const;
    bool WriteBuffer(const RtGpuBuffer& buffer,
                     const void* data,
                     VkDeviceSize size,
                     const char* label,
                     std::string& diagnostic,
                     RtSceneRecordObservation* observation = nullptr) const;
    bool WriteBufferRange(const RtGpuBuffer& buffer,
                          VkDeviceSize offset,
                          const void* data,
                          VkDeviceSize size,
                          const char* label,
                          std::string& diagnostic,
                          RtSceneRecordObservation* observation = nullptr) const;
    void DestroyBuffer(RtGpuBuffer& buffer) const;
    void DestroyAccelerationStructure(RtAccelerationStructure& accelerationStructure) const;
    VkDeviceAddress BufferAddress(VkBuffer buffer) const;
    std::uint32_t FindMemoryType(
        std::uint32_t typeBits,
        VkMemoryPropertyFlags flags,
        VkMemoryPropertyFlags* selectedFlags = nullptr) const;

private:
    VkPhysicalDevice physicalDevice_ = VK_NULL_HANDLE;
    VkDevice device_ = VK_NULL_HANDLE;
    PFN_vkDestroyAccelerationStructureKHR destroyAccelerationStructure_ = nullptr;
    PFN_vkGetBufferDeviceAddressKHR getBufferDeviceAddress_ = nullptr;
};

} // namespace horde::vulkan::raytracing
