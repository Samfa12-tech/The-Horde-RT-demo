#include "scene/GraveyardDressing.h"
#include "scene/TombDressing.h"
#include "scene/ChapterTerrainAdmission.h"
#include "scene/assets/AssetManifest.h"
#include "scene/assets/StaticPrimitiveGrouping.h"
#include "gameplay/traversal/ChapterTerrainMovement.h"
#include <algorithm>
#include <cstring>
#include <iostream>

namespace {
int failures=0;
void Check(bool ok,const char* message) { if(!ok) {++failures; std::cerr<<"FAIL: "<<message<<'\n';} }
using namespace horde::scene;
using namespace horde::gameplay::simulation;
using Surface=TriangleSupportSurface;
std::vector<assets::StaticRtVertex> FamilyTriangles(const assets::StaticMeshAsset& asset,unsigned material) {
    std::vector<assets::StaticRtVertex> result;
    for(const auto& p:asset.primitives)if(p.materialIndex==material)
        for(unsigned i=0;i<p.indexCount;++i)result.push_back(asset.vertices[p.vertexOffset+asset.indices[p.indexOffset+i]]);
    return result;
}
void TestNativeAdmission() {
    const std::filesystem::path root=std::filesystem::path(HORDE_RT_SOURCE_DIR)/"assets";
    std::string d; assets::StaticMeshAsset owner; assets::AssetManifest manifest;
    const auto collapse=root/"models/world/runtime/collapsed-entry";
    Check(assets::AssetManifest::Load(collapse/"asset.manifest.json",manifest,d) &&
        assets::StaticMeshAsset::Load(collapse/"collapsed-entry-lod0.runtime.glb",manifest,owner,d),"collapse fixture imports");
    TombDressingBuildReport tomb;
    Check(AppendPreparedTombDressing(root,owner,tomb,d),"accepted tomb families import");
    std::shared_ptr<const Surface> support;
    Check(AppendChapterTerrain(root,owner,support,d),"actual exported terrain imports");
    if(!support)return;
    const auto oldSupport=support;
    const auto unmodified=owner;
    const auto vertices=owner.vertices; const auto indices=owner.indices;
    const auto materialCount=owner.materials.size();
    GraveyardDressingReport report;
    const bool ok=AppendPreparedGraveyard(root,owner,support,report,d);
    Check(ok,"native graveyard admission must succeed");
    if(!ok) {std::cerr<<d<<'\n'; return;}
    Check(report.placements.size()>=10 && report.placements.size()<=20,"bounded marker/cover/offerings composition");
    Check(report.triangles>0 && report.triangles<10000 && report.contactTriangles==report.triangles,
        "actual native triangles supply both rendering and contact");
    Check(owner.materials.size()==materialCount,"graveyard reuses existing material/texture families");
    Check(std::memcmp(owner.vertices.data(),vertices.data(),vertices.size()*sizeof(vertices[0]))==0 &&
        std::equal(indices.begin(),indices.end(),owner.indices.begin()),"accepted owner prefix is byte-identical");
    Check(oldSupport!=support,"immutable prepared geometry replaces support atomically");
    float highest=0;
    for(const auto& p:report.placements)highest=std::max(highest,p.maximum[1]);
    for(const auto& p:report.placements) {
        const auto oldGround=oldSupport->Query(p.origin[0],p.origin[2]);
        const auto newGround=support->Query(p.origin[0],p.origin[2]);
        Check(oldGround.found && oldGround.worldY==newGround.worldY && oldGround.triangle==newGround.triangle,
            "nonwalkable dressing cannot replace the terrain support or tree grounding");
        Check(p.minimum[1]<=oldGround.worldY+.002f && p.maximum[1]>oldGround.worldY,
            "burial props are grounded with visible geometry above the surface");
        Check(support->Query(p.origin[0],p.origin[2]).walkable,"underlying actual terrain stays walkable");
        Check(!horde::gameplay::traversal::ChapterExteriorObstacleClear(*support,
            p.origin[0]-.8f,p.origin[2],p.origin[0]+.8f,p.origin[2],oldGround.worldY+.001f,oldGround.worldY+1.6f,.15f),
            "movement cannot pass through each visible solid");
        Check(support->NonWalkableVolumeClear(p.origin[0]-.8f,p.origin[2],p.origin[0]+.8f,p.origin[2],
            highest+.1f,highest+.5f,.15f),"buried pieces are not infinite vertical blockers");
    }
    for(const std::array<float,4> leg:std::array<std::array<float,4>,3>{{
        {{-33.7f,-12.8f,-33.7f,-13.8f}},{{-33.7f,-12.8f,-31.2f,-9.3f}},{{-31.2f,-9.3f,-28.7f,-5.8f}}
    }}) {
        const auto ground=support->Query(leg[0],leg[1]);
        Check(support->NonWalkableVolumeClear(leg[0],leg[1],leg[2],leg[3],ground.worldY+.001f,ground.worldY+1.6f,.25f),
            "rope approach and main clearing trail retain body clearance");
    }
    const auto admittedSupport=support; const auto admittedCount=owner.vertices.size();
    Check(!AppendPreparedGraveyard(root,owner,support,report,d),"duplicate append is rejected");
    Check(support==admittedSupport && owner.vertices.size()==admittedCount,"duplicate rejection preserves admitted owners");
    auto malformed=owner; malformed.materials.clear(); auto unchangedGround=oldSupport;
    Check(!AppendPreparedGraveyard(root,malformed,unchangedGround,report,d),"missing material families reject atomically");
    Check(unchangedGround==oldSupport && support==admittedSupport,"failed preparation keeps valid terrain/contact ownership");
    auto missingOwner=unmodified; unchangedGround=oldSupport;
    Check(!AppendPreparedGraveyard(root/"missing",missingOwner,unchangedGround,report,d),"missing actual source payload rejects intake");
    Check(unchangedGround==oldSupport && missingOwner.vertices.size()==unmodified.vertices.size(),
        "missing payload preserves the complete owner and support");
    auto grouped=owner;
    Check(assets::ConsolidateWorldBakedMaterialFamilies(grouped,d),"disconnected families consolidate");
    Check(grouped.primitives.size()==grouped.materials.size() && grouped.materials.size()==materialCount,
        "reusing graveyard families consumes no additional native material or primitive rows");
    for(unsigned material=0;material<owner.materials.size();++material) {
        const auto before=FamilyTriangles(owner,material),after=FamilyTriangles(grouped,material);
        Check(before.size()==after.size() && std::memcmp(before.data(),after.data(),before.size()*sizeof(before[0]))==0,
            "consolidation preserves each triangle corner's exact position/normal/tangent/UV bytes and order within its material");
    }
    auto broken=owner;broken.indices[0]=UINT32_MAX;
    Check(!assets::ConsolidateWorldBakedMaterialFamilies(broken,d) && broken.primitives.size()==owner.primitives.size(),
        "invalid local index rejects consolidation without replacing the owner");
    broken=owner;broken.primitives.back().indexCount-=3;
    Check(!assets::ConsolidateWorldBakedMaterialFamilies(broken,d),"unowned triangles cannot disappear during consolidation");
    std::cout<<"Placed "<<report.placements.size()<<" pieces / "<<report.triangles<<" render+contact triangles; CPU support "
        <<support->RetainedBytes()<<" bytes (before "<<oldSupport->RetainedBytes()<<").\n";
}
}
int main() {TestNativeAdmission(); if(failures)return 1; std::cout<<"Graveyard native geometry/contact checks passed.\n";}
