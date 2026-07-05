#include "ui/DiagnosticOverlay.h"

#include <sstream>

#include "vulkan/RtCapabilityReport.h"

namespace horde::ui
{

std::string BuildDiagnosticOverlayText(const vulkan::DeviceCapabilities& capabilities)
{
    return vulkan::BuildCapabilityTextReport(capabilities);
}

std::string BuildUnsupportedDeviceText(const vulkan::DeviceCapabilities& capabilities)
{
    std::ostringstream out;
    out << "Unsupported Vulkan RT device\n";
    out << "This project is RT or nothing. No fake fallback will be used.\n\n";
    out << vulkan::BuildCapabilityTextReport(capabilities);
    return out.str();
}

} // namespace horde::ui
