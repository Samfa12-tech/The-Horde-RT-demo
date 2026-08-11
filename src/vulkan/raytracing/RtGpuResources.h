#pragma once

#include <cstdint>
#include <string>

#include <vulkan/vulkan.h>

namespace horde::vulkan::raytracing
{

struct RtGpuBuffer
{
    VkBuffer buffer = VK_NULL_HANDLE;
    VkDeviceMemory memory = VK_NULL_HANDLE;
    VkDeviceAddress address = 0;
    VkDeviceSize size = 0;
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

    std::uint32_t PrimitiveCount() const { return vertexCount / 3u; }
};

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
                     std::string& diagnostic) const;
    void DestroyBuffer(RtGpuBuffer& buffer) const;
    void DestroyAccelerationStructure(RtAccelerationStructure& accelerationStructure) const;
    VkDeviceAddress BufferAddress(VkBuffer buffer) const;
    std::uint32_t FindMemoryType(std::uint32_t typeBits, VkMemoryPropertyFlags flags) const;

private:
    VkPhysicalDevice physicalDevice_ = VK_NULL_HANDLE;
    VkDevice device_ = VK_NULL_HANDLE;
    PFN_vkDestroyAccelerationStructureKHR destroyAccelerationStructure_ = nullptr;
    PFN_vkGetBufferDeviceAddressKHR getBufferDeviceAddress_ = nullptr;
};

} // namespace horde::vulkan::raytracing
