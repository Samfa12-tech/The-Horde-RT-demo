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
float UndersideY(const horde::scene::OverheadVolume& volume,V point) {
    return volume.bottomY+volume.bottomGradientXZ[0]*(point[0]-volume.bottomAnchorXZ[0])+
        volume.bottomGradientXZ[1]*(point[2]-volume.bottomAnchorXZ[1]);
}
float SharedRoofY(V point) {
    float roof=100;
    const auto include=[&](const auto& volumes) { for(const auto& volume:volumes) if(Inside(volume,point)) roof=std::min(roof,UndersideY(volume,point)); };
    include(horde::scene::kShowcaseLowOverheadVolumes); include(horde::scene::kShowcaseCeilingPatches);
    include(horde::scene::kShowcaseSkylightGrid);
    include(horde::scene::kShowcaseImportedOverheadVolumes);
    if(Inside(horde::scene::kShowcaseCollapseRoofSeam,point)) roof=std::min(roof,horde::scene::kShowcaseCollapseRoofSeam.bottomY);
    return roof;
}
struct OverheadTriangle {
    V a,b,c;
    float minX,maxX,minZ,maxZ;
};
// An independent vertical triangle intersection verifies the actual admitted
// masonry, including fractured shoulders that a smooth roof plane could miss.
bool TriangleY(const OverheadTriangle& triangle,V point,float& height) {
    if(point[0]<triangle.minX-1e-6f || point[0]>triangle.maxX+1e-6f ||
       point[2]<triangle.minZ-1e-6f || point[2]>triangle.maxZ+1e-6f) return false;
    const V ab=Add(triangle.b,Scale(triangle.a,-1)),ac=Add(triangle.c,Scale(triangle.a,-1));
    const float determinant=ab[0]*ac[2]-ab[2]*ac[0];
    const float dx=point[0]-triangle.a[0],dz=point[2]-triangle.a[2];
    const float u=(dx*ac[2]-dz*ac[0])/determinant;
    const float v=(ab[0]*dz-ab[2]*dx)/determinant;
    if(u< -1e-5f || v< -1e-5f || u+v>1.00001f) return false;
    height=triangle.a[1]+u*ab[1]+v*ac[1];return true;
}
std::vector<OverheadTriangle> ImportedStructuralUndersides(const horde::scene::assets::StaticMeshAsset& asset) {
    std::vector<OverheadTriangle> triangles;
    for(const auto& primitive:asset.primitives) {
        if(asset.materials[primitive.materialIndex].name!="MedievalWall02") continue;
        for(std::uint32_t index=0;index<primitive.indexCount;index+=3) {
            std::array<V,3> points{};
            for(std::size_t corner=0;corner<3;++corner) {
                const auto& position=asset.vertices[primitive.vertexOffset+asset.indices[primitive.indexOffset+index+corner]].position;
                points[corner]={{position[0],position[1],position[2]}};
            }
            const V normal=Cross(Add(points[1],Scale(points[0],-1)),Add(points[2],Scale(points[0],-1)));
            // Reject vertical walls and ground-facing bases below the eye. Keep
            // real overhead fracture faces as well as both rising roof wedges.
            if(normal[1]>=-1e-7f || std::min({points[0][1],points[1][1],points[2][1]})<kShowcaseEyeWorldY) continue;
            triangles.push_back({points[0],points[1],points[2],
                std::min({points[0][0],points[1][0],points[2][0]}),std::max({points[0][0],points[1][0],points[2][0]}),
                std::min({points[0][2],points[1][2],points[2][2]}),std::max({points[0][2],points[1][2],points[2][2]})});
        }
    }
    return triangles;
}
float Headroom(V point,const std::vector<OverheadTriangle>& triangles) {
    float roof=SharedRoofY(point);
    for(const auto& triangle:triangles) { float y=0;if(TriangleY(triangle,point,y)) roof=std::min(roof,y); }
    return roof-point[1];
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
    if(argc<2) { std::cerr<<"Pass repository root [--original-sweep | --roof-witness] [--capture-obj path]\n";return 2; }
    bool originalSweep=false,roofWitness=false;
    std::filesystem::path captureObj;
    for(int argument=2;argument<argc;++argument) {
        const std::string flag=argv[argument];
        if(flag=="--original-sweep" && !originalSweep) originalSweep=true;
        else if(flag=="--roof-witness" && !roofWitness) roofWitness=true;
        else if(flag=="--capture-obj" && captureObj.empty() && argument+1<argc) captureObj=argv[++argument];
        else { std::cerr<<"Invalid diagnostic argument: "<<flag<<'\n';return 2; }
    }
    if(roofWitness && (originalSweep || !captureObj.empty())) { std::cerr<<"Roof witness is a separate CPU diagnostic\n";return 2; }
    const std::filesystem::path root=argv[1]; std::string diagnostic;
    PlayerRenderSlot rig;
    horde::scene::SkinnedMeshAsset viewmodel;
    horde::scene::assets::AssetManifest manifest;
    horde::scene::assets::StaticMeshAsset torch;
    horde::scene::assets::AssetManifest collapseManifest;
    horde::scene::assets::StaticMeshAsset collapse;
    if(!rig.LoadAsset((root/"assets/models/player/runtime/gothic-traveller-lod0.runtime.glb").string(),diagnostic) ||
        !viewmodel.LoadClips((root/"assets/models/player/viewmodel/runtime/gothic-traveller-viewmodel.runtime.glb").string(),horde::scene::PlayerLocomotionClipSet(),diagnostic) ||
        !horde::scene::assets::AssetManifest::Load(root/"assets/models/props/runtime/asset.manifest.json",manifest,diagnostic) ||
        !horde::scene::assets::StaticMeshAsset::Load(root/"assets/models/props/runtime/gothic-hand-torch-lod0.runtime.glb",manifest,torch,diagnostic) ||
        !horde::scene::assets::AssetManifest::Load(root/"assets/models/world/runtime/collapsed-entry/asset.manifest.json",collapseManifest,diagnostic) ||
        !horde::scene::assets::StaticMeshAsset::Load(root/"assets/models/world/runtime/collapsed-entry/collapsed-entry-lod0.runtime.glb",collapseManifest,collapse,diagnostic)) { std::cerr<<diagnostic<<'\n';return 2; }
    const auto structuralUndersides=ImportedStructuralUndersides(collapse);
    if(structuralUndersides.empty()) { std::cerr<<"No imported structural underside triangles were inspected\n";return 2; }
    unsigned cases=0,failures=0,roofCases=0,skinnedRoofCases=0,planeSamples=0,diskSamples=0;
    float worstHeadroom=100,maxGripError=0,worstRoofHeadroom=100;
    HeldItemFixedStepInput worstRoofInput{};
    V worstRoofPoint{};
    float worstTargetGripY=0,worstFinalGripY=0;
    // Match authored clearance to the admitted near-route GLB, including the
    // arch just beyond the retained gameplay cap that the held flame can reach.
    // Sample real triangle vertices/edges/interiors, not fixture literals.
    for(const auto& triangle:structuralUndersides) for(int u=0;u<=6;++u) for(int v=0;v<=6-u;++v) {
        const V point=Add(Add(Scale(triangle.a,1-(u+v)/6.0f),Scale(triangle.b,u/6.0f)),Scale(triangle.c,v/6.0f));
        if(point[2]<2.92f-1e-5f || point[2]>3.68f+1e-5f) continue;
        ++planeSamples;
        if(SharedRoofY(point)>point[1]+1e-5f) {
            if(failures<12) std::cerr<<"Imported underside missing from clearance at "<<point[0]<<','<<point[1]<<','<<point[2]<<" sharedY="<<SharedRoofY(point)<<'\n';
            ++failures;
        }
    }
    // Check the conservative disk bound against actual plane points at multiple
    // radii and clipped boundaries. This includes oblique shoulder gradients.
    for(const auto& volume:horde::scene::kShowcaseImportedOverheadVolumes) {
        V centre{};for(const auto& corner:volume.footprint) {centre[0]+=corner[0]*.25f;centre[2]+=corner[1]*.25f;}
        for(const auto& corner:volume.footprint) for(float radius:{.0f,.05f,kHeldTorchEnvelopeRadius,.40f}) {
            for(float fraction:{0.0f,.5f,1.0f,1.1f}) {
                const V origin{{centre[0]+fraction*(corner[0]-centre[0]),0,centre[2]+fraction*(corner[1]-centre[2])}};
                const float bound=horde::scene::MinimumOverheadBottomY(volume,origin[0],origin[2],radius);
                for(int angle=0;angle<64;++angle) for(float ring:{0.0f,.5f,1.0f}) {
                    const float radians=angle*6.283185307f/64;
                    const V sample{{origin[0]+std::cos(radians)*radius*ring,0,origin[2]+std::sin(radians)*radius*ring}};
                    if(!Inside(volume,sample)) continue;
                    ++diskSamples;
                    if(bound>UndersideY(volume,sample)+1e-5f) { ++failures;std::cerr<<"Disk bound misses underside plane\n"; }
                }
            }
        }
    }
    if(planeSamples==0 || diskSamples==0) { std::cerr<<"Empty imported plane/disk validation\n";return 2; }
    std::vector<horde::scene::TexturedSkinnedRtVertex> roofVertices;
    std::vector<horde::scene::SkinnedPbrTangent> roofTangents;
    const auto inspect=[&](HeldItemFixedStepInput input,bool capture,bool nearRoof=false) {
        Solved solved;
        if(!Solve(rig,input,++cases,solved,diagnostic)) { ++failures;std::cerr<<"pose case="<<cases<<" xz="<<input.playerX<<','<<input.playerZ<<" yaw/pitch="<<input.playerYawRadians<<','<<input.playerPitchRadians<<" walk="<<input.walkTime<<": "<<diagnostic<<'\n';return; }
        float room=100;
        V limitingPoint{};
        const auto measure=[&](V point) { const float candidate=Headroom(point,structuralUndersides);if(candidate<room) {room=candidate;limitingPoint=point;} };
        for(const auto& vertex:torch.vertices) measure(Point(solved.rendered[0].worldFromItem,{{vertex.position[0],vertex.position[1],vertex.position[2]}}));
        // Main visible flame plus admitted .06m tip margin and lateral domain.
        for(float x:{-.105f,.105f}) for(float z:{-.105f,.105f}) for(float y:{0.0f,.4f})
            measure(Point(solved.light.worldFromFlame,{{x,y,z}}));
        measure(Point(solved.light.worldFromLight,{}));
        worstHeadroom=std::min(worstHeadroom,room);maxGripError=std::max(maxGripError,rig.LeftGripAgreement().positionErrorMetres);
        if(room < kHeldTorchOverheadGap-1e-5f) { if(failures<24) std::cerr<<"clearance case="<<cases<<" xyz="<<input.playerX<<','<<input.playerZ<<" yaw="<<input.playerYawRadians<<" pitch="<<input.playerPitchRadians<<" walk="<<input.walkTime<<" final headroom="<<room<<" gripError="<<rig.LeftGripAgreement().positionErrorMetres<<'\n';++failures; }
        if(nearRoof) {
            ++roofCases;
            if(room<worstRoofHeadroom) { worstRoofHeadroom=room;worstRoofInput=input;worstRoofPoint=limitingPoint;worstTargetGripY=solved.target.worldFromLeftHand[13];worstFinalGripY=solved.grip[13]; }
            if(!viewmodel.SkinPlayerPoseUniqueTextured(rig.SolvedPose(),roofVertices,roofTangents,diagnostic) ||
               roofVertices.size()!=15855 || roofTangents.size()!=roofVertices.size()) { ++failures;std::cerr<<"Actual final viewmodel skin: "<<diagnostic<<'\n';return; }
            ++skinnedRoofCases;
            // The same solved pose supplies both actual skins and final item
            // sockets. Retain finite world positions and the 15mm grip guard.
            for(const auto& vertex:roofVertices) {
                const V world=Add(solved.root,PlayerModelVectorToWorld(solved.basis,{{vertex.position[0],vertex.position[1],vertex.position[2]}}));
                if(!std::all_of(world.begin(),world.end(),[](float value){return std::isfinite(value);})) { ++failures;break; }
            }
            if(rig.UniqueVertices().empty() || rig.LeftGripAgreement().positionErrorMetres>kPlayerGripSocketToleranceMetres) ++failures;
            if(roofWitness) {
                std::cout<<"roof witness playerXZ="<<input.playerX<<','<<input.playerZ<<" yaw/pitch="<<input.playerYawRadians<<','<<input.playerPitchRadians
                    <<" targetGrip="<<solved.target.worldFromLeftHand[12]<<','<<solved.target.worldFromLeftHand[13]<<','<<solved.target.worldFromLeftHand[14]
                    <<" finalGrip="<<solved.grip[12]<<','<<solved.grip[13]<<','<<solved.grip[14]<<" lowering="<<solved.target.kinematics.torchOverheadLowering
                    <<" limitingPoint="<<limitingPoint[0]<<','<<limitingPoint[1]<<','<<limitingPoint[2]<<" headroom="<<room<<'\n';
            }
        }
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
    const auto printLimitingTriangle=[&](V point) {
        const OverheadTriangle* limiting=nullptr;float roof=SharedRoofY(point);
        for(const auto& triangle:structuralUndersides) {float y=0;if(TriangleY(triangle,point,y) && y<roof) {roof=y;limiting=&triangle;} }
        std::cout<<"limiting worldPoint="<<point[0]<<','<<point[1]<<','<<point[2]<<" roofY="<<roof<<" sharedY="<<SharedRoofY(point)<<'\n';
        if(limiting) for(const auto vertex:{limiting->a,limiting->b,limiting->c}) std::cout<<"limiting actual triangle vertex="<<vertex[0]<<','<<vertex[1]<<','<<vertex[2]<<'\n';
    };
    if(roofWitness) {
        HeldItemFixedStepInput input;input.playerMountProfile=PlayerMountProfile::AnatomicalBody;
        input.playerX=-1.56f;input.playerZ=3.05f;input.playerYawRadians=3.14159265f;input.playerPitchRadians=0;input.walkTime=2.37f;input.walkAmount=1;
        input.playerCombat.action=PlayerCombatAction::SwingActive;input.playerCombat.actionTime=.04f;
        inspect(input,false,true);printLimitingTriangle(worstRoofPoint);
        input.playerZ=2.83f;input.playerPitchRadians=-.32f;input.walkTime=.80f;input.walkAmount=.5f;input.playerCombat.action=PlayerCombatAction::Idle;
        inspect(input,false,true);
        std::cout<<"CPU roof witness subset only: poses="<<cases<<" actualViewmodelSkins="<<skinnedRoofCases<<" failures="<<failures<<'\n';
        return failures ? 1 : 0;
    }
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
    const unsigned legacyCases=cases;
    if(!originalSweep) {
        if(legacyCases!=1475) { ++failures;std::cerr<<"Legacy1,475-pose sweep changed\n"; }
        constexpr std::array<float,12> routeZ{{1.95f,2.17f,2.39f,2.61f,2.83f,2.91f,2.92f,2.93f,2.94f,2.95f,3.05f,3.16f}};
        for(std::size_t step=0;step<routeZ.size();++step) for(float x:{-1.56f,0.0f,1.56f})
            for(float pitch:{-.32f,0.0f,.28f}) for(int pose=0;pose<3;++pose)
                for(float yaw:{0.0f,1.5707963f,-1.5707963f,3.14159265f}) {
            HeldItemFixedStepInput input;input.playerMountProfile=PlayerMountProfile::AnatomicalBody;
            input.playerX=x;input.playerZ=routeZ[step];input.playerYawRadians=yaw;input.playerPitchRadians=pitch;
            input.walkTime=static_cast<float>(step)*.20f+pose*.37f;
            input.walkAmount=static_cast<float>((step+pose)%3)*.5f;
            input.playerCombat.action=pose==0 ? PlayerCombatAction::Idle : pose==1 ? PlayerCombatAction::SwingActive : PlayerCombatAction::ParryActive;
            input.playerCombat.actionTime=.04f;inspect(input,false,true);
        }
        // Oblique orientations and every transient sword/parry phase at the
        // roof join and last legal near-cap stance probe the final solved grip.
        unsigned phase=0;
        for(const auto action:{PlayerCombatAction::SwingWindup,PlayerCombatAction::SwingRecovery,
            PlayerCombatAction::UpwardSliceWindup,PlayerCombatAction::UpwardSliceActive,PlayerCombatAction::UpwardSliceRecovery,
            PlayerCombatAction::ParryStartup,PlayerCombatAction::ParryRecovery}) {
            for(float z:{2.93f,3.16f}) for(float x:{-1.56f,0.0f,1.56f}) for(float pitch:{-.32f,.28f})
                for(float yaw:{.78539816f,-.78539816f,2.35619449f,-2.35619449f}) {
                HeldItemFixedStepInput input;input.playerMountProfile=PlayerMountProfile::AnatomicalBody;
                input.playerX=x;input.playerZ=z;input.playerYawRadians=yaw;input.playerPitchRadians=pitch;
                input.walkTime=phase*.31f;input.walkAmount=static_cast<float>(phase%3)*.5f;
                input.playerCombat.action=action;input.playerCombat.actionTime=.03f;inspect(input,false,true);
            }
            ++phase;
        }
        if(roofCases!=1632 || skinnedRoofCases!=roofCases) { ++failures;std::cerr<<"Incomplete new real-rig/viewmodel sweep\n"; }
        // Changed entry ceiling: inspect the actual final skin/grip, torch,
        // flame and light below its interior and all four irregular joins.
        const unsigned beforeEntry = cases, beforeEntrySkins = skinnedRoofCases;
        const std::array<std::array<float,2>,5> entryPositions{{
            {{0.0f,-4.3f}},{{-0.58f,-4.3f}},{{0.47f,-4.3f}},
            {{0.0f,-3.5f}},{{0.0f,-5.15f}}}};
        for(const auto& position:entryPositions) for(float pitch:{-.32f,0.0f,.28f})
            for(float yaw:{0.0f,1.5707963f,-1.5707963f,3.14159265f})
                for(int pose=0;pose<3;++pose) {
            HeldItemFixedStepInput input;input.playerMountProfile=PlayerMountProfile::AnatomicalBody;
            input.playerX=position[0];input.playerZ=position[1];input.playerPitchRadians=pitch;
            input.playerYawRadians=yaw;input.walkTime=static_cast<float>(cases-beforeEntry)*.19f;
            input.walkAmount=pose*.5f;input.playerCombat.action=pose==0 ? PlayerCombatAction::Idle :
                pose==1 ? PlayerCombatAction::SwingActive : PlayerCombatAction::ParryActive;
            input.playerCombat.actionTime=.04f;inspect(input,false,true);
        }
        if(cases-beforeEntry!=180u || skinnedRoofCases-beforeEntrySkins!=180u) {
            ++failures;std::cerr<<"Incomplete closed-entry real-rig sweep\n";
        }
    }
    std::cout<<"imported structural underside triangles="<<structuralUndersides.size()<<" planeSamples="<<planeSamples<<" diskSamples="<<diskSamples<<'\n';
    std::cout<<"new roof final rig cases="<<roofCases<<" actualViewmodelVerticesPerPose=15855 skinnedRoofCases="<<skinnedRoofCases<<" worstHeadroom="<<worstRoofHeadroom
        <<" worstPoseXZ="<<worstRoofInput.playerX<<','<<worstRoofInput.playerZ<<" yaw/pitch="<<worstRoofInput.playerYawRadians<<','<<worstRoofInput.playerPitchRadians
        <<" walk="<<worstRoofInput.walkTime<<" action="<<static_cast<int>(worstRoofInput.playerCombat.action)<<" target/finalGripY="<<worstTargetGripY<<','<<worstFinalGripY<<'\n';
    if(roofCases) printLimitingTriangle(worstRoofPoint);
    std::cout<<"actual final rig cases="<<cases<<" worstHeadroom="<<worstHeadroom<<" maxGripError="<<maxGripError<<" failures="<<failures<<'\n';
    return failures ? 1 : 0;
}
