#pragma once
#include <vulkan/vulkan.h>
#include <string>

namespace horde::vulkan::raytracing {
// Command/fence ownership stays with the scene until polling proves completion
// or the platform supplies device-idle retirement proof. No queue-idle wait is
// part of submission or polling. Referenced buffers/AS share this lifetime.
struct RtGpuWorkLease {
    VkCommandBuffer command = VK_NULL_HANDLE;
    VkFence fence = VK_NULL_HANDLE;
    bool submitted = false;
    bool completionUnproved = false;
};
struct RtGpuWorkApi {
    VkDevice device{}; VkQueue queue{}; VkCommandPool pool{};
    PFN_vkAllocateCommandBuffers allocate = vkAllocateCommandBuffers;
    PFN_vkFreeCommandBuffers free = vkFreeCommandBuffers;
    PFN_vkBeginCommandBuffer begin = vkBeginCommandBuffer;
    PFN_vkEndCommandBuffer end = vkEndCommandBuffer;
    PFN_vkCreateFence createFence = vkCreateFence;
    PFN_vkDestroyFence destroyFence = vkDestroyFence;
    PFN_vkQueueSubmit submit = vkQueueSubmit;
    PFN_vkGetFenceStatus status = vkGetFenceStatus;
};
enum class RtGpuWorkStatus { Empty, Pending, Complete, Unproved };
inline void RetireRtGpuWorkAfterIdle(const RtGpuWorkApi& api, RtGpuWorkLease& owner) {
    if(owner.command) api.free(api.device,api.pool,1,&owner.command);
    if(owner.fence) api.destroyFence(api.device,owner.fence,nullptr);
    owner={};
}
inline bool SubmitRtGpuWork(const RtGpuWorkApi& api,RtGpuWorkLease& owner,
    void (*record)(VkCommandBuffer,void*),void* user,std::string& diagnostic) {
    if(owner.command || owner.fence || owner.submitted || !api.device || !api.queue || !api.pool || !record ||
       !api.allocate || !api.free || !api.begin || !api.end || !api.createFence || !api.destroyFence || !api.submit || !api.status) {
        diagnostic="Queued RT work requires an empty owner and a valid device/queue/pool.";return false;
    }
    const VkFenceCreateInfo fence{VK_STRUCTURE_TYPE_FENCE_CREATE_INFO};
    VkResult result=api.createFence(api.device,&fence,nullptr,&owner.fence);
    const VkCommandBufferAllocateInfo allocation{VK_STRUCTURE_TYPE_COMMAND_BUFFER_ALLOCATE_INFO,nullptr,api.pool,VK_COMMAND_BUFFER_LEVEL_PRIMARY,1};
    if(result==VK_SUCCESS) result=api.allocate(api.device,&allocation,&owner.command);
    const VkCommandBufferBeginInfo begin{VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO,nullptr,VK_COMMAND_BUFFER_USAGE_ONE_TIME_SUBMIT_BIT,nullptr};
    if(result==VK_SUCCESS) result=api.begin(owner.command,&begin);
    if(result==VK_SUCCESS) {
        try{record(owner.command,user);}catch(...){RetireRtGpuWorkAfterIdle(api,owner);diagnostic="Queued RT recording failed before submission.";return false;}
        result=api.end(owner.command);
    }
    const VkSubmitInfo submit{VK_STRUCTURE_TYPE_SUBMIT_INFO,nullptr,0,nullptr,nullptr,1,&owner.command,0,nullptr};
    if(result==VK_SUCCESS) {
        result=api.submit(api.queue,1,&submit,owner.fence);
        // Only memory-allocation errors guarantee unaffected submitted resources.
        // Device loss or an unknown submit outcome retains every referenced owner.
        if(result!=VK_SUCCESS && result!=VK_ERROR_OUT_OF_HOST_MEMORY && result!=VK_ERROR_OUT_OF_DEVICE_MEMORY) {
            owner.submitted=true;owner.completionUnproved=true;
            diagnostic="Queued RT submission has an unproved outcome.";return false;
        }
    }
    if(result!=VK_SUCCESS) {
        // Failure before submit, or submit explicitly guaranteed unaffected.
        RetireRtGpuWorkAfterIdle(api,owner);diagnostic="Queued RT command/fence allocation or submission failed.";return false;
    }
    owner.submitted=true;diagnostic.clear();return true;
}
inline RtGpuWorkStatus PollRtGpuWork(const RtGpuWorkApi& api,RtGpuWorkLease& owner,
    std::string& diagnostic) {
    if(!owner.submitted){diagnostic.clear();return RtGpuWorkStatus::Empty;}
    if(owner.completionUnproved){diagnostic="Queued RT work requires external retirement proof.";return RtGpuWorkStatus::Unproved;}
    const auto result=api.status(api.device,owner.fence);
    if(result==VK_NOT_READY){diagnostic.clear();return RtGpuWorkStatus::Pending;}
    if(result!=VK_SUCCESS){owner.completionUnproved=true;diagnostic="Queued RT build has unproved GPU completion.";return RtGpuWorkStatus::Unproved;}
    RetireRtGpuWorkAfterIdle(api,owner);diagnostic.clear();return RtGpuWorkStatus::Complete;
}
}
