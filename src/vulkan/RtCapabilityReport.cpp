#include "vulkan/RtCapabilityReport.h"

#include <iomanip>
#include <locale>
#include <sstream>
#include <string>

#include <vulkan/vulkan.h>

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
            if (static_cast<unsigned char>(c) < 0x20u)
            {
                constexpr char hex[] = "0123456789abcdef";
                escaped << "\\u00" << hex[static_cast<unsigned char>(c) >> 4u]
                        << hex[static_cast<unsigned char>(c) & 0x0fu];
            }
            else
            {
                escaped << c;
            }
            break;
        }
    }
    return escaped.str();
}

std::string FormatVkPackedVersion(const std::uint32_t value)
{
    const std::uint32_t major = VK_VERSION_MAJOR(value);
    const std::uint32_t minor = VK_VERSION_MINOR(value);
    const std::uint32_t patch = VK_VERSION_PATCH(value);
    return std::to_string(major) + "." + std::to_string(minor) + "." + std::to_string(patch);
}

bool IsMeasured(const float value)
{
    return value > 0.0f;
}

bool IsResolutionMeasured(const std::uint32_t width, const std::uint32_t height)
{
    return width > 0 && height > 0;
}

std::string JsonLegacyFloat(const float value, const bool available)
{
    if (!available) return "\"N/A\"";
    std::ostringstream number;
    number.imbue(std::locale::classic());
    number << std::fixed << std::setprecision(6) << value;
    return number.str();
}

} // namespace

std::string BuildCapabilityTextReport(const DeviceCapabilities& capabilities)
{
    std::ostringstream out;
    out << "Backend: " << capabilities.backend << '\n';
    out << "RT mode: " << ToString(capabilities.rtMode) << '\n';
    out << "Execution backend: " << ToString(capabilities.rtScene.executionBackend) << '\n';
    out << "GPU name: " << capabilities.identity.gpuName << '\n';
    out << "Vendor ID: " << capabilities.identity.vendorId << '\n';
    out << "Device ID: " << capabilities.identity.deviceId << '\n';
    out << "Driver version: " << FormatVkPackedVersion(capabilities.identity.driverVersion) << '\n';
    out << "Vulkan API version: " << FormatVkPackedVersion(capabilities.identity.vulkanApiVersion) << '\n';
    out << "VK_KHR_acceleration_structure: " << YesNo(capabilities.extensions.accelerationStructure) << '\n';
    out << "VK_KHR_ray_tracing_pipeline: " << YesNo(capabilities.extensions.rayTracingPipeline) << '\n';
    out << "VK_KHR_ray_query: " << YesNo(capabilities.extensions.rayQuery) << '\n';
    out << "VK_KHR_buffer_device_address: " << YesNo(capabilities.extensions.bufferDeviceAddress) << '\n';
    out << "VK_KHR_deferred_host_operations: " << YesNo(capabilities.extensions.deferredHostOperations) << '\n';

    if (IsResolutionMeasured(capabilities.performance.internalRenderWidth, capabilities.performance.internalRenderHeight))
    {
        out << "Internal render resolution: " << capabilities.performance.internalRenderWidth << "x" << capabilities.performance.internalRenderHeight << '\n';
    }
    else
    {
        out << "Internal render resolution: N/A\n";
    }

    if (IsMeasured(capabilities.performance.fps) || capabilities.performance.frameTimeMs > 0.0f)
    {
        out << "FPS / frame time: " << std::fixed << std::setprecision(2) << capabilities.performance.fps << " fps / "
            << capabilities.performance.frameTimeMs << " ms\n";
    }
    else
    {
        out << "FPS / frame time: N/A\n";
    }
    if (capabilities.performance.gpuRt.valid)
    {
        out << "GPU RT command-buffer time: " << std::fixed << std::setprecision(3)
            << capabilities.performance.gpuRt.latestMs << " ms latest / "
            << capabilities.performance.gpuRt.averageMs << " ms average ("
            << capabilities.performance.gpuRt.sampleCount << " samples)\n";
    }
    else
    {
        out << "GPU RT command-buffer time: N/A ("
            << capabilities.performance.gpuRt.status << ")\n";
    }

    out << "RT scene status: " << capabilities.rtScene.status << '\n';
    out << "RT scene geometry: " << capabilities.rtScene.geometry << '\n';
    if (capabilities.rtScene.dispatchWidth > 0 && capabilities.rtScene.dispatchHeight > 0)
    {
        out << "RT scene dispatch resolution: " << capabilities.rtScene.dispatchWidth << "x"
            << capabilities.rtScene.dispatchHeight << '\n';
    }
    else
    {
        out << "RT scene dispatch resolution: N/A\n";
    }
    out << "RT scene presented: " << (capabilities.rtScene.presented ? "yes" : "no") << '\n';

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
    out.imbue(std::locale::classic());
    out << "{\n";
    out << "  \"backend\": \"" << JsonEscape(capabilities.backend) << "\",\n";
    out << "  \"rtMode\": \"" << ToString(capabilities.rtMode) << "\",\n";
    out << "  \"executionBackend\": \"" << ToString(capabilities.rtScene.executionBackend) << "\",\n";
    out << "  \"gpuName\": \"" << JsonEscape(capabilities.identity.gpuName) << "\",\n";
    out << "  \"vendorId\": " << capabilities.identity.vendorId << ",\n";
    out << "  \"deviceId\": " << capabilities.identity.deviceId << ",\n";
    out << "  \"driverVersion\": " << capabilities.identity.driverVersion << ",\n";
    out << "  \"driverVersionText\": \"" << FormatVkPackedVersion(capabilities.identity.driverVersion) << "\",\n";
    out << "  \"vulkanApiVersion\": " << capabilities.identity.vulkanApiVersion << ",\n";
    out << "  \"vulkanApiVersionText\": \"" << FormatVkPackedVersion(capabilities.identity.vulkanApiVersion) << "\",\n";
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
    out << "  \"fps\": " << JsonLegacyFloat(capabilities.performance.fps, IsMeasured(capabilities.performance.fps)) << ",\n";
    out << "  \"frameTimeMs\": " << JsonLegacyFloat(capabilities.performance.frameTimeMs, capabilities.performance.frameTimeMs > 0.0f) << ",\n";
    out << "  \"gpuRtTiming\": {\n";
    out << "    \"status\": \"" << JsonEscape(capabilities.performance.gpuRt.status) << "\",\n";
    out << "    \"supported\": " << (capabilities.performance.gpuRt.supported ? "true" : "false") << ",\n";
    out << "    \"valid\": " << (capabilities.performance.gpuRt.valid ? "true" : "false") << ",\n";
    out << "    \"latestMs\": " << JsonLegacyFloat(capabilities.performance.gpuRt.latestMs, capabilities.performance.gpuRt.valid) << ",\n";
    out << "    \"averageMs\": " << JsonLegacyFloat(capabilities.performance.gpuRt.averageMs, capabilities.performance.gpuRt.valid) << ",\n";
    out << "    \"timestampPeriodNanoseconds\": " << capabilities.performance.gpuRt.timestampPeriodNanoseconds << ",\n";
    out << "    \"timestampValidBits\": " << capabilities.performance.gpuRt.timestampValidBits << ",\n";
    out << "    \"sampleCount\": " << capabilities.performance.gpuRt.sampleCount << ",\n";
    out << "    \"unavailableCount\": " << capabilities.performance.gpuRt.unavailableCount << ",\n";
    out << "    \"errorCount\": " << capabilities.performance.gpuRt.errorCount << "\n";
    out << "  },\n";
    out << "  \"rtScene\": {\n";
    out << "    \"status\": \"" << JsonEscape(capabilities.rtScene.status) << "\",\n";
    out << "    \"geometry\": \"" << JsonEscape(capabilities.rtScene.geometry) << "\",\n";
    out << "    \"dispatchWidth\": " << capabilities.rtScene.dispatchWidth << ",\n";
    out << "    \"dispatchHeight\": " << capabilities.rtScene.dispatchHeight << ",\n";
    out << "    \"presented\": " << (capabilities.rtScene.presented ? "true" : "false") << "\n";
    out << "  },\n";
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
