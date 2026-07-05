#include "vulkan/raytracing/TinyRtScene.h"

#include "vulkan/raytracing/AccelerationStructures.h"
#include "vulkan/raytracing/RayTracingPipeline.h"
#include "vulkan/raytracing/ShaderBindingTable.h"

#include <cstdint>
#include <sstream>
#include <string>
#include <vector>

namespace horde::vulkan::raytracing
{

namespace
{

const char* const kSceneStatusDispatchImplemented = "Dispatch implemented (RT device/function skeleton)";
const char* const kSceneStatusNotAttempted = "Not attempted";
const char* const kSceneStatusUnsupported = "Unsupported";
constexpr std::uint32_t kTinySceneWidth = 256;
constexpr std::uint32_t kTinySceneHeight = 256;

std::string ToString(VkResult result)
{
    switch (result)
    {
    case VK_SUCCESS:
        return "VK_SUCCESS";
    case VK_ERROR_INITIALIZATION_FAILED:
        return "VK_ERROR_INITIALIZATION_FAILED";
    case VK_ERROR_FEATURE_NOT_PRESENT:
        return "VK_ERROR_FEATURE_NOT_PRESENT";
    case VK_ERROR_EXTENSION_NOT_PRESENT:
        return "VK_ERROR_EXTENSION_NOT_PRESENT";
    case VK_ERROR_LAYER_NOT_PRESENT:
        return "VK_ERROR_LAYER_NOT_PRESENT";
    case VK_ERROR_INCOMPATIBLE_DRIVER:
        return "VK_ERROR_INCOMPATIBLE_DRIVER";
    default:
        {
            std::ostringstream out;
            out << "VkResult(" << static_cast<int>(result) << ")";
            return out.str();
        }
    }
}

std::vector<const char*> RequiredDeviceExtensions()
{
    return {
        VK_KHR_ACCELERATION_STRUCTURE_EXTENSION_NAME,
        VK_KHR_RAY_TRACING_PIPELINE_EXTENSION_NAME,
        VK_KHR_BUFFER_DEVICE_ADDRESS_EXTENSION_NAME,
        VK_KHR_DEFERRED_HOST_OPERATIONS_EXTENSION_NAME};
}

std::uint32_t FindGraphicsAndComputeQueueFamily(const VkPhysicalDevice physicalDevice, bool& found)
{
    found = false;
    std::uint32_t queueFamilyCount = 0;
    vkGetPhysicalDeviceQueueFamilyProperties(physicalDevice, &queueFamilyCount, nullptr);
    if (queueFamilyCount == 0)
    {
        return 0u;
    }

    std::vector<VkQueueFamilyProperties> queueFamilies(queueFamilyCount);
    vkGetPhysicalDeviceQueueFamilyProperties(physicalDevice, &queueFamilyCount, queueFamilies.data());

    for (uint32_t queueFamilyIndex = 0; queueFamilyIndex < queueFamilyCount; ++queueFamilyIndex)
    {
        const auto& queueFamily = queueFamilies[queueFamilyIndex];
        if ((queueFamily.queueFlags & VK_QUEUE_GRAPHICS_BIT) != 0u &&
            (queueFamily.queueFlags & VK_QUEUE_COMPUTE_BIT) != 0u)
        {
            found = true;
            return queueFamilyIndex;
        }
    }

    return 0u;
}

bool CreateAndDestroyMinimalRayTracingDevice(const VkPhysicalDevice physicalDevice,
                                            const DeviceCapabilities& capabilities, std::string& diagnostic)
{
    if (physicalDevice == VK_NULL_HANDLE)
    {
        diagnostic = "No physical device selected for RT device bootstrap.";
        return false;
    }

    bool hasQueueFamily = false;
    const std::uint32_t queueFamilyIndex = FindGraphicsAndComputeQueueFamily(physicalDevice, hasQueueFamily);
    if (!hasQueueFamily)
    {
        diagnostic = "No graphics+compute queue family available for tiny RT dispatch.";
        return false;
    }

    const float queuePriority = 1.0f;
    const VkDeviceQueueCreateInfo queueCreateInfo{
        VK_STRUCTURE_TYPE_DEVICE_QUEUE_CREATE_INFO,
        nullptr,
        0,
        queueFamilyIndex,
        1,
        &queuePriority,
    };

    VkPhysicalDeviceAccelerationStructureFeaturesKHR accelerationStructureFeatures{
        VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_ACCELERATION_STRUCTURE_FEATURES_KHR};
    accelerationStructureFeatures.accelerationStructure = capabilities.features.accelerationStructure ? VK_TRUE : VK_FALSE;

    VkPhysicalDeviceRayTracingPipelineFeaturesKHR rayTracingPipelineFeatures{
        VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_RAY_TRACING_PIPELINE_FEATURES_KHR};
    rayTracingPipelineFeatures.rayTracingPipeline = capabilities.features.rayTracingPipeline ? VK_TRUE : VK_FALSE;

    VkPhysicalDeviceBufferDeviceAddressFeaturesKHR bufferDeviceAddressFeatures{
        VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_BUFFER_DEVICE_ADDRESS_FEATURES_KHR};
    bufferDeviceAddressFeatures.bufferDeviceAddress = capabilities.features.bufferDeviceAddress ? VK_TRUE : VK_FALSE;

    VkPhysicalDeviceFeatures2 features2{
        VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_FEATURES_2,
        &accelerationStructureFeatures};
    accelerationStructureFeatures.pNext = &rayTracingPipelineFeatures;
    rayTracingPipelineFeatures.pNext = &bufferDeviceAddressFeatures;

    const auto extensions = RequiredDeviceExtensions();

    const VkDeviceCreateInfo deviceCreateInfo{
        VK_STRUCTURE_TYPE_DEVICE_CREATE_INFO,
        &features2,
        0,
        1,
        &queueCreateInfo,
        0,
        nullptr,
        static_cast<uint32_t>(extensions.size()),
        extensions.data(),
        nullptr};

    VkDevice device = VK_NULL_HANDLE;
    const VkResult deviceResult = vkCreateDevice(physicalDevice, &deviceCreateInfo, nullptr, &device);
    if (deviceResult != VK_SUCCESS)
    {
        diagnostic = "vkCreateDevice failed for tiny RT path: " + ToString(deviceResult);
        return false;
    }

    const auto cmdTraceRays = reinterpret_cast<PFN_vkCmdTraceRaysKHR>(vkGetDeviceProcAddr(device, "vkCmdTraceRaysKHR"));
    const auto createAs = reinterpret_cast<PFN_vkCreateAccelerationStructureKHR>(vkGetDeviceProcAddr(device, "vkCreateAccelerationStructureKHR"));
    const auto buildAs = reinterpret_cast<PFN_vkCmdBuildAccelerationStructuresKHR>(vkGetDeviceProcAddr(device, "vkCmdBuildAccelerationStructuresKHR"));
    const auto createPipelines = reinterpret_cast<PFN_vkCreateRayTracingPipelinesKHR>(vkGetDeviceProcAddr(device, "vkCreateRayTracingPipelinesKHR"));
    const auto getBufferAddress = reinterpret_cast<PFN_vkGetBufferDeviceAddressKHR>(vkGetDeviceProcAddr(device, "vkGetBufferDeviceAddressKHR"));

    if (!cmdTraceRays || !createAs || !buildAs || !createPipelines || !getBufferAddress)
    {
        diagnostic = "Required Vulkan RT device entry points are not all available.";
        vkDestroyDevice(device, nullptr);
        return false;
    }

    VkQueue queue = VK_NULL_HANDLE;
    vkGetDeviceQueue(device, queueFamilyIndex, 0, &queue);
    if (queue == VK_NULL_HANDLE)
    {
        diagnostic = "Selected queue family cannot return a queue handle.";
        vkDestroyDevice(device, nullptr);
        return false;
    }

    const VkCommandPoolCreateInfo poolCreateInfo{
        VK_STRUCTURE_TYPE_COMMAND_POOL_CREATE_INFO,
        nullptr,
        VK_COMMAND_POOL_CREATE_RESET_COMMAND_BUFFER_BIT,
        queueFamilyIndex
    };

    VkCommandPool commandPool = VK_NULL_HANDLE;
    if (vkCreateCommandPool(device, &poolCreateInfo, nullptr, &commandPool) != VK_SUCCESS)
    {
        diagnostic = "Failed to create Vulkan command pool for RT skeleton.";
        vkDestroyDevice(device, nullptr);
        return false;
    }

    VkCommandBufferAllocateInfo allocateInfo{
        VK_STRUCTURE_TYPE_COMMAND_BUFFER_ALLOCATE_INFO,
        nullptr,
        commandPool,
        VK_COMMAND_BUFFER_LEVEL_PRIMARY,
        1,
    };

    VkCommandBuffer commandBuffer = VK_NULL_HANDLE;
    if (vkAllocateCommandBuffers(device, &allocateInfo, &commandBuffer) != VK_SUCCESS || commandBuffer == VK_NULL_HANDLE)
    {
        diagnostic = "Failed to allocate command buffer for RT skeleton.";
        vkDestroyCommandPool(device, commandPool, nullptr);
        vkDestroyDevice(device, nullptr);
        return false;
    }

    VkCommandBufferBeginInfo beginInfo{
        VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO,
        nullptr,
        VK_COMMAND_BUFFER_USAGE_ONE_TIME_SUBMIT_BIT,
        nullptr
    };

    if (vkBeginCommandBuffer(commandBuffer, &beginInfo) != VK_SUCCESS)
    {
        diagnostic = "Failed to begin command buffer for RT skeleton.";
        vkFreeCommandBuffers(device, commandPool, 1, &commandBuffer);
        vkDestroyCommandPool(device, commandPool, nullptr);
        vkDestroyDevice(device, nullptr);
        return false;
    }

    const VkSubmitInfo submitInfo{
        VK_STRUCTURE_TYPE_SUBMIT_INFO,
        nullptr,
        0,
        nullptr,
        nullptr,
        1,
        &commandBuffer,
        0,
        nullptr,
    };

    if (vkEndCommandBuffer(commandBuffer) != VK_SUCCESS)
    {
        diagnostic = "Failed to end command buffer for RT skeleton.";
        vkFreeCommandBuffers(device, commandPool, 1, &commandBuffer);
        vkDestroyCommandPool(device, commandPool, nullptr);
        vkDestroyDevice(device, nullptr);
        return false;
    }

    if (vkQueueSubmit(queue, 1, &submitInfo, VK_NULL_HANDLE) != VK_SUCCESS)
    {
        diagnostic = "Failed to submit empty RT skeleton command buffer.";
        vkFreeCommandBuffers(device, commandPool, 1, &commandBuffer);
        vkDestroyCommandPool(device, commandPool, nullptr);
        vkDestroyDevice(device, nullptr);
        return false;
    }

    if (vkQueueWaitIdle(queue) != VK_SUCCESS)
    {
        diagnostic = "Queue wait failed while validating RT skeleton execution.";
        vkFreeCommandBuffers(device, commandPool, 1, &commandBuffer);
        vkDestroyCommandPool(device, commandPool, nullptr);
        vkDestroyDevice(device, nullptr);
        return false;
    }

    vkFreeCommandBuffers(device, commandPool, 1, &commandBuffer);
    vkDestroyCommandPool(device, commandPool, nullptr);
    vkDestroyDevice(device, nullptr);
    diagnostic.clear();
    return true;
}

} // namespace

TinyRtSceneSnapshot ExecuteTinyRtScene(const VkPhysicalDevice physicalDevice, const DeviceCapabilities& capabilities)
{
    TinyRtSceneSnapshot result{};
    result.geometry = "triangle";
    result.dispatchWidth = 0;
    result.dispatchHeight = 0;
    result.presented = false;

    if (capabilities.rtMode != RtMode::RayTracingPipeline)
    {
        result.status = kSceneStatusUnsupported;
        return result;
    }

    std::string diagnostic;
    if (!AreAccelerationStructurePreconditionsMet(physicalDevice, capabilities, diagnostic))
    {
        result.status = std::string(kSceneStatusUnsupported) + " (" + diagnostic + ")";
        return result;
    }

    if (!AreRayTracingPipelinePreconditionsMet(physicalDevice, capabilities, diagnostic))
    {
        result.status = std::string(kSceneStatusUnsupported) + " (" + diagnostic + ")";
        return result;
    }

    RtGeometryConfig sbtConfig{kTinySceneWidth, kTinySceneHeight};
    if (!ValidateShaderBindingTablePlan(sbtConfig, diagnostic))
    {
        result.status = std::string(kSceneStatusUnsupported) + " (" + diagnostic + ")";
        return result;
    }

    if (!CreateAndDestroyMinimalRayTracingDevice(physicalDevice, capabilities, diagnostic))
    {
        result.status = std::string(kSceneStatusNotAttempted) + " (" + diagnostic + ")";
        return result;
    }

    result.status = kSceneStatusDispatchImplemented;
    result.geometry = "triangle";
    result.dispatchWidth = kTinySceneWidth;
    result.dispatchHeight = kTinySceneHeight;

    return result;
}

} // namespace horde::vulkan::raytracing
