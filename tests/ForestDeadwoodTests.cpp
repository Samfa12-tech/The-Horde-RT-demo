#include "scene/ForestDeadwood.h"
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
    assets::StaticMeshAsset owner;assets::StaticMaterial material;material.name="ForestBark";material.metallicFactor=0;
    owner.materials.push_back(material);
    owner.nodeTransforms.push_back({"identity",{{1,0,0,0,0,1,0,0,0,0,1,0,0,0,0,1}}});
    assets::StaticRtVertex v{};v.position[0]=90;v.normal[1]=1;v.tangent[0]=1;v.tangent[3]=1;
    owner.vertices.push_back(v);v.position[0]=91;owner.vertices.push_back(v);v.position[2]=1;owner.vertices.push_back(v);
    owner.indices={0,1,2};owner.primitives.push_back({0,0,3,0,0});owner.bounds={{{90,0,0}},{{91,0,1}}};
    const auto original=owner;const auto originalSurface=surface;
    ForestDeadwoodReport report;const auto root=std::filesystem::path(HORDE_RT_SOURCE_DIR)/"assets";
    const auto admitted=AppendPreparedForestDeadwood(root,owner,surface,report,error);
    if(!admitted)std::cerr<<error<<'\n';
    check(admitted,"prepared log/root/stump/snag and collision must be admitted together");
    if(admitted) {
        check(report.placements.size()==10 && report.triangles==11884,"bounded placements preserve source triangle totals");
        std::array<unsigned,4> counts{};for(const auto& p:report.placements)++counts[p.source];
        check(counts==std::array<unsigned,4>{3,3,2,2},"all four prepared sources must be represented");
        check(owner.materials.size()==1,"existing bark family must be shared without texture growth");
        check(std::memcmp(owner.vertices.data(),original.vertices.data(),original.vertices.size()*sizeof(v))==0,"accepted geometry remains exact");
        check(surface!=originalSurface && surface->TriangleCount()==originalSurface->TriangleCount() &&
            surface->NonWalkableTriangleCount()==originalSurface->NonWalkableTriangleCount()+report.triangles,"ground and previous contact must survive");
        check(!surface->NonWalkableVolumeClear(79,81,81,81,1.1f,2,.24f),"previous graveyard/rock contact cannot disappear");
        for(const auto& p:report.placements) {
            check(horde::gameplay::simulation::ProjectWorldRoute(p.origin[0],p.origin[2]).distance>2.4f,"clear trail stays open");
            check(p.minimum[1]<1 && p.maximum[1]>1,"props are buried into actual support, not floating");
            check(!surface->NonWalkableVolumeClear(p.minimum[0]-.5f,p.origin[2],p.maximum[0]+.5f,p.origin[2],1.001f,2.3f,.24f),"rendered deadwood blocks a crossing capsule");
            check(surface->Query(p.origin[0],p.origin[2]).worldY==1,"deadwood never replaces terrain support");
        }
        const auto before=owner.indices;const auto beforeSurface=surface;
        check(!AppendPreparedForestDeadwood(root,owner,surface,report,error)&&owner.indices==before&&surface==beforeSurface,"repeat admission cannot duplicate geometry/contact");
    }
    auto unchanged=originalSurface;auto missing=original;
    check(!AppendPreparedForestDeadwood(root/"missing",missing,unchanged,report,error)&&unchanged==originalSurface&&missing.indices==original.indices,"missing native files reject both owners atomically");
    missing.materials[0].name="WrongFamily";
    check(!AppendPreparedForestDeadwood(root,missing,unchanged,report,error)&&unchanged==originalSurface,"missing shared material cannot admit partial contact");
    std::cout<<"Prepared deadwood/contact "<<(ok?"pass":"fail")<<'\n';return ok?0:1;
}
