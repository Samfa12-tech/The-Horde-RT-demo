#pragma once

#include "vulkan/DeviceCapabilities.h"
#include <cstdint>
#include <string>
#include <vector>
#include <vulkan/vulkan.h>

namespace horde::vulkan
{

class VulkanContext
{
public:
    VulkanContext() = default;
    ~VulkanContext();
    VulkanContext(const VulkanContext&) = delete;
    VulkanContext& operator=(const VulkanContext&) = delete;

    bool InitialiseForCapabilityProbe();

    DeviceCapabilities QueryDeviceCapabilities() const;
    void SetRtScenePresented(const bool presented);
    VkPhysicalDevice GetSelectedPhysicalDevice() const;
    const std::vector<std::string>& GetDiagnosticLog() const { return startupDiagnostics_; }

private:
    DeviceCapabilities ProbePhysicalDevice(const VkPhysicalDevice physicalDevice, const std::uint32_t deviceIndex) const;
    std::string VersionTextFromPacked(const std::uint32_t version) const;
    std::vector<std::string> startupDiagnostics_;
    DeviceCapabilities selectedCapabilities_;
    bool initialised_ = false;
    VkInstance instance_ = VK_NULL_HANDLE;
    VkPhysicalDevice selectedPhysicalDevice_ = VK_NULL_HANDLE;
};

} // namespace horde::vulkan
