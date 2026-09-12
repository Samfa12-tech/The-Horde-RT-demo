#pragma once

#include <cstdint>
#include <string>
#include <vector>

#include "vulkan/RtExecutionBackend.h"

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

struct GpuRtTimingSnapshot
{
    std::string status = "Not initialised";
    bool supported = false;
    bool valid = false;
    float latestMs = 0.0f;
    float averageMs = 0.0f;
    float timestampPeriodNanoseconds = 0.0f;
    std::uint32_t timestampValidBits = 0u;
    std::uint64_t sampleCount = 0u;
    std::uint64_t unavailableCount = 0u;
    std::uint64_t errorCount = 0u;
};

struct PerformanceSnapshot
{
    std::uint32_t internalRenderWidth = 0;
    std::uint32_t internalRenderHeight = 0;
    float fps = 0.0f;
    float frameTimeMs = 0.0f;
    GpuRtTimingSnapshot gpuRt;
};

struct RtSceneSnapshot
{
    std::string status = "Not attempted";
    std::string geometry = "Complete Horde showcase route with sequential animated skeleton and staff-lit lich";
    std::uint32_t dispatchWidth = 0;
    std::uint32_t dispatchHeight = 0;
    RtExecutionBackend executionBackend = RtExecutionBackend::Unsupported;
    bool presented = false;
};

struct DeviceCapabilities
{
    std::string backend = "Vulkan";
    RtMode rtMode = RtMode::Unsupported;
    DeviceIdentity identity;
    ExtensionSupport extensions;
    FeatureSupport features;
    PerformanceSnapshot performance;
    RtSceneSnapshot rtScene;
    std::vector<std::string> diagnostics;
};

std::string ToString(RtMode mode);
std::string ToString(RtExecutionBackend backend);
// Selection is useful failure evidence, never proof of successful presentation.
void BeginRtBackendSelection(RtSceneSnapshot& scene, RtExecutionBackend backend);

} // namespace horde::vulkan
