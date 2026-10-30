#include "scene/ForestRockDressing.h"
#include "scene/assets/StaticPrimitiveGrouping.h"
#include "gameplay/simulation/DevelopmentWorldRoute.h"
#include <algorithm>
#include <cstring>
#include <iostream>
int main() {
    using namespace horde::scene;
    using Surface=horde::gameplay::simulation::TriangleSupportSurface;
    bool ok=true;auto check=[&](bool value,const char* text){if(!value){std::cerr<<text<<'\n';ok=false;}};
    std::string error;
    std::array<Surface::Triangle,2> ground{};
    ground[0].points={Surface::Point{-100,1,-100},Surface::Point{-100,1,100},Surface::Point{100,1,100}};
    ground[1].points={Surface::Point{-100,1,-100},Surface::Point{100,1,100},Surface::Point{100,1,-100}};
    auto surface=Surface::Build(ground,error);
    const Surface::Triangle oldWall{{Surface::Point{80,1,80},Surface::Point{80,3,80},Surface::Point{80,1,82}}};
    surface=Surface::WithNonWalkableGeometry(*surface,std::span(&oldWall,1),error);
    horde::scene::assets::StaticMeshAsset owner;assets::StaticMaterial material;material.name="Boulder01Rock";
    owner.materials.push_back(material);
    owner.nodeTransforms.push_back({"identity",{{1,0,0,0,0,1,0,0,0,0,1,0,0,0,0,1}}});
    assets::StaticRtVertex v{};v.position[0]=90;v.normal[1]=1;v.tangent[0]=1;v.tangent[3]=1;
    owner.vertices.push_back(v);v.position[0]=91;owner.vertices.push_back(v);v.position[2]=1;owner.vertices.push_back(v);
    owner.indices={0,1,2};owner.primitives.push_back({0,0,3,0,0});owner.bounds={{{90,0,0}},{{91,0,1}}};
    const auto original=owner;const auto originalSurface=surface;
    ForestRockReport report;const auto root=std::filesystem::path(HORDE_RT_SOURCE_DIR)/"assets";
    const auto admitted=AppendPreparedForestRocks(root,owner,surface,report,error);
    check(admitted,"prepared rock geometry and collision must be admitted together");
    if(admitted) {
        check(report.placements.size()==12 && report.triangles==6168,"twelve rocks preserve the source triangle totals");
        check(owner.materials.size()==1,"admitted boulder maps must be shared");
        check(std::memcmp(owner.vertices.data(),original.vertices.data(),original.vertices.size()*sizeof(v))==0,"accepted geometry remains exact");
        check(surface!=originalSurface && surface->TriangleCount()==originalSurface->TriangleCount() &&
            surface->NonWalkableTriangleCount()==originalSurface->NonWalkableTriangleCount()+report.triangles,"ground and previous contact must survive");
        check(!surface->NonWalkableVolumeClear(79,81,81,81,1.1f,2,.24f),"previous graveyard-like wall cannot disappear");
        for(const auto& p:report.placements) {
            check(horde::gameplay::simulation::ProjectWorldRoute(p.origin[0],p.origin[2]).distance>2.4f,"clear trail stays open");
            check(!surface->NonWalkableVolumeClear(p.minimum[0]-.5f,p.origin[2],p.maximum[0]+.5f,p.origin[2],1.001f,2.3f,.24f),"rendered rocks block a crossing capsule");
            check(surface->Query(p.origin[0],p.origin[2]).worldY==1,"rock tops never replace terrain support");
        }
        const auto before=owner.indices;
        check(!AppendPreparedForestRocks(root,owner,surface,report,error) && owner.indices==before,"repeated admission cannot duplicate geometry or contact");
        check(assets::ConsolidateWorldBakedMaterialFamilies(owner,error) && owner.primitives.size()==1,"rocks share the existing material family");
    }
    auto missing=original;missing.materials[0].name="Missing";auto unchanged=originalSurface;
    check(!AppendPreparedForestRocks(root,missing,unchanged,report,error) && unchanged==originalSurface && missing.indices==original.indices,"missing material rejects atomically");
    auto absent=original;
    check(!AppendPreparedForestRocks(root/"missing",absent,unchanged,report,error) && unchanged==originalSurface && absent.indices==original.indices,"missing native files reject both owners");
    std::cout<<"Forest rock/contact "<<(ok?"pass":"fail")<<'\n';return ok?0:1;
}
