#include "scene/ForestDeadwood.h"
#include "scene/ChapterTerrainSurface.h"
#include "gameplay/simulation/DevelopmentWorldRoute.h"
#include "gameplay/traversal/ChapterTerrainMovement.h"
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
        check(report.placements.size()==10 && report.nonWalkableTriangles==11884 &&
            report.stepTriangles==20 && report.triangles==11904,"ten unchanged imported props plus one twenty-face authored root");
        std::array<unsigned,4> counts{};for(const auto& p:report.placements)++counts[p.source];
        check(counts==std::array<unsigned,4>{3,3,2,2},"all four prepared sources must be represented");
        check(owner.materials.size()==1,"existing bark family must be shared without texture growth");
        check(std::memcmp(owner.vertices.data(),original.vertices.data(),original.vertices.size()*sizeof(v))==0,"accepted geometry remains exact");
        check(surface!=originalSurface && surface->TriangleCount()==originalSurface->TriangleCount() &&
            surface->NonWalkableTriangleCount()==originalSurface->NonWalkableTriangleCount()+report.nonWalkableTriangles &&
            surface->StepTriangleCount()==20,"ground, previous contact and separate STEP lane must survive");
        check(!surface->NonWalkableVolumeClear(79,81,81,81,1.1f,2,.24f),"previous graveyard/rock contact cannot disappear");
        for(const auto& p:report.placements) {
            check(horde::gameplay::simulation::ProjectWorldRoute(p.origin[0],p.origin[2]).distance>2.4f,"clear trail stays open");
            check(p.minimum[1]<1 && p.maximum[1]>1,"props are buried into actual support, not floating");
            check(!surface->NonWalkableVolumeClear(p.minimum[0]-.5f,p.origin[2],p.maximum[0]+.5f,p.origin[2],1.001f,2.3f,.24f),"rendered deadwood blocks a crossing capsule");
            check(surface->Query(p.origin[0],p.origin[2]).worldY==1,"deadwood never replaces terrain support");
            check(!surface->StepSupport(p.origin[0],p.origin[2],p.origin[0],p.origin[2],1,3,.24f).found,
                "old off-trail imported props must never become eligible STEP tops");
        }
        check(report.rootVertices==60 && report.rootGeometryBytes==4080 && report.maximumRootRise<=.12001f,
            "authored root has a measured fixed native payload and bounded physical rise");
        check(owner.triangleRegions.size()==owner.indices.size()/3 && owner.triangleRegions.front()==assets::StaticTriangleRegion::Shared &&
            std::count(owner.triangleRegions.begin()+1,owner.triangleRegions.end(),assets::StaticTriangleRegion::ExteriorDressing)==11904,
            "all old deadwood and root triangles retain exterior provenance without relabeling existing geometry");
        check(owner.primitives.size()==original.primitives.size()+1 && owner.nodeTransforms.size()==original.nodeTransforms.size()+1,
            "root shares the existing deadwood primitive, native owner and bark material");
        const auto before=owner.indices;const auto beforeSurface=surface;
        check(!AppendPreparedForestDeadwood(root,owner,surface,report,error)&&owner.indices==before&&surface==beforeSurface,"repeat admission cannot duplicate geometry/contact");
    }
    auto unchanged=originalSurface;auto missing=original;
    check(!AppendPreparedForestDeadwood(root/"missing",missing,unchanged,report,error)&&unchanged==originalSurface&&missing.indices==original.indices,"missing native files reject both owners atomically");
    missing.materials[0].name="WrongFamily";
    check(!AppendPreparedForestDeadwood(root,missing,unchanged,report,error)&&unchanged==originalSurface,"missing shared material cannot admit partial contact");

    // Exercise the production-imported ground and the shared motor, rather than
    // certifying a root merely because it works on this fixture's flat plane.
    assets::AssetManifest terrainManifest;assets::StaticMeshAsset terrainAsset;
    const auto terrainDir=root/"models/world/runtime/chapter-terrain-v1";
    const bool terrainLoaded=assets::AssetManifest::Load(terrainDir/"asset.manifest.json",terrainManifest,error)&&
        assets::StaticMeshAsset::Load(terrainDir/"chapter-terrain-lod0.runtime.glb",terrainManifest,terrainAsset,error);
    check(terrainLoaded,"production terrain manifest/native mesh must import");
    auto production=terrainLoaded?PrepareChapterTerrainSurface(terrainAsset,error):nullptr;
    check(bool(production),"production terrain creates immutable shared support");
    if(production) {
        const auto bare=production;auto productionOwner=original;ForestDeadwoodReport built;
        const bool ready=AppendPreparedForestDeadwood(root,productionOwner,production,built,error);
        if(!ready)std::cerr<<error<<'\n';
        check(ready,"production terrain and bounded root must admit together");
        if(ready) {
            using namespace horde::gameplay;
            const auto a=simulation::kWorldRoutePoints[3],b=simulation::kWorldRoutePoints[4];
            const float length=std::hypot(b.x-a.x,b.z-a.z),fx=(b.x-a.x)/length,fz=(b.z-a.z)/length,rx=fz,rz=-fx;
            const float cx=built.trailRoot.origin[0],cz=built.trailRoot.origin[2];
            check(std::abs(cx-(-23.5f))<.001f && std::abs(cz-(-3.4f))<.001f,
                "one root is pinned to trail segment3 at fraction .40");
            check(built.maximumRootRise>=.09f && built.maximumRootRise<=.12001f,
                "real-terrain face/approach survey retains a visible <=12cm root");
            std::vector<Surface::Triangle> rendered;
            const auto primitive=productionOwner.primitives.back();
            for(std::size_t i=0;i<built.stepTriangles;++i) {
                Surface::Triangle triangle;triangle.surface=simulation::SupportSurface::Wood;
                for(unsigned corner=0;corner<3;++corner) {
                    const auto index=primitive.vertexOffset+productionOwner.indices[built.rootIndexOffset+i*3+corner];
                    check(index==built.rootVertexOffset+i*3+corner,"render index selects the exact generated root vertex");
                    const auto& p=productionOwner.vertices[index].position;triangle.points[corner]={p[0],p[1],p[2]};
                }
                rendered.push_back(triangle);
            }
            auto expected=Surface::WithStepGeometry(*bare,rendered,error);
            check(bool(expected),"exact final RT triangles independently build a valid STEP reference");
            if(expected)for(int cross=-8;cross<=8;++cross)for(int along=-8;along<=8;++along) {
                const float x=cx+rx*float(cross)*.1f+fx*float(along)*.1f;
                const float z=cz+rz*float(cross)*.1f+fz*float(along)*.1f;
                const auto g=bare->Query(x,z);
                const auto actual=production->StepSupport(x,z,x,z,g.worldY,g.worldY+.18f,.24f);
                const auto reference=expected->StepSupport(x,z,x,z,g.worldY,g.worldY+.18f,.24f);
                check(actual.found==reference.found && actual.worldY==reference.worldY && actual.normal==reference.normal &&
                    actual.triangle==reference.triangle && actual.surface==reference.surface,
                    "production STEP contacts equal the exact world-space rendered triangles");
                check(production->StepVolumeClear(x,z,x,z,g.worldY+.001f,g.worldY+.14f,.04f)==
                    expected->StepVolumeClear(x,z,x,z,g.worldY+.001f,g.worldY+.14f,.04f),
                    "side/bottom collision follows render geometry too");
                check(production->Query(x,z).worldY==g.worldY,"root STEP never changes bare tree/terrain placement samples");
            }
            const auto walk=[&](const Surface& ground,float sign,float cross,float height,bool& raised) {
                float x=cx+rx*cross-fx*.6f*sign,z=cz+rz*cross-fz*.6f*sign;
                float foot=traversal::ChapterExteriorStepSupport(ground,x,z,.24f).worldY;
                for(unsigned i=1;i<=40;++i) {
                    const float along=(-.6f+.03f*float(i))*sign;
                    const float toX=cx+rx*cross+fx*along,toZ=cz+rz*cross+fz*along;
                    const auto move=traversal::ResolveChapterStepMovement(ground,x,z,toX,toZ,foot,.24f,height);
                    if(!move.accepted) {
                        const auto fromGround=traversal::ChapterExteriorSupport(&ground,x,z),toGround=traversal::ChapterExteriorSupport(&ground,toX,toZ);
                        const auto current=traversal::ChapterExteriorStepSupport(ground,x,z,.24f),next=traversal::ChapterExteriorStepSupport(ground,toX,toZ,.24f);
                        const auto swept=ground.StepSupport(x,z,toX,toZ,std::min(fromGround.worldY,toGround.worldY),
                            std::min(foot+.18f,std::min(fromGround.worldY,toGround.worldY)+.18f),.24f);
                        const float raisedFoot=std::max({foot,next.worldY,swept.found?swept.worldY:foot});
                        std::cout<<"Root move rejected sign="<<sign<<" cross="<<cross<<" height="<<height<<" step="<<i
                            <<" ground="<<fromGround.worldY<<".."<<toGround.worldY<<" foot="<<foot<<" current="<<current.worldY
                            <<" next="<<next.worldY<<" swept="<<swept.found<<':'<<swept.worldY
                            <<" ground_clear="<<ground.GroundSweep(x,z,toX,toZ,fromGround.worldY,.24f)
                            <<" props_clear="<<traversal::ChapterExteriorObstacleClear(ground,x,z,toX,toZ,std::min(foot,next.worldY)+.001f,raisedFoot+height,.24f,false)
                            <<" step_clear="<<ground.StepVolumeClear(x,z,toX,toZ,raisedFoot+.001f,raisedFoot+height,.24f)
                            <<" body_clear="<<ground.HorizontalBodyVolumeClear(x,z,toX,toZ,raisedFoot,height,.24f)<<'\n';
                        return false;
                    }
                    raised|=move.support.worldY>ground.Query(toX,toZ).worldY+.01f;
                    foot=move.support.worldY;x=toX;z=toZ;
                }
                return true;
            };
            bool raised=false;
            check(walk(*production,1,0,1.8f,raised)&&raised,"production forward motor approach steps onto the real root");
            raised=false;check(walk(*production,-1,0,1.8f,raised)&&raised,"production reverse motor approach also clears the root");
            raised=false;check(walk(*production,1,.95f,1.8f,raised)&&!raised,"a walkable alternative beside the 1.10m cross-trail root stays open");
            const float roofY=bare->Query(cx,cz).worldY+1.82f;
            std::array<Surface::Triangle,2> roof{};
            roof[0].points={Surface::Point{cx-1,roofY,cz-1},Surface::Point{cx+1,roofY,cz-1},Surface::Point{cx+1,roofY,cz+1}};
            roof[1].points={Surface::Point{cx-1,roofY,cz-1},Surface::Point{cx+1,roofY,cz+1},Surface::Point{cx-1,roofY,cz+1}};
            auto covered=Surface::WithAdditionalNonWalkableGeometry(*production,roof,error);
            check(bool(covered),"headroom fixture retains actual terrain/root contact");
            if(covered) {
                raised=false;check(!walk(*covered,1,0,1.8f,raised),"standing cannot step its head through real overhead geometry");
                raised=false;check(walk(*covered,1,0,1.1f,raised)&&raised,"short body clears the same measured root/roof without exempting the roof");
            }
            std::cout<<"Trail root: triangles="<<built.stepTriangles<<" vertices="<<built.rootVertices
                <<" native_bytes="<<built.rootGeometryBytes<<" max_rise="<<built.maximumRootRise
                <<" bounds="<<built.trailRoot.minimum[0]<<','<<built.trailRoot.minimum[1]<<','<<built.trailRoot.minimum[2]
                <<".."<<built.trailRoot.maximum[0]<<','<<built.trailRoot.maximum[1]<<','<<built.trailRoot.maximum[2]<<'\n';
        }
    }
    // Existing STEP authority cannot be replaced, and a failed combined intake
    // must not publish the newly prepared mesh or discard previous collision.
    const std::array<Surface::Triangle,1> existingStep{{ground[0]}};
    auto occupied=Surface::WithStepGeometry(*originalSurface,existingStep,error);auto occupiedOwner=original;
    const auto occupiedBefore=occupied;
    check(!AppendPreparedForestDeadwood(root,occupiedOwner,occupied,report,error)&&occupied==occupiedBefore&&
        occupiedOwner.indices==original.indices,"preexisting STEP generation fails closed without replacement");
    const std::size_t oldContactCount=Surface::kMaximumTriangles-2-11884-20+1;
    const std::vector<Surface::Triangle> oldContact(oldContactCount,oldWall);
    auto crowded=Surface::Build(ground,error);crowded=Surface::WithNonWalkableGeometry(*crowded,oldContact,error);
    const auto crowdedBefore=crowded;auto crowdedOwner=original;
    check(!AppendPreparedForestDeadwood(root,crowdedOwner,crowded,report,error)&&crowded==crowdedBefore&&
        crowdedOwner.indices==original.indices,"ground plus old props plus new props plus STEP enforce one combined intake budget atomically");
    std::cout<<"Prepared deadwood/contact "<<(ok?"pass":"fail")<<'\n';return ok?0:1;
}
