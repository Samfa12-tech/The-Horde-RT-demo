#include "gameplay/items/HeldItemKinematics.h"
#include "gameplay/animation/PlayerAnimationState.h"
#include "scene/ShowcaseOverheadGeometry.h"
#include "scene/assets/AssetManifest.h"
#include "vulkan/raytracing/PlayerRenderSlot.h"
#include <algorithm>
#include <array>
#include <cmath>
#include <cstdint>
#include <filesystem>
#include <fstream>
#include <iostream>
#include <sstream>
#include <string>
#include <vector>

namespace {
using namespace horde::gameplay;
using namespace horde::gameplay::items;
using namespace horde::vulkan::raytracing;
using V = std::array<float,3>;
V Add(V a,V b) { for(int i=0;i<3;++i) a[i]+=b[i]; return a; }
V Scale(V a,float b) { for(auto& x:a) x*=b; return a; }
float Dot(V a,V b) { return a[0]*b[0]+a[1]*b[1]+a[2]*b[2]; }
V Cross(V a,V b) { return {{a[1]*b[2]-a[2]*b[1],a[2]*b[0]-a[0]*b[2],a[0]*b[1]-a[1]*b[0]}}; }
V Unit(V a) { return Scale(a,1/std::sqrt(Dot(a,a))); }
V Point(const HeldItemTransform& m,V v) { return {{m[12]+m[0]*v[0]+m[4]*v[1]+m[8]*v[2],m[13]+m[1]*v[0]+m[5]*v[1]+m[9]*v[2],m[14]+m[2]*v[0]+m[6]*v[1]+m[10]*v[2]}}; }
bool Inside(const horde::scene::OverheadVolume& volume,V point) {
    int first=0;
    for(std::size_t i=0;i<4;++i) {
        const auto a=volume.footprint[i],b=volume.footprint[(i+1)%4];
        const float cross=(b[0]-a[0])*(point[2]-a[1])-(b[1]-a[1])*(point[0]-a[0]);
        const int side=cross < -1e-6f ? -1 : cross > 1e-6f ? 1 : 0;
        if(first && side && first!=side) return false;
        if(side) first=side;
    }
    return true;
}
float Headroom(V point) {
    float room=100;
    const auto include=[&](const auto& volumes) { for(const auto& volume:volumes) if(Inside(volume,point)) room=std::min(room,volume.bottomY-point[1]); };
    include(horde::scene::kShowcaseLowOverheadVolumes); include(horde::scene::kShowcaseCeilingPatches);
    return room;
}
struct Solved { HeldItemFixedStepState target; HeldItemStates rendered; HeldLightState light; HeldItemTransform grip; V root; PlayerModelWorldBasis basis; };
// CPU reconstruction of the production scene's view->grounded rig conversion,
// followed by actual imported-rig solve and production item/socket composition.
bool Solve(PlayerRenderSlot& rig,const HeldItemFixedStepInput& input,std::uint64_t tick,Solved& out,std::string& diagnostic) {
    auto items=MakeDefaultHeldItemStates();
    if(!ResolveHeldItemsFixedStep(items,input,tick,out.target,diagnostic)) return false;
    horde::gameplay::animation::PlayerAnimationState state;
    horde::gameplay::animation::PlayerAnimationInput animationInput;
    animationInput.heldItemKinematics=out.target.kinematics;
    animationInput.playerCombat=input.playerCombat;
    animationInput.walkTime=input.walkTime; animationInput.walkAmount=input.walkAmount;
    state.StepFixed(animationInput,1.0f/60);
    auto animation=state.Snapshot();
    // Supply the same settled locomotion blend as a sustained walking snapshot.
    if(input.walkAmount>0) { for(int i=0;i<8;++i) state.StepFixed(animationInput,1.0f/60); animation=state.Snapshot(); }
    const V eye{{input.playerX,kShowcaseEyeWorldY,input.playerZ}},up{{0,1,0}};
    const V forward=Unit({{std::sin(input.playerYawRadians),-.05f+std::clamp(input.playerPitchRadians,-.32f,.28f),-std::cos(input.playerYawRadians)}});
    const V right=Unit(Cross(forward,up)),viewUp=Unit(Cross(right,forward));
    const auto view=[&](V p) { return Add(Add(Scale(right,p[0]),Scale(viewUp,p[1])),Scale(forward,p[2])); };
    const auto gait=EvaluateLowerBodyPose(input.walkTime,input.walkAmount);
    const V bodyForward{{std::sin(input.playerYawRadians),0,-std::cos(input.playerYawRadians)}},bodyRight{{std::cos(input.playerYawRadians),0,std::sin(input.playerYawRadians)}};
    const float c=std::cos(gait.torsoTwistRadians),s=std::sin(gait.torsoTwistRadians);
    out.basis=BuildPlayerModelWorldBasis(Unit(Add(Scale(bodyRight,c),Scale(bodyForward,-s))),Unit(Add(Scale(bodyForward,c),Scale(bodyRight,s))));
    out.root=GroundPlayerRootOnRouteFloor(Add(Add(eye,Scale(bodyRight,gait.pelvisSway)),V{{0,gait.pelvisBob*.65f,0}}),kRouteFloorWorldY,rig.BootGroundingOffsetMetres(animation));
    const auto modelVector=[&](V p) { return WorldVectorToPlayerModel(out.basis,view(p)); };
    const auto modelPoint=[&](V p) { return WorldVectorToPlayerModel(out.basis,Add(Add(eye,view(p)),Scale(out.root,-1))); };
    for(auto* arm:{&animation.leftIk,&animation.rightIk}) {
        arm->shoulder=modelPoint(arm->shoulder); arm->target=modelPoint(arm->target);
        arm->pole=modelVector(arm->pole); arm->gripX=modelVector(arm->gripX); arm->gripY=modelVector(arm->gripY); arm->gripZ=modelVector(arm->gripZ);
    }
    bool updated=false;
    if(!rig.PreparePose(animation,tick,PlayerCpuSkinCadence::Hz60,updated,diagnostic)) return false;
    const auto worldSocket=[&](const auto& bone) {
        const auto convert=[&](V p) { return PlayerModelVectorToWorld(out.basis,p); };
        const V x=Unit(convert({{bone[0],bone[1],bone[2]}})),rawY=convert({{bone[4],bone[5],bone[6]}});
        V y=Unit(Add(rawY,Scale(x,-Dot(rawY,x)))),z=Unit(Cross(x,y));
        if(Dot(z,convert({{bone[8],bone[9],bone[10]}}))<0) { y=Scale(y,-1);z=Scale(z,-1); }
        const V p=Add(out.root,convert({{bone[12],bone[13],bone[14]}}));
        HeldItemTransform m=IdentityHeldItemTransform();
        for(std::size_t i=0;i<3;++i) { m[i]=x[i];m[4+i]=y[i];m[8+i]=z[i];m[12+i]=p[i]; }
        return m;
    };
    const auto& sockets=rig.BoneSockets(); out.grip=worldSocket(sockets.leftGrip);
    return rig.ResolveHeldItemVisuals(items,out.grip,worldSocket(sockets.rightGrip),out.rendered,diagnostic) &&
        ComposeHeldLightState(out.rendered[0].worldFromItem,OriginalTorchFlameSocketTransform(),OriginalTorchLightSocketTransform(),1,out.light,diagnostic);
}
}
int main(int argc,char** argv) {
    if(argc<2) { std::cerr<<"Pass repository root [--original-sweep] [--capture-obj path]\n";return 2; }
    bool originalSweep=false;
    std::filesystem::path captureObj;
    for(int argument=2;argument<argc;++argument) {
        const std::string flag=argv[argument];
        if(flag=="--original-sweep" && !originalSweep) originalSweep=true;
        else if(flag=="--capture-obj" && captureObj.empty() && argument+1<argc) captureObj=argv[++argument];
        else { std::cerr<<"Invalid diagnostic argument: "<<flag<<'\n';return 2; }
    }
    const std::filesystem::path root=argv[1]; std::string diagnostic;
    PlayerRenderSlot rig;
    horde::scene::SkinnedMeshAsset viewmodel;
    horde::scene::assets::AssetManifest manifest;
    horde::scene::assets::StaticMeshAsset torch;
    if(!rig.LoadAsset((root/"assets/models/player/runtime/gothic-traveller-lod0.runtime.glb").string(),diagnostic) ||
        !viewmodel.LoadClips((root/"assets/models/player/viewmodel/runtime/gothic-traveller-viewmodel.runtime.glb").string(),horde::scene::PlayerLocomotionClipSet(),diagnostic) ||
        !horde::scene::assets::AssetManifest::Load(root/"assets/models/props/runtime/asset.manifest.json",manifest,diagnostic) ||
        !horde::scene::assets::StaticMeshAsset::Load(root/"assets/models/props/runtime/gothic-hand-torch-lod0.runtime.glb",manifest,torch,diagnostic)) { std::cerr<<diagnostic<<'\n';return 2; }
    unsigned cases=0,failures=0; float worstHeadroom=100,maxGripError=0;
    const auto inspect=[&](HeldItemFixedStepInput input,bool capture) {
        Solved solved;
        if(!Solve(rig,input,++cases,solved,diagnostic)) { ++failures;std::cerr<<"pose case="<<cases<<" xz="<<input.playerX<<','<<input.playerZ<<" yaw/pitch="<<input.playerYawRadians<<','<<input.playerPitchRadians<<" walk="<<input.walkTime<<": "<<diagnostic<<'\n';return; }
        float room=100;
        for(const auto& vertex:torch.vertices) room=std::min(room,Headroom(Point(solved.rendered[0].worldFromItem,{{vertex.position[0],vertex.position[1],vertex.position[2]}})));
        // Main visible flame plus admitted .06m tip margin and lateral domain.
        for(float x:{-.105f,.105f}) for(float z:{-.105f,.105f}) for(float y:{0.0f,.4f})
            room=std::min(room,Headroom(Point(solved.light.worldFromFlame,{{x,y,z}})));
        room=std::min(room,Headroom(Point(solved.light.worldFromLight,{})));
        worstHeadroom=std::min(worstHeadroom,room);maxGripError=std::max(maxGripError,rig.LeftGripAgreement().positionErrorMetres);
        if(room < kHeldTorchOverheadGap-1e-5f) { ++failures;std::cerr<<"clearance case="<<cases<<" xyz="<<input.playerX<<','<<input.playerZ<<" yaw="<<input.playerYawRadians<<" pitch="<<input.playerPitchRadians<<" walk="<<input.walkTime<<" final headroom="<<room<<" gripError="<<rig.LeftGripAgreement().positionErrorMetres<<'\n'; }
        if(capture) {
            std::cout<<"worst-bend targetGripY="<<solved.target.worldFromLeftHand[13]<<" finalGripY="<<solved.grip[13]<<" flameY="<<solved.light.worldFromFlame[13]<<" lightY="<<solved.light.worldFromLight[13]<<" finalMainEnvelopeHeadroom="<<room<<" root="<<solved.root[0]<<','<<solved.root[1]<<','<<solved.root[2]<<'\n';
            // Local RT captures are diagnostic evidence, never a CI dependency.
            if(captureObj.empty()) return;
            std::ifstream obj(captureObj);
            std::string line;std::getline(obj,line);std::getline(obj,line);
            if(!obj || line.find("model_to_world_row_major_3x4")==std::string::npos) { ++failures;return; }
            std::istringstream header(line.substr(line.find("3x4")+3));std::array<float,12> recorded{};
            for(auto& value:recorded) header>>value;
            const auto expected=std::array<float,12>{{solved.basis.modelXInWorld[0],0,solved.basis.modelZInWorld[0],solved.root[0],0,1,0,solved.root[1],solved.basis.modelXInWorld[2],0,solved.basis.modelZInWorld[2],solved.root[2]}};
            float error=0;for(std::size_t i=0;i<12;++i) error=std::max(error,std::abs(recorded[i]-expected[i]));
            std::cout<<"captured OBJ final model/world transform maxError="<<error<<'\n';if(error>1e-5f) ++failures;
            std::vector<horde::scene::TexturedSkinnedRtVertex> vertices;
            std::vector<horde::scene::SkinnedPbrTangent> tangents;
            if(!viewmodel.SkinPlayerPoseUniqueTextured(rig.SolvedPose(),vertices,tangents,diagnostic)) { ++failures;std::cerr<<diagnostic<<'\n';return; }
            std::size_t index=0;float vertexError=0;
            while(std::getline(obj,line)) if(line.starts_with("v ")) {
                V point{};std::istringstream row(line.substr(2));row>>point[0]>>point[1]>>point[2];
                if(index>=vertices.size()) { ++failures;break; }
                for(std::size_t axis=0;axis<3;++axis) vertexError=std::max(vertexError,std::abs(point[axis]-vertices[index].position[axis]));
                ++index;
            }
            std::cout<<"captured actual viewmodel vertices="<<index<<" maxError="<<vertexError<<'\n';
            if(index!=vertices.size() || vertexError>1e-5f) ++failures;
        }
    };
    HeldItemFixedStepInput capture;capture.playerMountProfile=PlayerMountProfile::AnatomicalBody;capture.playerX=4.2f;capture.playerZ=-10;capture.playerPitchRadians=-.04f;inspect(capture,true);
    for(int portal=0;portal<3;++portal) for(int step=0;step<=12;++step) for(float pitch:{-.32f,0.0f,.28f}) for(int pose=0;pose<3;++pose) for(float yaw:{0.0f,1.5707963f,-1.5707963f,3.14159265f}) {
        if(originalSweep && yaw!=0) continue;
        HeldItemFixedStepInput input;input.playerMountProfile=PlayerMountProfile::AnatomicalBody;
        input.playerX=portal==2 ? -28.5f-.1f*step : 0;
        input.playerZ=portal==2 ? -15.2f : (portal==0 ? -2.5f : -5.6f)-.1f*step;
        input.playerYawRadians=originalSweep && portal==2 ? -1.5707963f : yaw;input.playerPitchRadians=pitch;
        input.walkTime=originalSweep ? step/60.0f : step*.20f;input.walkAmount=1;
        input.playerCombat.action=pose==0 ? PlayerCombatAction::Idle : pose==1 ? PlayerCombatAction::SwingActive : PlayerCombatAction::ParryActive;
        input.playerCombat.actionTime=.04f;inspect(input,false);
    }
    if(!originalSweep) for(const auto action:{PlayerCombatAction::SwingWindup,PlayerCombatAction::SwingRecovery,
        PlayerCombatAction::UpwardSliceWindup,PlayerCombatAction::UpwardSliceActive,PlayerCombatAction::UpwardSliceRecovery,
        PlayerCombatAction::ParryStartup,PlayerCombatAction::ParryRecovery}) for(float pitch:{-.32f,.28f}) for(int step=0;step<5;++step) {
        HeldItemFixedStepInput input=capture;input.playerX=0;input.playerZ=-3.3f;input.playerPitchRadians=pitch;
        input.walkAmount=step*.25f;input.walkTime=step*.4f;input.playerCombat.action=action;input.playerCombat.actionTime=step*.03f;
        inspect(input,false);
    }
    std::cout<<"actual final rig cases="<<cases<<" worstHeadroom="<<worstHeadroom<<" maxGripError="<<maxGripError<<" failures="<<failures<<'\n';
    return failures ? 1 : 0;
}
