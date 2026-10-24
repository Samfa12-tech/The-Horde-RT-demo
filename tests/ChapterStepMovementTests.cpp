#include "gameplay/simulation/GameSimulation.h"
#include "gameplay/traversal/ChapterTerrainMovement.h"
#include "scene/ChapterTerrainSurface.h"
#include "scene/assets/AssetManifest.h"
#include <filesystem>
#include <iostream>
#include <memory>
#include <string_view>

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
bool WalkTo(sim::GameSimulation& game,float x,float z,bool crouch=false) {
    sim::InputSnapshot input;
    input.crouchHeld=crouch;
    for(unsigned tick=0;tick<6000;++tick) {
        const auto& s=game.Snapshot();const float dx=x-s.playerX,dz=z-s.playerZ;
        if(std::hypot(dx,dz)<.012f)return true;
        input.yawRadians=std::atan2(dx,-dz);input.moveForward=1;game.StepFixed(input);
        if(game.Snapshot().worldRoute.blocked){std::cerr<<"blocked at "<<s.playerX<<','<<s.playerZ<<" toward "<<x<<','<<z<<'\n';return false;}
    }
    std::cerr<<"movement deadline at "<<game.Snapshot().playerX<<','<<game.Snapshot().playerZ
             <<" paused="<<game.Snapshot().paused<<'\n';return false;
}
void TestPausedTerrainRebind(const std::shared_ptr<const Surface>& ground,
                            const std::shared_ptr<const Surface>& otherTerrain) {
    auto owned=std::make_unique<sim::GameSimulation>(sim::ProductionGameSimulationConfig());
    auto& game=*owned;
    // Isolate readiness/pose publication at the real F01 anchor. The ordinary
    // save admission supplies this state without replaying the whole ascent;
    // this regression is not evidence of a naturally completed journey.
    horde::gameplay::campaign::Checkpoint checkpoint;
    checkpoint.id=horde::gameplay::campaign::CheckpointId::SafeExit;
    checkpoint.zone=sim::WorldZoneId::TombExterior;checkpoint.position=tr::kExteriorLanding;
    checkpoint.yaw=0;checkpoint.pitch=-.15f;checkpoint.firstAscentCompleted=true;
    checkpoint.dialogue.grateConsumed=checkpoint.dialogue.rescueConsumed=
        checkpoint.dialogue.reunionStarted=checkpoint.dialogue.waitingForRaise=true;
    const auto restore=game.BeginCampaignRestore(checkpoint);
    const bool restored=restore && game.CommitCampaignRestore(*restore,
        {restore->worldGeneration,checkpoint.zone},ground,{});
    Check(restored,"rebind regression admits actual F01 terrain/held pose through the existing safe-side restore");
    if(!restored)return;
    sim::InputSnapshot paused;paused.paused=true;paused.yawRadians=game.Snapshot().playerYawRadians;
    paused.pitchRadians=game.Snapshot().playerPitchRadians;
    game.SynchronizePausedInput(paused,game.Snapshot().inputPublicationSequence);
    const auto reference=game.Snapshot();const auto oldGeneration=reference.worldRoute.generation;
    game.InvalidateWorldZoneReadiness();
    const auto missing=game.Snapshot();
    Check(missing.heldItemKinematics.leftHandLocal!=reference.heldItemKinematics.leftHandLocal,
          "missing authored terrain reproduces the historical exterior carry fallback at F01");
    const auto eventCount=game.Events().Size();const auto eventSequence=game.Events().NextSequence();
    const auto sameClockAndInput=[&](const auto& s) {
        return s.tickIndex==missing.tickIndex && s.inputPublicationSequence==missing.inputPublicationSequence &&
            s.paused==missing.paused && s.inputMoveForward==missing.inputMoveForward && s.inputMoveStrafe==missing.inputMoveStrafe &&
            s.lastConsumedAttackSequence==missing.lastConsumedAttackSequence &&
            s.lastConsumedParrySequence==missing.lastConsumedParrySequence &&
            s.lastConsumedDodgeSequence==missing.lastConsumedDodgeSequence &&
            s.lastConsumedRouteResetSequence==missing.lastConsumedRouteResetSequence &&
            s.lastConsumedRetrySequence==missing.lastConsumedRetrySequence &&
            s.lastConsumedInteractSequence==missing.lastConsumedInteractSequence &&
            s.lastConsumedToggleHeldLightPoseSequence==missing.lastConsumedToggleHeldLightPoseSequence &&
            s.lastConsumedRunToggleSequence==missing.lastConsumedRunToggleSequence &&
            s.lastConsumedClearRunIntentSequence==missing.lastConsumedClearRunIntentSequence &&
            s.lastConsumedCrouchToggleSequence==missing.lastConsumedCrouchToggleSequence &&
            s.lastConsumedClearCrouchIntentSequence==missing.lastConsumedClearCrouchIntentSequence &&
            s.worldRoute.generation==missing.worldRoute.generation &&
            s.playerX==missing.playerX && s.playerZ==missing.playerZ &&
            s.playerYawRadians==missing.playerYawRadians && s.playerPitchRadians==missing.playerPitchRadians &&
            game.Events().Size()==eventCount && game.Events().NextSequence()==eventSequence;
    };
    Check(!game.PublishChapterTerrain({oldGeneration,sim::WorldZoneId::TombExterior},ground) &&
          game.Snapshot().heldItemKinematics.leftHandLocal==missing.heldItemKinematics.leftHandLocal && sameClockAndInput(game.Snapshot()),
          "stale terrain publication neither repairs nor changes authoritative state");
    const sim::WorldZoneToken fresh{missing.worldRoute.generation,sim::WorldZoneId::TombExterior};
    Check(game.PublishChapterTerrain(fresh,ground),"actual new terrain generation rebind is accepted while paused");
    const auto rebound=game.Snapshot();
    Check(rebound.heldItemKinematics.leftHandLocal==reference.heldItemKinematics.leftHandLocal &&
          rebound.heldItemKinematics.rightHandLocal==reference.heldItemKinematics.rightHandLocal &&
          rebound.rewardLanternWorldFromHinge==reference.rewardLanternWorldFromHinge &&
          rebound.playerAnimation==reference.playerAnimation,
          "fresh terrain immediately republishes coherent held hands and player animation without a gameplay tick");
    Check(rebound.lanternPendulum.worldFromBody==reference.lanternPendulum.worldFromBody,
          "fresh terrain rebinds the lantern body to its corrected hinge without a physics tick");
    for(std::size_t i=0;i<rebound.fireEmitterCount;++i) {
        const auto& before=missing.fireEmitters[i];const auto& after=rebound.fireEmitters[i];
        Check(after.worldFromFlame==reference.fireEmitters[i].worldFromFlame &&
              after.worldFromLight==reference.fireEmitters[i].worldFromLight &&
              after.phase==before.phase && after.lowPassNoise==before.lowPassNoise &&
              after.leanX==before.leanX && after.leanZ==before.leanZ &&
              after.motionTurbulence==before.motionTurbulence,
              "zero-time terrain rebind refreshes fire anchors without advancing visual phase or motion");
    }
    Check(rebound.lanternPendulum.forwardAngleRadians==missing.lanternPendulum.forwardAngleRadians &&
          rebound.lanternPendulum.strafeAngleRadians==missing.lanternPendulum.strafeAngleRadians &&
          rebound.lanternPendulum.torsionAngleRadians==missing.lanternPendulum.torsionAngleRadians &&
          rebound.lanternPendulum.forwardAngularVelocity==missing.lanternPendulum.forwardAngularVelocity &&
          rebound.lanternPendulum.strafeAngularVelocity==missing.lanternPendulum.strafeAngularVelocity &&
          rebound.lanternPendulum.torsionAngularVelocity==missing.lanternPendulum.torsionAngularVelocity && sameClockAndInput(rebound),
          "terrain rebind preserves angular state, fixed tick, input counters and ordered feedback");
    Check(game.PublishChapterTerrain(fresh,ground) && game.Snapshot().playerAnimation==rebound.playerAnimation &&
          game.Snapshot().lanternPendulum==rebound.lanternPendulum && sameClockAndInput(game.Snapshot()),
          "same-pointer same-generation publication remains idempotent");
    Check(!game.PublishChapterTerrain(fresh,otherTerrain) && game.Snapshot().playerAnimation==rebound.playerAnimation &&
          game.Snapshot().lanternPendulum==rebound.lanternPendulum && sameClockAndInput(game.Snapshot()),
          "different terrain cannot replace the current admitted generation");
    constexpr float wideAspect=16.0f/9.0f;
    game.SetPresentationAspect(wideAspect);
    const auto wide=game.Snapshot();
    Check(wide.rewardLanternWorldFromHinge!=rebound.rewardLanternWorldFromHinge &&
          wide.playerAnimation.leftIk.target==wide.heldItemKinematics.leftHandLocal &&
          wide.playerAnimation.rightIk.target==wide.heldItemKinematics.rightHandLocal,
          "actual viewport aspect change immediately republishes its changed hand/animation targets");
    Check(wide.lanternPendulum.worldFromBody==
          horde::gameplay::interactions::ComposeLanternPendulumBodyTransform(
              wide.rewardLanternWorldFromHinge,wide.lanternPendulum.forwardAngleRadians,
              wide.lanternPendulum.strafeAngleRadians,wide.lanternPendulum.torsionAngleRadians,
              wide.heldItemKinematics.rewardLanternPresentationYawRadians),
          "aspect change reanchors the lantern body to the same newly solved hand immediately");
    Check(wide.lanternPendulum.forwardAngleRadians==rebound.lanternPendulum.forwardAngleRadians &&
          wide.lanternPendulum.strafeAngleRadians==rebound.lanternPendulum.strafeAngleRadians &&
          wide.lanternPendulum.torsionAngleRadians==rebound.lanternPendulum.torsionAngleRadians &&
          wide.lanternPendulum.forwardAngularVelocity==rebound.lanternPendulum.forwardAngularVelocity &&
          wide.lanternPendulum.strafeAngularVelocity==rebound.lanternPendulum.strafeAngularVelocity &&
          wide.lanternPendulum.torsionAngularVelocity==rebound.lanternPendulum.torsionAngularVelocity &&
          sameClockAndInput(wide),
          "aspect rebind preserves swing phase/velocity, tick, input and event order");
    for(std::size_t i=0;i<wide.fireEmitterCount;++i) {
        const auto& before=rebound.fireEmitters[i];const auto& after=wide.fireEmitters[i];
        Check(after.phase==before.phase && after.lowPassNoise==before.lowPassNoise &&
              after.leanX==before.leanX && after.leanZ==before.leanZ && after.motionTurbulence==before.motionTurbulence &&
              (i!=0 || (after.worldFromFlame==wide.heldLight.worldFromFlame &&
                        after.worldFromLight==wide.heldLight.worldFromLight)),
              "aspect change keeps fire/light anchors coherent without advancing phase or motion");
    }
    game.SetPresentationAspect(wideAspect);
    Check(game.Snapshot().playerAnimation==wide.playerAnimation &&
          game.Snapshot().lanternPendulum==wide.lanternPendulum &&
          game.Snapshot().heldLight.worldFromFlame==wide.heldLight.worldFromFlame &&
          game.Snapshot().heldLight.worldFromLight==wide.heldLight.worldFromLight && sameClockAndInput(game.Snapshot()),
          "repeated identical aspect remains a zero-time no-op");
}
}
int main(int argc,char** argv) {
    const bool rebindOnly=argc==2 && std::string_view(argv[1])=="--paused-terrain-rebind-only";
    const bool positiveLifecycle=argc==2 && std::string_view(argv[1])=="--positive-lifecycle";
    const bool rejections=argc==2 && std::string_view(argv[1])=="--rejections";
    if(argc>1 && !rebindOnly && !positiveLifecycle && !rejections){std::cerr<<"unknown chapter step test selection\n";return 2;}
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
    if(rebindOnly) {
        TestPausedTerrainRebind(ground,terrain);
        std::cout<<checks<<" paused terrain rebind checks, "<<failures<<" failures\n";
        return failures?1:0;
    }
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
    auto rootAndRoof=root;rootAndRoof.insert(rootAndRoof.end(),roof.begin(),roof.end());
    const auto taggedRoof=Surface::WithStepGeometry(*ground,rootAndRoof,error);
    const auto lowProfile=tr::ResolveChapterStepMovement(*taggedRoof,x-.5f,z,x,z,from.worldY,radius,1.2f);
    Check(lowProfile.accepted && std::abs(lowProfile.support.worldY-(base.worldY+.12f))<.0001f,
          "shorter support admission preserves root top even with overhead faces in the same STEP lane");
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
    // Both bounded selections retain the exact live-root prefix above and keep
    // its game/terrain owner alive through their selected scoped fixtures.
    // No arguments still exercises the original complete in-process sequence.
    if(!rejections) {
        sim::GameSimulation crouching(config);
        const auto restore=crouching.BeginCampaignRestore(c);
        Check(restore && crouching.CommitCampaignRestore(*restore,{restore->worldGeneration,c.zone},ceiling,{}),
              "crouch fixture restores real terrain and low roof");
        for(std::size_t zone=1;zone<sim::kWorldZones.size();++zone)
            crouching.PublishChapterTerrain({crouching.Snapshot().worldRoute.generation,static_cast<sim::WorldZoneId>(zone)},ceiling);
        neutral.yawRadians=c.yaw;neutral.pitchRadians=c.pitch;crouching.StepFixed(neutral);
        const auto standingSave=crouching.CaptureCampaignCheckpoint(c.id);
        Check(standingSave.has_value(),"standing safe-exit fixture is actually saveable before stance rejection");
        auto toggle=neutral;toggle.commands.crouchToggle=1;
        for(int tick=0;tick<20;++tick)crouching.StepFixed(toggle);
        Check(crouching.Snapshot().crouchActive && crouching.Snapshot().lastConsumedCrouchToggleSequence==1,
              "legacy diagnostic toggle is consumed once across repeated publications");
        toggle.commands.crouchToggle=2;
        for(int tick=0;tick<20;++tick)crouching.StepFixed(toggle);
        Check(!crouching.Snapshot().crouchActive,"fresh toggle returns to standing");
        toggle.commands.clearCrouchIntent=1;toggle.crouchHeld=true;crouching.StepFixed(toggle);
        Check(!crouching.Snapshot().crouchActive,"clear with held stance requires physical release");
        toggle.crouchHeld=false;crouching.StepFixed(toggle);toggle.crouchHeld=true;crouching.StepFixed(toggle);
        Check(crouching.Snapshot().crouchActive,"consumed neutral clear does not swallow next fresh hold");
        toggle.crouchHeld=false;for(int tick=0;tick<20;++tick)crouching.StepFixed(toggle);
        auto controls=neutral;controls.crouchHeld=true;
        const auto support=crouching.Snapshot().playerSupportWorldY;
        float previousDrop=0;
        for(int tick=0;tick<20;++tick){crouching.StepFixed(controls);const auto& s=crouching.Snapshot();
            Check(s.playerCrouchDropMetres>=previousDrop && s.playerCrouchDropMetres-previousDrop<=.0401f &&
                  s.playerSupportWorldY==support,"crouch eye transition is bounded and never lowers foot support");previousDrop=s.playerCrouchDropMetres;}
        Check(crouching.Snapshot().crouchActive && std::abs(previousDrop-.6f)<.0001f &&
              !crouching.CaptureCampaignCheckpoint(c.id),"active crouch rejects saves that would restore standing");
        const auto crouched=crouching.Snapshot();controls.paused=true;
        Check(crouching.AdvanceFrame(controls,.25)==0 && crouching.Snapshot().playerCrouchDropMetres==crouched.playerCrouchDropMetres,
              "pause freezes crouch camera/body without expanding through geometry");
        controls.paused=false;controls.crouchHeld=false;
        for(int tick=0;tick<20;++tick)crouching.StepFixed(controls);
        Check(!crouching.Snapshot().crouchActive,"open standing clearance permits smooth return");
        Check(WalkTo(crouching,sim::kWorldRoutePoints[2].x,sim::kWorldRoutePoints[2].z) &&
              WalkTo(crouching,sim::kWorldRoutePoints[3].x,sim::kWorldRoutePoints[3].z) && WalkTo(crouching,x-.7f,z),
              "crouch fixture approaches roof by actual production route");
        const auto crossX=[&](float target,bool sideways) {
            unsigned blockedTicks=0;bool safe=true;float oldDrop=crouching.Snapshot().playerCrouchDropMetres;
            for(int tick=0;tick<300;++tick) {
                const float dx=target-crouching.Snapshot().playerX;
                if(std::abs(dx)<.012f) {
                    std::cout<<"automatic crossing target="<<target<<" ticks="<<tick<<" blocked="<<blockedTicks<<'\n';
                    return safe && blockedTicks<=15;
                }
                controls={};controls.yawRadians=sideways?0.f:1.57079632679f;
                const float axis=std::clamp(dx*60.f/.8f,-1.f,1.f);
                if(sideways)controls.moveStrafe=axis;else controls.moveForward=axis;
                crouching.StepFixed(controls);const auto& state=crouching.Snapshot();
                blockedTicks+=state.worldRoute.blocked;
                safe=safe && std::abs(state.playerCrouchDropMetres-oldDrop)<=.0401f;
                oldDrop=state.playerCrouchDropMetres;
                if(std::abs(state.playerX-x)<.3f+radius)
                    safe=safe && state.playerSupportWorldY+sim::kStandingBodyHeight-oldDrop<=roofY+.001f;
            }
            return false;
        };
        Check(crossX(x,false) && crouching.Snapshot().crouchActive &&
              std::abs(crouching.Snapshot().playerSupportWorldY-(base.worldY+.12f))<.0001f,
              "ordinary input automatically lowers the body and steps onto root without a manual stance command");
        controls={};controls.yawRadians=crouching.Snapshot().playerYawRadians;
        for(int tick=0;tick<20;++tick)crouching.StepFixed(controls);
        Check(crouching.Snapshot().crouchStandingBlocked && std::abs(crouching.Snapshot().playerCrouchDropMetres-.6f)<.0001f,
              "release beneath ceiling retains safe posture and reports blocked standing");
        Check(crossX(x+1,false),"automatic crouch permits a safe forward exit without holding stance");
        controls.yawRadians=crouching.Snapshot().playerYawRadians;
        for(int tick=0;tick<20;++tick)crouching.StepFixed(controls);
        Check(!crouching.Snapshot().crouchActive && !crouching.Snapshot().crouchStandingBlocked,
              "standing resumes once full clearance is available");
        Check(crossX(x-1,false),"backwards input negotiates the same head-clearance passage");
        Check(crossX(x+1,true),"sideways input negotiates the same head-clearance passage");
        controls={};controls.yawRadians=crouching.Snapshot().playerYawRadians;
        for(int tick=0;tick<20;++tick)crouching.StepFixed(controls);
        Check(!crouching.Snapshot().crouchActive,"neutral input outside the passage returns to standing");
        controls.crouchHeld=true;for(int tick=0;tick<20;++tick)crouching.StepFixed(controls);
        const auto fresh=crouching.BeginCampaignRestore(c);
        Check(fresh && crouching.CommitCampaignRestore(*fresh,{fresh->worldGeneration,c.zone},ceiling,controls) &&
              !crouching.Snapshot().crouchActive,"load reconstructs standing at safe authored checkpoint");
        crouching.StepFixed(controls);
        Check(crouching.Snapshot().paused,"held stance cannot cross load neutral-release barrier");
    }
    if(!positiveLifecycle) for(const bool solidWall : {false,true}) {
        auto lowRoof=roof;
        for(auto& triangle:lowRoof)for(auto& point:triangle.points)point[1]=base.worldY+1.15f;
        const auto obstruction=solidWall?
            Surface::WithNonWalkableGeometry(*ground,Box(x,z,base.worldY,2.f),error):
            Surface::WithNonWalkableGeometry(*ground,lowRoof,error);
        sim::GameSimulation blocked(config);const auto restore=blocked.BeginCampaignRestore(c);
        Check(restore && blocked.CommitCampaignRestore(*restore,{restore->worldGeneration,c.zone},obstruction,{}),
              "negative fixture restores a solid wall or passage shorter than the crouched body");
        for(std::size_t zone=1;zone<sim::kWorldZones.size();++zone)
            blocked.PublishChapterTerrain({blocked.Snapshot().worldRoute.generation,static_cast<sim::WorldZoneId>(zone)},obstruction);
        neutral={};neutral.yawRadians=c.yaw;neutral.pitchRadians=c.pitch;blocked.StepFixed(neutral);
        Check(WalkTo(blocked,sim::kWorldRoutePoints[2].x,sim::kWorldRoutePoints[2].z) &&
              WalkTo(blocked,sim::kWorldRoutePoints[3].x,sim::kWorldRoutePoints[3].z) && WalkTo(blocked,x-.7f,z),
              "negative fixture reaches obstruction by ordinary movement");
        auto forward=neutral;forward.yawRadians=1.57079632679f;forward.moveForward=1;
        for(int tick=0;tick<150;++tick)blocked.StepFixed(forward);
        Check(blocked.Snapshot().worldRoute.blocked && !blocked.Snapshot().crouchActive && blocked.Snapshot().playerX<x,
              "automatic posture cannot bypass walls or passages too low even when fully crouched");
    }
    std::cout<<checks<<" chapter step movement checks, "<<failures<<" failures; retained step bytes="
             <<terrain->RetainedBytes()-ground->RetainedBytes()<<'\n';return failures?1:0;
}
