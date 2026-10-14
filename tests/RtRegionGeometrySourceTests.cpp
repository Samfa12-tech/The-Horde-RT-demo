#include "vulkan/raytracing/RtRegionGeometrySource.h"
#include "scene/assets/AssetManifest.h"
#include <cstring>
#include <iostream>
#include <algorithm>

using namespace horde::vulkan::raytracing;
using namespace horde::scene::assets;
int main() {
    bool ok=true;std::string error;
    const auto check=[&](bool v,const char* label){if(!v){ok=false;std::cerr<<label<<": "<<error<<'\n';}};
    const std::filesystem::path root{HORDE_RT_SOURCE_ASSET_ROOT};
    std::array<StaticMeshAsset,4> assets;
    const std::array<const char*,4> directories{{"models/weapons/runtime","models/player/runtime",
        "models/player/viewmodel/runtime","models/world/runtime/collapsed-entry"}};
    const std::array<const char*,4> names{{"gothic-arming-sword-rh-lod0.runtime.glb","gothic-traveller-lod0.runtime.glb",
        "gothic-traveller-viewmodel.runtime.glb","collapsed-entry-lod0.runtime.glb"}};
    for(unsigned i=0;i<4;++i){AssetManifest m;
        check(AssetManifest::Load(root/directories[i]/"asset.manifest.json",m,error) &&
            StaticMeshAsset::Load(root/directories[i]/names[i],m,assets[i],error),"actual native asset admission");}
    if(!ok)return 1;
    const auto pbr=static_cast<std::uint32_t>(RtInstanceFlag::StaticPbr);
    const std::array<StaticRtAssetRegistration,4> registrations{{
        {3,103,pbr,0,&assets[0]},
        {4,104,pbr,0,&assets[1],nullptr,RtGeometryRole::PlayerWorldBody},
        {20,120,pbr,0,&assets[2],&assets[1],RtGeometryRole::PlayerViewmodel},
        {17,117,pbr,0,&assets[3]}}};
    RtStaticMeshSlot source;check(source.Initialize(registrations,error),"native combined owner admission");
    if(!ok)return 1;
    const auto& owner=source.InstanceMetadata()[17];
    std::vector<std::uint32_t> selected;
    for(unsigned p=owner.primitiveBase;p<owner.primitiveBase+owner.primitiveCount;++p){
        const auto& r=source.PrimitiveMetadata()[p];check(r.indexCount>3,"fixture leaves a complete base primitive");
        selected.push_back(r.indexOffset/3);
    }
    if(!ok)return 1;
    RtRegionGeometrySourceResult result;
    check(RtRegionGeometrySource::Prepare(source,17,selected,128ull*1024*1024,result,error),"compact native root/detail preparation");
    if(!ok)return 1;
    const auto& base=*result.base;const auto& detail=*result.detail;
    check(!std::memcmp(base.InstanceMetadata().data(),source.InstanceMetadata().data(),sizeof(source.InstanceMetadata())),"instance identities remain exact");
    check(base.Materials().size()==source.Materials().size() && !std::memcmp(base.Materials().data(),source.Materials().data(),source.Materials().size()*sizeof(RtMaterialGpu)),"shared material/texture routing unchanged");
    check(base.GeometryTransforms()==source.GeometryTransforms(),"all primitive transforms unchanged");
    check(base.Indices().size()+detail.indices.size()==source.Indices().size(),"triangles partitioned without duplication");
    for(auto role:{RtGeometryRole::PlayerWorldBody,RtGeometryRole::PlayerViewmodel})
        check(base.Vertices(role).size()==source.Vertices(role).size() && !std::memcmp(base.Vertices(role).data(),source.Vertices(role).data(),source.Vertices(role).size()*sizeof(StaticRtVertex)),"complete held/player vertex domains preserved");
    std::vector<unsigned> seen(source.Indices().size()/3);
    for(auto id:detail.sourceTriangles)++seen[id];
    std::size_t detailCursor=0;
    for(unsigned p=0;p<source.PrimitiveMetadata().size();++p){
        const auto& old=source.PrimitiveMetadata()[p];const auto& retained=base.PrimitiveMetadata()[p];
        check(old.materialIndex==retained.materialIndex,"root global material identity");
        RtGeometryRole role=RtGeometryRole::Static;
        for(const auto& m:source.InstanceMetadata())if(p>=m.primitiveBase && p<m.primitiveBase+m.primitiveCount)role=static_cast<RtGeometryRole>(m.geometryRole);
        unsigned baseIndex=retained.indexOffset;
        for(unsigned i=old.indexOffset;i<old.indexOffset+old.indexCount;i+=3){
            const bool take=std::binary_search(selected.begin(),selected.end(),i/3);
            const RtPrimitiveMetadata* row=&retained;
            if(take){row=&detail.primitives[detailCursor++];check(row->materialIndex==old.materialIndex,"detail global material identity");}
            else ++seen[i/3];
            const auto offset=take?row->indexOffset:baseIndex;
            for(unsigned k=0;k<3;++k){
                const auto& actual=take?detail.vertices[row->vertexOffset+detail.indices[offset+k]]:
                    base.Vertices(role)[row->vertexOffset+base.Indices()[offset+k]];
                const auto& expected=source.Vertices(role)[old.vertexOffset+source.Indices()[i+k]];
                check(!std::memcmp(&actual,&expected,sizeof(actual)),"actual imported attributes/winding remain byte-identical");
            }
            if(!take)baseIndex+=3;
        }
    }
    check(std::all_of(seen.begin(),seen.end(),[](auto n){return n==1;}),"every original triangle owned exactly once");
    const auto* previous=result.base.get();const auto* page=result.detail.get();
    check(!RtRegionGeometrySource::Prepare(source,17,selected,1,result,error) && result.base.get()==previous && result.detail.get()==page,"failed preparation preserves prior usable CPU owners");
    if(ok)std::cout<<"Native root/detail source preparation preserves attributes, actor/held domains and shared materials; no GPU admission claim.\n";
    return ok?0:1;
}
