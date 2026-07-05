#pragma once

#include <cstdint>
#include <string>
#include <vector>

namespace horde::vulkan
{

enum class RtMode
{
    Unsupported,
    RayQuery,
    RayTracingPipeline
};

struct ExtensionSupport
{
    bool accelerationStructure = false;
    bool rayTracingPipeline = false;
    bool rayQuery = false;
    bool bufferDeviceAddress = false;
    bool deferredHostOperations = false;
};

struct FeatureSupport
{
    bool accelerationStructure = false;
    bool rayTracingPipeline = false;
    bool rayQuery = false;
    bool bufferDeviceAddress = false;
};

struct DeviceIdentity
{
    std::string gpuName = "Unknown GPU";
    std::uint32_t vendorId = 0;
    std::uint32_t deviceId = 0;
    std::uint32_t driverVersion = 0;
    std::uint32_t vulkanApiVersion = 0;
};

struct PerformanceSnapshot
{
    std::uint32_t internalRenderWidth = 0;
    std::uint32_t internalRenderHeight = 0;
    float fps = 0.0f;
    float frameTimeMs = 0.0f;
};

struct DeviceCapabilities
{
    std::string backend = "Vulkan";
    RtMode rtMode = RtMode::Unsupported;
    DeviceIdentity identity;
    ExtensionSupport extensions;
    FeatureSupport features;
    PerformanceSnapshot performance;
    std::vector<std::string> diagnostics;
};

std::string ToString(RtMode mode);
bool HasEssentialAccelerationStructureSupport(const DeviceCapabilities& capabilities);
bool HasPreferredPipelineSupport(const DeviceCapabilities& capabilities);

} // namespace horde::vulkan
