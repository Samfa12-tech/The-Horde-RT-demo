#include "vulkan/raytracing/RtGpuWorkLease.h"
#include <iostream>
#include <type_traits>
#include <cstdint>
using namespace horde::vulkan::raytracing;
template<class T> T Handle(unsigned id) {
    if constexpr(std::is_pointer_v<T>) return reinterpret_cast<T>(static_cast<std::uintptr_t>(id));
    else return static_cast<T>(id);
}
unsigned step,failStep,frees,destroys,records,submits; VkResult fenceStatus;
VkResult submitResult=VK_SUCCESS;
VkResult Result(){return ++step==failStep?VK_ERROR_OUT_OF_HOST_MEMORY:VK_SUCCESS;}
VKAPI_ATTR VkResult VKAPI_CALL Allocate(VkDevice,const VkCommandBufferAllocateInfo*,VkCommandBuffer* out){auto r=Result();if(r==VK_SUCCESS)*out=Handle<VkCommandBuffer>(4);return r;}
VKAPI_ATTR void VKAPI_CALL Free(VkDevice,VkCommandPool,std::uint32_t,const VkCommandBuffer*){++frees;}
VKAPI_ATTR VkResult VKAPI_CALL Begin(VkCommandBuffer,const VkCommandBufferBeginInfo*){return Result();}
VKAPI_ATTR VkResult VKAPI_CALL End(VkCommandBuffer){return Result();}
VKAPI_ATTR VkResult VKAPI_CALL Create(VkDevice,const VkFenceCreateInfo*,const VkAllocationCallbacks*,VkFence* out){auto r=Result();if(r==VK_SUCCESS)*out=Handle<VkFence>(5);return r;}
VKAPI_ATTR void VKAPI_CALL Destroy(VkDevice,VkFence,const VkAllocationCallbacks*){++destroys;}
VKAPI_ATTR VkResult VKAPI_CALL Submit(VkQueue,std::uint32_t,const VkSubmitInfo*,VkFence){++submits;const auto r=Result();return r==VK_SUCCESS?submitResult:r;}
VKAPI_ATTR VkResult VKAPI_CALL Status(VkDevice,VkFence){return fenceStatus;}
void Record(VkCommandBuffer,void*){++records;}
void ThrowRecord(VkCommandBuffer,void*){throw 1;}
int main(){
    RtGpuWorkApi api{Handle<VkDevice>(1),Handle<VkQueue>(2),Handle<VkCommandPool>(3),Allocate,Free,Begin,End,Create,Destroy,Submit,Status};
    int failures=0; auto require=[&](bool ok,const char* msg){if(!ok){++failures;std::cerr<<msg<<'\n';}};
    std::string error;
    for(failStep=0;failStep<=5;++failStep){
        step=frees=destroys=records=submits=0;fenceStatus=VK_NOT_READY;RtGpuWorkLease owner;
        const bool queued=SubmitRtGpuWork(api,owner,Record,nullptr,error);
        require(queued==(failStep==0),"every allocation/record/submit failure rejects");
        if(queued){
            require(owner.submitted&&owner.command&&owner.fence&&records==1&&submits==1,"submitted lease owns command/fence");
            require(!SubmitRtGpuWork(api,owner,Record,nullptr,error)&&records==1,"cannot overwrite pending work");
            for(unsigned n=0;n<8;++n) require(PollRtGpuWork(api,owner,error)==RtGpuWorkStatus::Pending&&frees==0&&destroys==0,"poll is nonblocking and retains in-flight owner");
            fenceStatus=VK_SUCCESS;
            require(PollRtGpuWork(api,owner,error)==RtGpuWorkStatus::Complete&&!owner.submitted&&frees==1&&destroys==1,"successful fence proof releases command/fence once");
            require(PollRtGpuWork(api,owner,error)==RtGpuWorkStatus::Empty,"completed lease cannot masquerade as new work");
        }else require(!owner.command&&!owner.fence&&!owner.submitted,"pre-submit failures leave empty owner");
        const auto oldFrees=frees,oldDestroys=destroys;
        RetireRtGpuWorkAfterIdle(api,owner);RetireRtGpuWorkAfterIdle(api,owner);
        require(frees==oldFrees&&destroys==oldDestroys,"retirement is idempotent");
    }
    failStep=0;step=frees=destroys=records=submits=0;RtGpuWorkLease owner;
    require(SubmitRtGpuWork(api,owner,Record,nullptr,error),"shutdown lease submits");
    RetireRtGpuWorkAfterIdle(api,owner);
    require(frees==1&&destroys==1&&!owner.submitted,"external device-idle proof can retire queued work at shutdown");
    step=frees=destroys=records=submits=0;
    require(!SubmitRtGpuWork(api,owner,ThrowRecord,nullptr,error)&&frees==1&&destroys==1&&submits==0,
        "throwing recording frees only never-submitted work");
    auto invalid=api;invalid.status=nullptr;
    require(!SubmitRtGpuWork(invalid,owner,Record,nullptr,error)&&!owner.command&&!owner.fence,
        "incomplete dispatch contract rejects before allocation");
    step=frees=destroys=records=submits=0;
    require(SubmitRtGpuWork(api,owner,Record,nullptr,error),"failed-poll lease submits");
    fenceStatus=VK_ERROR_DEVICE_LOST;
    require(PollRtGpuWork(api,owner,error)==RtGpuWorkStatus::Unproved&&owner.submitted&&frees==0,
        "failed completion proof retains referenced work");
    fenceStatus=VK_SUCCESS;
    require(PollRtGpuWork(api,owner,error)==RtGpuWorkStatus::Unproved&&frees==0,
        "unproved completion is sticky until external retirement proof");
    RetireRtGpuWorkAfterIdle(api,owner);
    step=frees=destroys=records=submits=0;submitResult=VK_ERROR_DEVICE_LOST;
    require(!SubmitRtGpuWork(api,owner,Record,nullptr,error)&&owner.command&&owner.fence&&owner.submitted&&
        frees==0&&destroys==0,"device-lost submit cannot prove resources were unaffected");
    fenceStatus=VK_NOT_READY;
    require(PollRtGpuWork(api,owner,error)==RtGpuWorkStatus::Unproved&&owner.submitted&&frees==0,
        "unproved submission remains owned until external retirement proof");
    RetireRtGpuWorkAfterIdle(api,owner);
    require(frees==1&&destroys==1&&!owner.submitted,"explicit teardown proof retires uncertain submit once");
    return failures?1:0;
}
