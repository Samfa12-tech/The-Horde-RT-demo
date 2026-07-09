#include "platform/windows/DiagnosticWindow.h"

#include <algorithm>
#include <filesystem>
#include <fstream>
#include <iostream>
#include <string>
#include <vector>

#ifndef WIN32_LEAN_AND_MEAN
#define WIN32_LEAN_AND_MEAN
#endif
#ifndef NOMINMAX
#define NOMINMAX
#endif
#include <windows.h>
#ifdef DeviceCapabilities
#undef DeviceCapabilities
#endif
#include <vulkan/vulkan.h>
#include <vulkan/vulkan_win32.h>

#include "ui/DiagnosticOverlay.h"
#include "vulkan/RtCapabilityReport.h"
#include "vulkan/VulkanContext.h"
#include "vulkan/raytracing/PresentableTinyRtScene.h"

namespace
{

constexpr char kWindowClassName[] = "HordeRtDiagnosticWindowClass";
constexpr char kWindowTitle[] = "Horde Lantern RT Diagnostic";
constexpr char kReportDirectory[] = "reports";
constexpr char kTextReportFilename[] = "vulkan_capability_report.txt";
constexpr char kJsonReportFilename[] = "vulkan_capability_report.json";
constexpr int kEditControlId = 101;
constexpr UINT kMaxFramesInFlight = 2u;

struct VulkanSurfaceContext
{
    HWND windowHandle = nullptr;
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
    std::vector<VkSemaphore> imageAvailableSemaphores;
    std::vector<VkSemaphore> renderFinishedSemaphores;
    std::vector<VkFence> inFlightFences;
    horde::vulkan::raytracing::PresentableTinyRtScene rtScene;
    bool useRtPath = false;
    uint32_t currentFrame = 0u;
};

bool WriteReportFile(const std::filesystem::path& path, const std::string& data);

std::string BuildDisplayText(const horde::vulkan::DeviceCapabilities& capabilities)
{
    if (capabilities.rtMode == horde::vulkan::RtMode::Unsupported)
    {
        return horde::ui::BuildUnsupportedDeviceText(capabilities);
    }
    return horde::ui::BuildDiagnosticOverlayText(capabilities);
}

std::string WindowSafeText(const std::string& value)
{
    std::string replaced = value;
    std::string out;
    out.reserve(replaced.size());
    for (const char c : replaced)
    {
        if (c == '\n')
        {
            out += "\r\n";
        }
        else
        {
            out += c;
        }
    }
    return out;
}

std::string MakeWindowTitle(const std::string& diagnosticText)
{
    std::string title = diagnosticText;
    if (title.empty())
    {
        return kWindowTitle;
    }

    const size_t firstNewLine = title.find('\n');
    if (firstNewLine != std::string::npos)
    {
        title = title.substr(0, firstNewLine);
    }

    if (title.size() > 80u)
    {
        title = title.substr(0, 77u) + "...";
    }

    if (title.empty())
    {
        return kWindowTitle;
    }

    return std::string("Horde RT Diagnostic - ") + title;
}

VkClearColorValue ClearColorForMode(const horde::vulkan::RtMode mode)
{
    switch (mode)
    {
    case horde::vulkan::RtMode::RayTracingPipeline:
        return { {0.04f, 0.36f, 0.06f, 1.0f} };
    case horde::vulkan::RtMode::RayQuery:
        return { {0.14f, 0.08f, 0.40f, 1.0f} };
    default:
        return { {0.28f, 0.04f, 0.04f, 1.0f} };
    }
}

bool CreateInstance(VkInstance& instance)
{
    const char* extensions[] = {VK_KHR_SURFACE_EXTENSION_NAME, VK_KHR_WIN32_SURFACE_EXTENSION_NAME};
    const VkApplicationInfo appInfo{
        VK_STRUCTURE_TYPE_APPLICATION_INFO,
        nullptr,
        "HordeLanternRTDiagnostic",
        VK_MAKE_VERSION(1, 0, 0),
        "horde_rt",
        VK_MAKE_VERSION(1, 0, 0),
        VK_API_VERSION_1_1};

    const VkInstanceCreateInfo createInfo{
        VK_STRUCTURE_TYPE_INSTANCE_CREATE_INFO,
        nullptr,
        0,
        &appInfo,
        0,
        nullptr,
        static_cast<uint32_t>(std::size(extensions)),
        extensions};

    const VkResult result = vkCreateInstance(&createInfo, nullptr, &instance);
    if (result != VK_SUCCESS)
    {
        std::cerr << "Failed to create Vulkan instance for diagnostic window: VkResult(" << result << ").\n";
        return false;
    }

    return true;
}

bool CreateSurface(VkInstance instance, HWND hwnd, VkSurfaceKHR& surface)
{
    const VkWin32SurfaceCreateInfoKHR createInfo{
        VK_STRUCTURE_TYPE_WIN32_SURFACE_CREATE_INFO_KHR,
        nullptr,
        0,
        GetModuleHandleA(nullptr),
        hwnd};

    const VkResult result = vkCreateWin32SurfaceKHR(instance, &createInfo, nullptr, &surface);
    if (result != VK_SUCCESS)
    {
        std::cerr << "Failed to create Vulkan Win32 surface: VkResult(" << result << ").\n";
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
                std::cerr << "Selected RayTracingPipeline device is missing required extension: " << extension << ".\n";
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
        std::cerr << "Failed to create Vulkan device for diagnostic swapchain: VkResult(" << createResult << ").\n";
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

    const VkExtent2D minExtent = capabilities.minImageExtent;
    const VkExtent2D maxExtent = capabilities.maxImageExtent;
    return {
        std::clamp(desiredWidth, minExtent.width, maxExtent.width),
        std::clamp(desiredHeight, minExtent.height, maxExtent.height)};
}

bool CreateSwapchain(VulkanSurfaceContext& ctx, HWND hwnd)
{
    VkSurfaceCapabilitiesKHR capabilities{};
    if (vkGetPhysicalDeviceSurfaceCapabilitiesKHR(ctx.physicalDevice, ctx.surface, &capabilities) != VK_SUCCESS)
    {
        std::cerr << "Failed to query Vulkan surface capabilities.\n";
        return false;
    }

    uint32_t formatCount = 0u;
    if (vkGetPhysicalDeviceSurfaceFormatsKHR(ctx.physicalDevice, ctx.surface, &formatCount, nullptr) != VK_SUCCESS ||
        formatCount == 0u)
    {
        return false;
    }

    std::vector<VkSurfaceFormatKHR> formats(formatCount);
    if (vkGetPhysicalDeviceSurfaceFormatsKHR(ctx.physicalDevice, ctx.surface, &formatCount, formats.data()) != VK_SUCCESS)
    {
        return false;
    }

    uint32_t presentModeCount = 0u;
    if (vkGetPhysicalDeviceSurfacePresentModesKHR(ctx.physicalDevice, ctx.surface, &presentModeCount, nullptr) != VK_SUCCESS ||
        presentModeCount == 0u)
    {
        return false;
    }

    std::vector<VkPresentModeKHR> presentModes(presentModeCount);
    if (vkGetPhysicalDeviceSurfacePresentModesKHR(ctx.physicalDevice, ctx.surface, &presentModeCount, presentModes.data()) != VK_SUCCESS)
    {
        return false;
    }

    VkSurfaceFormatKHR chosenFormat = formats[0];
    for (const VkSurfaceFormatKHR& candidate : formats)
    {
        if (candidate.format == VK_FORMAT_B8G8R8A8_UNORM &&
            candidate.colorSpace == VK_COLOR_SPACE_SRGB_NONLINEAR_KHR)
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

    RECT clientRect{};
    GetClientRect(hwnd, &clientRect);
    const uint32_t requestedWidth = static_cast<uint32_t>(std::max<long>(1, clientRect.right - clientRect.left));
    const uint32_t requestedHeight = static_cast<uint32_t>(std::max<long>(1, clientRect.bottom - clientRect.top));
    ctx.swapchainExtent = ClampExtent(capabilities, requestedWidth, requestedHeight);
    ctx.swapchainFormat = chosenFormat.format;
    ctx.swapchainColorSpace = chosenFormat.colorSpace;

    uint32_t imageCount = std::max(2u, capabilities.minImageCount);
    if (capabilities.maxImageCount > 0u && imageCount > capabilities.maxImageCount)
    {
        imageCount = capabilities.maxImageCount;
    }

    VkSwapchainCreateInfoKHR createInfo{
        VK_STRUCTURE_TYPE_SWAPCHAIN_CREATE_INFO_KHR,
        nullptr,
        0,
        ctx.surface,
        imageCount,
        ctx.swapchainFormat,
        ctx.swapchainColorSpace,
        ctx.swapchainExtent,
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

    if (vkCreateSwapchainKHR(ctx.device, &createInfo, nullptr, &ctx.swapchain) != VK_SUCCESS)
    {
        std::cerr << "Failed to create swapchain.\n";
        return false;
    }

    uint32_t imageArraySize = 0u;
    if (vkGetSwapchainImagesKHR(ctx.device, ctx.swapchain, &imageArraySize, nullptr) != VK_SUCCESS || imageArraySize == 0u)
    {
        return false;
    }

    ctx.swapchainImages.resize(imageArraySize);
    if (vkGetSwapchainImagesKHR(ctx.device, ctx.swapchain, &imageArraySize, ctx.swapchainImages.data()) != VK_SUCCESS)
    {
        return false;
    }
    ctx.swapchainImageLayouts.assign(ctx.swapchainImages.size(), VK_IMAGE_LAYOUT_UNDEFINED);

    ctx.swapchainImageViews.resize(ctx.swapchainImages.size());
    for (size_t index = 0u; index < ctx.swapchainImages.size(); ++index)
    {
        const VkImageViewCreateInfo viewCreateInfo{
            VK_STRUCTURE_TYPE_IMAGE_VIEW_CREATE_INFO,
            nullptr,
            0,
            ctx.swapchainImages[index],
            VK_IMAGE_VIEW_TYPE_2D,
            ctx.swapchainFormat,
            {
                VK_COMPONENT_SWIZZLE_IDENTITY,
                VK_COMPONENT_SWIZZLE_IDENTITY,
                VK_COMPONENT_SWIZZLE_IDENTITY,
                VK_COMPONENT_SWIZZLE_IDENTITY
            },
            {
                VK_IMAGE_ASPECT_COLOR_BIT,
                0u,
                1u,
                0u,
                1u
            }};
        if (vkCreateImageView(ctx.device, &viewCreateInfo, nullptr, &ctx.swapchainImageViews[index]) != VK_SUCCESS)
        {
            return false;
        }
    }

    const VkAttachmentDescription colorAttachment{
        0,
        ctx.swapchainFormat,
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

    const VkAttachmentDescription colorAttachmentArray[] = {colorAttachment};
    const VkSubpassDescription subpassArray[] = {subpass};
    const VkSubpassDependency dependency{
        VK_SUBPASS_EXTERNAL,
        0u,
        VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT,
        VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT,
        0u,
        VK_ACCESS_COLOR_ATTACHMENT_WRITE_BIT,
        VK_DEPENDENCY_BY_REGION_BIT};

    const VkRenderPassCreateInfo renderPassCreateInfo{
        VK_STRUCTURE_TYPE_RENDER_PASS_CREATE_INFO,
        nullptr,
        0,
        static_cast<uint32_t>(std::size(colorAttachmentArray)),
        colorAttachmentArray,
        1u,
        subpassArray,
        1u,
        &dependency};

    if (vkCreateRenderPass(ctx.device, &renderPassCreateInfo, nullptr, &ctx.renderPass) != VK_SUCCESS)
    {
        std::cerr << "Failed to create render pass.\n";
        return false;
    }

    ctx.swapchainFramebuffers.resize(ctx.swapchainImageViews.size());
    for (size_t index = 0u; index < ctx.swapchainImageViews.size(); ++index)
    {
        VkImageView attachments[] = {ctx.swapchainImageViews[index]};
        const VkFramebufferCreateInfo framebufferCreateInfo{
            VK_STRUCTURE_TYPE_FRAMEBUFFER_CREATE_INFO,
            nullptr,
            0,
            ctx.renderPass,
            1u,
            attachments,
            ctx.swapchainExtent.width,
            ctx.swapchainExtent.height,
            1u};
        if (vkCreateFramebuffer(ctx.device, &framebufferCreateInfo, nullptr, &ctx.swapchainFramebuffers[index]) != VK_SUCCESS)
        {
            return false;
        }
    }

    const VkCommandPoolCreateInfo commandPoolCreateInfo{
        VK_STRUCTURE_TYPE_COMMAND_POOL_CREATE_INFO,
        nullptr,
        VK_COMMAND_POOL_CREATE_RESET_COMMAND_BUFFER_BIT,
        ctx.graphicsQueueFamilyIndex};

    if (vkCreateCommandPool(ctx.device, &commandPoolCreateInfo, nullptr, &ctx.commandPool) != VK_SUCCESS)
    {
        return false;
    }

VkCommandBufferAllocateInfo commandBufferAllocateInfo{
        VK_STRUCTURE_TYPE_COMMAND_BUFFER_ALLOCATE_INFO,
        nullptr,
        ctx.commandPool,
        VK_COMMAND_BUFFER_LEVEL_PRIMARY,
        static_cast<uint32_t>(ctx.swapchainImageViews.size())};
    ctx.commandBuffers.resize(ctx.swapchainImageViews.size());
    if (vkAllocateCommandBuffers(ctx.device, &commandBufferAllocateInfo, ctx.commandBuffers.data()) != VK_SUCCESS)
    {
        return false;
    }

    for (size_t index = 0u; index < ctx.commandBuffers.size(); ++index)
    {
        VkClearValue clearValue{};
        clearValue.color.float32[0] = 0.0f;
        clearValue.color.float32[1] = 0.0f;
        clearValue.color.float32[2] = 0.0f;
        clearValue.color.float32[3] = 1.0f;
        const VkRenderPassBeginInfo renderPassBegin{
            VK_STRUCTURE_TYPE_RENDER_PASS_BEGIN_INFO,
            nullptr,
            ctx.renderPass,
            ctx.swapchainFramebuffers[index],
            {{0, 0}, ctx.swapchainExtent},
            1u,
            &clearValue};

        const VkCommandBufferBeginInfo commandBegin{
            VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO,
            nullptr,
            0u,
            nullptr};

        if (vkBeginCommandBuffer(ctx.commandBuffers[index], &commandBegin) != VK_SUCCESS)
        {
            return false;
        }

        vkCmdBeginRenderPass(ctx.commandBuffers[index], &renderPassBegin, VK_SUBPASS_CONTENTS_INLINE);
        vkCmdEndRenderPass(ctx.commandBuffers[index]);
        if (vkEndCommandBuffer(ctx.commandBuffers[index]) != VK_SUCCESS)
        {
            return false;
        }
    }

    ctx.imageAvailableSemaphores.resize(kMaxFramesInFlight);
    ctx.renderFinishedSemaphores.resize(kMaxFramesInFlight);
    ctx.inFlightFences.resize(kMaxFramesInFlight);
    VkSemaphoreCreateInfo semaphoreCreateInfo{VK_STRUCTURE_TYPE_SEMAPHORE_CREATE_INFO};
    VkFenceCreateInfo fenceCreateInfo{VK_STRUCTURE_TYPE_FENCE_CREATE_INFO, nullptr, VK_FENCE_CREATE_SIGNALED_BIT};
    for (UINT i = 0u; i < kMaxFramesInFlight; ++i)
    {
        if (vkCreateSemaphore(ctx.device, &semaphoreCreateInfo, nullptr, &ctx.imageAvailableSemaphores[i]) != VK_SUCCESS ||
            vkCreateSemaphore(ctx.device, &semaphoreCreateInfo, nullptr, &ctx.renderFinishedSemaphores[i]) != VK_SUCCESS ||
            vkCreateFence(ctx.device, &fenceCreateInfo, nullptr, &ctx.inFlightFences[i]) != VK_SUCCESS)
        {
            return false;
        }
    }

    return true;
}

void ReleaseSwapchainResources(VulkanSurfaceContext& ctx)
{
    if (ctx.device == VK_NULL_HANDLE)
    {
        return;
    }

    vkDeviceWaitIdle(ctx.device);
    ctx.rtScene.Destroy();

    if (ctx.commandPool != VK_NULL_HANDLE)
    {
        if (!ctx.commandBuffers.empty())
        {
            vkFreeCommandBuffers(ctx.device,
                                 ctx.commandPool,
                                 static_cast<uint32_t>(ctx.commandBuffers.size()),
                                 ctx.commandBuffers.data());
            ctx.commandBuffers.clear();
        }
        vkDestroyCommandPool(ctx.device, ctx.commandPool, nullptr);
        ctx.commandPool = VK_NULL_HANDLE;
    }

    for (VkFramebuffer framebuffer : ctx.swapchainFramebuffers)
    {
        if (framebuffer != VK_NULL_HANDLE)
        {
            vkDestroyFramebuffer(ctx.device, framebuffer, nullptr);
        }
    }
    ctx.swapchainFramebuffers.clear();

    for (VkImageView imageView : ctx.swapchainImageViews)
    {
        if (imageView != VK_NULL_HANDLE)
        {
            vkDestroyImageView(ctx.device, imageView, nullptr);
        }
    }
    ctx.swapchainImageViews.clear();

    if (ctx.renderPass != VK_NULL_HANDLE)
    {
        vkDestroyRenderPass(ctx.device, ctx.renderPass, nullptr);
        ctx.renderPass = VK_NULL_HANDLE;
    }

    if (ctx.swapchain != VK_NULL_HANDLE)
    {
        vkDestroySwapchainKHR(ctx.device, ctx.swapchain, nullptr);
        ctx.swapchain = VK_NULL_HANDLE;
    }

    for (VkSemaphore semaphore : ctx.imageAvailableSemaphores)
    {
        if (semaphore != VK_NULL_HANDLE)
        {
            vkDestroySemaphore(ctx.device, semaphore, nullptr);
        }
    }
    for (VkSemaphore semaphore : ctx.renderFinishedSemaphores)
    {
        if (semaphore != VK_NULL_HANDLE)
        {
            vkDestroySemaphore(ctx.device, semaphore, nullptr);
        }
    }
    for (VkFence fence : ctx.inFlightFences)
    {
        if (fence != VK_NULL_HANDLE)
        {
            vkDestroyFence(ctx.device, fence, nullptr);
        }
    }

    ctx.imageAvailableSemaphores.clear();
    ctx.renderFinishedSemaphores.clear();
    ctx.inFlightFences.clear();
    ctx.swapchainImageLayouts.clear();
    ctx.swapchainImages.clear();
    ctx.currentFrame = 0u;
}

bool InitialiseRtSceneForSwapchain(VulkanSurfaceContext& ctx)
{
    if (!ctx.useRtPath)
    {
        return true;
    }

    std::string diagnostic;
    if (!ctx.rtScene.Initialise(ctx.instance,
                                ctx.physicalDevice,
                                ctx.device,
                                ctx.graphicsQueue,
                                ctx.commandPool,
                                ctx.swapchainExtent,
                                diagnostic))
    {
        std::cerr << "Failed to initialise presentable RT scene: " << diagnostic << '\n';
        return false;
    }

    return true;
}

bool RecreateSwapchain(VulkanSurfaceContext& ctx)
{
    ReleaseSwapchainResources(ctx);
    if (ctx.windowHandle == nullptr)
    {
        return false;
    }

    return CreateSwapchain(ctx, ctx.windowHandle) && InitialiseRtSceneForSwapchain(ctx);
}

void DestroyRenderContext(VulkanSurfaceContext& ctx)
{
    if (ctx.device == VK_NULL_HANDLE)
    {
        if (ctx.surface != VK_NULL_HANDLE)
        {
            vkDestroySurfaceKHR(ctx.instance, ctx.surface, nullptr);
        }
        if (ctx.instance != VK_NULL_HANDLE)
        {
            vkDestroyInstance(ctx.instance, nullptr);
        }
        return;
    }

    vkDeviceWaitIdle(ctx.device);
    ctx.rtScene.Destroy();

    for (VkFence fence : ctx.inFlightFences)
    {
        if (fence != VK_NULL_HANDLE)
        {
            vkDestroyFence(ctx.device, fence, nullptr);
        }
    }
    for (VkSemaphore semaphore : ctx.imageAvailableSemaphores)
    {
        if (semaphore != VK_NULL_HANDLE)
        {
            vkDestroySemaphore(ctx.device, semaphore, nullptr);
        }
    }
    for (VkSemaphore semaphore : ctx.renderFinishedSemaphores)
    {
        if (semaphore != VK_NULL_HANDLE)
        {
            vkDestroySemaphore(ctx.device, semaphore, nullptr);
        }
    }
    for (VkFramebuffer framebuffer : ctx.swapchainFramebuffers)
    {
        if (framebuffer != VK_NULL_HANDLE)
        {
            vkDestroyFramebuffer(ctx.device, framebuffer, nullptr);
        }
    }
    for (VkImageView imageView : ctx.swapchainImageViews)
    {
        if (imageView != VK_NULL_HANDLE)
        {
            vkDestroyImageView(ctx.device, imageView, nullptr);
        }
    }
    if (ctx.commandPool != VK_NULL_HANDLE)
    {
        vkFreeCommandBuffers(ctx.device, ctx.commandPool,
                             static_cast<uint32_t>(ctx.commandBuffers.size()), ctx.commandBuffers.data());
        vkDestroyCommandPool(ctx.device, ctx.commandPool, nullptr);
    }
    if (ctx.renderPass != VK_NULL_HANDLE)
    {
        vkDestroyRenderPass(ctx.device, ctx.renderPass, nullptr);
    }
    if (ctx.swapchain != VK_NULL_HANDLE)
    {
        vkDestroySwapchainKHR(ctx.device, ctx.swapchain, nullptr);
    }
    vkDestroyDevice(ctx.device, nullptr);
    if (ctx.surface != VK_NULL_HANDLE)
    {
        vkDestroySurfaceKHR(ctx.instance, ctx.surface, nullptr);
    }
    if (ctx.instance != VK_NULL_HANDLE)
    {
        vkDestroyInstance(ctx.instance, nullptr);
    }

    ctx = {};
}

bool RenderFrame(VulkanSurfaceContext& ctx, const VkClearColorValue& clearColor, bool& rtFramePresented)
{
    rtFramePresented = false;
    if (ctx.commandBuffers.empty())
    {
        return false;
    }

    const VkResult waitResult = vkWaitForFences(ctx.device, 1u, &ctx.inFlightFences[ctx.currentFrame], VK_TRUE, UINT64_MAX);
    if (waitResult != VK_SUCCESS && waitResult != VK_TIMEOUT)
    {
        return false;
    }

    uint32_t imageIndex = 0u;
    const VkResult acquireResult = vkAcquireNextImageKHR(
        ctx.device,
        ctx.swapchain,
        UINT64_MAX,
        ctx.imageAvailableSemaphores[ctx.currentFrame],
        VK_NULL_HANDLE,
        &imageIndex);

    if (acquireResult == VK_ERROR_OUT_OF_DATE_KHR || acquireResult == VK_SUBOPTIMAL_KHR)
    {
        return RecreateSwapchain(ctx);
    }
    if (acquireResult != VK_SUCCESS)
    {
        return false;
    }

    VkCommandBufferBeginInfo beginInfo{
        VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO,
        nullptr,
        VK_COMMAND_BUFFER_USAGE_ONE_TIME_SUBMIT_BIT,
        nullptr};

    if (vkResetCommandBuffer(ctx.commandBuffers[imageIndex], 0u) != VK_SUCCESS ||
        vkBeginCommandBuffer(ctx.commandBuffers[imageIndex], &beginInfo) != VK_SUCCESS)
    {
        return false;
    }

    const bool useRtFrame = ctx.useRtPath && ctx.rtScene.IsReady();
    if (useRtFrame)
    {
        std::string diagnostic;
        if (!ctx.rtScene.RecordTraceAndCopy(ctx.commandBuffers[imageIndex],
                                            ctx.swapchainImages[imageIndex],
                                            ctx.swapchainImageLayouts[imageIndex],
                                            ctx.swapchainExtent,
                                            0.0f,
                                            0.0f,
                                            1.8f,
                                            0.0f,
                                            0.0f,
                                            4.7f,
                                            0.0f,
                                            diagnostic))
        {
            std::cerr << "Failed to record RT frame: " << diagnostic << '\n';
            return false;
        }
    }
    else
    {
        VkClearValue clearValue{};
        clearValue.color.float32[0] = clearColor.float32[0];
        clearValue.color.float32[1] = clearColor.float32[1];
        clearValue.color.float32[2] = clearColor.float32[2];
        clearValue.color.float32[3] = clearColor.float32[3];

        const VkRenderPassBeginInfo renderPassBegin{
            VK_STRUCTURE_TYPE_RENDER_PASS_BEGIN_INFO,
            nullptr,
            ctx.renderPass,
            ctx.swapchainFramebuffers[imageIndex],
            {{0, 0}, ctx.swapchainExtent},
            1u,
            &clearValue};
        vkCmdBeginRenderPass(ctx.commandBuffers[imageIndex], &renderPassBegin, VK_SUBPASS_CONTENTS_INLINE);
        vkCmdEndRenderPass(ctx.commandBuffers[imageIndex]);
        ctx.swapchainImageLayouts[imageIndex] = VK_IMAGE_LAYOUT_PRESENT_SRC_KHR;
    }

    if (vkEndCommandBuffer(ctx.commandBuffers[imageIndex]) != VK_SUCCESS)
    {
        return false;
    }

    if (vkResetFences(ctx.device, 1u, &ctx.inFlightFences[ctx.currentFrame]) != VK_SUCCESS)
    {
        return false;
    }

    VkPipelineStageFlags waitStages = useRtFrame ? VK_PIPELINE_STAGE_TRANSFER_BIT : VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT;
    VkSubmitInfo submitInfo{
        VK_STRUCTURE_TYPE_SUBMIT_INFO,
        nullptr,
        1u,
        &ctx.imageAvailableSemaphores[ctx.currentFrame],
        &waitStages,
        1u,
        &ctx.commandBuffers[imageIndex],
        1u,
        &ctx.renderFinishedSemaphores[ctx.currentFrame]};

    if (vkQueueSubmit(ctx.graphicsQueue, 1u, &submitInfo, ctx.inFlightFences[ctx.currentFrame]) != VK_SUCCESS)
    {
        return false;
    }

    VkPresentInfoKHR presentInfo{
        VK_STRUCTURE_TYPE_PRESENT_INFO_KHR,
        nullptr,
        1u,
        &ctx.renderFinishedSemaphores[ctx.currentFrame],
        1u,
        &ctx.swapchain,
        &imageIndex,
        nullptr};
    const VkResult presentResult = vkQueuePresentKHR(ctx.graphicsQueue, &presentInfo);
    if (presentResult == VK_ERROR_OUT_OF_DATE_KHR || presentResult == VK_SUBOPTIMAL_KHR)
    {
        return RecreateSwapchain(ctx);
    }
    if (presentResult != VK_SUCCESS)
    {
        return false;
    }

    rtFramePresented = useRtFrame;
    ctx.currentFrame = (ctx.currentFrame + 1u) % kMaxFramesInFlight;
    return true;
}

int RunDiagnosticSwapchainWindow(HWND hWnd,
                                 horde::vulkan::DeviceCapabilities& capabilities,
                                 const std::filesystem::path& textReportPath,
                                 const std::filesystem::path& jsonReportPath)
{
    VulkanSurfaceContext context;
    context.currentFrame = 0u;
    if (!CreateInstance(context.instance))
    {
        return 1;
    }

    if (!CreateSurface(context.instance, hWnd, context.surface))
    {
        DestroyRenderContext(context);
        return 1;
    }

    horde::vulkan::VulkanContext probe;
    if (!probe.InitialiseForCapabilityProbe())
    {
        DestroyRenderContext(context);
        return 1;
    }

    const horde::vulkan::DeviceCapabilities selectedCapabilities = probe.QueryDeviceCapabilities();
    const uint32_t desiredVendorId = selectedCapabilities.identity.vendorId;
    const uint32_t desiredDeviceId = selectedCapabilities.identity.deviceId;
    const std::string desiredDeviceName = selectedCapabilities.identity.gpuName;

    uint32_t physicalDeviceCount = 0u;
    if (vkEnumeratePhysicalDevices(context.instance, &physicalDeviceCount, nullptr) != VK_SUCCESS || physicalDeviceCount == 0u)
    {
        std::cerr << "No physical devices found for diagnostic swapchain.\n";
        DestroyRenderContext(context);
        return 1;
    }

    std::vector<VkPhysicalDevice> physicalDevices(physicalDeviceCount);
    vkEnumeratePhysicalDevices(context.instance, &physicalDeviceCount, physicalDevices.data());

    for (const VkPhysicalDevice candidate : physicalDevices)
    {
        VkPhysicalDeviceProperties properties{};
        vkGetPhysicalDeviceProperties(candidate, &properties);
        if (properties.vendorID == desiredVendorId &&
            properties.deviceID == desiredDeviceId &&
            desiredDeviceName == properties.deviceName)
        {
            context.physicalDevice = candidate;
            break;
        }
    }

    if (context.physicalDevice == VK_NULL_HANDLE)
    {
        for (const VkPhysicalDevice candidate : physicalDevices)
        {
            uint32_t queueFamilyCount = 0u;
            vkGetPhysicalDeviceQueueFamilyProperties(candidate, &queueFamilyCount, nullptr);
            if (queueFamilyCount == 0u)
            {
                continue;
            }

            std::vector<VkQueueFamilyProperties> queueFamilies(queueFamilyCount);
            vkGetPhysicalDeviceQueueFamilyProperties(candidate, &queueFamilyCount, queueFamilies.data());
            for (uint32_t index = 0u; index < queueFamilyCount; ++index)
            {
                if ((queueFamilies[index].queueFlags & VK_QUEUE_GRAPHICS_BIT) == 0u)
                {
                    continue;
                }

                VkBool32 support = VK_FALSE;
                vkGetPhysicalDeviceSurfaceSupportKHR(candidate, index, context.surface, &support);
                if (support == VK_TRUE)
                {
                    context.physicalDevice = candidate;
                    break;
                }
            }

            if (context.physicalDevice != VK_NULL_HANDLE)
            {
                break;
            }
        }
    }

    if (context.physicalDevice == VK_NULL_HANDLE)
    {
        DestroyRenderContext(context);
        return 1;
    }

    if (!FindGraphicsAndPresentQueueFamily(context.physicalDevice, context.surface, context.graphicsQueueFamilyIndex))
    {
        DestroyRenderContext(context);
        return 1;
    }

    context.useRtPath = capabilities.rtMode == horde::vulkan::RtMode::RayTracingPipeline;
    if (!CreateLogicalDevice(context.physicalDevice, context.graphicsQueueFamilyIndex, capabilities, context.device, context.graphicsQueue))
    {
        DestroyRenderContext(context);
        return 1;
    }

    context.windowHandle = hWnd;

    if (!CreateSwapchain(context, hWnd))
    {
        DestroyRenderContext(context);
        return 1;
    }
    if (!InitialiseRtSceneForSwapchain(context))
    {
        DestroyRenderContext(context);
        return 1;
    }

    const VkClearColorValue clearColor = ClearColorForMode(capabilities.rtMode);
    MSG message{};
    bool running = true;
    while (running)
    {
        while (PeekMessageA(&message, nullptr, 0, 0, PM_REMOVE) != 0)
        {
            if (message.message == WM_QUIT)
            {
                running = false;
                break;
            }
            TranslateMessage(&message);
            DispatchMessageA(&message);
        }

        if (!running)
        {
            break;
        }

        bool rtFramePresented = false;
        if (!RenderFrame(context, clearColor, rtFramePresented))
        {
            break;
        }
        if (rtFramePresented && !capabilities.rtScene.presented)
        {
            capabilities.rtScene.presented = true;
            capabilities.rtScene.status = "Presented via swapchain";
            capabilities.rtScene.geometry = "Horde Lantern corridor demo scene";
            capabilities.rtScene.dispatchWidth = context.rtScene.DispatchExtent().width;
            capabilities.rtScene.dispatchHeight = context.rtScene.DispatchExtent().height;
            capabilities.performance.internalRenderWidth = capabilities.rtScene.dispatchWidth;
            capabilities.performance.internalRenderHeight = capabilities.rtScene.dispatchHeight;
            WriteReportFile(textReportPath, horde::vulkan::BuildCapabilityTextReport(capabilities));
            WriteReportFile(jsonReportPath, horde::vulkan::BuildCapabilityJsonReport(capabilities));
        }
    }

    DestroyRenderContext(context);
    return running ? 0 : static_cast<int>(message.wParam);
}

bool WriteReportFile(const std::filesystem::path& path, const std::string& data)
{
    std::ofstream stream(path, std::ios::binary);
    if (!stream.good())
    {
        return false;
    }

    stream << data;
    return stream.good();
}

LRESULT CALLBACK DiagnosticWindowProc(HWND hWnd, UINT message, WPARAM wParam, LPARAM lParam)
{
    switch (message)
    {
    case WM_SIZE:
    {
        const int width = LOWORD(lParam);
        const int height = HIWORD(lParam);
        HWND child = GetDlgItem(hWnd, kEditControlId);
        if (child)
        {
            MoveWindow(child, 12, 12, width - 24, height - 24, TRUE);
        }
        return 0;
    }
    case WM_DESTROY:
        PostQuitMessage(0);
        return 0;
    default:
        break;
    }

    return DefWindowProc(hWnd, message, wParam, lParam);
}

int CreateAndShowWindow(const std::string& diagnosticText,
                        horde::vulkan::DeviceCapabilities& capabilities,
                        const std::filesystem::path& textReportPath,
                        const std::filesystem::path& jsonReportPath)
{
    const HINSTANCE instance = GetModuleHandleA(nullptr);
    WNDCLASSA windowClass{};
    windowClass.lpfnWndProc = DiagnosticWindowProc;
    windowClass.hInstance = instance;
    windowClass.lpszClassName = kWindowClassName;
    windowClass.hbrBackground = reinterpret_cast<HBRUSH>(COLOR_3DFACE + 1);
    windowClass.hCursor = LoadCursor(nullptr, IDC_ARROW);

    if (!RegisterClassA(&windowClass) && GetLastError() != ERROR_CLASS_ALREADY_EXISTS)
    {
        std::cerr << "Failed to register diagnostic window class." << std::endl;
        return 1;
    }

    HWND hWnd = CreateWindowExA(
        0,
        kWindowClassName,
        kWindowTitle,
        WS_OVERLAPPEDWINDOW | WS_VISIBLE,
        CW_USEDEFAULT,
        CW_USEDEFAULT,
        1000,
        700,
        nullptr,
        nullptr,
        instance,
        nullptr);
    if (!hWnd)
    {
        std::cerr << "Failed to create diagnostic window." << std::endl;
        return 1;
    }

    HFONT monoFont = CreateFontA(
        18,
        0,
        0,
        0,
        FW_NORMAL,
        FALSE,
        FALSE,
        FALSE,
        ANSI_CHARSET,
        OUT_DEFAULT_PRECIS,
        CLIP_DEFAULT_PRECIS,
        DEFAULT_QUALITY,
        FF_MODERN,
        "Consolas");
    if (!monoFont)
    {
    monoFont = static_cast<HFONT>(GetStockObject(ANSI_FIXED_FONT));
    }

    RECT clientRect{};
    GetClientRect(hWnd, &clientRect);
    HWND edit = CreateWindowExA(
        WS_EX_CLIENTEDGE,
        "EDIT",
        "",
        WS_CHILD | WS_VISIBLE | ES_AUTOVSCROLL | ES_MULTILINE | ES_READONLY | WS_VSCROLL,
        12,
        12,
        clientRect.right - clientRect.left - 24,
        clientRect.bottom - clientRect.top - 24,
        hWnd,
        reinterpret_cast<HMENU>(static_cast<INT_PTR>(kEditControlId)),
        instance,
        nullptr);
    if (!edit)
    {
        std::cerr << "Failed to create diagnostic text area." << std::endl;
        return 1;
    }

    SendMessageA(edit, WM_SETFONT, reinterpret_cast<WPARAM>(monoFont), TRUE);
    const std::string windowText = WindowSafeText(diagnosticText);
    const std::string windowTitle = MakeWindowTitle(diagnosticText);
    SetWindowTextA(edit, windowText.c_str());
    SetWindowTextA(hWnd, windowTitle.c_str());

    ShowWindow(hWnd, SW_SHOW);
    UpdateWindow(hWnd);
    SetForegroundWindow(hWnd);
    SetFocus(edit);

    return RunDiagnosticSwapchainWindow(hWnd, capabilities, textReportPath, jsonReportPath);
}

} // namespace

namespace horde::platform::windows
{

int RunDiagnosticWindow(const int showCommand)
{
    (void)showCommand;

    horde::vulkan::VulkanContext context;
    const bool initialised = context.InitialiseForCapabilityProbe();
    horde::vulkan::DeviceCapabilities capabilities = context.QueryDeviceCapabilities();

    const std::string diagnosticText = BuildDisplayText(capabilities);
    const std::string textReport = horde::vulkan::BuildCapabilityTextReport(capabilities);
    const std::string jsonReport = horde::vulkan::BuildCapabilityJsonReport(capabilities);

    std::cout << "=== Horde RT Diagnostic Window ===\n";
    std::cout << "Probe initialisation: " << (initialised ? "OK" : "Fallback") << "\n\n";
    std::cout << diagnosticText << "\n\n";

    std::error_code error;
    const std::filesystem::path reportDirectory = kReportDirectory;
    std::filesystem::create_directories(reportDirectory, error);
    if (error)
    {
        std::cerr << "Failed to create report directory '" << kReportDirectory << "': " << error.message() << '\n';
        return 1;
    }

    const std::filesystem::path textReportPath = reportDirectory / kTextReportFilename;
    const std::filesystem::path jsonReportPath = reportDirectory / kJsonReportFilename;

    if (!WriteReportFile(textReportPath, textReport))
    {
        std::cerr << "Failed to write text report to " << textReportPath << '\n';
        return 1;
    }

    if (!WriteReportFile(jsonReportPath, jsonReport))
    {
        std::cerr << "Failed to write JSON report to " << jsonReportPath << '\n';
        return 1;
    }

    std::cout << "Stored report (text): " << textReportPath << '\n';
    std::cout << "Stored report (json): " << jsonReportPath << '\n';

    return CreateAndShowWindow(diagnosticText, capabilities, textReportPath, jsonReportPath);
}

} // namespace horde::platform::windows
