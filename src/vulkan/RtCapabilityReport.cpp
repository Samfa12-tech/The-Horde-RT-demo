#include "vulkan/RtCapabilityReport.h"

#include <iomanip>
#include <sstream>

namespace horde::vulkan
{
namespace
{

std::string YesNo(const bool value)
{
    return value ? "yes" : "no";
}

std::string JsonEscape(const std::string& value)
{
    std::ostringstream escaped;
    for (const char c : value)
    {
        switch (c)
        {
        case '\\':
            escaped << "\\\\";
            break;
        case '"':
            escaped << "\\\"";
            break;
        case '\n':
            escaped << "\\n";
            break;
        case '\r':
            escaped << "\\r";
            break;
        case '\t':
            escaped << "\\t";
            break;
        default:
            escaped << c;
            break;
        }
    }
    return escaped.str();
}

} // namespace

std::string BuildCapabilityTextReport(const DeviceCapabilities& capabilities)
{
    std::ostringstream out;
    out << "Backend: " << capabilities.backend << '\n';
    out << "RT mode: " << ToString(capabilities.rtMode) << '\n';
    out << "GPU name: " << capabilities.identity.gpuName << '\n';
    out << "Vendor ID: " << capabilities.identity.vendorId << '\n';
    out << "Device ID: " << capabilities.identity.deviceId << '\n';
    out << "Driver version: " << capabilities.identity.driverVersion << '\n';
    out << "Vulkan API version: " << capabilities.identity.vulkanApiVersion << '\n';
    out << "VK_KHR_acceleration_structure: " << YesNo(capabilities.extensions.accelerationStructure) << '\n';
    out << "VK_KHR_ray_tracing_pipeline: " << YesNo(capabilities.extensions.rayTracingPipeline) << '\n';
    out << "VK_KHR_ray_query: " << YesNo(capabilities.extensions.rayQuery) << '\n';
    out << "VK_KHR_buffer_device_address: " << YesNo(capabilities.extensions.bufferDeviceAddress) << '\n';
    out << "VK_KHR_deferred_host_operations: " << YesNo(capabilities.extensions.deferredHostOperations) << '\n';
    out << "Internal render resolution: " << capabilities.performance.internalRenderWidth << "x" << capabilities.performance.internalRenderHeight << '\n';
    out << "FPS / frame time: " << std::fixed << std::setprecision(2) << capabilities.performance.fps << " fps / " << capabilities.performance.frameTimeMs << " ms" << '\n';

    if (!capabilities.diagnostics.empty())
    {
        out << "Diagnostics:" << '\n';
        for (const std::string& diagnostic : capabilities.diagnostics)
        {
            out << "- " << diagnostic << '\n';
        }
    }

    return out.str();
}

std::string BuildCapabilityJsonReport(const DeviceCapabilities& capabilities)
{
    std::ostringstream out;
    out << "{\n";
    out << "  \"backend\": \"" << JsonEscape(capabilities.backend) << "\",\n";
    out << "  \"rtMode\": \"" << ToString(capabilities.rtMode) << "\",\n";
    out << "  \"gpuName\": \"" << JsonEscape(capabilities.identity.gpuName) << "\",\n";
    out << "  \"vendorId\": " << capabilities.identity.vendorId << ",\n";
    out << "  \"deviceId\": " << capabilities.identity.deviceId << ",\n";
    out << "  \"driverVersion\": " << capabilities.identity.driverVersion << ",\n";
    out << "  \"vulkanApiVersion\": " << capabilities.identity.vulkanApiVersion << ",\n";
    out << "  \"extensions\": {\n";
    out << "    \"VK_KHR_acceleration_structure\": " << (capabilities.extensions.accelerationStructure ? "true" : "false") << ",\n";
    out << "    \"VK_KHR_ray_tracing_pipeline\": " << (capabilities.extensions.rayTracingPipeline ? "true" : "false") << ",\n";
    out << "    \"VK_KHR_ray_query\": " << (capabilities.extensions.rayQuery ? "true" : "false") << ",\n";
    out << "    \"VK_KHR_buffer_device_address\": " << (capabilities.extensions.bufferDeviceAddress ? "true" : "false") << ",\n";
    out << "    \"VK_KHR_deferred_host_operations\": " << (capabilities.extensions.deferredHostOperations ? "true" : "false") << "\n";
    out << "  },\n";
    out << "  \"features\": {\n";
    out << "    \"accelerationStructure\": " << (capabilities.features.accelerationStructure ? "true" : "false") << ",\n";
    out << "    \"rayTracingPipeline\": " << (capabilities.features.rayTracingPipeline ? "true" : "false") << ",\n";
    out << "    \"rayQuery\": " << (capabilities.features.rayQuery ? "true" : "false") << ",\n";
    out << "    \"bufferDeviceAddress\": " << (capabilities.features.bufferDeviceAddress ? "true" : "false") << "\n";
    out << "  },\n";
    out << "  \"internalRenderResolution\": {\n";
    out << "    \"width\": " << capabilities.performance.internalRenderWidth << ",\n";
    out << "    \"height\": " << capabilities.performance.internalRenderHeight << "\n";
    out << "  },\n";
    out << "  \"fps\": " << std::fixed << std::setprecision(2) << capabilities.performance.fps << ",\n";
    out << "  \"frameTimeMs\": " << capabilities.performance.frameTimeMs << ",\n";
    out << "  \"diagnostics\": [";

    for (std::size_t i = 0; i < capabilities.diagnostics.size(); ++i)
    {
        if (i > 0)
        {
            out << ", ";
        }
        out << "\"" << JsonEscape(capabilities.diagnostics[i]) << "\"";
    }

    out << "]\n";
    out << "}\n";
    return out.str();
}

} // namespace horde::vulkan
