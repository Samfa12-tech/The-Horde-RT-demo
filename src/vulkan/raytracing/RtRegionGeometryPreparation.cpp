#include "vulkan/raytracing/RtRegionGeometryPreparation.h"
#include "world/streaming/GeometryPageTable.h"
#include <algorithm>
#include <cassert>
#include <cmath>
#include <utility>

namespace horde::vulkan::raytracing {
RtRegionGeometryOwner::RtRegionGeometryOwner(RtRegionGeometryOwner&& other) noexcept
    :token(std::exchange(other.token,{})),payload(std::move(other.payload)),
     buffers(std::exchange(other.buffers,{})),blas(std::exchange(other.blas,{})) {}
bool RtRegionGeometryOwner::TransferTo(RtRegionGeometryOwner& destination) noexcept {
    if(this==&destination || !destination.Empty())return false;
    destination.token=std::exchange(token,{});destination.payload=std::move(payload);
    destination.buffers=std::exchange(buffers,{});destination.blas=std::exchange(blas,{});return true;
}
bool RtRegionGeometryOwner::ResetEmpty() noexcept {
    if(blas.handle || blas.address || !blas.backing.Empty() ||
       std::any_of(buffers.begin(),buffers.end(),[](const auto& b){return !b.Empty();}))return false;
    token={};payload.reset();return true;
}
bool RtRegionGeometryOwner::Empty() const noexcept {
    return !payload && token==horde::world::streaming::Token{} && !blas.handle && !blas.address && blas.backing.Empty() &&
        std::all_of(buffers.begin(),buffers.end(),[](const auto& b){return b.Empty();});
}
void RtRegionGeometryOwner::AccumulateResourceInventory(horde::telemetry::RtResourceInventory& r) const noexcept {
    for(const auto& b:buffers)AccumulateRtGpuBuffer(r,b);
    AccumulateRtGpuBuffer(r,blas.backing);
    if(blas.handle)AccumulateRtResourceCount(r.bottomLevelAccelerationStructureCount);
}
RtRegionGeometryPreparation::RtRegionGeometryPreparation(const RtGpuResources& resources,RtRegionGeometryBuildContext context)
    :resources_(resources),context_(context),uploads_(resources) {}
RtRegionGeometryPreparation::~RtRegionGeometryPreparation() {
    assert(!work_.submitted && "Retain pending/quarantined region owners until completion or device-idle cleanup");
    if(!work_.submitted)DestroyUnsubmitted(); // Offside owners have no TLAS/descriptor users.
}
bool RtRegionGeometryPreparation::Begin(horde::world::streaming::Token token,
    std::shared_ptr<const horde::world::streaming::GeometryPayload> payload,
    std::span<const RtMaterialGpu> materials,std::string& error) {
    if(phase_!=Phase::Idle || !owner_.Empty() || !token.worldGeneration || !token.requestGeneration || !token.region ||
       !payload || !context_.work.device || resources_.device_!=context_.work.device ||
       !context_.work.queue || !context_.work.pool || !context_.work.allocate || !context_.work.free ||
       !context_.work.begin || !context_.work.end || !context_.work.createFence || !context_.work.destroyFence ||
       !context_.work.submit || !context_.work.status ||
       !context_.sizes || !context_.create || !context_.address || !context_.build || !context_.scratchAlignment ||
       (context_.scratchAlignment&(context_.scratchAlignment-1)) || !context_.maxPrimitiveCount || !context_.maxGeometryCount ||
       !context_.consumerStages || (context_.consumerStages&~(VK_PIPELINE_STAGE_RAY_TRACING_SHADER_BIT_KHR|VK_PIPELINE_STAGE_COMPUTE_SHADER_BIT)) ||
       payload->vertices.empty() || payload->indices.empty() || payload->primitives.empty() ||
       payload->primitives.size()>kRtPrimitiveMetadataCapacity || payload->primitives.size()>context_.maxGeometryCount ||
       payload->primitives.size()!=payload->vertexCounts.size() || !payload->bounds.Valid() ||
       materials.empty() || materials.size()>kRtMaterialCapacity) {
        error="Region GPU preparation needs an empty owner, immutable bounded payload and admitted hardware context.";return false;
    }
    std::uint64_t triangles=0,indexEnd=0,vertexEnd=0;
    for(std::size_t p=0;p<payload->primitives.size();++p) {
        const auto& row=payload->primitives[p];
        if(!row.indexCount || row.indexCount%3 || row.indexOffset!=indexEnd ||
           indexEnd>payload->indices.size() || row.indexCount>payload->indices.size()-indexEnd ||
           !payload->vertexCounts[p] || row.vertexOffset!=vertexEnd || row.vertexOffset>=payload->vertices.size() ||
           payload->vertexCounts[p]>payload->vertices.size()-row.vertexOffset || row.materialIndex>=materials.size()) {
            error="Region GPU payload has an invalid primitive/index/vertex/material span.";return false;
        }
        indexEnd+=row.indexCount;vertexEnd+=payload->vertexCounts[p];triangles+=row.indexCount/3;
        const auto& m=materials[row.materialIndex];
        if((m.materialFlags[0]&(static_cast<unsigned>(RtMaterialFlag::Alpha)|static_cast<unsigned>(RtMaterialFlag::Transmission))) ||
           !std::isfinite(m.metallicRoughnessOcclusionTransmission[3]) || m.metallicRoughnessOcclusionTransmission[3]!=0) {
            error="First immutable region owner requires admitted opaque material rows.";return false;
        }
        for(std::size_t i=row.indexOffset;i<indexEnd;++i)if(payload->indices[i]>=payload->vertexCounts[p]) {
            error="Region GPU payload index leaves its primitive vertex domain.";return false;
        }
    }
    if(indexEnd!=payload->indices.size() || vertexEnd!=payload->vertices.size() || triangles>context_.maxPrimitiveCount) {
        error="Region GPU payload exceeds admitted geometry/primitive hardware limits.";return false;
    }
    for(const auto& v:payload->vertices) {
        for(const auto* attributes:{&v.position,&v.normal,&v.tangent,&v.uv0})
            for(const auto a:*attributes)if(!std::isfinite(a)) {error="Region GPU payload has nonfinite attributes.";return false;}
        for(unsigned a=0;a<3;++a)if(v.position[a]<payload->bounds.min[a] || v.position[a]>payload->bounds.max[a]) {
            error="Region GPU payload leaves its admitted contributor bounds.";return false;
        }
    }
    owner_.token=token;owner_.payload=std::move(payload);
    const auto& data=*owner_.payload;
    constexpr auto storage=VK_BUFFER_USAGE_STORAGE_BUFFER_BIT;
    constexpr auto geometry=storage|VK_BUFFER_USAGE_ACCELERATION_STRUCTURE_BUILD_INPUT_READ_ONLY_BIT_KHR;
    const std::array<RtGpuBufferUpload,3> uploads{{
        {data.vertices.data(),data.vertices.size()*sizeof(StaticRtVertex),geometry,true,false,"region vertices",VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT},
        {data.indices.data(),data.indices.size()*sizeof(std::uint32_t),geometry,true,false,"region indices",VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT},
        {data.primitives.data(),data.primitives.size()*sizeof(RtPrimitiveMetadata),storage,true,false,"region primitive metadata",VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT}}};
    if(!uploads_.Begin(uploads,error)){owner_.ResetEmpty();return false;}
    lastCopiedBytes_=0;phase_=Phase::Upload;return true;
}
void RtRegionGeometryPreparation::DestroyUnsubmitted() noexcept {
    uploads_.Cancel();
    resources_.DestroyAccelerationStructure(owner_.blas);
    for(auto& b:owner_.buffers)resources_.DestroyBuffer(b);
    resources_.DestroyBuffer(scratch_);owner_.ResetEmpty();geometries_.clear();ranges_.clear();buildInfo_={};phase_=Phase::Idle;lastCopiedBytes_=0;
}
void RtRegionGeometryPreparation::DestroyAfterDeviceIdle() noexcept {
    RetireRtGpuWorkAfterIdle(context_.work,work_);DestroyUnsubmitted();
}
bool RtRegionGeometryPreparation::Build(std::string& error) {
    const auto& payload=*owner_.payload;
    const auto& vertices=owner_.buffers[0];const auto& indices=owner_.buffers[1];
    if(!horde::world::streaming::ValidRange({vertices.address,vertices.size},16,sizeof(StaticRtVertex)) ||
       !horde::world::streaming::ValidRange({indices.address,indices.size},4,12) ||
       !horde::world::streaming::ValidRange({owner_.buffers[2].address,owner_.buffers[2].size},16,
                                         payload.primitives.size()*sizeof(RtPrimitiveMetadata))) {
        error="Region GPU upload produced an invalid address domain.";return false;
    }
    geometries_.resize(payload.primitives.size());ranges_.resize(payload.primitives.size());
    std::array<std::uint32_t,kRtPrimitiveMetadataCapacity> counts{};
    for(std::size_t p=0;p<payload.primitives.size();++p) {
        const auto& row=payload.primitives[p];auto& g=geometries_[p];
        g={VK_STRUCTURE_TYPE_ACCELERATION_STRUCTURE_GEOMETRY_KHR};g.geometryType=VK_GEOMETRY_TYPE_TRIANGLES_KHR;
        g.flags=VK_GEOMETRY_OPAQUE_BIT_KHR;auto& t=g.geometry.triangles;
        t={VK_STRUCTURE_TYPE_ACCELERATION_STRUCTURE_GEOMETRY_TRIANGLES_DATA_KHR};
        t.vertexFormat=VK_FORMAT_R32G32B32_SFLOAT;t.vertexStride=sizeof(StaticRtVertex);
        t.vertexData.deviceAddress=vertices.address+VkDeviceSize(row.vertexOffset)*sizeof(StaticRtVertex);
        t.maxVertex=payload.vertexCounts[p]-1;t.indexType=VK_INDEX_TYPE_UINT32;
        t.indexData.deviceAddress=indices.address+VkDeviceSize(row.indexOffset)*sizeof(std::uint32_t);
        counts[p]=row.indexCount/3;ranges_[p]={counts[p],0,0,0};
    }
    buildInfo_={VK_STRUCTURE_TYPE_ACCELERATION_STRUCTURE_BUILD_GEOMETRY_INFO_KHR};
    buildInfo_.type=VK_ACCELERATION_STRUCTURE_TYPE_BOTTOM_LEVEL_KHR;
    buildInfo_.flags=VK_BUILD_ACCELERATION_STRUCTURE_PREFER_FAST_TRACE_BIT_KHR;
    buildInfo_.mode=VK_BUILD_ACCELERATION_STRUCTURE_MODE_BUILD_KHR;
    buildInfo_.geometryCount=static_cast<std::uint32_t>(geometries_.size());buildInfo_.pGeometries=geometries_.data();
    VkAccelerationStructureBuildSizesInfoKHR sizes{VK_STRUCTURE_TYPE_ACCELERATION_STRUCTURE_BUILD_SIZES_INFO_KHR};
    context_.sizes(context_.work.device,VK_ACCELERATION_STRUCTURE_BUILD_TYPE_DEVICE_KHR,&buildInfo_,counts.data(),&sizes);
    if(!sizes.accelerationStructureSize || !sizes.buildScratchSize) {error="Region BLAS hardware size query is empty.";return false;}
    if(!resources_.CreateBuffer(sizes.accelerationStructureSize,
          VK_BUFFER_USAGE_ACCELERATION_STRUCTURE_STORAGE_BIT_KHR|VK_BUFFER_USAGE_SHADER_DEVICE_ADDRESS_BIT,
          VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT,true,owner_.blas.backing,error))return false;
    VkAccelerationStructureCreateInfoKHR create{VK_STRUCTURE_TYPE_ACCELERATION_STRUCTURE_CREATE_INFO_KHR};
    create.buffer=owner_.blas.backing.buffer;create.size=sizes.accelerationStructureSize;
    create.type=VK_ACCELERATION_STRUCTURE_TYPE_BOTTOM_LEVEL_KHR;
    if(context_.create(context_.work.device,&create,nullptr,&owner_.blas.handle)!=VK_SUCCESS || !owner_.blas.handle) {
        error="Region BLAS creation failed.";return false;
    }
    VkAccelerationStructureDeviceAddressInfoKHR address{VK_STRUCTURE_TYPE_ACCELERATION_STRUCTURE_DEVICE_ADDRESS_INFO_KHR};
    address.accelerationStructure=owner_.blas.handle;owner_.blas.address=context_.address(context_.work.device,&address);
    if(!owner_.blas.address){error="Region BLAS has no device address.";return false;}
    if(!resources_.CreateAlignedBuffer(sizes.buildScratchSize,context_.scratchAlignment,
        VK_BUFFER_USAGE_STORAGE_BUFFER_BIT|VK_BUFFER_USAGE_SHADER_DEVICE_ADDRESS_BIT,
        VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT,scratch_,error))return false;
    buildInfo_.dstAccelerationStructure=owner_.blas.handle;buildInfo_.scratchData.deviceAddress=scratch_.AlignedAddress();
    const auto record=[](VkCommandBuffer command,void* opaque) {
        const auto& self=*static_cast<RtRegionGeometryPreparation*>(opaque);
        const VkMemoryBarrier host{VK_STRUCTURE_TYPE_MEMORY_BARRIER,nullptr,VK_ACCESS_HOST_WRITE_BIT,
            VK_ACCESS_ACCELERATION_STRUCTURE_READ_BIT_KHR|VK_ACCESS_SHADER_READ_BIT};
        vkCmdPipelineBarrier(command,VK_PIPELINE_STAGE_HOST_BIT,
            VK_PIPELINE_STAGE_ACCELERATION_STRUCTURE_BUILD_BIT_KHR|self.context_.consumerStages,0,1,&host,0,nullptr,0,nullptr);
        std::array<const VkAccelerationStructureBuildRangeInfoKHR*,1> ranges{{self.ranges_.data()}};
        self.context_.build(command,1,&self.buildInfo_,ranges.data());
        const VkMemoryBarrier built{VK_STRUCTURE_TYPE_MEMORY_BARRIER,nullptr,VK_ACCESS_ACCELERATION_STRUCTURE_WRITE_BIT_KHR,
            VK_ACCESS_ACCELERATION_STRUCTURE_READ_BIT_KHR|VK_ACCESS_SHADER_READ_BIT};
        vkCmdPipelineBarrier(command,VK_PIPELINE_STAGE_ACCELERATION_STRUCTURE_BUILD_BIT_KHR,
            VK_PIPELINE_STAGE_ACCELERATION_STRUCTURE_BUILD_BIT_KHR|self.context_.consumerStages,0,1,&built,0,nullptr,0,nullptr);
    };
    if(!SubmitRtGpuWork(context_.work,work_,record,this,error)) {
        if(work_.submitted){phase_=Phase::Quarantined;return false;}
        return false;
    }
    phase_=Phase::BuildWait;return true;
}
bool RtRegionGeometryPreparation::Advance(horde::world::streaming::Token token,VkDeviceSize bytes,std::string& error) {
    lastCopiedBytes_=0;
    if(token!=owner_.token || !token.region || !bytes) {error="Region GPU preparation has stale identity or zero copy allowance.";return false;}
    if(phase_==Phase::Upload) {
        if(!uploads_.Advance(bytes,error)){DestroyUnsubmitted();failedToken_=token;phase_=Phase::Failed;return false;}
        lastCopiedBytes_=uploads_.LastAdvanceCopiedBytes();
        if(uploads_.Status()==RtGpuBufferPreparation::State::Ready) {
            if(!uploads_.Take(owner_.buffers,error)){DestroyUnsubmitted();failedToken_=token;phase_=Phase::Failed;return false;}
            phase_=Phase::Build;
        }
        return true;
    }
    if(phase_==Phase::Build) {
        if(Build(error))return true;
        if(phase_!=Phase::Quarantined){DestroyUnsubmitted();failedToken_=token;phase_=Phase::Failed;}
        return false;
    }
    if(phase_==Phase::BuildWait || phase_==Phase::CancelPending) {
        const auto status=PollRtGpuWork(context_.work,work_,error);
        if(status==RtGpuWorkStatus::Pending)return true;
        if(status!=RtGpuWorkStatus::Complete){phase_=Phase::Quarantined;return false;}
        if(phase_==Phase::CancelPending){DestroyUnsubmitted();return true;}
        resources_.DestroyBuffer(scratch_);phase_=Phase::Ready;return true;
    }
    error="Region GPU preparation is not advanceable.";return false;
}
bool RtRegionGeometryPreparation::RequestCancel(horde::world::streaming::Token token,std::string& error) {
    if(token!=owner_.token || !token.region || phase_==Phase::Quarantined) {error="Region cancellation identity/completion is unproved.";return false;}
    if(work_.submitted) {phase_=Phase::CancelPending;error.clear();return true;}
    DestroyUnsubmitted();error.clear();return true;
}
bool RtRegionGeometryPreparation::Take(horde::world::streaming::Token token,RtRegionGeometryOwner& destination,std::string& error) {
    if(token!=owner_.token || phase_!=Phase::Ready || !destination.Empty()) {error="Region adoption needs exact complete identity and empty destination.";return false;}
    if(!owner_.TransferTo(destination)){error="Region destination already owns resources.";return false;}
    geometries_.clear();ranges_.clear();buildInfo_={};phase_=Phase::Idle;
    error.clear();return true;
}
bool RtRegionGeometryPreparation::ResetFailed(horde::world::streaming::Token token) noexcept {
    if(phase_!=Phase::Failed || token!=failedToken_ || !owner_.Empty() || work_.submitted)return false;
    failedToken_={};phase_=Phase::Idle;return true;
}
void RtRegionGeometryPreparation::AccumulateResourceInventory(horde::telemetry::RtResourceInventory& r) const noexcept {
    owner_.AccumulateResourceInventory(r);uploads_.AccumulateResourceInventory(r);AccumulateRtGpuBuffer(r,scratch_);
}
}
