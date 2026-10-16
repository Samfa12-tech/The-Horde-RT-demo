#include "vulkan/raytracing/RtRegionGeometrySource.h"
#include "scene/assets/AssetManifest.h"
#include "vulkan/raytracing/RtLanternGeometryProfile.h"
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
    const auto lanternRoot=root/"models/props/runtime/reward-lantern-body";
    StaticMeshAsset canonicalLantern;AssetManifest lanternManifest;
    check(AssetManifest::Load(lanternRoot/"asset.manifest.json",lanternManifest,error) &&
        StaticMeshAsset::Load(lanternRoot/"reward-lantern-body-lod0.runtime.glb",lanternManifest,canonicalLantern,error),
        "native canonical lantern admission");
    if(!ok)return 1;
    for(auto quality:{DielectricQuality::High,DielectricQuality::Mobile})for(bool glass:{true,false})for(bool suffix:{false,true}) {
        auto lantern=canonicalLantern;
        const bool selectedProfile=SelectLanternGeometryForQuality(lantern,quality,error,glass);
        check(selectedProfile,"native optical geometry profile");if(!selectedProfile)continue;
        if(suffix) {
            const std::array<std::uint32_t,3> unused{{lantern.indices[0],lantern.indices[1],lantern.indices[2]}};
            lantern.indices.insert(lantern.indices.end(),unused.begin(),unused.end());
            if(!lantern.triangleRegions.empty())lantern.triangleRegions.push_back(lantern.triangleRegions.front());
        }
        RtStaticMeshSlot profiled;
        std::array<StaticRtAssetRegistration,4> rows{{
            {8,108,pbr|static_cast<std::uint32_t>(RtInstanceFlag::Transmissive),0,&lantern},
            {4,104,pbr,0,&assets[1],nullptr,RtGeometryRole::PlayerWorldBody},
            {20,120,pbr,0,&assets[2],&assets[1],RtGeometryRole::PlayerViewmodel},
            {17,117,pbr,0,&assets[3]}}};
        if(suffix)std::rotate(rows.begin(),rows.begin()+1,rows.end());
        const bool admitted=profiled.Initialize(rows,error);
        check(admitted,"profile retains canonical backing storage");if(!admitted)continue;
        const auto& world=profiled.InstanceMetadata()[17];
        const std::array<std::uint32_t,1> pick{{profiled.PrimitiveMetadata()[world.primitiveBase].indexOffset/3}};
        RtRegionGeometrySourceResult prepared;
        const bool ready=RtRegionGeometrySource::Prepare(profiled,17,pick,128ull*1024*1024,prepared,error);
        if(!ready)std::cerr<<"Profile "<<(quality==DielectricQuality::High?"High":"Mobile")<<" glass="<<glass<<'\n';
        check(ready,"High/Mobile and Glass On/Off native source conversion");
        if(!ready)continue;
        std::size_t activeIndices=0;
        for(const auto& row:profiled.PrimitiveMetadata())activeIndices+=row.indexCount;
        check(prepared.base->Indices().size()+prepared.detail->indices.size()==activeIndices,
              "all active triangles retained exactly once; excluded pane backing is not rendered");
        check(prepared.detail->sourceTriangles==std::vector<std::uint32_t>(pick.begin(),pick.end()),
              "source-global selected triangle identity survives excluded pane ranges");
        check(prepared.base->Materials().size()==profiled.Materials().size() &&
              !std::memcmp(prepared.base->Materials().data(),profiled.Materials().data(),profiled.Materials().size()*sizeof(RtMaterialGpu)) &&
              !std::memcmp(prepared.base->InstanceMetadata().data(),profiled.InstanceMetadata().data(),sizeof(profiled.InstanceMetadata())),
              "optical material, instance and texture identities remain unchanged");
        check(prepared.base->GeometryTransforms()==profiled.GeometryTransforms(),"sparse primitive transforms preserved");
        for(auto role:{RtGeometryRole::PlayerWorldBody,RtGeometryRole::PlayerViewmodel})
            check(prepared.base->Vertices(role).size()==profiled.Vertices(role).size() &&
                  !std::memcmp(prepared.base->Vertices(role).data(),profiled.Vertices(role).data(),profiled.Vertices(role).size()*sizeof(StaticRtVertex)),
                  "sparse conversion preserves complete pose/grip vertex domains");
        for(std::size_t p=0;p<profiled.PrimitiveMetadata().size();++p) {
            const auto& old=profiled.PrimitiveMetadata()[p];const auto& retained=prepared.base->PrimitiveMetadata()[p];
            RtGeometryRole role=RtGeometryRole::Static;
            for(const auto& owner:profiled.InstanceMetadata())if(p>=owner.primitiveBase && p<owner.primitiveBase+owner.primitiveCount)
                role=static_cast<RtGeometryRole>(owner.geometryRole);
            std::size_t offset=retained.indexOffset;
            for(std::size_t i=old.indexOffset;i<old.indexOffset+old.indexCount;i+=3) {
                if(i/3==pick[0])continue;
                for(unsigned k=0;k<3;++k) {
                    const auto& a=prepared.base->Vertices(role)[retained.vertexOffset+prepared.base->Indices()[offset+k]];
                    const auto& b=profiled.Vertices(role)[old.vertexOffset+profiled.Indices()[i+k]];
                    check(!std::memcmp(&a,&b,sizeof(a)),"profiled active attributes and winding preserved exactly");
                }
                offset+=3;
            }
        }
        const auto* acceptedBase=prepared.base.get();const auto* acceptedPage=prepared.detail.get();
        const auto unchanged=[&]{return prepared.base.get()==acceptedBase && prepared.detail.get()==acceptedPage;};
        check(!RtRegionGeometrySource::Prepare(profiled,17,pick,64u*1024u,prepared,error) && unchanged(),
              "bounded source preparation failure retains both accepted owners");
        const std::array<std::uint32_t,2> duplicate{{pick[0],pick[0]}};
        check(!RtRegionGeometrySource::Prepare(profiled,17,duplicate,128ull*1024*1024,prepared,error) && unchanged(),
              "normalization does not admit duplicate selections");
        const std::array<std::uint32_t,1> otherOwner{{profiled.PrimitiveMetadata().front().indexOffset/3}};
        check(!RtRegionGeometrySource::Prepare(profiled,17,otherOwner,128ull*1024*1024,prepared,error) && unchanged(),
              "normalization cannot transfer another owner's referenced triangle");
        std::size_t nextIndex=0;
        for(const auto& row:profiled.PrimitiveMetadata()) {
            if(row.indexOffset>nextIndex) {
                const std::array<std::uint32_t,1> orphan{{static_cast<std::uint32_t>(nextIndex/3)}};
                check(!RtRegionGeometrySource::Prepare(profiled,17,orphan,128ull*1024*1024,prepared,error) && unchanged(),
                      "unreferenced quality backing cannot become selectable geometry");
            }
            nextIndex=row.indexOffset+row.indexCount;
        }
        if(suffix) {
            check(nextIndex<profiled.Indices().size(),"explicit unreferenced suffix exists");
            const std::array<std::uint32_t,1> orphan{{static_cast<std::uint32_t>(nextIndex/3)}};
            check(!RtRegionGeometrySource::Prepare(profiled,17,orphan,128ull*1024*1024,prepared,error) && unchanged(),
                  "trailing unreferenced source storage remains unselectable");
        }
    }
    if(ok)std::cout<<"Native root/detail source preparation preserves attributes, actor/held domains and shared materials; no GPU admission claim.\n";
    return ok?0:1;
}
