#include "vulkan/raytracing/PresentableTinyRtScene.h"
#if HORDE_RT_REGION_PAGE_PROTOTYPE
#include <chrono>
#include <thread>
#include "gameplay/simulation/GameSimulation.h"
#include <algorithm>

namespace horde::vulkan::raytracing {
bool PresentableTinyRtScene::PrepareRegionPageSource(std::string& error) {
    if(!developmentRescueJourney_ || !chapterActorReady_ || sceneProfile_!=RtSceneProfile::Showcase || !ordinaryStaticOmissions_ ||
       !fullStaticGeometrySource_ || regionPageSource_ || !regionPageOwner_.Empty()) {
        error="Private region-page prototype requires the admitted chapter source/selection.";return false;
    }
    RtRegionGeometrySourceResult result;
    if(!RtRegionGeometrySource::Prepare(*fullStaticGeometrySource_,kCollapseInstanceIndex,
        ordinaryStaticOmissions_->Triangles(),128ull*1024*1024,result,error))return false;
    staticMeshSlot_=std::move(*result.base);regionPageSource_=std::move(result.detail);return true;
}
bool PresentableTinyRtScene::CreateInitialRegionPage(std::string& error) {
    VkPhysicalDeviceAccelerationStructurePropertiesKHR as{VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_ACCELERATION_STRUCTURE_PROPERTIES_KHR};
    VkPhysicalDeviceProperties2 properties{VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_PROPERTIES_2,&as};
    vkGetPhysicalDeviceProperties2(physicalDevice_,&properties);
    RtRegionGeometryBuildContext context{{device_,queue_,commandPool_},
        vkGetAccelerationStructureBuildSizesKHR_,vkCreateAccelerationStructureKHR_,
        vkGetAccelerationStructureDeviceAddressKHR_,vkCmdBuildAccelerationStructuresKHR_,
        as.minAccelerationStructureScratchOffsetAlignment,as.maxPrimitiveCount,as.maxGeometryCount,
        executionPolicy_.shaderPipelineStage};
    regionPagePreparation_=std::make_unique<RtRegionGeometryPreparation>(gpuResources_,context);
    const horde::world::streaming::Token token{staticGeometryGeneration_,1,1};
    if(!regionPagePreparation_->Begin(token,regionPageSource_,staticMeshSlot_.Materials(),error))return false;
    // Heavy startup only. Normal return uses bounded application-turn advances.
    // A timed-out/unproved submission stays owned until platform idle teardown.
    const auto end=std::chrono::steady_clock::now()+std::chrono::seconds(10);
    while(regionPagePreparation_->Status()!=RtRegionGeometryPreparation::Phase::Ready){
        if(!regionPagePreparation_->Advance(token,256u*1024u,error))return false;
        horde::telemetry::RtResourceInventory measured;
        regionPagePreparation_->AccumulateResourceInventory(measured);
        regionPreparationPeakBytes_=std::max(regionPreparationPeakBytes_,measured.deviceLocalBytes);
        if(std::chrono::steady_clock::now()>end){error="Initial region-page GPU preparation did not complete.";return false;}
        if(regionPagePreparation_->Status()==RtRegionGeometryPreparation::Phase::BuildWait)std::this_thread::yield();
    }
    return regionPagePreparation_->Take(token,regionPageOwner_,error);
}
bool PresentableTinyRtScene::ApplyRegionPage(
    std::array<VkAccelerationStructureInstanceKHR,kTlasInstanceCount>& instances,
    std::array<RtInstanceMetadata,kRtInstanceMetadataCapacity>& metadata,
    horde::world::streaming::GeometryPageTable& table,std::string& error) const {
    using namespace horde::world::streaming;
    for(std::size_t i=0;i<instances.size();++i)if(i!=kRegionPageTlasSlot && instances[i].mask &&
        instances[i].instanceCustomIndex==kRegionPageMetadataSlot) {
        error="Region metadata lane is referenced by another live instance.";return false;
    }
    if(instances[kRegionPageTlasSlot].mask || metadata[kRegionPageMetadataSlot].primitiveCount ||
       (metadata[kRegionPageMetadataSlot].flags&static_cast<std::uint32_t>(RtInstanceFlag::StaticPbr))) {
        error="Region page cannot overwrite a live scene-profile lane.";return false;
    }
    std::array<GeometryPageAddresses,kGeometryPages> pages{};
    pages[0]={{staticVertexBuffer_.address,staticVertexBuffer_.size},
        {staticIndexBuffer_.address,staticIndexBuffer_.size},
        {primitiveMetadataBuffer_.address,primitiveMetadataBuffer_.size},
        static_cast<std::uint32_t>(staticMeshSlot_.PrimitiveMetadata().size())};
    if(regionPageVisible_ && !regionPageOwner_.Empty()) {
        if(!regionPageOwner_.payload || !regionPageOwner_.blas.address || regionPageOwner_.token.worldGeneration!=staticGeometryGeneration_) {
            error="Region page owner has incomplete or stale GPU identity.";return false;
        }
        const auto count=static_cast<std::uint32_t>(regionPageOwner_.payload->primitives.size());
        // One logical imported asset, two storage owners. Preserve object and
        // material-route identity while replacing only primitive/page storage.
        auto& row=metadata[kRegionPageMetadataSlot];
        row=staticMeshSlot_.InstanceMetadata()[kCollapseInstanceIndex];
        row.primitiveBase=0;row.primitiveCount=count;
        row.flags=static_cast<std::uint32_t>(RtInstanceFlag::StaticPbr)|(1u<<kGeometryPageShift);
        auto& instance=instances[kRegionPageTlasSlot];instance={};
        instance.transform={{{1,0,0,0},{0,1,0,0},{0,0,1,0}}};
        instance.instanceCustomIndex=kRegionPageMetadataSlot;instance.mask=0x01;
        instance.flags=VK_GEOMETRY_INSTANCE_TRIANGLE_FACING_CULL_DISABLE_BIT_KHR;
        instance.accelerationStructureReference=regionPageOwner_.blas.address;
        pages[1]={{regionPageOwner_.buffers[0].address,regionPageOwner_.buffers[0].size},
            {regionPageOwner_.buffers[1].address,regionPageOwner_.buffers[1].size},
            {regionPageOwner_.buffers[2].address,regionPageOwner_.buffers[2].size},count};
    }
    if(!BuildGeometryPageTable(metadata,pages,table,staticMeshSlot_.InstanceMetadata(),worldSurfaceBuffer_.size/sizeof(std::uint32_t))){error="Region page address/metadata admission failed.";return false;}
    return true;
}
void PresentableTinyRtScene::DestroyRegionPage() noexcept {
    // Same platform-supplied device-idle contract as the surrounding scene.
    if(regionPagePreparation_)regionPagePreparation_->DestroyAfterDeviceIdle();
    regionPagePreparation_.reset();
    gpuResources_.DestroyAccelerationStructure(regionPageOwner_.blas);
    for(auto& buffer:regionPageOwner_.buffers)gpuResources_.DestroyBuffer(buffer);
    regionPageOwner_.ResetEmpty();regionPageSource_.reset();
    regionStreaming_={};regionCutover_=RegionCutover::None;regionCutoverToken_={};
    regionPageVisible_=true;regionRetirementAwaitingFrame_=false;regionPreparationRecovery_={};regionPreparationPeakBytes_=0;
}
bool PresentableTinyRtScene::AdvanceRegionPage(horde::gameplay::simulation::GameSimulation& simulation,
    const RtSceneFrameInputs& frame,bool safe,bool inner,bool allowEviction,StaticDetailFrame& decision,std::string& error) {
    using namespace horde::world::streaming;
    if(regionStreaming_.Definitions().empty()) {
        horde::telemetry::RtResourceInventory page,shared;
        regionPageOwner_.AccumulateResourceInventory(page);
        for(const auto* b:std::array{&staticVertexBuffer_,&staticIndexBuffer_,&primitiveMetadataBuffer_,
            &materialMetadataBuffer_,&instanceMetadataBuffer_,&staticGeometryTransformBuffer_,
            &vertexBuffer_,&indexBuffer_,&transformBuffer_,&worldSurfaceBuffer_,&blas_.backing,&collapseBlas_.backing})
            AccumulateRtGpuBuffer(shared,*b);
        for(const auto* t:std::array{&staticBaseColor_,&staticNormal_,&staticOrm_,&staticEmissive_})
            AccumulateRtMemoryAllocation(shared,t->memory,t->allocationSize,t->memoryPropertyFlags);
        StreamingRules rules;rules.maxResidentDetailOwners=1;rules.retireDelaySeconds=.25f;
        // The prototype's measured managed working set has no owner-approved
        // pressure target yet. Dynamic/other application allocations are separate.
        const std::array<RegionDefinition,2> regions{{
            {2,{{0,0,0},{0,0,0}},true,shared.deviceLocalBytes,{}},
            {1,regionPageSource_->bounds,false,regionPreparationPeakBytes_,{2}}}};
        if(!regionStreaming_.Configure(regions,rules,staticGeometryGeneration_,error) ||
           !regionStreaming_.SeedResident(2,shared.deviceLocalBytes) ||
           !regionStreaming_.SeedResident(1,page.deviceLocalBytes))return false;
        regionPageOwner_.token=regionStreaming_.States()[1].token;
    }
    const auto snapshot=simulation.Snapshot();
    const bool mayRetire=allowEviction && safe && inner;
    const std::array<RegionId,1> required{{1}};
    const std::array<Contribution,1> contribution{{{1,safe}}};
    Plan plan;
    const double seconds=std::chrono::duration<double>(std::chrono::steady_clock::now().time_since_epoch()).count();
    if(!regionStreaming_.Evaluate({staticGeometryGeneration_,seconds,{frame.cameraX,.7175f,frame.cameraZ},{},
        mayRetire?std::span<const RegionId>{}:std::span<const RegionId>{required},contribution},plan,error))return false;
    regionCutover_=RegionCutover::None;
    if(plan.count) {
        const auto action=plan.actions[0];regionCutoverToken_=action.token;
        switch(action.kind) {
        case ActionKind::Prepare:
            if(!regionPagePreparation_->Begin(action.token,regionPageSource_,staticMeshSlot_.Materials(),error)) {
                ++ordinaryStaticDetailEvidence_.preparationFailures;
                if(!regionPreparationRecovery_.Recover(regionStreaming_,*regionPagePreparation_,action.token,error))return false;
            }
            break;
        case ActionKind::AdvancePreparation: {
            if(!regionPagePreparation_->Advance(action.token,256u*1024u,error)) {
                ++ordinaryStaticDetailEvidence_.preparationFailures;
                if(!regionPreparationRecovery_.Recover(regionStreaming_,*regionPagePreparation_,action.token,error))return false;
                break;
            }
            horde::telemetry::RtResourceInventory memory;regionPagePreparation_->AccumulateResourceInventory(memory);
            if(!regionStreaming_.ObservePreparationAllocation(action.token,memory.deviceLocalBytes))return false;
            if(regionPagePreparation_->Status()==RtRegionGeometryPreparation::Phase::Ready &&
               !regionStreaming_.MarkPrepared(action.token,memory.deviceLocalBytes))return false;
            break;}
        case ActionKind::Adopt:
            regionCutover_=RegionCutover::Adopt;decision=StaticDetailFrame::Cutover;break;
        case ActionKind::Retire: {
            const horde::gameplay::simulation::StaticDetailLease lease{staticGeometryGeneration_,snapshot.worldRoute.generation,
                -27.5f,-16.6f,-16.55f,-13.85f,false};
            if(!safe || !simulation.HoldStaticDetailLease(lease))return false;
            regionCutover_=RegionCutover::Disable;decision=StaticDetailFrame::Cutover;break;}
        case ActionKind::CancelPreparation:
            if(!regionPagePreparation_->RequestCancel(action.token,error))return false;
            if(regionPagePreparation_->Status()==RtRegionGeometryPreparation::Phase::CancelPending) {
                if(!regionPagePreparation_->Advance(action.token,256u*1024u,error))return false;
            }
            if(regionPagePreparation_->Status()==RtRegionGeometryPreparation::Phase::Idle &&
               !regionStreaming_.MarkPreparationCancelled(action.token))return false;
            break;
        case ActionKind::CancelRetirement:
            // A completed removal is acknowledged before this evaluator runs;
            // until then the retained owner remains reactivatable, never freed.
            if(!regionStreaming_.MarkRetirementCancelled(action.token))return false;
            regionPageVisible_=true;regionRetirementAwaitingFrame_=false;
            staticGeometryRetainedAllTriangles_=true;ordinaryStaticReduced_=false;
            break;
        }
    }
    horde::telemetry::RtResourceInventory managed;
    regionPageOwner_.AccumulateResourceInventory(managed);
    regionPagePreparation_->AccumulateResourceInventory(managed);
    ordinaryStaticDetailEvidence_.peakOverlapOwnerBytes=std::max(
        ordinaryStaticDetailEvidence_.peakOverlapOwnerBytes,managed.deviceLocalBytes);
    if(!regionPageVisible_ && !safe && decision!=StaticDetailFrame::Cutover) {
        decision=StaticDetailFrame::WaitForRestore;++ordinaryStaticDetailEvidence_.waitFrames;
    }
    error.clear();return true;
}
bool PresentableTinyRtScene::RetireCompletedRegionPage(bool (*proof)(void*,std::string&),void* user,std::string& error) {
    if(!regionRetirementAwaitingFrame_)return true;
    const auto token=regionPageOwner_.token;
    const auto eligible=[&] {
        return !regionPageVisible_ && !regionPageOwner_.Empty() && regionPageOwner_.token==token &&
            tlasInstanceDefinitionsValid_ && !tlasPendingDefinitionsValid_ &&
            std::none_of(tlasBuiltInstances_.begin(),tlasBuiltInstances_.end(),[&](const auto& instance){
                return instance.accelerationStructureReference==regionPageOwner_.blas.address;}) &&
            std::any_of(regionStreaming_.States().begin(),regionStreaming_.States().end(),[&](const auto& state){
                return state.token==token && state.phase==horde::world::streaming::Phase::Retiring;});
    };
    if(!proof || !eligible() || !proof(user,error) || !eligible()) {
        if(error.empty())error="Region retirement lacks a current completed TLAS/reference proof.";
        return false;
    }
    horde::telemetry::RtResourceInventory memory;regionPageOwner_.AccumulateResourceInventory(memory);
    gpuResources_.DestroyAccelerationStructure(regionPageOwner_.blas);
    for(auto& b:regionPageOwner_.buffers)gpuResources_.DestroyBuffer(b);
    regionPageOwner_.ResetEmpty();
    if(!regionStreaming_.MarkRetired(token)){error="Completed region retirement has stale scheduler identity.";return false;}
    regionRetirementAwaitingFrame_=false;
    auto& e=ordinaryStaticDetailEvidence_;++e.unloads;e.omittedTriangles=regionPageSource_->indices.size()/3;
    e.savedOwnerBytes=memory.deviceLocalBytes;e.peakSavedOwnerBytes=std::max(e.peakSavedOwnerBytes,e.savedOwnerBytes);
    return true;
}
}
#endif
