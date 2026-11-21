#include "vulkan/DeviceCapabilities.h"
#include "vulkan/RtCapabilityReport.h"
#include "vulkan/VulkanContext.h"
#include "vulkan/raytracing/RtDeviceEnablePlan.h"

#include <cstdint>
#include <iostream>
#include <limits>
#include <string>

namespace
{

int failures = 0;

void Check(const bool condition, const char* message)
{
    if (!condition)
    {
        std::cerr << "Driver capability report test failed: " << message << '\n';
        ++failures;
    }
}

bool Contains(const std::string& value, const std::string& needle)
{
    return value.find(needle) != std::string::npos;
}

horde::vulkan::DeviceCapabilities MakeCapabilities()
{
    horde::vulkan::DeviceCapabilities capabilities;
    capabilities.identity.gpuName = "Test GPU";
    capabilities.identity.vendorId = 0x1234u;
    capabilities.identity.deviceId = 0x5678u;
    capabilities.identity.driverVersion = std::numeric_limits<std::uint32_t>::max();
    capabilities.identity.vulkanApiVersion = (1u << 22u) | (2u << 12u) | 3u;
    return capabilities;
}

struct FeatureQuery
{
    std::vector<VkStructureType> chain;
    bool pipeline = true;
};

VKAPI_ATTR void VKAPI_CALL ObserveFeatures(VkPhysicalDevice device, VkPhysicalDeviceFeatures2* features)
{
    auto& observed = *reinterpret_cast<FeatureQuery*>(device);
    for (auto* node = static_cast<VkBaseOutStructure*>(features->pNext); node; node = node->pNext)
    {
        observed.chain.push_back(node->sType);
        switch (node->sType)
        {
        case VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_ACCELERATION_STRUCTURE_FEATURES_KHR:
            reinterpret_cast<VkPhysicalDeviceAccelerationStructureFeaturesKHR*>(node)->accelerationStructure = VK_TRUE;
            break;
        case VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_RAY_TRACING_PIPELINE_FEATURES_KHR:
            reinterpret_cast<VkPhysicalDeviceRayTracingPipelineFeaturesKHR*>(node)->rayTracingPipeline =
                observed.pipeline ? VK_TRUE : VK_FALSE;
            break;
        case VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_RAY_QUERY_FEATURES_KHR:
            reinterpret_cast<VkPhysicalDeviceRayQueryFeaturesKHR*>(node)->rayQuery = VK_TRUE;
            break;
        case VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_BUFFER_DEVICE_ADDRESS_FEATURES:
            reinterpret_cast<VkPhysicalDeviceBufferDeviceAddressFeatures*>(node)->bufferDeviceAddress = VK_TRUE;
            break;
        default:
            Check(false, "unexpected feature in physical-device query chain");
        }
    }
}

void CheckFeatureQueryAdmission()
{
    using namespace horde::vulkan;
    constexpr auto address = VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_BUFFER_DEVICE_ADDRESS_FEATURES;
    auto capabilities = MakeCapabilities();
    capabilities.extensionsEnumerated = true;
    capabilities.extensions.bufferDeviceAddress = true;
    FeatureQuery observed;
    const auto device = reinterpret_cast<VkPhysicalDevice>(&observed);
    QueryRtDeviceFeatures(device, ObserveFeatures, capabilities);
    Check(observed.chain == std::vector{address} && capabilities.features.bufferDeviceAddress &&
        !capabilities.featuresQueried.accelerationStructure && !capabilities.featuresQueried.rayTracingPipeline &&
        !capabilities.featuresQueried.rayQuery &&
        raytracing::SelectRtExecutionBackend(capabilities, true) == RtExecutionBackend::Unsupported,
        "AMD-shaped extension inventory queries only BDA and remains unsupported");
    const auto report = BuildCapabilityJsonReport(capabilities);
    Check(Contains(report, "\"extensionsEnumerated\": true") && Contains(report, "\"featuresQueried\": {") &&
        Contains(report, "\"accelerationStructure\": false"),
        "raw capability JSON records checked enumeration and the query coverage");

    observed.chain.clear();
    observed.pipeline = false;
    capabilities.extensions = {true, true, true, false, true};
    QueryRtDeviceFeatures(device, ObserveFeatures, capabilities);
    Check(observed.chain.size() == 4u && observed.chain.back() == address &&
        capabilities.featuresQueried.rayTracingPipeline && !capabilities.features.rayTracingPipeline &&
        raytracing::SelectRtExecutionBackend(capabilities, true) == RtExecutionBackend::RayQueryCompute,
        "observed false pipeline feature preserves real ray query and promoted BDA");

    observed.chain.clear();
    capabilities.extensionsEnumerated = false;
    QueryRtDeviceFeatures(device, ObserveFeatures, capabilities);
    Check(observed.chain == std::vector{address} && !capabilities.features.rayQuery,
        "unconfirmed extension enumeration cannot admit extension feature structures");
    QueryRtDeviceFeatures(device, nullptr, capabilities);
    Check(!capabilities.featuresQueried.bufferDeviceAddress && !capabilities.features.bufferDeviceAddress,
        "unavailable query clears prior coverage and results");

    observed.chain.clear();
    capabilities.extensions = {};
    capabilities.extensionsEnumerated = true;
    capabilities.identity.vulkanApiVersion = VK_API_VERSION_1_1;
    QueryRtDeviceFeatures(device, ObserveFeatures, capabilities);
    Check(observed.chain.empty() && !capabilities.featuresQueried.bufferDeviceAddress,
        "Vulkan 1.1 without the BDA extension cannot query its feature");
}

} // namespace

int main()
{
    using horde::vulkan::BuildCapabilityJsonReport;
    using horde::vulkan::BuildCapabilityTextReport;
    using horde::vulkan::FormatDriverVersionText;
    CheckFeatureQueryAdmission();

    constexpr std::uint32_t rawDriverVersion = std::numeric_limits<std::uint32_t>::max();
    const std::string rawDescription = "raw 4294967295 (vendor-specific)";
    Check(FormatDriverVersionText(rawDriverVersion) == rawDescription,
          "maximum raw driver version is described as vendor-specific decimal");

    auto capabilities = MakeCapabilities();
    auto& properties = capabilities.identity.driverProperties;
    properties.available = false;
    properties.id = 42u;
    properties.name = "stale-driver-name";
    properties.info = "stale-driver-info";
    properties.conformanceVersion = {9u, 8u, 7u, 6u};
    const std::string unavailableJson = BuildCapabilityJsonReport(capabilities);
    const std::string unavailableText = BuildCapabilityTextReport(capabilities);
    Check(Contains(unavailableJson, "\"driverVersion\": 4294967295"),
          "numeric driver version remains available in JSON");
    Check(Contains(unavailableJson, "\"driverVersionText\": \"raw 4294967295 (vendor-specific)\""),
          "driver version text does not apply Vulkan bit packing");
    Check(Contains(unavailableJson, "\"vulkanApiVersionText\": \"1.2.3\""),
          "Vulkan API version retains Vulkan packed-version formatting");
    Check(Contains(unavailableJson, "\"vendorId\": 4660"),
          "an unknown vendor keeps the maximum driver value on the raw vendor-independent path");
    Check(Contains(unavailableJson, "\"available\": false") &&
              Contains(unavailableJson, "\"driverId\": null") &&
              Contains(unavailableJson, "\"driverName\": null") &&
              Contains(unavailableJson, "\"driverInfo\": null") &&
              Contains(unavailableJson, "\"conformanceVersion\": null"),
          "unavailable driver properties are represented by null values");
    Check(!Contains(unavailableJson, "stale-driver") && !Contains(unavailableJson, "\"driverId\": 42") &&
              !Contains(unavailableText, "stale-driver") && !Contains(unavailableText, "Driver ID: 42") &&
              Contains(unavailableText, rawDescription) && Contains(unavailableText, "Driver ID: N/A"),
          "unavailable properties do not leak stale metadata and text uses N/A");

    auto nvidiaCapabilities = MakeCapabilities();
    nvidiaCapabilities.identity.vendorId = 0x10deu;
    nvidiaCapabilities.identity.driverVersion = 61047u;
    nvidiaCapabilities.identity.driverProperties.available = true;
    nvidiaCapabilities.identity.driverProperties.info = "610.47";
    const std::string nvidiaJson = BuildCapabilityJsonReport(nvidiaCapabilities);
    Check(Contains(nvidiaJson, "\"vendorId\": 4318") &&
              Contains(nvidiaJson, "\"driverVersion\": 61047") &&
              Contains(nvidiaJson, "\"driverVersionText\": \"raw 61047 (vendor-specific)\"") &&
              Contains(nvidiaJson, "\"driverInfo\": \"610.47\""),
          "NVIDIA raw version remains decimal while driverInfo independently exposes 610.47");

    properties.available = true;
    properties.id = 7u;
    properties.name = "Driver \"quoted\" \\ path\nline\rreturn\ttab\x01";
    properties.info = "info \\ slash \"quote\nnext\rreturn\ttab\x02";
    properties.conformanceVersion = {1u, 3u, 2u, 7u};
    const std::string populatedJson = BuildCapabilityJsonReport(capabilities);
    const std::string populatedText = BuildCapabilityTextReport(capabilities);
    Check(Contains(populatedJson, "\"available\": true") && Contains(populatedJson, "\"driverId\": 7"),
          "available driver properties retain availability and numeric ID");
    Check(Contains(populatedJson,
                   "\"driverName\": \"Driver \\\"quoted\\\" \\\\ path\\nline\\rreturn\\ttab\\u0001\"") &&
              Contains(populatedJson,
                   "\"driverInfo\": \"info \\\\ slash \\\"quote\\nnext\\rreturn\\ttab\\u0002\""),
          "JSON escapes quotes, backslashes, standard controls, and other control bytes");
    Check(Contains(populatedJson, "\"conformanceVersion\": \"1.3.2.7\""),
          "available conformance version is rendered as four dotted components");
    Check(Contains(populatedText, "Driver name: Driver \"quoted\"") && Contains(populatedText, "1.3.2.7") &&
              Contains(populatedText, "Driver info: info") && !Contains(populatedText, "Driver ID: N/A"),
          "text report includes available metadata values");

    properties.id = 0u;
    properties.name.clear();
    properties.info.clear();
    properties.conformanceVersion = {0u, 0u, 0u, 0u};
    const std::string emptyButAvailableJson = BuildCapabilityJsonReport(capabilities);
    Check(Contains(emptyButAvailableJson, "\"available\": true") &&
              Contains(emptyButAvailableJson, "\"driverId\": 0") &&
              Contains(emptyButAvailableJson, "\"driverName\": \"\"") &&
              Contains(emptyButAvailableJson, "\"driverInfo\": \"\"") &&
              Contains(emptyButAvailableJson, "\"conformanceVersion\": \"0.0.0.0\""),
          "zero and empty metadata remain distinct from unavailable properties");

    const auto baseIdentity = capabilities.identity;
    auto candidateIdentity = baseIdentity;
    candidateIdentity.driverProperties.available = false;
    candidateIdentity.driverProperties.id = 999u;
    candidateIdentity.driverProperties.name = "other metadata";
    Check(horde::vulkan::raytracing::SameRtDeviceIdentity(baseIdentity, candidateIdentity),
          "appended driver metadata does not change raw RT device selection identity");
    candidateIdentity.driverVersion -= 1u;
    Check(!horde::vulkan::raytracing::SameRtDeviceIdentity(baseIdentity, candidateIdentity),
          "raw driver version remains part of RT device selection identity");

    return failures == 0 ? 0 : 1;
}
