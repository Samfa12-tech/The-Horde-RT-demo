#include "app/HordeLanternApp.h"

#include "ui/DiagnosticOverlay.h"
#include "vulkan/VulkanContext.h"

namespace horde::app
{

std::string HordeLanternApp::BuildPhase0DiagnosticString() const
{
    vulkan::VulkanContext context;
    const bool initialised = context.InitialiseForCapabilityProbe();
    vulkan::DeviceCapabilities capabilities = context.QueryDeviceCapabilities();

    if (!initialised)
    {
        capabilities.diagnostics.push_back("Vulkan capability-probe initialisation is not implemented in this scaffold.");
    }

    return ui::BuildUnsupportedDeviceText(capabilities);
}

} // namespace horde::app
