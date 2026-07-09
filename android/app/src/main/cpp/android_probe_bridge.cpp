#include <android/log.h>
#include <android/native_window.h>
#include <android/native_window_jni.h>

#include <algorithm>
#include <atomic>
#include <cmath>
#include <chrono>
#include <cerrno>
#include <cstring>
#include <fstream>
#include <mutex>
#include <string>
#include <thread>
#include <vector>
#include <sys/stat.h>

#include <jni.h>

#include <vulkan/vulkan.h>
#include <vulkan/vulkan_android.h>

#include "ui/DiagnosticOverlay.h"
#include "vulkan/RtCapabilityReport.h"
#include "vulkan/VulkanContext.h"
#include "vulkan/raytracing/PresentableTinyRtScene.h"

namespace
{

constexpr const char* kTag = "HordeRtProbeBridge";
constexpr const char* kReportDirectory = "reports";
constexpr const char* kTextReportFilename = "vulkan_capability_report.txt";
constexpr const char* kJsonReportFilename = "vulkan_capability_report.json";
// One frame in flight keeps the dynamically refit held-torch TLAS safely synchronized with its host-written instance buffer.
constexpr uint32_t kMaxFramesInFlight = 1u;
constexpr float kPlayerCollisionRadius = 0.24f;

struct CollisionRect
{
    float minX;
    float maxX;
    float minZ;
    float maxZ;
};

// These volumes match the two low arch posts in PresentableTinyRtScene.
constexpr CollisionRect kCorridorObstacles[] = {
    {-1.20f, -0.78f, -3.48f, -3.32f},
    {0.78f, 1.20f, -3.48f, -3.32f},
};

struct SwapchainContext
{
    ANativeWindow* window = nullptr;
    VkInstance instance = VK_NULL_HANDLE;
    VkPhysicalDevice physicalDevice = VK_NULL_HANDLE;
    VkDevice device = VK_NULL_HANDLE;
    VkQueue graphicsQueue = VK_NULL_HANDLE;
    uint32_t graphicsQueueFamilyIndex = 0u;
    VkSurfaceKHR surface = VK_NULL_HANDLE;
    VkSwapchainKHR swapchain = VK_NULL_HANDLE;
    VkFormat swapchainFormat = VK_FORMAT_B8G8R8A8_UNORM;
    VkColorSpaceKHR swapchainColorSpace = VK_COLOR_SPACE_SRGB_NONLINEAR_KHR;
    VkExtent2D swapchainExtent{};
    VkRenderPass renderPass = VK_NULL_HANDLE;
    std::vector<VkImage> swapchainImages;
    std::vector<VkImageLayout> swapchainImageLayouts;
    std::vector<VkImageView> swapchainImageViews;
    std::vector<VkFramebuffer> swapchainFramebuffers;
    VkCommandPool commandPool = VK_NULL_HANDLE;
    std::vector<VkCommandBuffer> commandBuffers;
    // Keep two slots allocated until the legacy cleanup path is simplified; only slot zero is active.
    VkSemaphore imageAvailableSemaphores[2u] = {};
    VkSemaphore renderFinishedSemaphores[2u] = {};
    VkFence inFlightFences[2u] = {};
    VkClearColorValue clearColor = {{0.12f, 0.04f, 0.18f, 1.0f}};
    horde::vulkan::DeviceCapabilities capabilities;
    horde::vulkan::raytracing::PresentableTinyRtScene rtScene;
    float cameraYaw = 0.0f;
    float cameraPitch = 0.0f;
    float lanternStrength = 1.8f;
    float walkTime = 0.0f;
    float cameraX = 0.0f;
    float cameraZ = 4.7f;
    float moveStrafe = 0.0f;
    float moveForward = 0.0f;
    std::string reportDirectory;
    bool useRtPath = false;
    uint32_t currentFrame = 0u;
};

SwapchainContext gSwapchainContext{};
std::atomic<bool> gSwapchainRunning{false};
std::thread gSwapchainThread;
std::mutex gSwapchainMutex;

std::string BuildDisplayText(const horde::vulkan::DeviceCapabilities& capabilities)
{
    if (capabilities.rtMode == horde::vulkan::RtMode::Unsupported)
    {
        return horde::ui::BuildUnsupportedDeviceText(capabilities);
    }
    return horde::ui::BuildDiagnosticOverlayText(capabilities);
}

std::string BuildReportDirectory(const std::string& baseDirectory)
{
    return baseDirectory + '/' + kReportDirectory;
}

bool EnsureDirectoryExists(const std::string& path)
{
    return mkdir(path.c_str(), 0755) == 0 || errno == EEXIST;
}

bool WriteTextFile(const std::string& path, const std::string& data)
{
    std::ofstream stream(path, std::ios::binary);
    if (!stream.good())
    {
        return false;
    }

    stream << data;
    return stream.good();
}

horde::vulkan::DeviceCapabilities RunProbe()
{
    horde::vulkan::VulkanContext context;
    context.InitialiseForCapabilityProbe();
    return context.QueryDeviceCapabilities();
}

VkClearColorValue ClearColorForMode(const horde::vulkan::RtMode mode)
{
    switch (mode)
    {
    case horde::vulkan::RtMode::RayTracingPipeline:
        return {{0.04f, 0.34f, 0.08f, 1.0f}};
    case horde::vulkan::RtMode::RayQuery:
        return {{0.14f, 0.07f, 0.42f, 1.0f}};
    default:
        return {{0.33f, 0.04f, 0.04f, 1.0f}};
    }
}

bool CreateInstance(VkInstance& instance)
{
    const char* extensions[] = {VK_KHR_SURFACE_EXTENSION_NAME, VK_KHR_ANDROID_SURFACE_EXTENSION_NAME};
    const VkApplicationInfo appInfo{
        VK_STRUCTURE_TYPE_APPLICATION_INFO,
        nullptr,
        "HordeLanternRTDiagnostic",
        VK_MAKE_VERSION(1, 0, 0),
        "horde_rt",
        VK_MAKE_VERSION(1, 0, 0),
        VK_API_VERSION_1_1};

    VkInstanceCreateInfo createInfo{};
    createInfo.sType = VK_STRUCTURE_TYPE_INSTANCE_CREATE_INFO;
    createInfo.pApplicationInfo = &appInfo;
    createInfo.enabledExtensionCount = static_cast<uint32_t>(std::size(extensions));
    createInfo.ppEnabledExtensionNames = extensions;

    const VkResult result = vkCreateInstance(&createInfo, nullptr, &instance);
    if (result != VK_SUCCESS)
    {
        __android_log_print(ANDROID_LOG_ERROR, kTag, "Failed to create Vulkan instance for Android diagnostic: VkResult(%d)", result);
        return false;
    }

    return true;
}

bool CreateSurface(VkInstance instance, ANativeWindow* window, VkSurfaceKHR& surface)
{
    VkAndroidSurfaceCreateInfoKHR createInfo{};
    createInfo.sType = VK_STRUCTURE_TYPE_ANDROID_SURFACE_CREATE_INFO_KHR;
    createInfo.window = window;

    const VkResult result = vkCreateAndroidSurfaceKHR(instance, &createInfo, nullptr, &surface);
    if (result != VK_SUCCESS)
    {
        __android_log_print(ANDROID_LOG_ERROR, kTag, "Failed to create Android Vulkan surface: VkResult(%d)", result);
        return false;
    }

    return true;
}

bool FindGraphicsAndPresentQueueFamily(VkPhysicalDevice physicalDevice, VkSurfaceKHR surface, uint32_t& queueFamilyIndex)
{
    queueFamilyIndex = 0u;
    uint32_t queueFamilyCount = 0u;
    vkGetPhysicalDeviceQueueFamilyProperties(physicalDevice, &queueFamilyCount, nullptr);
    if (queueFamilyCount == 0u)
    {
        return false;
    }

    std::vector<VkQueueFamilyProperties> queueFamilies(queueFamilyCount);
    vkGetPhysicalDeviceQueueFamilyProperties(physicalDevice, &queueFamilyCount, queueFamilies.data());
    for (uint32_t index = 0u; index < queueFamilyCount; ++index)
    {
        if ((queueFamilies[index].queueFlags & VK_QUEUE_GRAPHICS_BIT) == 0u)
        {
            continue;
        }

        VkBool32 presentSupport = VK_FALSE;
        vkGetPhysicalDeviceSurfaceSupportKHR(physicalDevice, index, surface, &presentSupport);
        if (presentSupport == VK_TRUE)
        {
            queueFamilyIndex = index;
            return true;
        }
    }

    return false;
}

bool HasDeviceExtension(VkPhysicalDevice physicalDevice, const char* extensionName)
{
    uint32_t extensionCount = 0u;
    if (vkEnumerateDeviceExtensionProperties(physicalDevice, nullptr, &extensionCount, nullptr) != VK_SUCCESS)
    {
        return false;
    }

    std::vector<VkExtensionProperties> extensions(extensionCount);
    vkEnumerateDeviceExtensionProperties(physicalDevice, nullptr, &extensionCount, extensions.data());
    for (const VkExtensionProperties& extension : extensions)
    {
        if (std::string(extension.extensionName) == extensionName)
        {
            return true;
        }
    }
    return false;
}

void PushPlayerOutOfRect(float& x, float& z, const CollisionRect& rect)
{
    const float minX = rect.minX - kPlayerCollisionRadius;
    const float maxX = rect.maxX + kPlayerCollisionRadius;
    const float minZ = rect.minZ - kPlayerCollisionRadius;
    const float maxZ = rect.maxZ + kPlayerCollisionRadius;
    if (x <= minX || x >= maxX || z <= minZ || z >= maxZ)
    {
        return;
    }

    const float pushLeft = x - minX;
    const float pushRight = maxX - x;
    const float pushBack = z - minZ;
    const float pushForward = maxZ - z;
    const float smallestPush = std::min({pushLeft, pushRight, pushBack, pushForward});
    if (smallestPush == pushLeft)
    {
        x = minX;
    }
    else if (smallestPush == pushRight)
    {
        x = maxX;
    }
    else if (smallestPush == pushBack)
    {
        z = minZ;
    }
    else
    {
        z = maxZ;
    }
}

void ResolvePlayerCollision(float& x, float& z)
{
    // The entrance remains open, but movement inside the corridor respects the stone walls.
    z = std::clamp(z, -4.75f, 4.90f);
    if (z <= 3.20f)
    {
        x = std::clamp(x, -1.58f, 1.58f);
        for (const CollisionRect& obstacle : kCorridorObstacles)
        {
            PushPlayerOutOfRect(x, z, obstacle);
        }
    }
    else
    {
        x = std::clamp(x, -2.40f, 2.40f);
    }
}

bool CreateLogicalDevice(VkPhysicalDevice physicalDevice,
                         uint32_t graphicsQueueFamilyIndex,
                         const horde::vulkan::DeviceCapabilities& capabilities,
                         VkDevice& device,
                         VkQueue& graphicsQueue)
{
    const float queuePriority = 1.0f;
    const VkDeviceQueueCreateInfo queueCreateInfo{
        VK_STRUCTURE_TYPE_DEVICE_QUEUE_CREATE_INFO,
        nullptr,
        0,
        graphicsQueueFamilyIndex,
        1u,
        &queuePriority};
    std::vector<const char*> extensions{VK_KHR_SWAPCHAIN_EXTENSION_NAME};
    const bool enableRayTracing = capabilities.rtMode == horde::vulkan::RtMode::RayTracingPipeline;
    if (enableRayTracing)
    {
        const char* rtExtensions[] = {
            VK_KHR_ACCELERATION_STRUCTURE_EXTENSION_NAME,
            VK_KHR_RAY_TRACING_PIPELINE_EXTENSION_NAME,
            VK_KHR_RAY_QUERY_EXTENSION_NAME,
            VK_KHR_BUFFER_DEVICE_ADDRESS_EXTENSION_NAME,
            VK_KHR_DEFERRED_HOST_OPERATIONS_EXTENSION_NAME,
            VK_KHR_SPIRV_1_4_EXTENSION_NAME,
            VK_KHR_SHADER_FLOAT_CONTROLS_EXTENSION_NAME};
        for (const char* extension : rtExtensions)
        {
            if (!HasDeviceExtension(physicalDevice, extension))
            {
                __android_log_print(ANDROID_LOG_ERROR, kTag, "Selected RayTracingPipeline device is missing required extension: %s", extension);
                return false;
            }
            extensions.push_back(extension);
        }
    }

    VkPhysicalDeviceAccelerationStructureFeaturesKHR accelerationStructureFeatures{
        VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_ACCELERATION_STRUCTURE_FEATURES_KHR};
    accelerationStructureFeatures.accelerationStructure = enableRayTracing ? VK_TRUE : VK_FALSE;
    VkPhysicalDeviceRayTracingPipelineFeaturesKHR rayTracingPipelineFeatures{
        VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_RAY_TRACING_PIPELINE_FEATURES_KHR};
    rayTracingPipelineFeatures.rayTracingPipeline = enableRayTracing ? VK_TRUE : VK_FALSE;
    VkPhysicalDeviceRayQueryFeaturesKHR rayQueryFeatures{
        VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_RAY_QUERY_FEATURES_KHR};
    rayQueryFeatures.rayQuery = enableRayTracing ? VK_TRUE : VK_FALSE;
    VkPhysicalDeviceBufferDeviceAddressFeaturesKHR bufferDeviceAddressFeatures{
        VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_BUFFER_DEVICE_ADDRESS_FEATURES_KHR};
    bufferDeviceAddressFeatures.bufferDeviceAddress = enableRayTracing ? VK_TRUE : VK_FALSE;
    VkPhysicalDeviceFeatures2 features2{VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_FEATURES_2};
    features2.pNext = &accelerationStructureFeatures;
    accelerationStructureFeatures.pNext = &rayTracingPipelineFeatures;
    rayTracingPipelineFeatures.pNext = &rayQueryFeatures;
    rayQueryFeatures.pNext = &bufferDeviceAddressFeatures;

    const VkDeviceCreateInfo createInfo{
        VK_STRUCTURE_TYPE_DEVICE_CREATE_INFO,
        enableRayTracing ? &features2 : nullptr,
        0,
        1u,
        &queueCreateInfo,
        0,
        nullptr,
        static_cast<uint32_t>(extensions.size()),
        extensions.data(),
        nullptr};

    const VkResult createResult = vkCreateDevice(physicalDevice, &createInfo, nullptr, &device);
    if (createResult != VK_SUCCESS)
    {
        __android_log_print(ANDROID_LOG_ERROR, kTag, "Failed to create Vulkan device for diagnostic surface: VkResult(%d)", createResult);
        return false;
    }

    vkGetDeviceQueue(device, graphicsQueueFamilyIndex, 0u, &graphicsQueue);
    return graphicsQueue != VK_NULL_HANDLE;
}

VkExtent2D ClampExtent(const VkSurfaceCapabilitiesKHR& capabilities, uint32_t desiredWidth, uint32_t desiredHeight)
{
    if (capabilities.currentExtent.width != UINT32_MAX && capabilities.currentExtent.height != UINT32_MAX)
    {
        return capabilities.currentExtent;
    }

    return {
        std::clamp(desiredWidth, capabilities.minImageExtent.width, capabilities.maxImageExtent.width),
        std::clamp(desiredHeight, capabilities.minImageExtent.height, capabilities.maxImageExtent.height)};
}

bool CreateSwapchain(SwapchainContext& context)
{
    VkSurfaceCapabilitiesKHR capabilities{};
    if (vkGetPhysicalDeviceSurfaceCapabilitiesKHR(context.physicalDevice, context.surface, &capabilities) != VK_SUCCESS)
    {
        __android_log_print(ANDROID_LOG_ERROR, kTag, "Failed to query Android surface capabilities.");
        return false;
    }

    uint32_t formatCount = 0u;
    if (vkGetPhysicalDeviceSurfaceFormatsKHR(context.physicalDevice, context.surface, &formatCount, nullptr) != VK_SUCCESS || formatCount == 0u)
    {
        return false;
    }

    std::vector<VkSurfaceFormatKHR> formats(formatCount);
    if (vkGetPhysicalDeviceSurfaceFormatsKHR(context.physicalDevice, context.surface, &formatCount, formats.data()) != VK_SUCCESS)
    {
        return false;
    }

    uint32_t presentModeCount = 0u;
    if (vkGetPhysicalDeviceSurfacePresentModesKHR(context.physicalDevice, context.surface, &presentModeCount, nullptr) != VK_SUCCESS ||
        presentModeCount == 0u)
    {
        return false;
    }

    std::vector<VkPresentModeKHR> presentModes(presentModeCount);
    if (vkGetPhysicalDeviceSurfacePresentModesKHR(context.physicalDevice, context.surface, &presentModeCount, presentModes.data()) != VK_SUCCESS)
    {
        return false;
    }

    VkSurfaceFormatKHR chosenFormat = formats[0];
    for (const VkSurfaceFormatKHR& candidate : formats)
    {
        if (candidate.format == VK_FORMAT_B8G8R8A8_UNORM && candidate.colorSpace == VK_COLOR_SPACE_SRGB_NONLINEAR_KHR)
        {
            chosenFormat = candidate;
            break;
        }
    }

    VkPresentModeKHR chosenPresentMode = VK_PRESENT_MODE_FIFO_KHR;
    for (const VkPresentModeKHR candidate : presentModes)
    {
        if (candidate == VK_PRESENT_MODE_MAILBOX_KHR)
        {
            chosenPresentMode = candidate;
            break;
        }
    }

    ANativeWindow* window = context.window;
    const uint32_t width = static_cast<uint32_t>(ANativeWindow_getWidth(window));
    const uint32_t height = static_cast<uint32_t>(ANativeWindow_getHeight(window));
    context.swapchainExtent = ClampExtent(capabilities, std::max(1u, width), std::max(1u, height));
    context.swapchainFormat = chosenFormat.format;
    context.swapchainColorSpace = chosenFormat.colorSpace;

    uint32_t imageCount = std::max(2u, capabilities.minImageCount);
    if (capabilities.maxImageCount > 0u && imageCount > capabilities.maxImageCount)
    {
        imageCount = capabilities.maxImageCount;
    }

    VkSwapchainCreateInfoKHR createInfo{
        VK_STRUCTURE_TYPE_SWAPCHAIN_CREATE_INFO_KHR,
        nullptr,
        0,
        context.surface,
        imageCount,
        context.swapchainFormat,
        context.swapchainColorSpace,
        context.swapchainExtent,
        1u,
        VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT | VK_IMAGE_USAGE_TRANSFER_DST_BIT,
        VK_SHARING_MODE_EXCLUSIVE,
        0,
        nullptr,
        capabilities.currentTransform,
        VK_COMPOSITE_ALPHA_OPAQUE_BIT_KHR,
        chosenPresentMode,
        VK_TRUE,
        VK_NULL_HANDLE};

    if (vkCreateSwapchainKHR(context.device, &createInfo, nullptr, &context.swapchain) != VK_SUCCESS)
    {
        __android_log_print(ANDROID_LOG_ERROR, kTag, "Failed to create swapchain.");
        return false;
    }

    uint32_t imageArraySize = 0u;
    if (vkGetSwapchainImagesKHR(context.device, context.swapchain, &imageArraySize, nullptr) != VK_SUCCESS || imageArraySize == 0u)
    {
        return false;
    }

    context.swapchainImages.resize(imageArraySize);
    if (vkGetSwapchainImagesKHR(context.device, context.swapchain, &imageArraySize, context.swapchainImages.data()) != VK_SUCCESS)
    {
        return false;
    }
    context.swapchainImageLayouts.assign(context.swapchainImages.size(), VK_IMAGE_LAYOUT_UNDEFINED);

    context.swapchainImageViews.resize(context.swapchainImages.size());
    for (size_t index = 0u; index < context.swapchainImages.size(); ++index)
    {
        const VkImageViewCreateInfo viewCreateInfo{
            VK_STRUCTURE_TYPE_IMAGE_VIEW_CREATE_INFO,
            nullptr,
            0,
            context.swapchainImages[index],
            VK_IMAGE_VIEW_TYPE_2D,
            context.swapchainFormat,
            {VK_COMPONENT_SWIZZLE_IDENTITY, VK_COMPONENT_SWIZZLE_IDENTITY, VK_COMPONENT_SWIZZLE_IDENTITY,
             VK_COMPONENT_SWIZZLE_IDENTITY},
            {VK_IMAGE_ASPECT_COLOR_BIT, 0u, 1u, 0u, 1u}};
        if (vkCreateImageView(context.device, &viewCreateInfo, nullptr, &context.swapchainImageViews[index]) != VK_SUCCESS)
        {
            return false;
        }
    }

    const VkAttachmentDescription colorAttachment{
        0,
        context.swapchainFormat,
        VK_SAMPLE_COUNT_1_BIT,
        VK_ATTACHMENT_LOAD_OP_CLEAR,
        VK_ATTACHMENT_STORE_OP_STORE,
        VK_ATTACHMENT_LOAD_OP_DONT_CARE,
        VK_ATTACHMENT_STORE_OP_DONT_CARE,
        VK_IMAGE_LAYOUT_UNDEFINED,
        VK_IMAGE_LAYOUT_PRESENT_SRC_KHR};

    const VkAttachmentReference colorAttachmentReference{0u, VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL};
    const VkSubpassDescription subpass{
        0,
        VK_PIPELINE_BIND_POINT_GRAPHICS,
        0u,
        nullptr,
        1u,
        &colorAttachmentReference,
        nullptr,
        nullptr,
        0u,
        nullptr};

    const VkAttachmentDescription attachmentArray[] = {colorAttachment};
    const VkSubpassDescription subpassArray[] = {subpass};
    VkSubpassDependency dependency{};
    dependency.srcSubpass = VK_SUBPASS_EXTERNAL;
    dependency.dstSubpass = 0u;
    dependency.srcStageMask = VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT;
    dependency.dstStageMask = VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT;
    dependency.dstAccessMask = VK_ACCESS_COLOR_ATTACHMENT_WRITE_BIT;
    dependency.srcAccessMask = 0u;
    dependency.dependencyFlags = VK_DEPENDENCY_BY_REGION_BIT;

    const VkRenderPassCreateInfo renderPassCreateInfo{
        VK_STRUCTURE_TYPE_RENDER_PASS_CREATE_INFO,
        nullptr,
        0,
        static_cast<uint32_t>(std::size(attachmentArray)),
        attachmentArray,
        1u,
        subpassArray,
        1u,
        &dependency};

    if (vkCreateRenderPass(context.device, &renderPassCreateInfo, nullptr, &context.renderPass) != VK_SUCCESS)
    {
        __android_log_print(ANDROID_LOG_ERROR, kTag, "Failed to create render pass.");
        return false;
    }

    context.swapchainFramebuffers.resize(context.swapchainImageViews.size());
    for (size_t index = 0u; index < context.swapchainImageViews.size(); ++index)
    {
        VkImageView attachments[] = {context.swapchainImageViews[index]};
        const VkFramebufferCreateInfo framebufferCreateInfo{
            VK_STRUCTURE_TYPE_FRAMEBUFFER_CREATE_INFO,
            nullptr,
            0,
            context.renderPass,
            1u,
            attachments,
            context.swapchainExtent.width,
            context.swapchainExtent.height,
            1u};
        if (vkCreateFramebuffer(context.device, &framebufferCreateInfo, nullptr, &context.swapchainFramebuffers[index]) != VK_SUCCESS)
        {
            return false;
        }
    }

    const VkCommandPoolCreateInfo commandPoolCreateInfo{
        VK_STRUCTURE_TYPE_COMMAND_POOL_CREATE_INFO,
        nullptr,
        VK_COMMAND_POOL_CREATE_RESET_COMMAND_BUFFER_BIT,
        context.graphicsQueueFamilyIndex};

    if (vkCreateCommandPool(context.device, &commandPoolCreateInfo, nullptr, &context.commandPool) != VK_SUCCESS)
    {
        return false;
    }

    VkCommandBufferAllocateInfo commandBufferAllocateInfo{
        VK_STRUCTURE_TYPE_COMMAND_BUFFER_ALLOCATE_INFO,
        nullptr,
        context.commandPool,
        VK_COMMAND_BUFFER_LEVEL_PRIMARY,
        static_cast<uint32_t>(context.swapchainImageViews.size())};
    context.commandBuffers.resize(context.swapchainImageViews.size());
    if (vkAllocateCommandBuffers(context.device, &commandBufferAllocateInfo, context.commandBuffers.data()) != VK_SUCCESS)
    {
        return false;
    }

    context.imageAvailableSemaphores[0u] = VK_NULL_HANDLE;
    context.imageAvailableSemaphores[1u] = VK_NULL_HANDLE;
    context.renderFinishedSemaphores[0u] = VK_NULL_HANDLE;
    context.renderFinishedSemaphores[1u] = VK_NULL_HANDLE;
    context.inFlightFences[0u] = VK_NULL_HANDLE;
    context.inFlightFences[1u] = VK_NULL_HANDLE;
    VkSemaphoreCreateInfo semaphoreCreateInfo{VK_STRUCTURE_TYPE_SEMAPHORE_CREATE_INFO};
    VkFenceCreateInfo fenceCreateInfo{VK_STRUCTURE_TYPE_FENCE_CREATE_INFO, nullptr, VK_FENCE_CREATE_SIGNALED_BIT};
    for (uint32_t i = 0u; i < kMaxFramesInFlight; ++i)
    {
        if (vkCreateSemaphore(context.device, &semaphoreCreateInfo, nullptr, &context.imageAvailableSemaphores[i]) != VK_SUCCESS ||
            vkCreateSemaphore(context.device, &semaphoreCreateInfo, nullptr, &context.renderFinishedSemaphores[i]) != VK_SUCCESS ||
            vkCreateFence(context.device, &fenceCreateInfo, nullptr, &context.inFlightFences[i]) != VK_SUCCESS)
        {
            return false;
        }
    }

    return true;
}

VkPhysicalDevice FindMatchingPhysicalDevice(
    const VkInstance instance,
    const horde::vulkan::DeviceCapabilities& capabilities,
    const VkSurfaceKHR surface)
{
    uint32_t physicalDeviceCount = 0u;
    if (vkEnumeratePhysicalDevices(instance, &physicalDeviceCount, nullptr) != VK_SUCCESS || physicalDeviceCount == 0u)
    {
        return VK_NULL_HANDLE;
    }

    std::vector<VkPhysicalDevice> physicalDevices(physicalDeviceCount);
    vkEnumeratePhysicalDevices(instance, &physicalDeviceCount, physicalDevices.data());

    for (const VkPhysicalDevice candidate : physicalDevices)
    {
        VkPhysicalDeviceProperties properties{};
        vkGetPhysicalDeviceProperties(candidate, &properties);
        if (properties.vendorID == capabilities.identity.vendorId &&
            properties.deviceID == capabilities.identity.deviceId &&
            capabilities.identity.gpuName == properties.deviceName)
        {
            uint32_t queueFamilyIndex = 0u;
            if (FindGraphicsAndPresentQueueFamily(candidate, surface, queueFamilyIndex))
            {
                return candidate;
            }
        }
    }

    for (const VkPhysicalDevice candidate : physicalDevices)
    {
        uint32_t queueFamilyIndex = 0u;
        if (FindGraphicsAndPresentQueueFamily(candidate, surface, queueFamilyIndex))
        {
            return candidate;
        }
    }

    return VK_NULL_HANDLE;
}

void ReleaseSwapchainResources(SwapchainContext& context)
{
    if (context.device == VK_NULL_HANDLE)
    {
        return;
    }

    vkDeviceWaitIdle(context.device);
    context.rtScene.Destroy();

    if (context.commandPool != VK_NULL_HANDLE)
    {
        if (!context.commandBuffers.empty())
        {
            vkFreeCommandBuffers(context.device,
                                 context.commandPool,
                                 static_cast<uint32_t>(context.commandBuffers.size()),
                                 context.commandBuffers.data());
            context.commandBuffers.clear();
        }
        vkDestroyCommandPool(context.device, context.commandPool, nullptr);
        context.commandPool = VK_NULL_HANDLE;
    }

    for (VkFramebuffer framebuffer : context.swapchainFramebuffers)
    {
        if (framebuffer != VK_NULL_HANDLE)
        {
            vkDestroyFramebuffer(context.device, framebuffer, nullptr);
        }
    }
    context.swapchainFramebuffers.clear();

    for (VkImageView imageView : context.swapchainImageViews)
    {
        if (imageView != VK_NULL_HANDLE)
        {
            vkDestroyImageView(context.device, imageView, nullptr);
        }
    }
    context.swapchainImageViews.clear();

    if (context.renderPass != VK_NULL_HANDLE)
    {
        vkDestroyRenderPass(context.device, context.renderPass, nullptr);
        context.renderPass = VK_NULL_HANDLE;
    }

    if (context.swapchain != VK_NULL_HANDLE)
    {
        vkDestroySwapchainKHR(context.device, context.swapchain, nullptr);
        context.swapchain = VK_NULL_HANDLE;
    }

    if (context.inFlightFences[0] != VK_NULL_HANDLE)
    {
        vkDestroyFence(context.device, context.inFlightFences[0], nullptr);
        context.inFlightFences[0] = VK_NULL_HANDLE;
    }
    if (context.inFlightFences[1] != VK_NULL_HANDLE)
    {
        vkDestroyFence(context.device, context.inFlightFences[1], nullptr);
        context.inFlightFences[1] = VK_NULL_HANDLE;
    }
    if (context.imageAvailableSemaphores[0] != VK_NULL_HANDLE)
    {
        vkDestroySemaphore(context.device, context.imageAvailableSemaphores[0], nullptr);
        context.imageAvailableSemaphores[0] = VK_NULL_HANDLE;
    }
    if (context.imageAvailableSemaphores[1] != VK_NULL_HANDLE)
    {
        vkDestroySemaphore(context.device, context.imageAvailableSemaphores[1], nullptr);
        context.imageAvailableSemaphores[1] = VK_NULL_HANDLE;
    }
    if (context.renderFinishedSemaphores[0] != VK_NULL_HANDLE)
    {
        vkDestroySemaphore(context.device, context.renderFinishedSemaphores[0], nullptr);
        context.renderFinishedSemaphores[0] = VK_NULL_HANDLE;
    }
    if (context.renderFinishedSemaphores[1] != VK_NULL_HANDLE)
    {
        vkDestroySemaphore(context.device, context.renderFinishedSemaphores[1], nullptr);
        context.renderFinishedSemaphores[1] = VK_NULL_HANDLE;
    }

    context.swapchainImageLayouts.clear();
    context.swapchainImages.clear();
    context.currentFrame = 0u;
}

bool InitialiseRtSceneForSwapchain(SwapchainContext& context)
{
    if (!context.useRtPath)
    {
        return true;
    }

    std::string diagnostic;
    if (!context.rtScene.Initialise(context.instance,
                                    context.physicalDevice,
                                    context.device,
                                    context.graphicsQueue,
                                    context.commandPool,
                                    context.swapchainExtent,
                                    context.swapchainFormat,
                                    diagnostic))
    {
        __android_log_print(ANDROID_LOG_ERROR, kTag, "Failed to initialise presentable RT scene: %s", diagnostic.c_str());
        return false;
    }

    return true;
}

bool RecreateSwapchain(SwapchainContext& context)
{
    ReleaseSwapchainResources(context);
    return CreateSwapchain(context) && InitialiseRtSceneForSwapchain(context);
}

void DestroySwapchainContext(SwapchainContext& context)
{
    if (context.device == VK_NULL_HANDLE)
    {
        if (context.surface != VK_NULL_HANDLE && context.instance != VK_NULL_HANDLE)
        {
            vkDestroySurfaceKHR(context.instance, context.surface, nullptr);
        }
        if (context.instance != VK_NULL_HANDLE)
        {
            vkDestroyInstance(context.instance, nullptr);
        }
        if (context.window != nullptr)
        {
            ANativeWindow_release(context.window);
            context.window = nullptr;
        }
        context = {};
        return;
    }

    vkDeviceWaitIdle(context.device);
    context.rtScene.Destroy();
    for (VkSemaphore semaphore : context.imageAvailableSemaphores)
    {
        if (semaphore != VK_NULL_HANDLE)
        {
            vkDestroySemaphore(context.device, semaphore, nullptr);
        }
    }
    for (VkSemaphore semaphore : context.renderFinishedSemaphores)
    {
        if (semaphore != VK_NULL_HANDLE)
        {
            vkDestroySemaphore(context.device, semaphore, nullptr);
        }
    }

    for (VkFence fence : context.inFlightFences)
    {
        if (fence != VK_NULL_HANDLE)
        {
            vkDestroyFence(context.device, fence, nullptr);
        }
    }

    if (context.commandPool != VK_NULL_HANDLE)
    {
        vkFreeCommandBuffers(context.device, context.commandPool, static_cast<uint32_t>(context.commandBuffers.size()), context.commandBuffers.data());
        vkDestroyCommandPool(context.device, context.commandPool, nullptr);
    }
    for (VkFramebuffer framebuffer : context.swapchainFramebuffers)
    {
        if (framebuffer != VK_NULL_HANDLE)
        {
            vkDestroyFramebuffer(context.device, framebuffer, nullptr);
        }
    }
    for (VkImageView imageView : context.swapchainImageViews)
    {
        if (imageView != VK_NULL_HANDLE)
        {
            vkDestroyImageView(context.device, imageView, nullptr);
        }
    }
    if (context.renderPass != VK_NULL_HANDLE)
    {
        vkDestroyRenderPass(context.device, context.renderPass, nullptr);
    }
    if (context.swapchain != VK_NULL_HANDLE)
    {
        vkDestroySwapchainKHR(context.device, context.swapchain, nullptr);
    }
    vkDestroyDevice(context.device, nullptr);
    if (context.surface != VK_NULL_HANDLE && context.instance != VK_NULL_HANDLE)
    {
        vkDestroySurfaceKHR(context.instance, context.surface, nullptr);
    }
    if (context.instance != VK_NULL_HANDLE)
    {
        vkDestroyInstance(context.instance, nullptr);
    }
    if (context.window != nullptr)
    {
        ANativeWindow_release(context.window);
        context.window = nullptr;
    }

    context = {};
}

bool RenderFrame(SwapchainContext& context, bool& rtFramePresented)
{
    rtFramePresented = false;
    if (context.commandBuffers.empty())
    {
        return false;
    }

    const VkResult waitResult = vkWaitForFences(context.device, 1u, &context.inFlightFences[context.currentFrame], VK_TRUE, UINT64_MAX);
    if (waitResult != VK_SUCCESS && waitResult != VK_TIMEOUT)
    {
        return false;
    }

    uint32_t imageIndex = 0u;
    const VkResult acquireResult = vkAcquireNextImageKHR(
        context.device,
        context.swapchain,
        UINT64_MAX,
        context.imageAvailableSemaphores[context.currentFrame],
        VK_NULL_HANDLE,
        &imageIndex);

    if (acquireResult == VK_ERROR_OUT_OF_DATE_KHR || acquireResult == VK_SUBOPTIMAL_KHR)
    {
        return RecreateSwapchain(context);
    }
    if (acquireResult != VK_SUCCESS)
    {
        return false;
    }

    VkCommandBufferResetFlags resetFlags = 0;
    if (vkResetCommandBuffer(context.commandBuffers[imageIndex], resetFlags) != VK_SUCCESS)
    {
        return false;
    }

    const VkCommandBufferBeginInfo beginInfo{
        VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO,
        nullptr,
        VK_COMMAND_BUFFER_USAGE_ONE_TIME_SUBMIT_BIT,
        nullptr};

    if (vkBeginCommandBuffer(context.commandBuffers[imageIndex], &beginInfo) != VK_SUCCESS)
    {
        return false;
    }

    const bool useRtFrame = context.useRtPath && context.rtScene.IsReady();
    if (useRtFrame)
    {
        context.walkTime += 0.045f;
        const float moveAmount = std::clamp(std::abs(context.moveForward) + std::abs(context.moveStrafe), 0.0f, 1.0f);
        if (moveAmount > 0.02f)
        {
            const float forwardX = std::sin(context.cameraYaw);
            const float forwardZ = -std::cos(context.cameraYaw);
            const float rightX = std::cos(context.cameraYaw);
            const float rightZ = std::sin(context.cameraYaw);
            const float speed = 0.032f;
            context.cameraX += (forwardX * context.moveForward + rightX * context.moveStrafe) * speed;
            context.cameraZ += (forwardZ * context.moveForward + rightZ * context.moveStrafe) * speed;
            ResolvePlayerCollision(context.cameraX, context.cameraZ);
        }
        std::string diagnostic;
        if (!context.rtScene.RecordTraceAndCopy(context.commandBuffers[imageIndex],
                                                context.swapchainImages[imageIndex],
                                                context.swapchainImageLayouts[imageIndex],
                                                context.swapchainExtent,
                                                context.cameraYaw,
                                                context.cameraPitch,
                                                context.lanternStrength,
                                                context.walkTime,
                                                context.cameraX,
                                                context.cameraZ,
                                                moveAmount,
                                                diagnostic))
        {
            __android_log_print(ANDROID_LOG_ERROR, kTag, "Failed to record RT frame: %s", diagnostic.c_str());
            return false;
        }
    }
    else
    {
        const VkClearValue clearValue = {context.clearColor};
        const VkRenderPassBeginInfo renderPassBegin{
            VK_STRUCTURE_TYPE_RENDER_PASS_BEGIN_INFO,
            nullptr,
            context.renderPass,
            context.swapchainFramebuffers[imageIndex],
            {{0, 0}, context.swapchainExtent},
            1u,
            &clearValue};
        vkCmdBeginRenderPass(context.commandBuffers[imageIndex], &renderPassBegin, VK_SUBPASS_CONTENTS_INLINE);
        vkCmdEndRenderPass(context.commandBuffers[imageIndex]);
        context.swapchainImageLayouts[imageIndex] = VK_IMAGE_LAYOUT_PRESENT_SRC_KHR;
    }

    if (vkEndCommandBuffer(context.commandBuffers[imageIndex]) != VK_SUCCESS)
    {
        return false;
    }

    if (vkResetFences(context.device, 1u, &context.inFlightFences[context.currentFrame]) != VK_SUCCESS)
    {
        return false;
    }

    VkPipelineStageFlags waitStages = useRtFrame ? VK_PIPELINE_STAGE_TRANSFER_BIT : VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT;
    const VkSubmitInfo submitInfo{
        VK_STRUCTURE_TYPE_SUBMIT_INFO,
        nullptr,
        1u,
        &context.imageAvailableSemaphores[context.currentFrame],
        &waitStages,
        1u,
        &context.commandBuffers[imageIndex],
        1u,
        &context.renderFinishedSemaphores[context.currentFrame]};

    if (vkQueueSubmit(context.graphicsQueue, 1u, &submitInfo, context.inFlightFences[context.currentFrame]) != VK_SUCCESS)
    {
        return false;
    }

    VkPresentInfoKHR presentInfo{};
    presentInfo.sType = VK_STRUCTURE_TYPE_PRESENT_INFO_KHR;
    presentInfo.waitSemaphoreCount = 1u;
    presentInfo.pWaitSemaphores = &context.renderFinishedSemaphores[context.currentFrame];
    presentInfo.swapchainCount = 1u;
    presentInfo.pSwapchains = &context.swapchain;
    presentInfo.pImageIndices = &imageIndex;
    const VkResult presentResult = vkQueuePresentKHR(context.graphicsQueue, &presentInfo);
    if (presentResult == VK_ERROR_OUT_OF_DATE_KHR || presentResult == VK_SUBOPTIMAL_KHR)
    {
        return RecreateSwapchain(context);
    }
    if (presentResult != VK_SUCCESS)
    {
        return false;
    }

    rtFramePresented = useRtFrame;
    context.currentFrame = (context.currentFrame + 1u) % kMaxFramesInFlight;
    return true;
}

void SwapchainRenderLoop()
{
    while (gSwapchainRunning.load(std::memory_order_acquire))
    {
        bool rtFramePresented = false;
        if (!RenderFrame(gSwapchainContext, rtFramePresented))
        {
            __android_log_print(ANDROID_LOG_ERROR, kTag, "Diagnostic surface render loop ended unexpectedly.");
            break;
        }
        if (rtFramePresented && !gSwapchainContext.capabilities.rtScene.presented)
        {
            gSwapchainContext.capabilities.rtScene.presented = true;
            gSwapchainContext.capabilities.rtScene.status = "Presented via swapchain";
            gSwapchainContext.capabilities.rtScene.geometry = "Horde Lantern corridor demo scene";
            gSwapchainContext.capabilities.rtScene.dispatchWidth = gSwapchainContext.rtScene.DispatchExtent().width;
            gSwapchainContext.capabilities.rtScene.dispatchHeight = gSwapchainContext.rtScene.DispatchExtent().height;
            gSwapchainContext.capabilities.performance.internalRenderWidth = gSwapchainContext.capabilities.rtScene.dispatchWidth;
            gSwapchainContext.capabilities.performance.internalRenderHeight = gSwapchainContext.capabilities.rtScene.dispatchHeight;

            WriteTextFile(gSwapchainContext.reportDirectory + '/' + kTextReportFilename, BuildDisplayText(gSwapchainContext.capabilities));
            WriteTextFile(gSwapchainContext.reportDirectory + '/' + kJsonReportFilename, horde::vulkan::BuildCapabilityJsonReport(gSwapchainContext.capabilities));
            __android_log_print(ANDROID_LOG_INFO, kTag, "RT frame reached Android swapchain presentation.");
        }
        std::this_thread::sleep_for(std::chrono::milliseconds(16));
    }

    gSwapchainRunning.store(false, std::memory_order_release);

    SwapchainContext cleanup = std::move(gSwapchainContext);
    gSwapchainContext = {};
    DestroySwapchainContext(cleanup);
}

bool StartSurfaceInternal(ANativeWindow* window,
                          horde::vulkan::DeviceCapabilities capabilities,
                          const std::string& reportDirectory)
{
    if (window == nullptr)
    {
        __android_log_print(ANDROID_LOG_ERROR, kTag, "startDiagnosticSurface called with null window.");
        return false;
    }

    if (gSwapchainRunning.load(std::memory_order_acquire))
    {
        __android_log_print(ANDROID_LOG_WARN, kTag, "Diagnostic surface already running.");
        ANativeWindow_release(window);
        return false;
    }

    SwapchainContext context;
    context.window = window;
    context.capabilities = capabilities;
    context.reportDirectory = reportDirectory;
    context.useRtPath = capabilities.rtMode == horde::vulkan::RtMode::RayTracingPipeline;
    context.clearColor = ClearColorForMode(capabilities.rtMode);

    if (!CreateInstance(context.instance))
    {
        DestroySwapchainContext(context);
        return false;
    }

    if (!CreateSurface(context.instance, context.window, context.surface))
    {
        DestroySwapchainContext(context);
        return false;
    }

    context.physicalDevice = FindMatchingPhysicalDevice(context.instance, capabilities, context.surface);
    if (context.physicalDevice == VK_NULL_HANDLE)
    {
        __android_log_print(ANDROID_LOG_ERROR, kTag, "No matching physical device found for Android diagnostic surface.");
        DestroySwapchainContext(context);
        return false;
    }

    if (!FindGraphicsAndPresentQueueFamily(context.physicalDevice, context.surface, context.graphicsQueueFamilyIndex))
    {
        __android_log_print(ANDROID_LOG_ERROR, kTag, "Could not find graphics+present queue family on Android.");
        DestroySwapchainContext(context);
        return false;
    }

    if (!CreateLogicalDevice(context.physicalDevice, context.graphicsQueueFamilyIndex, capabilities, context.device, context.graphicsQueue))
    {
        DestroySwapchainContext(context);
        return false;
    }

    if (!CreateSwapchain(context))
    {
        DestroySwapchainContext(context);
        return false;
    }
    if (!InitialiseRtSceneForSwapchain(context))
    {
        DestroySwapchainContext(context);
        return false;
    }

    gSwapchainContext = std::move(context);
    gSwapchainRunning.store(true, std::memory_order_release);
    gSwapchainThread = std::thread(SwapchainRenderLoop);

    return true;
}

void StopSurfaceInternal()
{
    const bool wasRunning = gSwapchainRunning.exchange(false, std::memory_order_acq_rel);
    if (!wasRunning)
    {
        if (gSwapchainThread.joinable())
        {
            gSwapchainThread.join();
        }
        return;
    }

    if (gSwapchainThread.joinable())
    {
        gSwapchainThread.join();
    }
}

} // namespace

extern "C" JNIEXPORT jstring JNICALL
Java_com_samfa12_hordelanternrt_ProbeBridge_getTextReport(JNIEnv* env, jclass)
{
    const horde::vulkan::DeviceCapabilities capabilities = RunProbe();
    const std::string reportText = BuildDisplayText(capabilities);
    return env->NewStringUTF(reportText.c_str());
}

extern "C" JNIEXPORT jstring JNICALL
Java_com_samfa12_hordelanternrt_ProbeBridge_getJsonReport(JNIEnv* env, jclass)
{
    const horde::vulkan::DeviceCapabilities capabilities = RunProbe();
    const std::string reportJson = horde::vulkan::BuildCapabilityJsonReport(capabilities);
    return env->NewStringUTF(reportJson.c_str());
}

extern "C" JNIEXPORT jboolean JNICALL
Java_com_samfa12_hordelanternrt_ProbeBridge_writeReports(JNIEnv* env, jclass, jstring baseDirectory)
{
    const char* baseDirectoryUtf = env->GetStringUTFChars(baseDirectory, nullptr);
    if (!baseDirectoryUtf)
    {
        return JNI_FALSE;
    }

    const std::string baseDirectoryValue(baseDirectoryUtf);
    env->ReleaseStringUTFChars(baseDirectory, baseDirectoryUtf);

    const horde::vulkan::DeviceCapabilities capabilities = RunProbe();
    const std::string textReport = BuildDisplayText(capabilities);
    const std::string jsonReport = horde::vulkan::BuildCapabilityJsonReport(capabilities);

    const std::string reportDirectory = BuildReportDirectory(baseDirectoryValue);
    if (!EnsureDirectoryExists(reportDirectory))
    {
        __android_log_print(ANDROID_LOG_ERROR, kTag, "Failed to create report directory: %s", reportDirectory.c_str());
        return JNI_FALSE;
    }

    if (!WriteTextFile(reportDirectory + '/' + kTextReportFilename, textReport))
    {
        __android_log_print(ANDROID_LOG_ERROR, kTag, "Failed to write text report file.");
        return JNI_FALSE;
    }

    if (!WriteTextFile(reportDirectory + '/' + kJsonReportFilename, jsonReport))
    {
        __android_log_print(ANDROID_LOG_ERROR, kTag, "Failed to write JSON report file.");
        return JNI_FALSE;
    }

    return JNI_TRUE;
}

extern "C" JNIEXPORT jboolean JNICALL
Java_com_samfa12_hordelanternrt_ProbeBridge_startDiagnosticSurface(JNIEnv* env, jclass, jobject surface, jstring baseDirectory)
{
    if (surface == nullptr)
    {
        __android_log_print(ANDROID_LOG_ERROR, kTag, "startDiagnosticSurface called with null Java Surface.");
        return JNI_FALSE;
    }

    if (baseDirectory == nullptr)
    {
        __android_log_print(ANDROID_LOG_ERROR, kTag, "startDiagnosticSurface called with null base directory.");
        return JNI_FALSE;
    }

    const char* baseDirectoryUtf = env->GetStringUTFChars(baseDirectory, nullptr);
    if (!baseDirectoryUtf)
    {
        return JNI_FALSE;
    }

    const std::string baseDirectoryValue(baseDirectoryUtf);
    env->ReleaseStringUTFChars(baseDirectory, baseDirectoryUtf);

    const std::string reportDirectory = BuildReportDirectory(baseDirectoryValue);
    if (!EnsureDirectoryExists(reportDirectory))
    {
        __android_log_print(ANDROID_LOG_ERROR, kTag, "Failed to create report directory: %s", reportDirectory.c_str());
        return JNI_FALSE;
    }

    ANativeWindow* window = ANativeWindow_fromSurface(env, surface);
    if (!window)
    {
        __android_log_print(ANDROID_LOG_ERROR, kTag, "Failed to resolve ANativeWindow.");
        return JNI_FALSE;
    }

    std::lock_guard<std::mutex> lock(gSwapchainMutex);
    const horde::vulkan::DeviceCapabilities capabilities = RunProbe();

    // Write updated report from shared probe source.
    const std::string textReport = BuildDisplayText(capabilities);
    const std::string jsonReport = horde::vulkan::BuildCapabilityJsonReport(capabilities);
    if (!WriteTextFile(reportDirectory + '/' + kTextReportFilename, textReport) ||
        !WriteTextFile(reportDirectory + '/' + kJsonReportFilename, jsonReport))
    {
        ANativeWindow_release(window);
        return JNI_FALSE;
    }

    if (!StartSurfaceInternal(window, capabilities, reportDirectory))
    {
        return JNI_FALSE;
    }

    __android_log_print(ANDROID_LOG_INFO, kTag, "Started Android diagnostic surface rendering loop.");
    return JNI_TRUE;
}

extern "C" JNIEXPORT void JNICALL
Java_com_samfa12_hordelanternrt_ProbeBridge_stopDiagnosticSurface(JNIEnv*, jclass)
{
    std::lock_guard<std::mutex> lock(gSwapchainMutex);
    StopSurfaceInternal();
}

extern "C" JNIEXPORT void JNICALL
Java_com_samfa12_hordelanternrt_ProbeBridge_setViewControls(JNIEnv*, jclass, jfloat yaw, jfloat pitch, jfloat lanternStrength, jfloat moveStrafe, jfloat moveForward)
{
    std::lock_guard<std::mutex> lock(gSwapchainMutex);
    gSwapchainContext.cameraYaw = static_cast<float>(yaw);
    gSwapchainContext.cameraPitch = std::clamp(static_cast<float>(pitch), -0.32f, 0.28f);
    gSwapchainContext.lanternStrength = std::clamp(static_cast<float>(lanternStrength), 0.65f, 2.4f);
    gSwapchainContext.moveStrafe = std::clamp(static_cast<float>(moveStrafe), -1.0f, 1.0f);
    gSwapchainContext.moveForward = std::clamp(static_cast<float>(moveForward), -1.0f, 1.0f);
}
