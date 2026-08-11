#include "vulkan/raytracing/RtGpuResources.h"

#include <cstring>

namespace horde::vulkan::raytracing
{

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
                                             const VkMemoryPropertyFlags flags) const
{
    VkPhysicalDeviceMemoryProperties memoryProperties{};
    vkGetPhysicalDeviceMemoryProperties(physicalDevice_, &memoryProperties);
    for (std::uint32_t i = 0; i < memoryProperties.memoryTypeCount; ++i)
    {
        if ((typeBits & (1u << i)) != 0u &&
            (memoryProperties.memoryTypes[i].propertyFlags & flags) == flags)
        {
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
                                  std::string& diagnostic) const
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
    const std::uint32_t memoryType = FindMemoryType(requirements.memoryTypeBits, memoryFlags);
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
    out.address = deviceAddress ? BufferAddress(out.buffer) : 0u;
    diagnostic.clear();
    return true;
}

bool RtGpuResources::WriteBuffer(const RtGpuBuffer& buffer,
                                 const void* data,
                                 const VkDeviceSize size,
                                 const char* label,
                                 std::string& diagnostic) const
{
    if (buffer.memory == VK_NULL_HANDLE || data == nullptr || size == 0u || size > buffer.size)
    {
        diagnostic = std::string("Invalid ") + label + " upload.";
        return false;
    }

    void* mapped = nullptr;
    if (vkMapMemory(device_, buffer.memory, 0u, size, 0u, &mapped) != VK_SUCCESS || mapped == nullptr)
    {
        diagnostic = std::string("Failed to map ") + label + " memory.";
        return false;
    }
    std::memcpy(mapped, data, static_cast<std::size_t>(size));
    vkUnmapMemory(device_, buffer.memory);
    return true;
}

void RtGpuResources::DestroyBuffer(RtGpuBuffer& buffer) const
{
    if (device_ != VK_NULL_HANDLE)
    {
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
