#include "scene/ForestUnderstory.h"
#include "scene/assets/StaticPrimitiveGrouping.h"
#include "gameplay/simulation/DevelopmentWorldRoute.h"
#include <algorithm>
#include <cmath>
#include <cstring>
#include <iostream>

int main() {
    using namespace horde::scene;
    using Surface=horde::gameplay::simulation::TriangleSupportSurface;
    bool ok=true;
    auto check=[&](bool value,const char* message){if(!value){std::cerr<<"FAIL: "<<message<<'\n';ok=false;}};
    std::string diagnostic;
    std::array<Surface::Triangle,2> triangles{};
    triangles[0].points={Surface::Point{-100,1,-100},Surface::Point{-100,1,100},Surface::Point{100,1,100}};
    triangles[1].points={Surface::Point{-100,1,-100},Surface::Point{100,1,100},Surface::Point{100,1,-100}};
    const auto surface=Surface::Build(triangles,diagnostic);
    check(bool(surface),"bounded actual triangle surface is built");
    if(!surface)return 1;
    assets::StaticMeshAsset owner;
    assets::StaticMaterial material;material.name="ForestAlder";material.metallicFactor=0;
    material.baseColorTexture=0;material.textureGroup=3;material.deferTextureAllocation=true;
    owner.materials.push_back(material);
    owner.nodeTransforms.push_back({"identity",{{1,0,0,0,0,1,0,0,0,0,1,0,0,0,0,1}}});
    assets::StaticRtVertex v{};v.position[0]=90;v.normal[1]=1;v.tangent[0]=1;v.tangent[3]=1;
    owner.vertices.push_back(v);v.position[0]=91;owner.vertices.push_back(v);
    v.position[2]=1;owner.vertices.push_back(v);
    owner.indices={0,1,2};owner.primitives.push_back({0,0,3,0,0});
    owner.bounds={{{90,0,0}},{{91,0,1}}};
    const auto original=owner;
    const auto root=std::filesystem::path(HORDE_RT_SOURCE_DIR)/"assets";
    ForestUnderstoryReport report;
    const auto admitted=AppendPreparedForestUnderstory(root,owner,*surface,report,diagnostic);
    check(admitted,"the two real prepared fern GLBs must import and produce understory");
    if(!admitted){std::cerr<<diagnostic<<'\n';return 1;}
    check(report.placements.size()==18 && report.triangles==17712,"bounded varied clumps use exact native source triangles");
    check(owner.materials.size()==original.materials.size(),"no added texture/material families");
    check(std::memcmp(owner.vertices.data(),original.vertices.data(),original.vertices.size()*sizeof(v))==0 &&
        std::equal(original.indices.begin(),original.indices.end(),owner.indices.begin()),"existing geometry prefix is preserved");
    for(const auto& p:report.placements) {
        const auto projection=horde::gameplay::simulation::ProjectWorldRoute(p.origin[0],p.origin[2]);
        check(projection.valid && projection.distance>2.4f,"fern centres leave the trail and rope approaches open");
        check(std::abs(p.minimum[1]-(p.supportY-.03f))<.0001f && std::abs(p.supportY-1)<.0001f,
            "native minimum is embedded in resolved terrain, not a ribbon-floor guess");
    }
    const auto before=owner.indices.size();
    check(assets::ConsolidateWorldBakedMaterialFamilies(owner,diagnostic) && owner.primitives.size()==1 &&
        owner.indices.size()==before,"foliage shares the existing material primitive row without dropping triangles");
    auto slopeTriangles=triangles;
    for(auto& triangle:slopeTriangles)for(auto& p:triangle.points)p[1]=.3f*p[0]+1;
    const auto slope=Surface::Build(slopeTriangles,diagnostic);
    auto tilted=original;ForestUnderstoryReport slopeReport;
    check(AppendPreparedForestUnderstory(root,tilted,*slope,slopeReport,diagnostic),"sloped terrain admits the same bounded sources");
    if(slopeReport.placements.size()==18) {
        for(const auto& p:slopeReport.placements)
            check(p.surfaceNormal[0]<-.28f && p.surfaceNormal[1]>.95f,"plant basis follows authoritative bank normal");
        for(std::size_t i=original.vertices.size();i<tilted.vertices.size();++i) {
            const auto& v=tilted.vertices[i];
            check(v.position[1]-.3f*v.position[0]-1>=-.0302f,
                "sloping banks cannot bury fern leaves through the support plane");
        }
    }
    auto failed=original;failed.materials[0].name="Missing";
    ForestUnderstoryReport rejected;
    check(!AppendPreparedForestUnderstory(root,failed,*surface,rejected,diagnostic) && failed.indices==original.indices &&
        failed.vertices.size()==original.vertices.size() && rejected.placements.empty(),"missing admitted material rejects atomically");
    auto absent=original;
    check(!AppendPreparedForestUnderstory(root/"missing-source",absent,*surface,rejected,diagnostic) &&
        absent.indices==original.indices && rejected.placements.empty(),"missing native payload rejects atomically");
    triangles[0].points={Surface::Point{-100,1,-100},Surface::Point{-100,1,100},Surface::Point{0,1,100}};
    triangles[1].points={Surface::Point{-100,1,-100},Surface::Point{0,1,100},Surface::Point{0,1,-100}};
    const auto tiny=Surface::Build(triangles,diagnostic);
    auto gap=original;
    check(!AppendPreparedForestUnderstory(root,gap,*tiny,rejected,diagnostic) && gap.indices==original.indices &&
        rejected.placements.empty(),"missing surface at any clump cannot publish partial geometry");
    std::cout<<"Understory native surface/material/rollback checks "<<(ok?"passed":"failed")<<".\n";
    return ok?0:1;
}
