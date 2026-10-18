#include "gameplay/simulation/GameSimulation.h"
#include "gameplay/traversal/ChapterTerrainMovement.h"
#include "scene/ChapterTerrainSurface.h"
#include "scene/assets/AssetManifest.h"
#include <filesystem>
#include <iostream>

namespace sim=horde::gameplay::simulation;
namespace tr=horde::gameplay::traversal;
using Surface=sim::TriangleSupportSurface;
namespace {
unsigned checks=0,failures=0;
void Check(bool value,const char* message){++checks;if(!value){++failures;std::cerr<<message<<'\n';}}
std::vector<Surface::Triangle> Box(float x,float z,float y,float height) {
    using P=Surface::Point;
    const P a{x-.008f,y,z-1},b{x-.008f,y,z+1},c{x+.008f,y,z+1},d{x+.008f,y,z-1};
    const P e{x-.008f,y+height,z-1},f{x-.008f,y+height,z+1},g{x+.008f,y+height,z+1},h{x+.008f,y+height,z-1};
    return {{{e,f,g},sim::SupportSurface::Wood},{{e,g,h},sim::SupportSurface::Wood},
            {{a,b,f},sim::SupportSurface::Wood},{{a,f,e},sim::SupportSurface::Wood},
            {{d,h,g},sim::SupportSurface::Wood},{{d,g,c},sim::SupportSurface::Wood},
            {{a,e,h},sim::SupportSurface::Wood},{{a,h,d},sim::SupportSurface::Wood},
            {{b,c,g},sim::SupportSurface::Wood},{{b,g,f},sim::SupportSurface::Wood}};
}
bool WalkTo(sim::GameSimulation& game,float x,float z) {
    sim::InputSnapshot input;
    for(unsigned tick=0;tick<6000;++tick) {
        const auto& s=game.Snapshot();const float dx=x-s.playerX,dz=z-s.playerZ;
        if(std::hypot(dx,dz)<.012f)return true;
        input.yawRadians=std::atan2(dx,-dz);input.moveForward=1;game.StepFixed(input);
        if(game.Snapshot().worldRoute.blocked){std::cerr<<"blocked at "<<s.playerX<<','<<s.playerZ<<" toward "<<x<<','<<z<<'\n';return false;}
    }
    std::cerr<<"movement deadline at "<<game.Snapshot().playerX<<','<<game.Snapshot().playerZ
             <<" paused="<<game.Snapshot().paused<<'\n';return false;
}
}
int main() {
    const auto directory=std::filesystem::path(HORDE_RT_SOURCE_DIR)/"assets/models/world/runtime/chapter-terrain-v1";
    horde::scene::assets::AssetManifest manifest;horde::scene::assets::StaticMeshAsset mesh;std::string error;
    if(!horde::scene::assets::AssetManifest::Load(directory/"asset.manifest.json",manifest,error)||
       !horde::scene::assets::StaticMeshAsset::Load(directory/"chapter-terrain-lod0.runtime.glb",manifest,mesh,error)){
        std::cerr<<error;return 1;}
    const auto ground=horde::scene::PrepareChapterTerrainSurface(mesh,error);if(!ground){std::cerr<<error;return 1;}
    constexpr float x=-23.5f,z=-3.4f,radius=.24f;
    const auto base=tr::ChapterExteriorSupport(ground.get(),x,z);
    Check(base.grounded && tr::ChapterExteriorMovementClear(*ground,x-.5f,z,x+.5f,z,base.worldY,radius),
          "production fixture starts on admitted clear forest terrain away from shaft/trunks");
    const auto root=Box(x,z,base.worldY,.12f);
    const auto terrain=Surface::WithStepGeometry(*ground,root,error);
    Check(bool(terrain),"disposable root attaches to actual imported terrain");if(!terrain)return 1;
    const auto from=tr::ChapterExteriorStepSupport(*terrain,x-.5f,z,radius);
    const auto onto=tr::ResolveChapterStepMovement(*terrain,x-.5f,z,x,z,from.worldY,radius);
    Check(onto.accepted && onto.support.surface==sim::SupportSurface::Wood &&
          std::abs(onto.support.worldY-(base.worldY+.12f))<.0001f,"thin eligible root automatically raises support within18cm");
    const auto across=tr::ResolveChapterStepMovement(*terrain,x-.5f,z,x+.5f,z,from.worldY,radius);
    Check(across.accepted && std::abs(across.support.worldY-ground->Query(x+.5f,z).worldY)<.0001f,
          "continuous sweep crosses root without endpoint sampling or phantom support beyond it");
    const auto off=tr::ResolveChapterStepMovement(*terrain,x,z,x+.5f,z,onto.support.worldY,radius);
    Check(off.accepted && off.support.worldY<onto.support.worldY,"descent returns to the real ground");
    const auto reverse=tr::ResolveChapterStepMovement(*terrain,x+.5f,z,x,z,off.support.worldY,radius);
    Check(reverse.accepted,"root can be approached from the reverse direction");
    Check(!tr::ResolveChapterStepMovement(*terrain,x-.5f,z,x,z,from.worldY+.1f,radius).accepted,
          "stale/forged foot height cannot authorize a lift");
    const auto ordinary=Surface::WithNonWalkableGeometry(*ground,root,error);
    Check(ordinary && !tr::ResolveChapterStepMovement(*ordinary,x-.5f,z,x+.5f,z,from.worldY,radius).accepted,
          "identical ordinary prop remains unsteppable");
    const auto tall=Surface::WithStepGeometry(*ground,Box(x,z,base.worldY,.24f),error);
    Check(tall && !tr::ResolveChapterStepMovement(*tall,x-.5f,z,x+.5f,z,from.worldY,radius).accepted,
          "higher root cannot be bypassed by clipping support to18cm");
    const float roofY=base.worldY+1.87f;
    const std::array<Surface::Triangle,2> roof{{{{{{x-.3f,roofY,z-.6f},{x+.3f,roofY,z+.6f},{x-.3f,roofY,z+.6f}}},sim::SupportSurface::Wood},
                                               {{{{x-.3f,roofY,z-.6f},{x+.3f,roofY,z-.6f},{x+.3f,roofY,z+.6f}}},sim::SupportSurface::Wood}}};
    const auto ceiling=Surface::WithNonWalkableGeometry(*terrain,roof,error);
    Check(ceiling && !tr::ResolveChapterStepMovement(*ceiling,x-.5f,z,x,z,from.worldY,radius).accepted,
          "standing body cannot step into a low ceiling");
    Check(ceiling && tr::ResolveChapterStepMovement(*ceiling,x-.5f,z,x,z,from.worldY,radius,1.2f).accepted,
          "explicit shorter body clearance is independent of foot support (no crouch input/pose claim)");
    Check(!tr::ChapterExteriorCarryClear(*ceiling,x-.5f,z,x,z,from.worldY,radius),
          "raised held-item clearance remains solid to the roof");
    const auto flatQuad=[&](float a,float b) {
        return std::array<Surface::Triangle,2>{{{{{{a,0,-100},{a,0,100},{b,0,100}}}},
                                               {{{{a,0,-100},{b,0,100},{b,0,-100}}}}}};
    };
    const auto flat=Surface::Build(flatQuad(-100,100),error);
    for(float height:{.18f,.181f}) {
        const auto bounded=Surface::WithStepGeometry(*flat,Box(x,z,0,height),error);
        const auto motion=tr::ResolveChapterStepMovement(*bounded,x-.5f,z,x,z,0,radius);
        Check(motion.accepted==(height==.18f),"motor admits exact18cm and rejects18.1cm on a level baseline");
    }
    std::vector<Surface::Triangle> cracked;
    for(const auto part:{flatQuad(-100,x-.002f),flatQuad(x+.002f,100)})
        cracked.insert(cracked.end(),part.begin(),part.end());
    const auto crackedGround=Surface::Build(cracked,error);
    const auto bridge=Surface::WithStepGeometry(*crackedGround,Box(x,z,0,.12f),error);
    Check(bridge && !tr::ResolveChapterStepMovement(*bridge,x-.5f,z,x+.5f,z,0,radius).accepted,
          "eligible root cannot bridge a thin discontinuity in required bare ground");

    // Actual shared simulation, starting from a real admitted safe checkpoint;
    // movement below is ordinary60Hz input, never a teleport or pose import.
    auto config=sim::ProductionGameSimulationConfig();config.movementSpeedMetresPerSecond=.8f;
    sim::GameSimulation game(config);horde::gameplay::campaign::Checkpoint c;
    c.id=horde::gameplay::campaign::CheckpointId::SafeExit;c.zone=sim::WorldZoneId::TombExterior;
    c.position=tr::kExteriorLanding;c.firstAscentCompleted=true;
    c.dialogue.grateConsumed=c.dialogue.rescueConsumed=c.dialogue.reunionStarted=c.dialogue.waitingForRaise=true;
    const auto token=game.BeginCampaignRestore(c);
    Check(token && game.CommitCampaignRestore(*token,{token->worldGeneration,c.zone},terrain,{}),"real checkpoint restore admits root-bearing terrain");
    for(std::size_t zone=1;zone<sim::kWorldZones.size();++zone)
        Check(game.PublishChapterTerrain({game.Snapshot().worldRoute.generation,static_cast<sim::WorldZoneId>(zone)},terrain),"same immutable terrain publishes to route readiness");
    sim::InputSnapshot neutral;neutral.yawRadians=c.yaw;neutral.pitchRadians=c.pitch;
    game.StepFixed(neutral);
    Check(!game.Snapshot().paused,"fresh neutral input rebases the restored absolute camera before movement");
    Check(WalkTo(game,sim::kWorldRoutePoints[2].x,sim::kWorldRoutePoints[2].z) &&
          WalkTo(game,sim::kWorldRoutePoints[3].x,sim::kWorldRoutePoints[3].z) && WalkTo(game,x-.5f,z),
          "shared simulation reaches root via actual production forest path");
    Check(WalkTo(game,x,z) && std::abs(game.Snapshot().playerSupportWorldY-(base.worldY+.12f))<.0001f,
          "snapshot retains stepped support instead of replacing it with bare ground");
    neutral.yawRadians=game.Snapshot().playerYawRadians;game.StepFixed(neutral);
    Check(std::abs(game.Snapshot().playerSupportWorldY-(base.worldY+.12f))<.0001f,
          "stationary support refresh keeps the admitted root height");
    const auto stepped=game.Snapshot();sim::InputSnapshot paused;paused.paused=true;
    Check(game.AdvanceFrame(paused,.25)==0 && game.Snapshot().playerSupportWorldY==stepped.playerSupportWorldY,
          "pause does not advance or lose admitted step support");
    Check(WalkTo(game,x+.5f,z) && std::abs(game.Snapshot().playerSupportWorldY-ground->Query(game.Snapshot().playerX,game.Snapshot().playerZ).worldY)<.0001f,
          "live movement steps down to actual terrain without hovering");
    Check(WalkTo(game,x,z) && WalkTo(game,x-.5f,z),"actual simulation also steps onto and off root in reverse");
    const auto oldGeneration=game.Snapshot().worldRoute.generation;game.InvalidateWorldZoneReadiness();
    Check(!game.PublishChapterTerrain({oldGeneration,sim::WorldZoneId::ForestApproach},terrain),
          "retired terrain generation cannot resurrect step support after reconstruction");
    std::cout<<checks<<" chapter step movement checks, "<<failures<<" failures; retained step bytes="
             <<terrain->RetainedBytes()-ground->RetainedBytes()<<'\n';return failures?1:0;
}
