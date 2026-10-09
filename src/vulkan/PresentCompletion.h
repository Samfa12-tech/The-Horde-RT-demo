#pragma once

#include <vulkan/vulkan.h>
#include <algorithm>
#include <cstring>
#include <vector>

namespace horde::vulkan
{
enum class PresentCompletionMode { Unextended, MaintenanceKHR, MaintenanceEXT };
struct PresentSurfaceSupport { bool khr = false, ext = false; };
struct PresentDeviceSupport { bool khr = false, ext = false, feature = false; };
inline PFN_vkGetPhysicalDeviceFeatures2 ResolvePresentFeaturesQuery(VkInstance instance,
    PFN_vkGetInstanceProcAddr getProc = vkGetInstanceProcAddr)
{
    auto query = reinterpret_cast<PFN_vkGetPhysicalDeviceFeatures2>(getProc(instance, "vkGetPhysicalDeviceFeatures2"));
    if (!query) query = reinterpret_cast<PFN_vkGetPhysicalDeviceFeatures2>(getProc(instance, "vkGetPhysicalDeviceFeatures2KHR"));
    return query;
}
inline bool HasPresentExtension(const std::vector<VkExtensionProperties>& extensions, const char* name)
{
    return std::any_of(extensions.begin(), extensions.end(), [name](const auto& extension) {
        return std::strcmp(extension.extensionName, name) == 0;
    });
}
// Device-only query is also suitable for the headless capability report. No
// logical device, WSI objects or GPU work are created by either query.
inline PresentDeviceSupport QueryPresentDeviceSupport(VkPhysicalDevice physicalDevice, VkInstance instance)
{
    PresentDeviceSupport support;
    uint32_t count = 0;
    if (vkEnumerateDeviceExtensionProperties(physicalDevice, nullptr, &count, nullptr) != VK_SUCCESS) return support;
    std::vector<VkExtensionProperties> extensions(count);
    if (vkEnumerateDeviceExtensionProperties(physicalDevice, nullptr, &count, extensions.data()) != VK_SUCCESS) return support;
    support.khr = HasPresentExtension(extensions, "VK_KHR_swapchain_maintenance1");
    support.ext = HasPresentExtension(extensions, "VK_EXT_swapchain_maintenance1");
    if (support.khr || support.ext)
    {
        // EXT and promoted KHR types/sTypes are ABI aliases. EXT spelling keeps
        // this shared path compatible with the accepted Android NDK26 headers.
        VkPhysicalDeviceSwapchainMaintenance1FeaturesEXT maintenance{
            VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_SWAPCHAIN_MAINTENANCE_1_FEATURES_EXT};
        VkPhysicalDeviceFeatures2 features{VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_FEATURES_2};
        features.pNext = &maintenance;
        // API24's link stub has only Vulkan1.0 exports. Resolve instance-level
        // entry points from the actual1.2 instance instead of linking core1.1.
        const auto getFeatures = ResolvePresentFeaturesQuery(instance);
        if (getFeatures) getFeatures(physicalDevice, &features);
        support.feature = maintenance.swapchainMaintenance1 == VK_TRUE;
    }
    return support;
}
inline PresentSurfaceSupport AppendOptionalPresentInstanceExtensions(std::vector<const char*>& enabled)
{
    PresentSurfaceSupport support;
    uint32_t count = 0;
    if (vkEnumerateInstanceExtensionProperties(nullptr, &count, nullptr) != VK_SUCCESS) return support;
    std::vector<VkExtensionProperties> extensions(count);
    if (vkEnumerateInstanceExtensionProperties(nullptr, &count, extensions.data()) != VK_SUCCESS) return support;
    if (!HasPresentExtension(extensions, "VK_KHR_get_surface_capabilities2")) return support;
    support.khr = HasPresentExtension(extensions, "VK_KHR_surface_maintenance1");
    support.ext = HasPresentExtension(extensions, "VK_EXT_surface_maintenance1");
    if (support.khr || support.ext) enabled.push_back("VK_KHR_get_surface_capabilities2");
    if (support.khr) enabled.push_back("VK_KHR_surface_maintenance1");
    if (support.ext) enabled.push_back("VK_EXT_surface_maintenance1");
    return support;
}
inline PresentCompletionMode SelectPresentCompletion(PresentSurfaceSupport surface, PresentDeviceSupport device)
{
    if (!device.feature) return PresentCompletionMode::Unextended;
    if (surface.khr && device.khr) return PresentCompletionMode::MaintenanceKHR;
    if (surface.ext && device.ext) return PresentCompletionMode::MaintenanceEXT;
    return PresentCompletionMode::Unextended;
}
inline const char* PresentCompletionExtension(PresentCompletionMode mode)
{
    return mode == PresentCompletionMode::MaintenanceKHR ? "VK_KHR_swapchain_maintenance1" :
        mode == PresentCompletionMode::MaintenanceEXT ? "VK_EXT_swapchain_maintenance1" : nullptr;
}
inline const char* PresentCompletionDiagnostic(PresentCompletionMode mode)
{
    return mode == PresentCompletionMode::MaintenanceKHR ? "Presentation retirement: KHR maintenance1 fences enabled." :
        mode == PresentCompletionMode::MaintenanceEXT ? "Presentation retirement: EXT maintenance1 fences enabled." :
        "Presentation retirement: unextended idle fallback; formal shutdown/retirement completion proof unavailable.";
}
inline bool PresentOperationEnqueued(VkResult result)
{
    // OOM cannot enqueue; unknown/device-loss results cannot justify normal
    // teardown. Retain ownership conservatively for any other returned error.
    return result != VK_ERROR_OUT_OF_HOST_MEMORY && result != VK_ERROR_OUT_OF_DEVICE_MEMORY;
}
struct PresentFenceDispatch
{
    PFN_vkCreateFence create = vkCreateFence;
    PFN_vkWaitForFences wait = vkWaitForFences;
    PFN_vkResetFences reset = vkResetFences;
    PFN_vkDestroyFence destroy = vkDestroyFence;
};
// Bounded per-swapchain-image ownership. Graphics idle remains separate; this
// class only proves retirement of presentation payloads. No automatic destroy.
class PresentCompletionFences
{
    struct Slot { VkFence fence = VK_NULL_HANDLE; bool pending = false; };
public:
    PresentCompletionFences() = default;
    PresentCompletionFences(const PresentCompletionFences&) = delete;
    PresentCompletionFences& operator=(const PresentCompletionFences&) = delete;
    PresentCompletionFences(PresentCompletionFences&&) = default;
    PresentCompletionFences& operator=(PresentCompletionFences&&) = default;
    static constexpr uint64_t kWaitNanoseconds = 2'000'000'000ull;
    bool Create(VkDevice device, size_t imageCount, bool enabled, PresentFenceDispatch dispatch = {})
    {
        if (!slots_.empty()) return false;
        device_ = device; dispatch_ = dispatch;
        if (!enabled) return true;
        if (imageCount == 0u) return false;
        try { slots_.resize(imageCount); } catch (...) { return false; }
        VkFenceCreateInfo info{VK_STRUCTURE_TYPE_FENCE_CREATE_INFO};
        for (auto& slot : slots_)
            if (dispatch_.create(device_, &info, nullptr, &slot.fence) != VK_SUCCESS) return false;
        return true; // Partial allocation is retained for explicit safe cleanup.
    }
    VkResult Prepare(size_t image)
    {
        if (slots_.empty()) return VK_SUCCESS;
        if (image >= slots_.size() || slots_[image].fence == VK_NULL_HANDLE) return VK_ERROR_INITIALIZATION_FAILED;
        auto& slot = slots_[image];
        if (slot.pending)
        {
            const auto result = dispatch_.wait(device_, 1, &slot.fence, VK_TRUE, kWaitNanoseconds);
            if (result != VK_SUCCESS) return result;
            slot.pending = false;
        }
        return dispatch_.reset(device_, 1, &slot.fence);
    }
    VkFence Fence(size_t image) const { return slots_.empty() ? VK_NULL_HANDLE : slots_.at(image).fence; }
    void Presented(size_t image, VkResult result)
    {
        if (!slots_.empty()) slots_.at(image).pending = PresentOperationEnqueued(result);
    }
    VkResult Drain()
    {
        std::vector<VkFence> pending;
        try { for (const auto& slot : slots_) if (slot.pending) pending.push_back(slot.fence); }
        catch (...) { return VK_ERROR_OUT_OF_HOST_MEMORY; }
        if (pending.empty()) return VK_SUCCESS;
        const auto result = dispatch_.wait(device_, static_cast<uint32_t>(pending.size()), pending.data(), VK_TRUE, kWaitNanoseconds);
        if (result == VK_SUCCESS) for (auto& slot : slots_) slot.pending = false;
        return result;
    }
    bool DestroyCompleted()
    {
        if (std::any_of(slots_.begin(), slots_.end(), [](const auto& slot) { return slot.pending; })) return false;
        for (const auto& slot : slots_) if (slot.fence != VK_NULL_HANDLE) dispatch_.destroy(device_, slot.fence, nullptr);
        slots_.clear(); device_ = VK_NULL_HANDLE;
        return true;
    }
private:
    VkDevice device_ = VK_NULL_HANDLE;
    PresentFenceDispatch dispatch_{};
    std::vector<Slot> slots_;
};
}
