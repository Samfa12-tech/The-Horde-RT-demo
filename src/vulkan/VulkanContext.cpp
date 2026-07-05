#include "vulkan/VulkanContext.h"

namespace horde::vulkan
{

bool VulkanContext::InitialiseForCapabilityProbe()
{
    // Not implemented yet. Returning false is intentional: this scaffold must
    // not pretend that Vulkan or RT support has been queried.
    return false;
}

DeviceCapabilities VulkanContext::QueryDeviceCapabilities() const
{
    DeviceCapabilities capabilities;
    capabilities.rtMode = RtMode::Unsupported;
    capabilities.diagnostics.push_back("Scaffold only: real Vulkan physical-device and RT feature probing is not implemented yet.");
    capabilities.diagnostics.push_back("Next task: enumerate Vulkan devices and query RT extensions/features on Windows and Android.");
    return capabilities;
}

} // namespace horde::vulkan
