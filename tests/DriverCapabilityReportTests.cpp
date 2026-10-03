#include "vulkan/DeviceCapabilities.h"
#include "vulkan/RtCapabilityReport.h"
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

} // namespace

int main()
{
    using horde::vulkan::BuildCapabilityJsonReport;
    using horde::vulkan::BuildCapabilityTextReport;
    using horde::vulkan::FormatDriverVersionText;

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
