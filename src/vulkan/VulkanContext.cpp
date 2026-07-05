#include "vulkan/VulkanContext.h"

#include "vulkan/raytracing/RayTracingRequirements.h"

#include <algorithm>
#include <iomanip>
#include <sstream>
#include <string>
#include <vector>

namespace horde::vulkan
{

namespace
{

constexpr char kNoProbeResultMessage[] = "No Vulkan physical devices could be enumerated.";

std::string ToHex(std::uint32_t value)
{
    std::ostringstream stream;
    stream << "0x" << std::hex << std::uppercase << std::setw(8) << std::setfill('0') << value;
    return stream.str();
}

bool HasDuplicate(const std::vector<std::string>& values, const std::string& value)
{
    return std::find(values.begin(), values.end(), value) != values.end();
}

void AddMissing(std::vector<std::string>& diagnostics, const std::string& value)
{
    if (!HasDuplicate(diagnostics, value))
    {
        diagnostics.push_back(value);
    }
}

void AppendMissingRequirementsForRayTracingPipeline(const ExtensionSupport& extensions, const FeatureSupport& features,
                                                  std::vector<std::string>& diagnostics)
{
    if (!extensions.accelerationStructure)
    {
        AddMissing(diagnostics, "Missing required extension: VK_KHR_acceleration_structure");
    }
    if (extensions.accelerationStructure && !features.accelerationStructure)
    {
        AddMissing(diagnostics, "Missing required feature: VkPhysicalDeviceAccelerationStructureFeaturesKHR::accelerationStructure");
    }

    if (!extensions.rayTracingPipeline)
    {
        AddMissing(diagnostics, "Missing required extension: VK_KHR_ray_tracing_pipeline");
    }
    if (extensions.rayTracingPipeline && !features.rayTracingPipeline)
    {
        AddMissing(diagnostics, "Missing required feature: VkPhysicalDeviceRayTracingPipelineFeaturesKHR::rayTracingPipeline");
    }

    if (!extensions.bufferDeviceAddress)
    {
        AddMissing(diagnostics, "Missing required extension: VK_KHR_buffer_device_address");
    }
    if (extensions.bufferDeviceAddress && !features.bufferDeviceAddress)
    {
        AddMissing(diagnostics, "Missing required feature: VkPhysicalDeviceBufferDeviceAddressFeatures::bufferDeviceAddress");
    }

    if (!extensions.deferredHostOperations)
    {
        AddMissing(diagnostics, "Missing required extension: VK_KHR_deferred_host_operations");
    }
}

void AppendMissingRequirementsForRayQuery(const ExtensionSupport& extensions, const FeatureSupport& features,
                                         std::vector<std::string>& diagnostics)
{
    if (!extensions.accelerationStructure)
    {
        AddMissing(diagnostics, "Missing required extension: VK_KHR_acceleration_structure");
    }
    if (extensions.accelerationStructure && !features.accelerationStructure)
    {
        AddMissing(diagnostics, "Missing required feature: VkPhysicalDeviceAccelerationStructureFeaturesKHR::accelerationStructure");
    }

    if (!extensions.rayQuery)
    {
        AddMissing(diagnostics, "Missing required extension: VK_KHR_ray_query");
    }
    if (extensions.rayQuery && !features.rayQuery)
    {
        AddMissing(diagnostics, "Missing required feature: VkPhysicalDeviceRayQueryFeaturesKHR::rayQuery");
    }

    if (!extensions.bufferDeviceAddress)
    {
        AddMissing(diagnostics, "Missing required extension: VK_KHR_buffer_device_address");
    }
    if (extensions.bufferDeviceAddress && !features.bufferDeviceAddress)
    {
        AddMissing(diagnostics, "Missing required feature: VkPhysicalDeviceBufferDeviceAddressFeatures::bufferDeviceAddress");
    }
}

void AppendExtensionLine(std::vector<std::string>& diagnostics, const std::string& extension, const bool supported)
{
    diagnostics.push_back(extension + ": " + std::string(supported ? "yes" : "no"));
}

void AppendCandidateSummary(std::vector<std::string>& diagnostics, const std::uint32_t deviceIndex,
                           const DeviceCapabilities& capabilities)
{
    diagnostics.push_back("Candidate device " + std::to_string(deviceIndex) + ": " + capabilities.identity.gpuName);
    diagnostics.push_back("  RT mode: " + ToString(capabilities.rtMode));
    diagnostics.push_back("  Vendor ID: " + std::to_string(capabilities.identity.vendorId));
    diagnostics.push_back("  Device ID: " + std::to_string(capabilities.identity.deviceId));
    diagnostics.push_back("  VK_KHR_acceleration_structure: " + std::string(capabilities.extensions.accelerationStructure ? "yes" : "no"));
    diagnostics.push_back("  VK_KHR_ray_tracing_pipeline: " + std::string(capabilities.extensions.rayTracingPipeline ? "yes" : "no"));
    diagnostics.push_back("  VK_KHR_ray_query: " + std::string(capabilities.extensions.rayQuery ? "yes" : "no"));
    diagnostics.push_back("  VK_KHR_buffer_device_address: " + std::string(capabilities.extensions.bufferDeviceAddress ? "yes" : "no"));
    diagnostics.push_back("  VK_KHR_deferred_host_operations: " + std::string(capabilities.extensions.deferredHostOperations ? "yes" : "no"));

    if (capabilities.diagnostics.empty())
    {
        return;
    }

    diagnostics.push_back("  Diagnostics:");
    for (const std::string& line : capabilities.diagnostics)
    {
        diagnostics.push_back("    - " + line);
    }
}

bool IsBetterMode(const RtMode candidateMode, const RtMode currentMode)
{
    return static_cast<int>(candidateMode) > static_cast<int>(currentMode);
}

} // namespace

VulkanContext::~VulkanContext()
{
    if (instance_ != VK_NULL_HANDLE)
    {
        vkDestroyInstance(instance_, nullptr);
        instance_ = VK_NULL_HANDLE;
    }
}

std::string VulkanContext::VersionTextFromPacked(const std::uint32_t version) const
{
    const std::uint32_t major = VK_VERSION_MAJOR(version);
    const std::uint32_t minor = VK_VERSION_MINOR(version);
    const std::uint32_t patch = VK_VERSION_PATCH(version);
    return std::to_string(major) + "." + std::to_string(minor) + "." + std::to_string(patch);
}

DeviceCapabilities VulkanContext::ProbePhysicalDevice(const VkPhysicalDevice physicalDevice, const std::uint32_t deviceIndex) const
{
    DeviceCapabilities capabilities;

    VkPhysicalDeviceProperties properties{};
    vkGetPhysicalDeviceProperties(physicalDevice, &properties);

    capabilities.identity.gpuName = properties.deviceName[0] ? properties.deviceName : "Unknown GPU";
    capabilities.identity.vendorId = properties.vendorID;
    capabilities.identity.deviceId = properties.deviceID;
    capabilities.identity.driverVersion = properties.driverVersion;
    capabilities.identity.vulkanApiVersion = properties.apiVersion;

    capabilities.diagnostics.push_back("Inspecting candidate " + std::to_string(deviceIndex));
    capabilities.diagnostics.push_back("Driver version packed: " + std::to_string(capabilities.identity.driverVersion));
    capabilities.diagnostics.push_back("Driver version: " + VersionTextFromPacked(capabilities.identity.driverVersion));
    capabilities.diagnostics.push_back("Vulkan API version: " + VersionTextFromPacked(capabilities.identity.vulkanApiVersion));

    std::uint32_t extensionCount = 0;
    vkEnumerateDeviceExtensionProperties(physicalDevice, nullptr, &extensionCount, nullptr);
    std::vector<VkExtensionProperties> extensions(extensionCount);
    vkEnumerateDeviceExtensionProperties(physicalDevice, nullptr, &extensionCount, extensions.data());

    for (const auto& extension : extensions)
    {
        if (extension.extensionName == std::string(VK_KHR_ACCELERATION_STRUCTURE_EXTENSION_NAME))
        {
            capabilities.extensions.accelerationStructure = true;
        }
        if (extension.extensionName == std::string(VK_KHR_RAY_TRACING_PIPELINE_EXTENSION_NAME))
        {
            capabilities.extensions.rayTracingPipeline = true;
        }
        if (extension.extensionName == std::string(VK_KHR_RAY_QUERY_EXTENSION_NAME))
        {
            capabilities.extensions.rayQuery = true;
        }
        if (extension.extensionName == std::string(VK_KHR_BUFFER_DEVICE_ADDRESS_EXTENSION_NAME))
        {
            capabilities.extensions.bufferDeviceAddress = true;
        }
        if (extension.extensionName == std::string(VK_KHR_DEFERRED_HOST_OPERATIONS_EXTENSION_NAME))
        {
            capabilities.extensions.deferredHostOperations = true;
        }
    }

    VkPhysicalDeviceFeatures2 features2{};
    VkPhysicalDeviceAccelerationStructureFeaturesKHR accelerationStructureFeatures{
        VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_ACCELERATION_STRUCTURE_FEATURES_KHR};
    VkPhysicalDeviceRayTracingPipelineFeaturesKHR rayTracingPipelineFeatures{
        VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_RAY_TRACING_PIPELINE_FEATURES_KHR};
    VkPhysicalDeviceRayQueryFeaturesKHR rayQueryFeatures{VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_RAY_QUERY_FEATURES_KHR};
    VkPhysicalDeviceBufferDeviceAddressFeatures bufferDeviceAddressFeatures{
        VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_BUFFER_DEVICE_ADDRESS_FEATURES_KHR};

    features2.sType = VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_FEATURES_2;
    features2.pNext = &accelerationStructureFeatures;
    accelerationStructureFeatures.pNext = &rayTracingPipelineFeatures;
    rayTracingPipelineFeatures.pNext = &rayQueryFeatures;
    rayQueryFeatures.pNext = &bufferDeviceAddressFeatures;

    bool features2Retrieved = false;

    const auto getPhysicalDeviceFeatures2 = reinterpret_cast<PFN_vkGetPhysicalDeviceFeatures2>(
        vkGetInstanceProcAddr(instance_, "vkGetPhysicalDeviceFeatures2"));
    if (getPhysicalDeviceFeatures2 != nullptr)
    {
        getPhysicalDeviceFeatures2(physicalDevice, &features2);
        features2Retrieved = true;
    }
    else
    {
        const auto getPhysicalDeviceFeatures2KHR = reinterpret_cast<PFN_vkGetPhysicalDeviceFeatures2KHR>(
            vkGetInstanceProcAddr(instance_, "vkGetPhysicalDeviceFeatures2KHR"));
        if (getPhysicalDeviceFeatures2KHR != nullptr)
        {
            getPhysicalDeviceFeatures2KHR(physicalDevice, &features2);
            features2Retrieved = true;
        }
    }

    if (!features2Retrieved)
    {
        capabilities.diagnostics.push_back("vkGetPhysicalDeviceFeatures2 unavailable on this platform; assuming RT extension features unavailable.");
    }

    capabilities.features.accelerationStructure = accelerationStructureFeatures.accelerationStructure == VK_TRUE;
    capabilities.features.rayTracingPipeline = rayTracingPipelineFeatures.rayTracingPipeline == VK_TRUE;
    capabilities.features.rayQuery = rayQueryFeatures.rayQuery == VK_TRUE;
    capabilities.features.bufferDeviceAddress = bufferDeviceAddressFeatures.bufferDeviceAddress == VK_TRUE;

    capabilities.rtMode = raytracing::EvaluateRtMode(capabilities.extensions, capabilities.features);

    if (capabilities.rtMode == RtMode::Unsupported)
    {
        std::vector<std::string> missing;
        AppendMissingRequirementsForRayTracingPipeline(capabilities.extensions, capabilities.features, missing);
        AppendMissingRequirementsForRayQuery(capabilities.extensions, capabilities.features, missing);

        if (missing.empty())
        {
            capabilities.diagnostics.push_back("No ray tracing pipeline or ray query support was selected.");
        }
        else
        {
            capabilities.diagnostics.insert(capabilities.diagnostics.end(), missing.begin(), missing.end());
        }
    }

    capabilities.diagnostics.push_back("RT mode selected: " + ToString(capabilities.rtMode));
    return capabilities;
}

bool VulkanContext::InitialiseForCapabilityProbe()
{
    if (initialised_)
    {
        return true;
    }

    startupDiagnostics_.clear();
    selectedCapabilities_ = DeviceCapabilities{};
    selectedCapabilities_.rtMode = RtMode::Unsupported;

    const VkApplicationInfo appInfo{
        VK_STRUCTURE_TYPE_APPLICATION_INFO, nullptr, "HordeLanternRTCapabilityProbe", 1, "horde_rt", 1,
        VK_API_VERSION_1_2};

    const VkInstanceCreateInfo instanceCreateInfo{
        VK_STRUCTURE_TYPE_INSTANCE_CREATE_INFO, nullptr, 0, &appInfo, 0, nullptr, 0, nullptr};

    const VkResult instanceResult = vkCreateInstance(&instanceCreateInfo, nullptr, &instance_);
    if (instanceResult != VK_SUCCESS)
    {
        startupDiagnostics_.push_back("Failed to create Vulkan instance.");
        startupDiagnostics_.push_back("vkCreateInstance result: " + std::to_string(static_cast<int>(instanceResult)));
        return false;
    }

    startupDiagnostics_.push_back("Vulkan instance created.");

    std::uint32_t deviceCount = 0;
    const VkResult deviceCountResult = vkEnumeratePhysicalDevices(instance_, &deviceCount, nullptr);
    if (deviceCountResult != VK_SUCCESS || deviceCount == 0)
    {
        startupDiagnostics_.push_back("vkEnumeratePhysicalDevices returned no devices.");
        startupDiagnostics_.push_back(kNoProbeResultMessage);
        selectedCapabilities_.diagnostics.push_back(kNoProbeResultMessage);
        initialised_ = true;
        return true;
    }

    startupDiagnostics_.push_back("Found " + std::to_string(deviceCount) + " candidate physical device(s).");

    std::vector<VkPhysicalDevice> physicalDevices(deviceCount);
    vkEnumeratePhysicalDevices(instance_, &deviceCount, physicalDevices.data());

    bool hasSelectedCandidate = false;

    for (std::uint32_t deviceIndex = 0; deviceIndex < deviceCount; ++deviceIndex)
    {
        DeviceCapabilities capabilities = ProbePhysicalDevice(physicalDevices[deviceIndex], deviceIndex);
        AppendCandidateSummary(startupDiagnostics_, deviceIndex, capabilities);
        startupDiagnostics_.push_back(std::string(78, '-'));

        if (!hasSelectedCandidate || IsBetterMode(capabilities.rtMode, selectedCapabilities_.rtMode))
        {
            selectedCapabilities_ = capabilities;
            hasSelectedCandidate = true;
        }
    }

    if (selectedCapabilities_.backend.empty())
    {
        selectedCapabilities_.backend = "Vulkan";
    }

    startupDiagnostics_.push_back("Selected device: " + selectedCapabilities_.identity.gpuName + " (" + ToString(selectedCapabilities_.rtMode) + ")");

    if (selectedCapabilities_.identity.vendorId == 0 && selectedCapabilities_.identity.deviceId == 0)
    {
        selectedCapabilities_.diagnostics.push_back(kNoProbeResultMessage);
    }

    if (selectedCapabilities_.rtMode == RtMode::Unsupported && selectedCapabilities_.diagnostics.empty())
    {
        selectedCapabilities_.diagnostics.push_back("Unsupported: no valid Vulkan RT requirements found.");
    }

    selectedCapabilities_.diagnostics.push_back("Internal render resolution: not measured yet.");
    selectedCapabilities_.diagnostics.push_back("FPS / frame time: not measured yet.");

    initialised_ = true;
    return true;
}

DeviceCapabilities VulkanContext::QueryDeviceCapabilities() const
{
    if (!initialised_)
    {
        DeviceCapabilities notInitialised;
        notInitialised.backend = "Vulkan";
        notInitialised.rtMode = RtMode::Unsupported;
        notInitialised.diagnostics.push_back("Probe not initialised. Call InitialiseForCapabilityProbe() before QueryDeviceCapabilities().");
        notInitialised.diagnostics.push_back("No measurement taken.");
        return notInitialised;
    }

    DeviceCapabilities report = selectedCapabilities_;
    if (report.backend.empty())
    {
        report.backend = "Vulkan";
    }

    return report;
}

} // namespace horde::vulkan
