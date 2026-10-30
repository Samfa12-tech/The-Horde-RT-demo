// Exercise the same private reflection implementation without creating Vulkan
// state. Production fixtures retain their original descriptor-based profile.
#include "../src/vulkan/raytracing/RtPipelineBundleContracts.cpp"
#include <iostream>
#include <vector>
#include "vulkan/raytracing/PresentableTinyRtScene.h"
#if !HORDE_RT_REGION_PAGE_PROTOTYPE
#error "This fixture requires the explicit private page shader profile."
#endif
namespace horde::vulkan::raytracing {
struct PresentableTinyRtSceneRegionPageTestAccess {
    static bool Check() {
        using namespace horde::world::streaming;
        PresentableTinyRtScene s;std::string error;
        horde::scene::assets::StaticMeshAsset asset;
        asset.vertices.resize(4);asset.indices={0,1,2,1,3,2};asset.primitives={{0,0,6,0,0}};
        asset.materials.resize(1);horde::scene::assets::StaticNodeTransform node;
        node.world={1,0,0,0,0,1,0,0,0,0,1,0,0,0,0,1};asset.nodeTransforms.push_back(node);
        const std::array<StaticRtAssetRegistration,1> registrations{{
            {s.kCollapseInstanceIndex,117,static_cast<unsigned>(RtInstanceFlag::StaticPbr),0,&asset}}};
        if(!s.staticMeshSlot_.Initialize(registrations,error)){std::cerr<<error<<'\n';return false;}
        s.staticVertexBuffer_.address=0x10000;s.staticVertexBuffer_.size=256;
        s.staticIndexBuffer_.address=0x20000;s.staticIndexBuffer_.size=24;
        s.primitiveMetadataBuffer_.address=0x30000;s.primitiveMetadataBuffer_.size=16;
        auto payload=std::make_shared<GeometryPayload>();payload->primitives={{0,0,6,0}};
        std::array<VkAccelerationStructureInstanceKHR,s.kTlasInstanceCount> rootInstances{};
        auto rootMetadata=s.staticMeshSlot_.InstanceMetadata();GeometryPageTable rootTable;
        rootInstances[s.kRegionPageTlasSlot].mask=1; // Root-only scenes keep their ordinary lane ownership.
        bool ok=s.PrepareRegionPageSource(error) && s.CreateInitialRegionPage(error) &&
            !s.regionPagePreparation_ && s.ApplyRegionPage(rootInstances,rootMetadata,rootTable,error) &&
            std::memcmp(rootTable.instances.data(),rootMetadata.data(),sizeof(rootMetadata))==0;
        s.regionPageSource_=payload;
        auto& owner=s.regionPageOwner_;owner.payload=payload;owner.token={s.staticGeometryGeneration_,1,1};
        owner.blas.address=0x70000;
        owner.buffers[0].address=0x40000;owner.buffers[0].size=256;
        owner.buffers[1].address=0x50000;owner.buffers[1].size=24;
        owner.buffers[2].address=0x60000;owner.buffers[2].size=16;
        std::array<VkAccelerationStructureInstanceKHR,s.kTlasInstanceCount> instances{};
        auto metadata=s.staticMeshSlot_.InstanceMetadata();GeometryPageTable table;
        ok&=s.ApplyRegionPage(instances,metadata,table,error) &&
            instances[s.kRegionPageTlasSlot].instanceCustomIndex==s.kRegionPageMetadataSlot &&
            !(s.kRegionPageMetadataSlot>=10 && s.kRegionPageMetadataSlot<=13);
        instances={};instances[5].mask=1;instances[5].instanceCustomIndex=s.kRegionPageMetadataSlot;
        metadata=s.staticMeshSlot_.InstanceMetadata();
        ok&=!s.ApplyRegionPage(instances,metadata,table,error);
        instances={};instances[s.kRegionPageTlasSlot].mask=1;
        ok&=!s.ApplyRegionPage(instances,metadata,table,error);
        instances={};metadata[s.kRegionPageMetadataSlot].flags=static_cast<unsigned>(RtInstanceFlag::StaticPbr);
        metadata[s.kRegionPageMetadataSlot].primitiveCount=1;
        ok&=!s.ApplyRegionPage(instances,metadata,table,error);
        ok&=!s.BeginStaticGeometryReplacement(s.staticGeometryGeneration_,{},error) &&
            error.find("independent region-page")!=std::string::npos;
        StreamingRules rules;rules.prepareDistance=rules.retainDistance=0;rules.retireDelaySeconds=0;
        const std::array<RegionDefinition,1> regions{{{1,{{0,0,24},{1,1,25}},false,1,{}}}};
        ok&=s.regionStreaming_.Configure(regions,rules,s.staticGeometryGeneration_,error) && s.regionStreaming_.SeedResident(1,1);
        owner.token=s.regionStreaming_.States()[0].token;
        const std::array<Contribution,1> mayLeave{{{1,true}}};Plan plan;
        ok&=s.regionStreaming_.Evaluate({s.staticGeometryGeneration_,0,{0,0,0},{},{},mayLeave},plan,error);
        ok&=s.regionStreaming_.Evaluate({s.staticGeometryGeneration_,1,{0,0,0},{},{},mayLeave},plan,error) &&
            s.regionStreaming_.States()[0].phase==Phase::Retiring;
        s.regionPageSource_=payload;s.regionPageVisible_=false;s.regionRetirementAwaitingFrame_=true;
        s.tlasInstanceDefinitionsValid_=true;
        unsigned proofs=0;
        const auto deny=[](void* opaque,std::string&){++*static_cast<unsigned*>(opaque);return false;};
        const auto prove=[](void* opaque,std::string&){++*static_cast<unsigned*>(opaque);return true;};
        ok&=!s.RetireCompletedRegionPage(deny,&proofs,error) && !owner.Empty() && proofs==1;
        s.tlasPendingDefinitionsValid_=true;
        ok&=!s.RetireCompletedRegionPage(prove,&proofs,error) && proofs==1;
        s.tlasPendingDefinitionsValid_=false;s.tlasBuiltInstances_[23].accelerationStructureReference=owner.blas.address;
        ok&=!s.RetireCompletedRegionPage(prove,&proofs,error) && proofs==1; // Even mask0 remains a stored AS reference.
        s.tlasBuiltInstances_[23].accelerationStructureReference=0;
        const auto token=owner.token;
        const auto stale=[](void* opaque,std::string&){++static_cast<PresentableTinyRtScene*>(opaque)->regionPageOwner_.token.requestGeneration;return true;};
        ok&=!s.RetireCompletedRegionPage(stale,&s,error) && !owner.Empty();owner.token=token;
        ok&=s.RetireCompletedRegionPage(prove,&proofs,error) && owner.Empty() && proofs==2 &&
            s.regionStreaming_.States()[0].phase==Phase::Absent && s.ordinaryStaticDetailEvidence_.unloads==1;
        // Synthetic addresses have no Vulkan owners. Disarm before destructor.
        owner.blas={};owner.buffers={};owner.ResetEmpty();
        s.staticVertexBuffer_={};s.staticIndexBuffer_={};s.primitiveMetadataBuffer_={};
        return ok;
    }
};
}
int main(){
    using namespace horde::vulkan::raytracing;
    using horde::vulkan::RtExecutionBackend;
    bool ok=true;const auto check=[&](bool value,const char* what){if(!value){ok=false;std::cerr<<what<<'\n';}};
    check(PresentableTinyRtSceneRegionPageTestAccess::Check(),"page lane rejects live/registered aliases and stays outside legacy player attribution");
    for(auto backend:{RtExecutionBackend::RayTracingPipeline,RtExecutionBackend::RayQueryCompute}){
        RtPipelineBundlePreflight p;std::string error;
        check(ResolveCompiledRtPipelineBundlePreflight(p,error,backend),"exact private pair must pass full hash/entry/atomic/binding preflight");
        if(!ok){std::cerr<<error<<'\n';return 1;}
        for(const auto& artifact:p.strategies){
            ReflectedModuleContract reflected;
            check(ReflectModule(artifact.words,backend,reflected) && reflected.physicalStorageBufferAddresses &&
                MatchesDescriptorBindingRoster(reflected,p.descriptorIo),"private module uses exact address-geometry roster and capability");
            auto missing=reflected;
            for(std::size_t i=0;i<missing.descriptorBindingCount;++i)if(missing.descriptorBindings[i]==kRtBindingInstanceMetadata){
                missing.descriptorBindings[i]=kRtBindingPrimitiveMetadata;break;
            }
            check(!MatchesDescriptorBindingRoster(missing,p.descriptorIo),"missing address table cannot be replaced with a legacy binding");
            auto extra=reflected;extra.descriptorBindings[extra.descriptorBindingCount++]=kRtBindingStaticIndices;
            check(!MatchesDescriptorBindingRoster(extra,p.descriptorIo),"unexpected descriptor remains rejected");
            std::vector<std::uint32_t> words(artifact.words.begin(),artifact.words.end());bool changed=false;
            for(std::size_t offset=5;offset<words.size();){
                const auto count=words[offset]>>16,opcode=words[offset]&0xffffu;
                if(opcode==17 && count==2 && words[offset+1]==5347){words[offset+1]=1;changed=true;break;}
                offset+=count;
            }
            check(changed && ReflectModule(words,backend,reflected) && !reflected.physicalStorageBufferAddresses,
                "descriptor absence cannot stand in for physical address capability");
        }
    }
    if(ok)std::cout<<"Exact private Pipeline/Compute storage-profile preflight and missing/extra/capability negatives passed; no device claim.\n";
    return ok?0:1;
}
