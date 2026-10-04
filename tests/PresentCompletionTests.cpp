#include "vulkan/PresentCompletion.h"
#include <iostream>
#include <unordered_map>

namespace {
using namespace horde::vulkan;
bool passed = true;
void Check(bool value, const char* message) { if (!value) { passed = false; std::cerr << message << '\n'; } }
struct Fake {
    int creates = 0, failCreate = -1, destroys = 0, waits = 0, resets = 0;
    uint32_t lastWaitCount = 0;
    VkResult waitResult = VK_SUCCESS;
    std::unordered_map<VkFence, bool> live;
} fake;
VKAPI_ATTR VkResult VKAPI_CALL Create(VkDevice, const VkFenceCreateInfo* info, const VkAllocationCallbacks*, VkFence* fence) {
    Check(info->flags == 0u, "Presentation fence is initially unsignaled");
    if (++fake.creates == fake.failCreate) return VK_ERROR_OUT_OF_HOST_MEMORY;
    *fence = reinterpret_cast<VkFence>(static_cast<uintptr_t>(fake.creates));
    fake.live[*fence] = true;
    return VK_SUCCESS;
}
VKAPI_ATTR VkResult VKAPI_CALL Wait(VkDevice, uint32_t count, const VkFence* fences, VkBool32 all, uint64_t timeout) {
    ++fake.waits; fake.lastWaitCount = count;
    Check(all == VK_TRUE && timeout == PresentCompletionFences::kWaitNanoseconds, "Bounded wait covers all selected present operations");
    for (uint32_t i = 0; i < count; ++i) Check(fake.live.contains(fences[i]), "Only allocated live fences waited");
    return fake.waitResult;
}
VKAPI_ATTR VkResult VKAPI_CALL Reset(VkDevice, uint32_t count, const VkFence* fences) {
    ++fake.resets;
    Check(count == 1u && fake.live.contains(fences[0]), "Per-image fence reset has valid ownership");
    return VK_SUCCESS;
}
VKAPI_ATTR void VKAPI_CALL Destroy(VkDevice, VkFence fence, const VkAllocationCallbacks*) {
    Check(fake.live.erase(fence) == 1u, "Each allocated presentation fence destroyed exactly once"); ++fake.destroys;
}
PresentFenceDispatch Dispatch() { return {Create, Wait, Reset, Destroy}; }
VkDevice Device() { return reinterpret_cast<VkDevice>(static_cast<uintptr_t>(100u)); }
bool coreQueryAvailable = false, khrQueryAvailable = false;
int coreLookups = 0, khrLookups = 0;
VkInstance expectedInstance = VK_NULL_HANDLE;
VKAPI_ATTR void VKAPI_CALL Features(VkPhysicalDevice, VkPhysicalDeviceFeatures2*) {}
VKAPI_ATTR PFN_vkVoidFunction VKAPI_CALL GetProc(VkInstance instance, const char* name) {
    Check(instance == expectedInstance, "Features dispatch uses the actual owning instance");
    if (std::strcmp(name, "vkGetPhysicalDeviceFeatures2") == 0) {
        ++coreLookups;
        return coreQueryAvailable ? reinterpret_cast<PFN_vkVoidFunction>(Features) : nullptr;
    }
    Check(std::strcmp(name, "vkGetPhysicalDeviceFeatures2KHR") == 0, "Only the approved core/KHR instance queries are attempted");
    ++khrLookups;
    return khrQueryAvailable ? reinterpret_cast<PFN_vkVoidFunction>(Features) : nullptr;
}
void FeaturesDispatch() {
    expectedInstance = reinterpret_cast<VkInstance>(static_cast<uintptr_t>(200u));
    coreQueryAvailable = khrQueryAvailable = true; coreLookups = khrLookups = 0;
    Check(ResolvePresentFeaturesQuery(expectedInstance, GetProc) == Features && coreLookups == 1 && khrLookups == 0,
          "Prefer core instance dispatch without requiring a core1.1 link symbol");
    coreQueryAvailable = false;
    Check(ResolvePresentFeaturesQuery(expectedInstance, GetProc) == Features && khrLookups == 1,
          "KHR instance dispatch supports older loader exports");
    khrQueryAvailable = false;
    Check(ResolvePresentFeaturesQuery(expectedInstance, GetProc) == nullptr && khrLookups == 2,
          "Unavailable dispatch cannot invent optional feature support");
}
void Selection() {
    Check(SelectPresentCompletion({true,true},{true,true,true}) == PresentCompletionMode::MaintenanceKHR, "Prefer complete KHR dependency set");
    Check(SelectPresentCompletion({false,true},{true,true,true}) == PresentCompletionMode::MaintenanceEXT, "EXT remains admitted when KHR instance dependency absent");
    Check(SelectPresentCompletion({true,false},{false,true,true}) == PresentCompletionMode::Unextended, "Do not mix mismatched extension dependencies");
    Check(SelectPresentCompletion({true,true},{true,true,false}) == PresentCompletionMode::Unextended, "Advertisement without feature cannot enable fences");
    Check(SelectPresentCompletion({}, {}) == PresentCompletionMode::Unextended, "Optional feature never becomes an RT backend requirement");
}
void Lifecycle() {
    fake = {}; PresentCompletionFences fences;
    Check(fences.Create(Device(),3u,true,Dispatch()), "Per-image fence allocation succeeds");
    Check(fences.Prepare(0u) == VK_SUCCESS && fake.waits == 0, "Unused fence does not wait for nonexistent present");
    fences.Presented(0u,VK_SUCCESS);
    fake.waitResult = VK_TIMEOUT;
    Check(fences.Prepare(0u) == VK_TIMEOUT && fake.resets == 1, "Timeout cannot reset pending fence");
    Check(!fences.DestroyCompleted() && fake.destroys == 0, "Timeout cannot authorize destruction");
    fake.waitResult = VK_SUCCESS;
    Check(fences.Prepare(0u) == VK_SUCCESS && fake.resets == 2, "Verified previous operation permits per-image reuse");
    fences.Presented(0u,VK_SUBOPTIMAL_KHR);
    Check(fences.Prepare(1u) == VK_SUCCESS, "Another image uses its own fence");
    fences.Presented(1u,VK_ERROR_OUT_OF_DATE_KHR);
    Check(fences.Prepare(2u) == VK_SUCCESS, "Third image owns independent presentation retirement");
    fences.Presented(2u,VK_ERROR_SURFACE_LOST_KHR);
    fake.waitResult = VK_TIMEOUT;
    Check(fences.Drain() == VK_TIMEOUT && fake.lastWaitCount == 3u && !fences.DestroyCompleted(), "Recreation must retain every queued error/success present on timeout");
    fake.waitResult = VK_SUCCESS;
    Check(fences.Drain() == VK_SUCCESS && fences.DestroyCompleted() && fake.destroys == 3, "All historical present fences authorize retirement once complete");
    Check(fences.DestroyCompleted() && fake.destroys == 3, "Repeated cleanup never double-destroys");
    Check(fences.Create(Device(),1u,true,Dispatch()) && fences.Prepare(0u) == VK_SUCCESS, "Recreation has fresh unused fence ownership");
    fences.Presented(0u,VK_ERROR_OUT_OF_HOST_MEMORY);
    const int waits = fake.waits;
    Check(fences.Drain() == VK_SUCCESS && fake.waits == waits, "OOM did not enqueue, so never wait for its unassociated fence");
    Check(fences.DestroyCompleted(), "Graphics-idle OOM cleanup may destroy unused present fence");
}
void PartialAndFallback() {
    fake = {}; fake.failCreate = 2;
    PresentCompletionFences partial;
    Check(!partial.Create(Device(),3u,true,Dispatch()) && partial.Drain() == VK_SUCCESS, "Partial initialization has no invented pending present");
    Check(partial.DestroyCompleted() && fake.destroys == 1 && fake.live.empty(), "Partial cleanup destroys only actual allocated fence");
    fake = {}; PresentCompletionFences fallback;
    Check(fallback.Create(Device(),3u,false,Dispatch()) && fallback.Fence(2u) == VK_NULL_HANDLE && fallback.Prepare(2u) == VK_SUCCESS,
          "Unextended path neither allocates nor waits presentation fences");
    fallback.Presented(2u,VK_SUCCESS);
    Check(fallback.Drain() == VK_SUCCESS && fallback.DestroyCompleted() && fake.creates == 0, "Fallback remains explicit, not a fake completion fence");
    Check(!PresentOperationEnqueued(VK_ERROR_OUT_OF_DEVICE_MEMORY) && PresentOperationEnqueued(VK_ERROR_OUT_OF_DATE_KHR), "Error classification follows normative enqueue distinction");
    fake = {}; PresentCompletionFences lost;
    Check(lost.Create(Device(),1u,true,Dispatch()) && lost.Prepare(0u) == VK_SUCCESS, "Device-loss fixture initialized");
    lost.Presented(0u,VK_ERROR_DEVICE_LOST); fake.waitResult = VK_ERROR_DEVICE_LOST;
    Check(lost.Drain() == VK_ERROR_DEVICE_LOST && !lost.DestroyCompleted(), "Device loss never invents successful retirement proof");
}
}
int main() { FeaturesDispatch(); Selection(); Lifecycle(); PartialAndFallback(); return passed ? 0 : 1; }
