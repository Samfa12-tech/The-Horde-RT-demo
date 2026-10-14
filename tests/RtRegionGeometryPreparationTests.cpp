#include "vulkan/raytracing/RtRegionGeometryPreparation.h"
// Reuse the unchanged allocation/mapping fault fixture; its original contract
// remains a separate executed CTest. This does not execute Vulkan hardware.
#define main UnusedOriginalBufferBatchContract
#include "RtGpuBufferBatchTests.cpp"
#undef main

namespace {
unsigned commandFrees,fenceDestroys,asCreates,asDestroys,buildCalls,queueSubmits,barriers;
bool asAlive,badBarrier,badBuild;
enum class RegionFailure { None, Sizes, Create, Address, Fence, Command, Begin, End };
RegionFailure regionFailure=RegionFailure::None;
VkResult regionFence=VK_NOT_READY,regionSubmit=VK_SUCCESS;
VkPipelineStageFlags expectedConsumers;
VKAPI_ATTR VkResult VKAPI_CALL AllocateCommands(VkDevice,const VkCommandBufferAllocateInfo*,VkCommandBuffer* p){if(regionFailure==RegionFailure::Command)return VK_ERROR_OUT_OF_HOST_MEMORY;*p=Handle<VkCommandBuffer>(101);return VK_SUCCESS;}
VKAPI_ATTR void VKAPI_CALL FreeCommands(VkDevice,VkCommandPool,std::uint32_t,const VkCommandBuffer*){++commandFrees;}
VKAPI_ATTR VkResult VKAPI_CALL BeginCommand(VkCommandBuffer,const VkCommandBufferBeginInfo*){return regionFailure==RegionFailure::Begin?VK_ERROR_OUT_OF_HOST_MEMORY:VK_SUCCESS;}
VKAPI_ATTR VkResult VKAPI_CALL EndCommand(VkCommandBuffer){return regionFailure==RegionFailure::End?VK_ERROR_OUT_OF_HOST_MEMORY:VK_SUCCESS;}
VKAPI_ATTR VkResult VKAPI_CALL CreateFence(VkDevice,const VkFenceCreateInfo*,const VkAllocationCallbacks*,VkFence* p){if(regionFailure==RegionFailure::Fence)return VK_ERROR_OUT_OF_HOST_MEMORY;*p=Handle<VkFence>(102);return VK_SUCCESS;}
VKAPI_ATTR void VKAPI_CALL DestroyFence(VkDevice,VkFence,const VkAllocationCallbacks*){++fenceDestroys;}
VKAPI_ATTR VkResult VKAPI_CALL Submit(VkQueue,std::uint32_t,const VkSubmitInfo*,VkFence){++queueSubmits;return regionSubmit;}
VKAPI_ATTR VkResult VKAPI_CALL FenceStatus(VkDevice,VkFence){return regionFence;}
VKAPI_ATTR void VKAPI_CALL BuildSizes(VkDevice,VkAccelerationStructureBuildTypeKHR,const VkAccelerationStructureBuildGeometryInfoKHR* info,const std::uint32_t* counts,VkAccelerationStructureBuildSizesInfoKHR* out){
    badBuild|=info->geometryCount!=1 || counts[0]!=2;if(regionFailure==RegionFailure::Sizes)return;out->accelerationStructureSize=256;out->buildScratchSize=128;
}
VKAPI_ATTR VkResult VKAPI_CALL CreateAs(VkDevice,const VkAccelerationStructureCreateInfoKHR* info,const VkAllocationCallbacks*,VkAccelerationStructureKHR* out){
    badBuild|=!state.buffers[Id(info->buffer)] || asAlive;if(regionFailure==RegionFailure::Create)return VK_ERROR_OUT_OF_DEVICE_MEMORY;asAlive=true;++asCreates;*out=Handle<VkAccelerationStructureKHR>(103);return VK_SUCCESS;
}
VKAPI_ATTR void VKAPI_CALL DestroyAs(VkDevice,VkAccelerationStructureKHR,const VkAllocationCallbacks*){badBuild|=!asAlive;asAlive=false;++asDestroys;}
VKAPI_ATTR VkDeviceAddress VKAPI_CALL AsAddress(VkDevice,const VkAccelerationStructureDeviceAddressInfoKHR*){return regionFailure==RegionFailure::Address?0:0x700000;}
VKAPI_ATTR void VKAPI_CALL BuildAs(VkCommandBuffer,std::uint32_t count,const VkAccelerationStructureBuildGeometryInfoKHR* info,const VkAccelerationStructureBuildRangeInfoKHR* const* ranges){
    ++buildCalls;badBuild|=count!=1 || !asAlive || ranges[0][0].primitiveCount!=2 || info->geometryCount!=1 || info->scratchData.deviceAddress%256;
    const auto& g=info->pGeometries[0];badBuild|=g.flags!=VK_GEOMETRY_OPAQUE_BIT_KHR || g.geometry.triangles.maxVertex!=3 ||
        g.geometry.triangles.vertexData.deviceAddress!=4096 || g.geometry.triangles.indexData.deviceAddress!=8192;
}
void Reset(){state={};commandFrees=fenceDestroys=asCreates=asDestroys=buildCalls=queueSubmits=barriers=0;asAlive=badBarrier=badBuild=false;regionFence=VK_NOT_READY;regionSubmit=VK_SUCCESS;regionFailure=RegionFailure::None;}
RtRegionGeometryBuildContext Context(VkPipelineStageFlags consumers){
    expectedConsumers=consumers;
    return {{Handle<VkDevice>(2),Handle<VkQueue>(3),Handle<VkCommandPool>(4),AllocateCommands,FreeCommands,BeginCommand,EndCommand,CreateFence,DestroyFence,Submit,FenceStatus},BuildSizes,CreateAs,AsAddress,BuildAs,256,100,32,consumers};
}
std::shared_ptr<const horde::world::streaming::GeometryPayload> Payload(){
    auto p=std::make_shared<horde::world::streaming::GeometryPayload>();p->vertices.resize(4);
    for(unsigned i=0;i<4;++i){p->vertices[i].position={float(i),0,float(i%2),1};p->vertices[i].normal={0,1,0,0};}
    p->indices={0,1,2,1,3,2};p->vertexCounts={4};p->primitives={{0,0,6,0}};p->sourceTriangles={0,1};p->bounds={{0,0,0},{3,0,1}};return p;
}
void DestroyTaken(RtGpuResources& resources,RtRegionGeometryOwner& owner){
    // Test has no descriptor/TLAS consumers and a completed build fence.
    resources.DestroyAccelerationStructure(owner.blas);for(auto& b:owner.buffers)resources.DestroyBuffer(b);owner.ResetEmpty();
}
}
extern "C" VKAPI_ATTR void VKAPI_CALL vkCmdPipelineBarrier(VkCommandBuffer,VkPipelineStageFlags source,VkPipelineStageFlags destination,VkDependencyFlags,std::uint32_t count,const VkMemoryBarrier*,std::uint32_t,const VkBufferMemoryBarrier*,std::uint32_t,const VkImageMemoryBarrier*){
    ++barriers;badBarrier|=count!=1 || (destination&~(VK_PIPELINE_STAGE_ACCELERATION_STRUCTURE_BUILD_BIT_KHR|expectedConsumers)) ||
        !(destination&expectedConsumers) || !(source&(VK_PIPELINE_STAGE_HOST_BIT|VK_PIPELINE_STAGE_ACCELERATION_STRUCTURE_BUILD_BIT_KHR));
}
int main(){
    static_assert(!std::is_copy_constructible_v<RtRegionGeometryOwner> && !std::is_copy_assignable_v<RtRegionGeometryOwner>);
    static_assert(std::is_nothrow_move_constructible_v<RtRegionGeometryOwner> && !std::is_move_assignable_v<RtRegionGeometryOwner>);
    using Token=horde::world::streaming::Token;
    bool ok=true;std::string error;const Token token{1,1,20},stale{1,2,20};
    std::array<RtMaterialGpu,1> materials{};
    RtGpuResources resources;resources.Bind(Handle<VkPhysicalDevice>(1),Handle<VkDevice>(2),DestroyAs,Address);
    for(auto consumer:{VK_PIPELINE_STAGE_RAY_TRACING_SHADER_BIT_KHR,VK_PIPELINE_STAGE_COMPUTE_SHADER_BIT}){
        Reset();RtRegionGeometryPreparation prep(resources,Context(consumer));RtRegionGeometryOwner taken;
        ok&=Require(prep.Begin(token,Payload(),materials,error) && state.calls==0,"valid begin allocates nothing");
        ok&=Require(!prep.Begin(token,Payload(),materials,error),"cannot overwrite active preparation");
        ok&=Require(!prep.Advance(stale,31,error) && !state.calls,"stale upload cannot allocate");
        unsigned steps=0;
        while(prep.Status()==RtRegionGeometryPreparation::Phase::Upload && steps++<100){
            const auto before=state.row;ok&=Require(prep.Advance(token,31,error) && state.row<=before+1 && prep.LastCopiedBytes()<=31,"bounded row allocation and copy");
            ok&=Require(!prep.Take(token,taken,error) && taken.Empty(),"partial upload cannot publish addresses");
        }
        ok&=Require(prep.Status()==RtRegionGeometryPreparation::Phase::Build && prep.Advance(token,31,error) && !prep.LastCopiedBytes() && buildCalls==1 && barriers==2 && !badBarrier && !badBuild,"one immutable BLAS submission uses admitted backend stages and exact geometry");
        const auto alive=state.allocations-state.frees;
        for(unsigned i=0;i<3;++i)ok&=Require(prep.Advance(token,31,error) && state.allocations-state.frees==alive && !prep.Take(token,taken,error),"pending build retains every source/backing/scratch owner");
        regionFence=VK_SUCCESS;
        ok&=Require(prep.Advance(token,31,error) && prep.Status()==RtRegionGeometryPreparation::Phase::Ready && state.frees==1 && commandFrees==1 && fenceDestroys==1,"fence completion releases scratch/command once");
        ok&=Require(!prep.Take(stale,taken,error) && prep.Take(token,taken,error) && taken.blas.address && taken.buffers[2].address,"only complete exact-token owner can transfer");
        ok&=Require(!taken.ResetEmpty() && !taken.TransferTo(taken),"live owner cannot be cleared or self-transferred");
        RtRegionGeometryOwner moved(std::move(taken));
        RtRegionGeometryOwner occupied;occupied.token=token;
        ok&=Require(taken.Empty() && !moved.TransferTo(occupied) && moved.blas.handle && occupied.ResetEmpty(),"move empties source and refuses occupied destination");
        ok&=Require(moved.TransferTo(taken) && moved.Empty(),"explicit transfer preserves sole ownership");
        DestroyTaken(resources,taken);ok&=Require(NoLiveResources() && !asAlive && asDestroys==1,"taken owner explicit cleanup is exact");
    }
    for(bool unproved:{false,true}){
        Reset();RtRegionGeometryPreparation prep(resources,Context(VK_PIPELINE_STAGE_COMPUTE_SHADER_BIT));
        ok&=Require(prep.Begin(token,Payload(),materials,error),"cancellation setup");
        while(prep.Status()==RtRegionGeometryPreparation::Phase::Upload)ok&=Require(prep.Advance(token,256,error),"complete offside upload");
        regionSubmit=unproved?VK_ERROR_DEVICE_LOST:VK_SUCCESS;
        const bool submitted=prep.Advance(token,256,error);
        ok&=Require(submitted!=unproved,"unproved submission rejected");
        const auto frees=state.frees;
        if(unproved){
            ok&=Require(prep.Status()==RtRegionGeometryPreparation::Phase::Quarantined && !prep.RequestCancel(token,error) && state.frees==frees,"unproved work cannot be canceled/free'd");
            prep.DestroyAfterDeviceIdle();
        }else{
            ok&=Require(prep.RequestCancel(token,error) && prep.Status()==RtRegionGeometryPreparation::Phase::CancelPending && prep.Advance(token,256,error) && state.frees==frees,"cancel request does not destroy pending GPU input");
            regionFence=VK_SUCCESS;ok&=Require(prep.Advance(token,256,error) && prep.Status()==RtRegionGeometryPreparation::Phase::Idle,"real completion acknowledges cancellation");
        }
        ok&=Require(NoLiveResources() && !asAlive,"cancellation/idle-proof quarantine release retires all resources");
    }
    Reset();{
        RtRegionGeometryPreparation prep(resources,Context(VK_PIPELINE_STAGE_COMPUTE_SHADER_BIT));
        auto bad=materials;bad[0].materialFlags[0]=static_cast<unsigned>(RtMaterialFlag::Transmission);
        ok&=Require(!prep.Begin(token,Payload(),bad,error) && !state.calls,"uncertain transparency rejected before GPU allocation");
        ok&=Require(prep.Begin(token,Payload(),materials,error) && prep.Advance(token,16,error) && prep.RequestCancel(token,error) && NoLiveResources(),"pre-submit cancellation releases partial upload");
    }
    // Every known pre-submit failure is recoverable only after exact-token
    // acknowledgement. Retry uses the same wrapper, not a replacement fixture.
    const auto recover=[&](RtRegionGeometryPreparation& prep){
        ok&=Require(prep.Status()==RtRegionGeometryPreparation::Phase::Failed && NoLiveResources() && !asAlive,"known failure releases offside owners");
        ok&=Require(!prep.ResetFailed(stale) && !prep.Begin(stale,Payload(),materials,error) && prep.ResetFailed(token),"failure retry needs exact acknowledgement");
        Reset();regionFence=VK_SUCCESS;
        ok&=Require(prep.Begin(stale,Payload(),materials,error),"same owner accepts explicit retry");
        unsigned turns=0;
        while(prep.Status()!=RtRegionGeometryPreparation::Phase::Ready && turns++<20)ok&=Require(prep.Advance(stale,256,error),"retry completes bounded preparation");
        RtRegionGeometryOwner ready;
        ok&=Require(prep.Take(stale,ready,error),"retry publishes complete owner");
        DestroyTaken(resources,ready);ok&=Require(NoLiveResources() && !asAlive && !badBuild,"retry cleanup leaves no duplicates or leaks");
    };
    for(auto failure:{Failure::Create,Failure::Allocate,Failure::Bind,Failure::Map,Failure::NullMap,Failure::Address})
        for(unsigned row=1;row<=5;++row){
            // BLAS backing/scratch aren't mapped, so mapping failures only
            // apply to the three host-visible immutable upload rows.
            if(row>3 && (failure==Failure::Map || failure==Failure::NullMap))continue;
            // AS backing is addressed by VkBuffer at creation, not its BDA;
            // AS-address failure is separately injected through AsAddress.
            if(row==4 && failure==Failure::Address)continue;
            Reset();RtRegionGeometryPreparation prep(resources,Context(VK_PIPELINE_STAGE_COMPUTE_SHADER_BIT));
            state.failure=failure;state.failureRow=row;
            ok&=Require(prep.Begin(token,Payload(),materials,error),"allocation failure setup");
            unsigned turns=0;
            while(prep.Status()!=RtRegionGeometryPreparation::Phase::Failed && turns++<20)prep.Advance(token,256,error);
            recover(prep);
        }
    for(auto failure:{RegionFailure::Sizes,RegionFailure::Create,RegionFailure::Address,RegionFailure::Fence,RegionFailure::Command,RegionFailure::Begin,RegionFailure::End}){
        Reset();RtRegionGeometryPreparation prep(resources,Context(VK_PIPELINE_STAGE_COMPUTE_SHADER_BIT));regionFailure=failure;
        ok&=Require(prep.Begin(token,Payload(),materials,error),"AS/work failure setup");
        unsigned turns=0;
        while(prep.Status()!=RtRegionGeometryPreparation::Phase::Failed && turns++<20)prep.Advance(token,256,error);
        recover(prep);
    }
    for(auto result:{VK_ERROR_OUT_OF_HOST_MEMORY,VK_ERROR_OUT_OF_DEVICE_MEMORY}){
        Reset();RtRegionGeometryPreparation prep(resources,Context(VK_PIPELINE_STAGE_COMPUTE_SHADER_BIT));regionSubmit=result;
        ok&=Require(prep.Begin(token,Payload(),materials,error),"unaffected submit failure setup");
        unsigned turns=0;
        while(prep.Status()!=RtRegionGeometryPreparation::Phase::Failed && turns++<20)prep.Advance(token,256,error);
        recover(prep);
    }
    if(ok)std::cout<<"Region offside GPU ownership/upload/build/cancel contracts passed with injected Vulkan; no device pass.\n";
    return ok?0:1;
}
