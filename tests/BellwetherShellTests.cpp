#include "scene/BellwetherShell.h"
#include "gameplay/effects/BellwetherHearth.h"
#include "gameplay/traversal/BellwetherEastRoute.h"
#include "gameplay/traversal/ChapterTerrainMovement.h"
#include "gameplay/traversal/BellwetherNorthRoute.h"
#include "gameplay/traversal/WaysideArchive.h"
#include "gameplay/traversal/BellwetherTavernDoor.h"
#include "gameplay/traversal/BellwetherWestRoute.h"
#include "scene/ChapterTerrainSurface.h"
#include "scene/ChapterRegionalWater.h"
#include "scene/assets/AssetManifest.h"
#include "scene/atmosphere/ChimneySmoke.h"
#include "scene/assets/StaticPrimitiveGrouping.h"
#include "scene/assets/WorldTriangleKey.h"
#include "../assets/models/world/source/bellwether-tavern-v1/staging.generated.h"
#include <algorithm>
#include <cstring>
#include <cmath>
#include <iostream>
#include <map>

namespace {
bool CheckTavernDetailCandidates(const std::filesystem::path& directory, const horde::scene::assets::StaticMeshAsset& town) {
    using namespace horde::scene::assets;
    using namespace horde::gameplay::bellwether;
    using Surface=horde::gameplay::simulation::TriangleSupportSurface;
    constexpr std::array identity{1.f,0.f,0.f,0.f, 0.f,1.f,0.f,0.f, 0.f,0.f,1.f,0.f, 0.f,0.f,0.f,1.f};
    std::string diagnostic;
    const auto load=[&](const std::string& name,const char* node,std::size_t triangles,StaticMeshAsset& mesh) {
        AssetManifest manifest;
        return AssetManifest::Load(directory/(name+".manifest.json"),manifest,diagnostic) &&
               StaticMeshAsset::Load(directory/(name+".glb"),manifest,mesh,diagnostic) &&
               mesh.indices.size()==triangles*3 && mesh.nodeTransforms.size()==1 &&
               mesh.nodeTransforms.front().name==node && mesh.nodeTransforms.front().world==identity;
    };
    const auto triangles=[](const StaticMeshAsset& mesh) {
        std::vector<Surface::Triangle> result;
        result.reserve(mesh.indices.size()/3);
        for(const auto& primitive:mesh.primitives) {
            for(std::size_t i=0;i<primitive.indexCount;i+=3) {
                Surface::Triangle triangle;
                for(unsigned j=0;j<3;++j) {
                    const auto index=mesh.indices[primitive.indexOffset+i+j];
                    const auto& p=mesh.vertices[primitive.vertexOffset+index].position;
                    triangle.points[j]={p[0],p[1],p[2]};
                }
                result.push_back(triangle);
            }
        }
        return result;
    };
    const auto keys=[](const StaticMeshAsset& mesh) {
        std::map<WorldTriangleKey,std::size_t> result;
        for(const auto& primitive:mesh.primitives) {
            for(std::size_t i=0;i<primitive.indexCount;i+=3)++result[KeyWorldTriangle(mesh,primitive,i)];
        }
        return result;
    };
    StaticMeshAsset shared,near,mid,far;
    constexpr auto furnitureNode="BellwetherTavernFurniture";
    if(!load("tavern-architecture","BellwetherTavernArchitecture",542,shared) ||
       !load("tavern-furniture-lod0",furnitureNode,1012,near) ||
       !load("tavern-furniture-lod1",furnitureNode,276,mid) ||
       !load("tavern-furniture-lod2",furnitureNode,144,far)) {
        std::cerr<<"B01 detail candidate import: "<<diagnostic<<'\n';return false;
    }
    auto townTriangles=keys(town);
    for(const auto* selected:{&shared,&near}) {
        for(const auto& [triangle,count]:keys(*selected)) {
            const auto found=townTriangles.find(triangle);
            if(found==townTriangles.end() || found->second<count)return false;
            found->second-=count;
        }
    }
    const auto sharedTriangles=triangles(shared);
    const auto architecture=Surface::Build(sharedTriangles,diagnostic);
    constexpr float floorY=kSouthDoorHinge[1];
    if(!architecture ||
       !architecture->HorizontalVolumeClear(82.3f,120.f,82.3f,118.55f,floorY+.05f,floorY+2.45f,.10f) ||
       architecture->HorizontalVolumeClear(84.f,120.f,84.f,118.55f,floorY+.05f,floorY+2.45f,.10f) ||
       !architecture->HorizontalVolumeClear(86.88f,119.3f,86.88f,118.7f,floorY+2.03f,floorY+2.05f,.01f))
        return false;
    for(const auto* mesh:{&near,&mid,&far}) {
        const auto furnitureTriangles=triangles(*mesh);
        const auto furniture=Surface::Build(furnitureTriangles,diagnostic);
        if(!furniture || mesh->bounds.minimum!=near.bounds.minimum || mesh->bounds.maximum!=near.bounds.maximum ||
           mesh->materials.size()!=1 || mesh->materials.front().name!="HordeTavernTimber")return false;
        for(const auto& mark:{kChairSeat,kTableTop}) {
            if(furniture->HorizontalVolumeClear(mark[0],mark[2],mark[0],mark[2],mark[1]-.002f,mark[1]+.002f,.025f))
                return false;
        }
    }
    return true;
}
}

int main(int argc,char** argv) {
    if(argc>2) {
        std::cerr<<"Pass at most one B01 detail candidate directory.\n";return 2;
    }
    using namespace horde::scene;
    bool ok=true;
    const auto check=[&](bool v,const char* m){if(!v){ok=false;std::cerr<<"FAIL: "<<m<<'\n';}};
    assets::StaticMeshAsset world;
    for(const auto* name:{"MedievalWall02","TombEarthenware","TombCharredWick"}) {
        assets::StaticMaterial m;m.name=name;world.materials.push_back(m);
    }
    world.nodeTransforms.push_back({"identity",{{1,0,0,0,0,1,0,0,0,0,1,0,0,0,0,1}}});
    assets::StaticRtVertex v{};v.position[0]=-40;v.normal[1]=1;v.tangent[0]=1;v.tangent[3]=1;
    world.vertices.push_back(v);v.position[0]=-39;world.vertices.push_back(v);
    v.position[2]=1;world.vertices.push_back(v);world.indices={0,1,2};world.primitives.push_back({0,0,3,0,0});
    world.bounds={{{-40,0,0}},{{-39,0,1}}};const auto before=world;
    std::string error;const auto root=std::filesystem::path(HORDE_RT_SOURCE_DIR)/"assets";
    const auto terrainRoot=root/"models/world/runtime/chapter-terrain-v1";
    assets::AssetManifest terrainManifest;
    assets::StaticMeshAsset terrainMesh;
    if(!assets::AssetManifest::Load(terrainRoot/"asset.manifest.json",terrainManifest,error) ||
       !assets::StaticMeshAsset::Load(terrainRoot/"chapter-terrain-lod0.runtime.glb",terrainManifest,terrainMesh,error)) {
        std::cerr<<error<<'\n';return 1;
    }
    const auto originalTerrain=PrepareChapterTerrainSurface(terrainMesh,error);
    if(!originalTerrain) {
        std::cerr<<error<<'\n';
        return 1;
    }
    // Use the actual RT vertices and the same native support query as walking.
    // The end segments taper beneath the main river. Interior samples cover
    // the bed, the retaining banks and the short drop through the wheel bay.
    // Physical wheel envelope from the linked Blender source layout.
    constexpr std::array<float,3> axle{137.2f,-21.10f,109.35f};
    constexpr float radius=1.50f,halfWidth=.88f;
    const auto& waterFaces=ChapterRegionalWaterTriangles();
    for(std::size_t section=1;section+2<kBellwetherMillraceStations.size();++section) {
        const auto face=kChapterOriginalRegionalWaterTriangleCount+section*2;
        const auto& first=waterFaces[face];
        const auto& second=waterFaces[face+1];
        bool bed=true,banks=true;
        for(int step=0;step<=12;++step) {
            const float t=float(step)/12;
            const float surface=std::lerp(first[0][1],second[1][1],t);
            for(float across:{0.f,.15f,.5f,.85f,1.f}) {
                const auto coordinate=[&](std::size_t axis) {
                    const float left=std::lerp(first[0][axis],second[1][axis],t);
                    const float right=std::lerp(first[2][axis],first[1][axis],t);
                    return std::lerp(left,right,across);
                };
                const float z=coordinate(2);
                const auto ground=originalTerrain->Query(coordinate(0),z);
                const bool wheelBay=z>axle[2]-radius-1.f && z<axle[2]+radius+.18f;
                const float cover=ground.worldY-surface;
                if(across==0.f || across==1.f) banks=banks && ground.found && cover>.025f;
                else bed=bed && ground.found && (wheelBay || cover>-.43f) && cover<-.34f;
            }
        }
        check(bed,"millrace has continuous native bed support under each graded RT segment");
        check(banks,"millrace water ends inside raised terrain along both bed edges");
    }
    bool wheelClearsTerrain=true;
    for(int step=-32;step<=32;++step) {
        const float dz=radius*static_cast<float>(step)/32.f;
        const float rimBottom=axle[1]-std::sqrt(std::max(0.f,radius*radius-dz*dz));
        for(float dx:{-halfWidth,-halfWidth*.5f,0.f,halfWidth*.5f,halfWidth}) {
            const auto bed=originalTerrain->Query(axle[0]+dx,axle[2]+dz);
            wheelClearsTerrain=wheelClearsTerrain && bed.found && bed.worldY<rimBottom-.15f;
        }
    }
    check(wheelClearsTerrain,"the complete wheel rotation envelope clears the actual native terrain by at least 15 cm");
    auto support=originalTerrain;
    assets::StaticMeshAsset glazing,cover,ledger,door;
    const auto result=AppendBellwetherShell(root,world,support,error,&glazing,&cover,&ledger,&door);
    if(!result)std::cerr<<error<<'\n';
    check(result,"real exported town and its tavern interior enter the native world owner");
    using Tier=BellwetherFurnitureDetail;
    const auto select=[](float beyond,Tier current) {
        return SelectBellwetherFurnitureDetail(82.3f,
            horde::gameplay::bellwether::kRoomMax[2]+beyond,current);
    };
    const std::array approach{std::pair{30.f,Tier::Far},std::pair{17.f,Tier::Far},
        std::pair{15.f,Tier::Mid},std::pair{5.f,Tier::Mid},std::pair{3.f,Tier::Near},
        std::pair{0.f,Tier::Near},std::pair{5.f,Tier::Near},std::pair{7.f,Tier::Mid},
        std::pair{19.f,Tier::Mid},std::pair{21.f,Tier::Far},std::pair{19.f,Tier::Far},
        std::pair{8.f,Tier::Mid},std::pair{2.f,Tier::Near}};
    auto tier=Tier::Near;
    for(const auto& [position,wanted]:approach) {
        tier=select(position,tier);
        check(tier==wanted,"room entry, retreat and repeat approach keep the tier through hysteresis bands");
    }
    check(SelectBellwetherFurnitureDetail(100.f,108.f,Tier::Far)==Tier::Near,
          "the full interior perimeter selects original furniture");
    if(result) {
        if(argc==2) {
            auto base=before;
            auto baseSupport=originalTerrain;
            horde::scene::assets::StaticMeshAsset furniture;
            const bool isolated=AppendBellwetherShell(root,base,baseSupport,error,
                nullptr,nullptr,nullptr,nullptr,Tier::Near,&furniture);
            if(!isolated)std::cerr<<error<<'\n';
            check(isolated && base.indices.size()+kBellwetherFurnitureTriangles.front()*3==world.indices.size() &&
                  furniture.materials.size()==1 && furniture.primitives.size()==kBellwetherFurnitureTriangles.size() &&
                  baseSupport->RetainedBytes()==support->RetainedBytes(),
                  "staging tiers separates the original triangles once and preserves authored collision");
            if(isolated && furniture.primitives.size()==kBellwetherFurnitureTriangles.size())
                for(std::size_t lod=0;lod<furniture.primitives.size();++lod)
                    check(furniture.primitives[lod].indexCount==kBellwetherFurnitureTriangles[lod]*3,
                          "each isolated tier owns exactly its admitted triangles");
            check(CheckTavernDetailCandidates(argv[1],world),
                  "source-review detail candidates match the tavern registration, apertures and furniture levels");
            for(const auto detail:{BellwetherFurnitureDetail::Mid,BellwetherFurnitureDetail::Far}) {
                auto comparison=before;
                auto comparisonSupport=originalTerrain;
                const auto level=static_cast<std::size_t>(detail);
                const auto delta=kBellwetherFurnitureTriangles.front()-kBellwetherFurnitureTriangles[level];
                const bool admitted=AppendBellwetherShell(root,comparison,comparisonSupport,error,
                    nullptr,nullptr,nullptr,nullptr,detail);
                if(!admitted)std::cerr<<error<<'\n';
                check(admitted && comparison.indices.size()+delta*3==world.indices.size() &&
                      comparison.vertices.size()<world.vertices.size() && comparison.materials.size()==world.materials.size() &&
                      comparison.primitives.size()==world.primitives.size(),
                      "each fixed render tier replaces only the source furniture under the same owning primitive");
                if(admitted)check(comparisonSupport->RetainedBytes()==support->RetainedBytes() &&
                      comparisonSupport->Query(82.3f,119.2f).worldY==support->Query(82.3f,119.2f).worldY &&
                      !comparisonSupport->NonWalkableVolumeClear(82.f,107.3f,82.f,107.3f,
                          horde::gameplay::bellwether::kChairSeat[1]-.002f,horde::gameplay::bellwether::kChairSeat[1]+.002f,.025f),
                      "render detail preserves the original gameplay contact and route support");
            }
        }
        check(world.indices.size()==before.indices.size()+(kBellwetherSolidTriangles+kBellwetherGroundTriangles)*3,
            "the complete bounded town and tavern geometry is rendered");
        check(world.materials.size()==before.materials.size(),"no texture layer or material row expansion");
        check(support->NonWalkableTriangleCount()==kBellwetherSolidTriangles+kBellwetherGlazingTriangles &&
              support->TriangleCount()==originalTerrain->TriangleCount()+kBellwetherGroundTriangles,
            "render contact preserves the authored distinction between solids and walkable surfaces");
        using namespace horde::gameplay::bellwether;
        {
            using namespace horde::gameplay::traversal;
            constexpr float radius=horde::gameplay::kPlayerCollisionRadius;
            const auto centre=kBellwetherWellCentre;
            const auto view=kBellwetherWellView;
            const auto ground=support->Query(centre[0],centre[2]);
            check(ground.found && ground.walkable && std::abs(ground.worldY-centre[1])<.03f &&
                  !support->NonWalkableVolumeClear(centre[0]-1.5f,centre[2],centre[0]+1.5f,centre[2],
                      ground.worldY+.05f,ground.worldY+1.8f,radius),
                  "the covered well joins the opaque town collision and blocks a player sweep across its footprint");
            const auto square=kBellwetherTavernRoute[kBellwetherWestJunction];
            const auto follow=[&](BellwetherStation start, BellwetherStation target) {
                auto point=start;
                auto contact=ChapterExteriorStepSupport(*support,point.x,point.z,radius,1.8f);
                const float length=std::hypot(target.x-point.x,target.z-point.z);
                const int steps=std::max(1,static_cast<int>(std::ceil(length/.05f)));
                for(int i=1;i<=steps && contact.grounded;++i) {
                    const float t=static_cast<float>(i)/steps;
                    const BellwetherStation next{std::lerp(start.x,target.x,t),std::lerp(start.z,target.z,t)};
                    const auto move=ResolveChapterStepMovement(*support,point.x,point.z,next.x,next.z,
                        contact.worldY,radius);
                    if(!move.accepted)return false;
                    contact=move.support;
                    point=next;
                }
                return contact.grounded;
            };
            check(follow(square,{view[0],view[1]}) && follow({view[0],view[1]},square),
                  "the validated south well view is reachable from the square and back along grounded collision support");
            const auto& garden=kBellwetherGardenApproach;
            const auto stall=kBellwetherWestApproach[1];
            check(follow(stall,garden[0]) && follow(garden[0],garden[1]) && follow(garden[1],garden.back()) &&
                  follow(garden.back(),kBellwetherGardenView) && follow(kBellwetherGardenView,garden.back()) &&
                  follow(garden.back(),kBellwetherGardenAisle) && follow(kBellwetherGardenAisle,garden.back()) &&
                  follow(garden.back(),garden[1]) && follow(garden[1],garden[0]) && follow(garden[0],stall),
                  "the west stall connects to the dry garden aisle and view without crossing the raised beds");
            constexpr auto gardenCentre=kBellwetherGardenCentre;
            const auto gardenGround=support->Query(gardenCentre[0],gardenCentre[1]);
            check(gardenGround.found && gardenGround.walkable &&
                  !support->NonWalkableVolumeClear(gardenCentre[0],gardenCentre[1],gardenCentre[0],gardenCentre[1]+1.f,
                      gardenGround.worldY+.04f,gardenGround.worldY+1.8f,radius) &&
                  !support->NonWalkableVolumeClear(gardenCentre[0],gardenCentre[1],gardenCentre[0],gardenCentre[1]-1.f,
                      gardenGround.worldY+.04f,gardenGround.worldY+1.8f,radius),
                  "both timber bed edges remain solid on either side of the garden aisle");
            const auto& yard=kBellwetherServiceYardApproach;
            const auto branch=kBellwetherWestApproach.front();
            check(follow(branch,yard[0]) && follow(yard[0],yard[1]) && follow(yard[1],yard.back()) &&
                  follow(yard.back(),kBellwetherServiceYardView) && follow(kBellwetherServiceYardView,yard[1]) &&
                  follow(yard[1],yard[0]) && follow(yard[0],branch),
                  "the service yard is accessible from the west lane and stays off the tavern ramp");
            const auto& listen=kBellwetherPrivyListenPoint;
            const auto& mouth=kBellwetherPrivyMouth;
            check(support->NonWalkableVolumeClear(yard.back().x,yard.back().z,listen[0],listen[2],
                      listen[1]-.025f,listen[1]+.025f,.025f) &&
                  !support->NonWalkableVolumeClear(listen[0],listen[2],mouth[0],mouth[2],
                      mouth[1]-.025f,mouth[1]+.025f,.025f),
                  "the listener can approach the door while the solid leaf conceals the occupant");
            for(const auto& rag:{kPrivyRagBasket,kPrivyRagShelf}) {
                check(follow(yard[1],yard.back()) && std::hypot(yard.back().x-rag[0],yard.back().z-rag[2])<2.f &&
                      support->NonWalkableVolumeClear(yard.back().x,yard.back().z,rag[0],rag[2],
                          rag[1]+.07f,rag[1]+.11f,.025f),
                      "a grounded visitor can reach the cloth above both the basket and the door shelf");
            }
            const auto smithStation=[](const auto& point) { return BellwetherStation{point[0],point[1]}; };
            const auto& smith=kBellwetherSmithApproachXZ;
            bool smithRoundTrip=follow(square,smithStation(smith.front())) &&
                follow(smithStation(smith.front()),square);
            for(std::size_t i=1;i<smith.size();++i) {
                const auto before=smithStation(smith[i-1]), after=smithStation(smith[i]);
                smithRoundTrip=smithRoundTrip && follow(before,after) && follow(after,before);
            }
            const auto covered=smithStation(smith.back());
            const auto& door=kBellwetherSmithDoorApproachXZ;
            const auto turn=smithStation(door.front()), step=smithStation(door.back());
            check(smithRoundTrip && follow(covered,turn) && follow(turn,step) &&
                  follow(step,turn) && follow(turn,covered),
                  "the square, open smith yard and west door approach stay connected over grounded support");
            const auto roadNorth=BellwetherMapStation(9.f,-18.f), roadSouth=BellwetherMapStation(18.f,-37.f);
            check(follow(roadNorth,roadSouth) && follow(roadSouth,roadNorth),
                  "the mapped southeast road remains walkable past the smith yard in both directions");
            using horde::gameplay::effects::SelectBellwetherExteriorLamp;
            const auto& wick=kBellwetherSmithLampLight;
            check(SelectBellwetherExteriorLamp(wick[0],wick[2])==4 &&
                  SelectBellwetherExteriorLamp(square.x,square.z)==2 &&
                  SelectBellwetherExteriorLamp(kRelicStallLampLight[0],kRelicStallLampLight[2])==3,
                  "the smith wick shares the exterior light slot with the preserved square and west-lane choices");
            const auto anvil=smithStation(kBellwetherSmithAnvilXZ);
            const auto floor=support->Query(anvil.x,anvil.z);
            check(floor.found && floor.walkable &&
                  !support->NonWalkableVolumeClear(anvil.x-1.f,anvil.z,anvil.x+1.f,anvil.z,
                      floor.worldY+.08f,floor.worldY+1.8f,radius),
                  "the anvil and its stump are solid above the walkable hardstand");
            const auto& graveyard=kBellwetherGraveyardApproach;
            const auto church=kBellwetherChurchApproach.back();
            bool graveyardRoundTrip=follow(church,graveyard.front()) && follow(graveyard.front(),church);
            for(std::size_t i=1;i<graveyard.size();++i) {
                if(follow(graveyard[i-1],graveyard[i]) && follow(graveyard[i],graveyard[i-1]))continue;
                std::cerr<<"Graveyard path blocked between stations "<<i-1<<" and "<<i<<'\n';
                graveyardRoundTrip=false;
            }
            check(graveyardRoundTrip,
                  "the original church service connects to the last grave row and back through the dry south entrance");
            const auto wallGround=support->Query(98.2f,71.25f);
            check(wallGround.found && !support->NonWalkableVolumeClear(98.2f,72.2f,98.2f,70.3f,
                      wallGround.worldY+.08f,wallGround.worldY+.85f,radius),
                  "the dry-stone boundary blocks a player sweep alongside the open gate");
            for(const auto& marker:kBellwetherGraveyardMarkers) {
                const auto ground=support->Query(marker[0],marker[1]);
                check(ground.found && ground.walkable &&
                      !support->NonWalkableVolumeClear(marker[0]-.54f,marker[1],marker[0]+.54f,marker[1],
                          ground.worldY+.04f,ground.worldY+.80f,radius),
                      "each original marker has contact on the authored terrain");
            }
        }
        {
            using namespace horde::gameplay::traversal;
            const auto& cache=kRoadSilverCacheReach;
            const auto& approach=kRoadSilverCacheApproach;
            const auto fork=kBellwetherTavernRoute[kBellwetherChurchJunction];
            const auto nextRoad=kBellwetherTavernRoute[kBellwetherChurchJunction+1];
            const auto church=kBellwetherNorthApproach[1];
            const float floor=cache[1]-.64f;
            constexpr float radius=horde::gameplay::kPlayerCollisionRadius;
            check(support->NonWalkableVolumeClear(approach[0],approach[1],cache[0],cache[2],
                      cache[1]-.025f,cache[1]+.025f,.025f),
                "the road cache has a clear reach mark above the lid");
            check(!support->NonWalkableVolumeClear(approach[0],approach[1],cache[0],cache[2],
                      floor+.21f,floor+.47f,radius),
                "the box blocks movement through its solid side");
            const auto clearFork = [&](BellwetherStation end) {
                auto last=fork;
                auto contact=ChapterExteriorStepSupport(*support,last.x,last.z,radius,1.78f);
                const float routeLength=std::hypot(end.x-fork.x,end.z-fork.z);
                for(int i=1;i<=120 && contact.grounded;++i) {
                    const float t=(i*.05f)/routeLength;
                    const BellwetherStation next{std::lerp(fork.x,end.x,t),std::lerp(fork.z,end.z,t)};
                    const auto move=ResolveChapterStepMovement(*support,last.x,last.z,next.x,next.z,contact.worldY,radius);
                    if(!move.accepted)return false;
                    contact=move.support;
                    last=next;
                }
                return contact.grounded;
            };
            check(clearFork(nextRoad),"the grounded player capsule clears the nearby tavern path");
            check(clearFork(church),"the grounded player capsule clears the nearby church path");
        }
        {
            using namespace horde::gameplay::traversal;
            const auto stand=kBellwetherRelicDealerStand;
            const auto approach=kBellwetherRelicDealerApproach;
            const auto ground=support->Query(stand.x,stand.z);
            const float sight=ground.worldY+1.55f;
            check(ground.found && ground.walkable &&
                  support->NonWalkableVolumeClear(approach.x,approach.z,stand.x,stand.z,sight-.02f,sight+.02f,.02f) &&
                  !support->NonWalkableVolumeClear(82.3f,131.7f,stand.x,stand.z,sight-.02f,sight+.02f,.02f) &&
                  !support->NonWalkableVolumeClear(96.f,127.5f,stand.x,stand.z,sight-.02f,sight+.02f,.02f),
                  "the authored stock wing screens the dealer from the square and east road and keeps the talk aisle open");
            check(!support->NonWalkableVolumeClear(66.f,129.2f,approach.x,approach.z,
                      ground.worldY+.06f,ground.worldY+1.72f,horde::gameplay::kPlayerCollisionRadius) &&
                  support->NonWalkableVolumeClear(kRelicStallApproachTurn[0],kRelicStallApproachTurn[1],approach.x,
                      approach.z,ground.worldY+.06f,ground.worldY+1.72f,horde::gameplay::kPlayerCollisionRadius),
                  "the player must go round the solid display and reaches the dealer along the clear return aisle");
        }
        constexpr auto mill=horde::gameplay::traversal::kBellwetherMillApproach.back();
        const auto landing=support->Query(mill.x,mill.z);
        check(landing.found && landing.walkable && landing.triangle>=originalTerrain->TriangleCount() &&
              std::abs(landing.worldY-(kMillWestDoor[1]-.015f))<.001f &&
              !support->NonWalkableVolumeClear(mill.x,mill.z,kMillWestDoor[0]+.3f,kMillWestDoor[2],
                  landing.worldY+.05f,landing.worldY+1.8f,.24f),
              "the mill landing carries the player to the physically closed west door");
        const auto workGround=support->Query(kMillWorkMark[0],kMillWorkMark[2]);
        check(workGround.found && workGround.walkable && std::abs(workGround.worldY-kMillWorkMark[1])<.001f &&
              support->NonWalkableVolumeClear(kMillWorkMark[0],kMillWorkMark[2],kMillWorkMark[0],kMillWorkMark[2],
                  workGround.worldY+.05f,workGround.worldY+1.8f,.24f) &&
              !support->NonWalkableVolumeClear(kMillWorkMark[0],kMillWorkMark[2],kMillWorkMark[0]+1.2f,kMillWorkMark[2],
                  workGround.worldY+.05f,workGround.worldY+1.8f,.24f),
              "the workbench has a clear standing mark but blocks movement through its timber");
        for(float pierX:{135.82f,138.60f}) {
            check(!support->NonWalkableVolumeClear(pierX,108.6f,pierX,110.1f,-21.82f,-21.26f,.08f),
                  "each dressed bearing pier is an admitted physical world solid");
        }
        check(!support->NonWalkableVolumeClear(137.33f,106.2f,137.33f,107.2f,-19.85f,-19.12f,.04f) &&
              support->NonWalkableVolumeClear(137.33f,106.2f,137.33f,107.2f,-20.23f,-20.01f,.04f),
              "the raised sluice boards have collision and leave the headrace clear below");
        constexpr auto shore=horde::gameplay::traversal::kBellwetherRiverLanding.back();
        const auto shoreGround=support->Query(shore.x,shore.z);
        const auto riverY=ChapterRegionalWaterSurfaceAt(shore.x,shore.z);
        check(shoreGround.found && shoreGround.walkable && shoreGround.triangle>=originalTerrain->TriangleCount() &&
              riverY && *riverY-shoreGround.worldY>.15f && *riverY-shoreGround.worldY<.28f &&
              !support->NonWalkableVolumeClear(shore.x,shore.z,shore.x+.55f,shore.z,
                  shoreGround.worldY+.05f,shoreGround.worldY+1.8f,.24f),
              "the river terrace supplies shallow stone support and the timber rail blocks the end of the landing");
        {
            using namespace horde::gameplay::traversal;
            auto point=kBellwetherMillApproach[1];
            constexpr float radius=horde::gameplay::kPlayerCollisionRadius;
            float foot=ChapterExteriorStepSupport(*support,point.x,point.z,radius,1.8f).worldY;
            const auto follow=[&](BellwetherStation target, bool dry=false) {
                const auto start=point;
                const int steps=std::max(1,static_cast<int>(std::ceil(std::hypot(target.x-start.x,target.z-start.z)/.05f)));
                for(int i=1;i<=steps;++i) {
                    const float t=static_cast<float>(i)/steps;
                    const BellwetherStation next{std::lerp(start.x,target.x,t),std::lerp(start.z,target.z,t)};
                    const auto result=ResolveChapterStepMovement(*support,point.x,point.z,next.x,next.z,foot,radius);
                    if(!result.accepted) {
                        std::cerr<<"Mill or river sweep blocked at "<<next.x<<','<<next.z
                                 <<" from "<<point.x<<','<<point.z<<" foot "<<foot
                                 <<" next foot "<<result.support.worldY<<'\n';
                        return false;
                    }
                    const auto water=ChapterRegionalWaterSurfaceAt(next.x,next.z);
                    if(dry && water && result.support.worldY<=*water+.05f) {
                        std::cerr<<"Dry mill path meets water at "<<next.x<<','<<next.z<<'\n';
                        return false;
                    }
                    point=next;
                    foot=result.support.worldY;
                }
                return true;
            };
            const auto& route=kBellwetherRiverLanding;
            check(follow(kBellwetherSalvagerMark) && std::all_of(route.begin(),route.end(),follow) &&
                  std::all_of(route.rbegin(),route.rend(),follow) && follow(kBellwetherSalvagerMark),
                  "ground and body sweeps stay continuous on the way into the river and back");
            const auto dry=[&](BellwetherStation target) { return follow(target,true); };
            check(std::all_of(route.begin(),route.end()-2,dry) &&
                  std::all_of(kBellwetherMillCrossing.begin(),kBellwetherMillCrossing.end(),dry) &&
                  std::all_of(kBellwetherMillCrossing.rbegin(),kBellwetherMillCrossing.rend(),dry),
                  "the ordinary standing capsule walks dry from the old causeway to the wheel and across the millrace in both directions");
            const auto turnout=kBellwetherMillCrossing.back();
            const auto turnoutGround=support->Query(turnout.x,turnout.z);
            check(!support->NonWalkableVolumeClear(turnout.x,turnout.z,turnout.x+1.f,turnout.z,
                      turnoutGround.worldY+.1f,turnoutGround.worldY+1.8f,radius),
                  "the turnout fence blocks a walk off the elevated east end");
        }
        constexpr auto wayside=horde::gameplay::traversal::kBellwetherWaysideApproach.back();
        auto waysideGround=support->Query(wayside.x,wayside.z);
        check(std::abs(wayside.x-kWaysideInspection[0])<.001f && std::abs(wayside.z-kWaysideInspection[2])<.001f &&
              waysideGround.found && waysideGround.walkable && std::abs(waysideGround.worldY-kWaysideInspection[1])<.001f,
            "the mapped wayside route ends on the authored archive inspection mark");
        check(support->NonWalkableVolumeClear(wayside.x,wayside.z,wayside.x,wayside.z-.55f,
                  waysideGround.worldY+.20f,waysideGround.worldY+1.7f,.32f) &&
              support->NonWalkableVolumeClear(kWaysideMechanism[0],kWaysideMechanism[2]+.2f,
                  kWaysideMechanism[0],kWaysideMechanism[2]-.25f,
                  kWaysideMechanism[1]-.05f,kWaysideMechanism[1]+.05f,.01f),
            "the inspection approach is clear and the moving stone has no duplicate static contact");
        const auto* hinged=support->HingedContact();
        check(hinged && hinged->TriangleCount()==kWaysideCoverTriangles && cover.indices.size()==kWaysideCoverTriangles*3,
            "the one local cover mesh supplies native ray geometry and the independent contact owner");
        check(ledger.indices.size()==kWaysideRegisterTriangles*3 &&
              ledger.bounds.minimum[0]<kWaysideRoadRegister[0] && ledger.bounds.maximum[0]>kWaysideRoadRegister[0] &&
              ledger.bounds.minimum[1]<kWaysideRoadRegister[1] && ledger.bounds.maximum[1]>kWaysideRoadRegister[1] &&
              ledger.bounds.minimum[2]<kWaysideRoadRegister[2] && ledger.bounds.maximum[2]<kWaysideMechanism[2],
            "the register is rendered inside the stone niche at its authored pickup mark");
        if(hinged) {
            const auto straight=[&](const ArchivePose& pose) {
                return pose.VolumeClear(*hinged,kWaysideMechanism[0],kWaysideMechanism[2]+.2f,
                    kWaysideMechanism[0],kWaysideMechanism[2]-.25f,kWaysideMechanism[1]-.05f,kWaysideMechanism[1]+.05f,.01f);
            };
            const auto open=ArchivePose::AtOpening(1.f);
            check(!straight({}) && straight(open) &&
                  !open.VolumeClear(*hinged,kWaysideHinge[0]-.05f,kWaysideHinge[2]+.65f,
                      kWaysideHinge[0]+.25f,kWaysideHinge[2]+.65f,kWaysideMechanism[1]-.05f,kWaysideMechanism[1]+.05f,.01f),
                "the opened cover clears its former contact and blocks at the actual rotated pose");
        }
        constexpr float southDoorX=82.3f,southWallZ=119.2f;
        const auto bare=originalTerrain->Query(southDoorX,southWallZ);
        const auto after=support->Query(southDoorX,southWallZ);
        check(glazing.primitives.size()==1 && glazing.materials.size()==1 &&
              (glazing.materials[0].flags & assets::kStaticMaterialCertifiedClosedVolume) &&
              glazing.indices.size()==kBellwetherGlazingTriangles*3 &&
              !support->NonWalkableVolumeClear(86.88f,118.7f,86.88f,119.3f,
                  after.worldY+2.03f,after.worldY+2.05f,.01f),
            "the separately rendered dielectric is a closed volume with contact across the clear pane");
        check(bare.found && after.found && after.walkable && after.triangle>=originalTerrain->TriangleCount() &&
              std::abs(after.worldY-(-8.70955f))<.001f && after.worldY-bare.worldY>2.5f,
            "real ramp and floor meet at the original tavern floor height above current ground");
        check(support->NonWalkableVolumeClear(southDoorX,121.f,southDoorX,118.8f,after.worldY+.01f,after.worldY+1.8f,.24f) &&
             !support->NonWalkableVolumeClear(southDoorX+1.5f,121.f,southDoorX+1.5f,118.8f,
                 after.worldY+.01f,after.worldY+1.8f,.24f),
             "the mapped south opening admits the capsule while the adjacent wall stays solid");
        const auto& hinge=kSouthDoorHinge;
        const auto* doorContact=support->HingedContact(
            horde::gameplay::simulation::TriangleSupportSurface::HingedPart::TavernDoor);
        check(doorContact && doorContact->TriangleCount()==kSouthDoorTriangles &&
              door.indices.size()==kSouthDoorTriangles*3 && door.bounds.minimum[0]>-.05f &&
              door.bounds.maximum[0]<kSouthDoorSwingRadius && door.bounds.minimum[1]>=0.f,
              "the dedicated render leaf and contact have identical hinge-local geometry");
        if(doorContact) {
            const auto open=TavernDoor::PoseAt(1.f);
            const auto closed=TavernDoor::PoseAt(0.f);
            const auto central=[&](const auto& pose) {
                return pose.VolumeClear(*doorContact,southDoorX,121.f,southDoorX,116.1f,
                    after.worldY+.01f,after.worldY+1.8f,.28f);
            };
            check(central(open) && !central(closed) &&
                  !open.VolumeClear(*doorContact,hinge[0]-.43f,hinge[2]-1.1f,hinge[0]+.42f,hinge[2]-1.1f,
                      after.worldY+.9f,after.worldY+1.22f,.07f),
                  "the same leaf blocks the mapped threshold closed and swings clear when open");
        }
        using namespace horde::gameplay::bellwether;
        const auto exactHorizontalFace=[&](const auto& mark) {
            constexpr float epsilon=.001f;
            return !support->NonWalkableVolumeClear(mark[0],mark[2],mark[0],mark[2],
                mark[1]-epsilon,mark[1]+epsilon,.025f) &&
                support->NonWalkableVolumeClear(mark[0],mark[2],mark[0],mark[2],
                mark[1]+epsilon,mark[1]+2*epsilon,.025f);
        };
        check(exactHorizontalFace(kChairSeat) && exactHorizontalFace(kTableTop) &&
              std::abs(support->Query(kChairSeat[0],kChairSeat[2]).worldY-after.worldY)<.001f,
            "the authored contact marks match the seat and table surfaces above the shared floor");
        check(support->NonWalkableVolumeClear(kChairTurn[0],kChairTurn[2],kChairApproach[0],kChairApproach[2],
                  after.worldY+.001f,after.worldY+1.8f,.24f) &&
              !support->NonWalkableVolumeClear(kChairApproach[0],kChairApproach[2],kChairSeat[0],kChairSeat[2],
                  after.worldY+.001f,after.worldY+1.8f,.24f),
            "the real chair leaves a clear approach and blocks the standing player from walking through the seat");
        check(std::memcmp(world.vertices.data(),before.vertices.data(),before.vertices.size()*sizeof(v))==0,
            "accepted geometry prefix remains byte-identical");
        check(world.bounds.maximum[0]>120 && world.bounds.maximum[2]>170,
            "town retains the map's southeast footprint");
        for(const auto& cap:atmosphere::kBellwetherChimneyCaps) {
            unsigned topCorners=0;
            for(std::size_t i=before.vertices.size();i<world.vertices.size();++i) {
                const auto& vertex=world.vertices[i];const auto& p=vertex.position;
                if(vertex.normal[1]>.99f && std::abs(p[1]-cap[1])<.001f &&
                   std::abs(std::abs(p[0]-cap[0])-.55f)<.001f &&
                   std::abs(std::abs(p[2]-cap[2])-.5f)<.001f)++topCorners;
            }
            check(topCorners==4,"each smoke origin matches four actual native chimney cap corners");
        }
        bool slopingNormal=false;
        for(std::size_t i=before.vertices.size();i<world.vertices.size();++i) {
            const auto& n=world.vertices[i].normal;
            slopingNormal|=std::abs(n[1])>.1f && std::abs(n[1])<.99f;
        }
        check(slopingNormal,"real pitched roof normals survive native import");
        // The v4 decorative north doorway was replaced by real wall geometry.
        // A ray at the old portal post now hits the preserved north wall plane.
        float closestZ=104.2f;
        for(std::size_t j=1;j<world.primitives.size();++j) {
            const auto& p=world.primitives[j];
            for(std::size_t k=0;k<p.indexCount;k+=3) {
                const auto& a=world.vertices[p.vertexOffset+world.indices[p.indexOffset+k]].position;
                const auto& b=world.vertices[p.vertexOffset+world.indices[p.indexOffset+k+1]].position;
                const auto& c=world.vertices[p.vertexOffset+world.indices[p.indexOffset+k+2]].position;
                const float determinant=(b[0]-a[0])*(c[1]-a[1])-(c[0]-a[0])*(b[1]-a[1]);
                if(std::abs(determinant)<1e-6f)continue;
                const float x=89.26f-a[0],y=-7.4845508f-a[1];
                const float u=(x*(c[1]-a[1])-(c[0]-a[0])*y)/determinant;
                const float v=((b[0]-a[0])*y-x*(b[1]-a[1]))/determinant;
                if(u<0 || v<0 || u+v>1)continue;
                const float z=a[2]+u*(b[2]-a[2])+v*(c[2]-a[2]);
                if(z>=102.f)closestZ=std::min(closestZ,z);
            }
        }
        check(std::abs(closestZ-103.2f)<.001f,
            "the north facade is closed at the original footprint");
        const auto admittedIndices=world.indices;
        const auto admittedSupport=support;
        check(!AppendBellwetherShell(root,world,support,error) && world.indices==admittedIndices &&
              support==admittedSupport,
            "repeated admission preserves the existing render owner and contact");
        for(std::size_t j=1;j<world.primitives.size();++j) {
            const auto& p=world.primitives[j];double volume=0;
            for(std::size_t k=0;k<p.indexCount;k+=3) {
                const auto& a=world.vertices[p.vertexOffset+world.indices[p.indexOffset+k]].position;
                const auto& b=world.vertices[p.vertexOffset+world.indices[p.indexOffset+k+1]].position;
                const auto& c=world.vertices[p.vertexOffset+world.indices[p.indexOffset+k+2]].position;
                volume+=(double(a[0])*(double(b[1])*c[2]-double(b[2])*c[1])+
                    double(a[1])*(double(b[2])*c[0]-double(b[0])*c[2])+
                    double(a[2])*(double(b[0])*c[1]-double(b[1])*c[0]))/6;
            }
            if(world.nodeTransforms[p.nodeTransformIndex].name=="BellwetherSolid")
                check(volume>.005,"each solid town material range has outward native winding");
        }
        check(assets::ConsolidateWorldBakedMaterialFamilies(world,error) && world.primitives.size()==3,
            "town and furniture coalesce into the three existing opaque world families");
        const auto withoutCrops=world.indices.size();
        check(!AppendBellwetherGardenCrops(root,world,error) && world.indices.size()==withoutCrops,
              "garden foliage cannot allocate a replacement for a missing alder family");
        assets::StaticMaterial alder;
        alder.name="ForestAlder";
        alder.textureGroup=3;
        alder.deferTextureAllocation=true;
        world.materials.push_back(alder);
        const auto alderRow=world.materials.size()-1;
        world.primitives.front().materialIndex=static_cast<std::uint32_t>(alderRow);
        const auto withCrops=withoutCrops+3*kBellwetherGardenCropTriangles;
        check(AppendBellwetherGardenCrops(root,world,error) && world.indices.size()==withCrops &&
              world.primitives.back().materialIndex==alderRow && world.materials.size()==alderRow+1 &&
              world.triangleRegions.back()==assets::StaticTriangleRegion::Shared,
              "garden crop geometry joins the existing atlas family as shared world detail without contact");
        check(!AppendBellwetherGardenCrops(root,world,error) && world.indices.size()==withCrops &&
              assets::ConsolidateWorldBakedMaterialFamilies(world,error) && world.primitives.size()==3,
              "garden foliage rejects duplicate admission and shares an existing consolidated primitive");
    }
    auto missing=before;missing.materials[1].name="Missing";
    auto fallback=originalTerrain;
    check(!AppendBellwetherShell(root,missing,fallback,error) && missing.indices==before.indices &&
          fallback==originalTerrain,"missing admitted clay rejects without partial publication");
    auto absent=before;
    check(!AppendBellwetherShell(root/"missing",absent,fallback,error) && absent.indices==before.indices &&
          fallback==originalTerrain,"missing native source rejects without partial publication");
    fallback.reset();
    check(!AppendBellwetherShell(root,absent,fallback,error) && absent.indices==before.indices &&
          !fallback,"unavailable terrain cannot publish render geometry without matching contact");
    std::cout<<"Bellwether native placement/material/rollback "<<(ok?"passed":"failed")<<".\n";
    return ok?0:1;
}
