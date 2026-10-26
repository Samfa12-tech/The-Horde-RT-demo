#include "scene/assets/SkinnedMeshAsset.h"
#include "scene/assets/StaticMeshAsset.h"
#include "scene/assets/GltfDocument.h"
#include "vulkan/raytracing/PlayerRenderSlot.h"
#include "cgltf/cgltf.h"

#include <algorithm>
#include <cmath>
#include <cstring>
#include <filesystem>
#include <iostream>
#include <limits>

namespace {
using namespace horde::scene;
using namespace horde::vulkan::raytracing;
using namespace horde::gameplay::animation;
using namespace horde::gameplay::items;
int failures=0;
void Check(bool value,const char* message) { if(!value){++failures;std::cerr<<"FAIL: "<<message<<'\n';} }
HeldItemTransform Rigid(SkinnedNodeTransform node) {
    for(unsigned col=0;col<3;++col) {
        const auto start=4*col;
        const float length=std::hypot(node[start],node[start+1],node[start+2]);
        for(unsigned row=0;row<3;++row)node[start+row]/=length;
    }
    return node;
}
void SetTarget(PlayerArmIkTarget& target,const HeldItemTransform& grip) {
    target.target={grip[12],grip[13],grip[14]};
    target.gripX={grip[0],grip[1],grip[2]};target.gripY={grip[4],grip[5],grip[6]};
    target.gripZ={grip[8],grip[9],grip[10]};target.pole={1,0,1};
}
template<class T> bool Exact(const std::vector<T>& a,const std::vector<T>& b) {
    return a.size()==b.size() && std::memcmp(a.data(),b.data(),a.size()*sizeof(T))==0;
}
}

int main(int argc,char** argv) {
    if(argc!=2)return 2;
    const std::filesystem::path root(argv[1]);
    const auto original=root/"assets/models/player/runtime/gothic-traveller-lod0.runtime.glb";
    const auto candidate=root/"assets/models/player/source/flask-native-binding-r1/gothic-traveller-flask-candidate.runtime.glb";
    PlayerRenderSlot baseline,unbound,bound;
    SkinnedMeshAsset reference,view;
    const SkinnedClipSet bindings{{{{"Idle",true,true},{"Walking",true,true},
                                   {"FlaskUse",false,true},{"FlaskCancel",false,true}}}};
    std::string error;
    if(!baseline.LoadAsset(original.string(),error)||!unbound.LoadAsset(candidate.string(),error)||
       !bound.LoadUtilityCandidateAsset(candidate.string(),error)||
       !reference.LoadClips(candidate.string(),bindings,error)||
       !view.LoadClips((root/"assets/models/player/viewmodel/runtime/gothic-traveller-viewmodel.runtime.glb").string(),
                       PlayerLocomotionClipSet(),error)) {std::cerr<<error<<'\n';return 1;}
    std::array<SkinnedNodeTransform,2> idle{};
    const std::array<std::string_view,2> names{"LeftGrip","RightGrip"};
    if(!reference.NodeTransforms(SkinnedClip::Idle,0,names,idle,error))return 1;
    const auto flaskDirectory=root/"assets/models/props/runtime/healing-flask-v1";
    horde::scene::assets::AssetManifest manifest;
    horde::scene::assets::StaticMeshAsset flask;
    if(!horde::scene::assets::AssetManifest::Load(flaskDirectory/"asset.manifest.json",manifest,error)||
       !horde::scene::assets::StaticMeshAsset::Load(flaskDirectory/"healing-flask-v1.runtime.glb",manifest,flask,error)) {
        std::cerr<<error<<'\n';return 1;
    }
    const auto socket=std::find_if(flask.sockets.begin(),flask.sockets.end(),[](const auto& s){return s.name=="Grip";});
    if(socket==flask.sockets.end())return 1;
    const auto itemFromGrip=socket->world;
    PlayerAnimationSnapshot animation;
    SetTarget(animation.leftIk,Rigid(idle[0]));SetTarget(animation.rightIk,Rigid(idle[1]));
    // A real nonzero left solve must survive the authored right-arm request.
    animation.leftIk.target[2]+=.04f;animation.leftIk.target[1]-=.025f;
    bool updated=false;
    if(!baseline.PreparePose(animation,100,PlayerCpuSkinCadence::Hz30,updated,error)||
       !unbound.PreparePose(animation,100,PlayerCpuSkinCadence::Hz30,updated,error)||
       !bound.PreparePose(animation,100,PlayerCpuSkinCadence::Hz30,updated,error)) {
        std::cerr<<error<<'\n';return 1;
    }
    Check(Exact(baseline.UniqueVertices(),unbound.UniqueVertices()) &&
          Exact(baseline.UniqueTangents(),unbound.UniqueTangents()) &&
          Exact(baseline.UniqueVertices(),bound.UniqueVertices()),
          "ordinary callers preserve exact original pose/vertices despite appended candidate clips");
    const auto left=baseline.BoneSockets().leftGrip;
    Check(!unbound.PrepareUtilityCandidatePose(animation,{},100,PlayerCpuSkinCadence::Hz30,updated,error) &&
          !updated && !unbound.SolvedPose().IsValid(),
          "normal asset binding rejects utility before same-tick cadence reuse");

    float maximumRightError=0,maximumLeftDifference=0;
    std::size_t samples=0;
    for(const auto kind:{PlayerUtilityClip::Use,PlayerUtilityClip::Cancel}) {
        const auto clip=kind==PlayerUtilityClip::Use?SkinnedClip::Attack:SkinnedClip::Dead;
        for(float fraction:{0.f,.16f,.24f,.44f,.48f,.56f,.70666667f,.8f,1.f}) {
            const float seconds=reference.ClipDuration(clip)*fraction;
            if(!bound.PrepareUtilityCandidatePose(animation,{kind,seconds},101,PlayerCpuSkinCadence::Hz30,updated,error)) {
                std::cerr<<error<<'\n';return 1;
            }
            Check(updated,"changed utility samples invalidate same/odd-tick cadence without a second clock");
            SkinnedNodeTransform expected{};
            if(!reference.NodeTransform(clip,seconds,"RightGrip",expected,error))return 1;
            const auto agreement=MeasureTransformAgreement(Rigid(expected),Rigid(bound.BoneSockets().rightGrip));
            maximumRightError=std::max(maximumRightError,agreement.positionErrorMetres);
            Check(agreement.positionErrorMetres<=kPlayerGripSocketToleranceMetres &&
                  agreement.orientationErrorRadians<=kPlayerGripOrientationToleranceRadians,
                  "native candidate preserves exact authored right hand and palm orientation");
            for(unsigned i=0;i<16;++i)
                maximumLeftDifference=std::max(maximumLeftDifference,std::abs(left[i]-bound.BoneSockets().leftGrip[i]));
            Check(bound.LeftSocketErrorMetres()<=kPlayerGripSocketToleranceMetres,
                  "nonzero left-light IK retains existing grip contract");
            std::vector<TexturedSkinnedRtVertex> vertices;std::vector<SkinnedPbrTangent> tangents;
            Check(view.SkinPlayerPoseUniqueTextured(bound.SolvedPose(),vertices,tangents,error),
                  "unchanged first-person mesh consumes the single solved body palette");
            const auto finalGrip=Rigid(bound.BoneSockets().rightGrip);
            const auto carried=MultiplyHeldItemTransforms(finalGrip,InverseRigidHeldItemTransform(itemFromGrip));
            const auto reconstructed=MultiplyHeldItemTransforms(carried,itemFromGrip);
            Check(MeasureTransformAgreement(finalGrip,reconstructed).positionErrorMetres<.000001f,
                  "hand-driven flask attachment applies its inverse Grip once");
            ++samples;
        }
    }
    Check(maximumLeftDifference<.00001f,"right utility motion cannot move the accepted left-light solve");

    // Read actual skin ownership rather than assuming a screen-space foot mask.
    horde::scene::assets::GltfDocument document;
    if(!document.Load(candidate,document,error))return 1;
    const auto* data=document.Data();
    std::vector<std::size_t> unaffected;
    std::size_t offset=0;
    for(std::size_t p=0;p<data->meshes[0].primitives_count;++p) {
        const auto& primitive=data->meshes[0].primitives[p];
        const auto* joints=cgltf_find_accessor(&primitive,cgltf_attribute_type_joints,0);
        const auto* weights=cgltf_find_accessor(&primitive,cgltf_attribute_type_weights,0);
        if(!joints || !weights || joints->count!=weights->count)return 1;
        for(std::size_t v=0;v<joints->count;++v) {
            cgltf_uint ids[4]{};cgltf_float values[4]{};
            if(!cgltf_accessor_read_uint(joints,v,ids,4)||!cgltf_accessor_read_float(weights,v,values,4))return 1;
            bool right=false;
            for(unsigned w=0;w<4;++w)if(values[w]!=0.f) {
                if(ids[w]>=data->skins[0].joints_count)return 1;
                for(auto* node=data->skins[0].joints[ids[w]];node;node=node->parent)
                    if(node->name && std::string_view(node->name)=="RightArm")right=true;
            }
            if(!right)unaffected.push_back(offset+v);
        }
        offset+=joints->count;
    }
    Check(!unaffected.empty() && offset==bound.UniqueVertices().size(),
          "actual skin weights identify non-right geometry including the walking legs");
    float walkingSensitivity=0.f;
    const auto idleVertices=baseline.UniqueVertices();
    for(float baseTime:{.08f,.35f,.72f}) {
        auto walking=animation;walking.locomotionClip=PlayerLocomotionClip::Walk;
        walking.locomotionBlend=1.f;walking.locomotionTime=baseTime;
        SkinnedNodeTransform baseShoulder{};
        if(!reference.NodeTransform(SkinnedClip::Walking,baseTime,"RightShoulder",baseShoulder,error)||
           !reference.NodeTransform(SkinnedClip::Walking,baseTime,"LeftGrip",idle[0],error))return 1;
        SetTarget(walking.leftIk,Rigid(idle[0]));walking.leftIk.target[2]+=.04f;walking.leftIk.target[1]-=.025f;
        if(!baseline.PreparePose(walking,200,PlayerCpuSkinCadence::Hz60,updated,error))return 1;
        const auto walkingVertices=baseline.UniqueVertices();const auto hips=baseline.BoneSockets().hips;
        const auto walkingLeft=baseline.BoneSockets().leftGrip;
        for(auto i:unaffected)for(unsigned axis=0;axis<3;++axis)
            walkingSensitivity=std::max(walkingSensitivity,std::abs(walkingVertices[i].position[axis]-idleVertices[i].position[axis]));
        for(float time:{.4f,.6f,.8f}) {
            if(!bound.PrepareUtilityCandidatePose(walking,{PlayerUtilityClip::Use,time},200,PlayerCpuSkinCadence::Hz30,updated,error)) {
                std::cerr<<error<<'\n';return 1;
            }
            SkinnedNodeTransform sourceShoulder{},sourceGrip{};
            if(!reference.NodeTransform(SkinnedClip::Attack,time,"RightShoulder",sourceShoulder,error)||
               !reference.NodeTransform(SkinnedClip::Attack,time,"RightGrip",sourceGrip,error))return 1;
            const auto anchor=MultiplyHeldItemTransforms(Rigid(baseShoulder),InverseRigidHeldItemTransform(Rigid(sourceShoulder)));
            const auto expected=MultiplyHeldItemTransforms(anchor,Rigid(sourceGrip));
            const auto measured=MeasureTransformAgreement(expected,Rigid(bound.BoneSockets().rightGrip));
            Check(measured.positionErrorMetres<=kPlayerGripSocketToleranceMetres &&
                  measured.orientationErrorRadians<=kPlayerGripOrientationToleranceRadians,
                  "right hand target rebases at locomotion shoulder without importing the source Idle body");
            Check(hips==bound.BoneSockets().hips && walkingLeft==bound.BoneSockets().leftGrip,
                  "walking hips and independent left-light solve retain exact base state");
            bool exact=true;
            for(auto i:unaffected)exact=exact && std::memcmp(&walkingVertices[i],&bound.UniqueVertices()[i],sizeof(TexturedSkinnedRtVertex))==0;
            Check(exact,"all non-right vertices, including walking legs and feet, retain exact base bytes");
            std::vector<TexturedSkinnedRtVertex> primary;std::vector<SkinnedPbrTangent> primaryTangents;
            Check(view.SkinPlayerPoseUniqueTextured(bound.SolvedPose(),primary,primaryTangents,error),
                  "walking return targets retain the same solved first-person palette");
        }
    }
    Check(walkingSensitivity>.01f,"walking witness actually changes base geometry rather than accepting a frozen Idle body");
    if(!baseline.PreparePose(animation,101,PlayerCpuSkinCadence::Hz60,updated,error))return 1;
    Check(bound.PreparePose(animation,101,PlayerCpuSkinCadence::Hz30,updated,error) && updated &&
          Exact(bound.UniqueVertices(),baseline.UniqueVertices()),
          "leaving the utility at the same tick restores the original pose instead of stale candidate data");
    for(int failure=0;failure<9;++failure) {
        auto invalid=animation;PlayerUtilitySample sample{};
        if(failure==0)sample.seconds=std::numeric_limits<float>::quiet_NaN();
        if(failure==1)sample.seconds=-.01f;
        if(failure==2)sample.seconds=1.26f;
        if(failure==3)sample.clip=static_cast<PlayerUtilityClip>(255);
        if(failure==4)invalid.locomotionClip=static_cast<PlayerLocomotionClip>(255);
        if(failure==5)invalid.combatLayer.guardPoseWeight=1.f;
        if(failure==6)invalid.combatLayer.action=PlayerUpperBodyAction::Sword;
        if(failure==7)invalid.dodge.weight=1.f;
        if(failure==8)invalid.crouchPelvisDropMetres=.1f;
        Check(!bound.PrepareUtilityCandidatePose(invalid,sample,101,PlayerCpuSkinCadence::Hz30,updated,error) &&
              !updated && !bound.SolvedPose().IsValid(),
              "invalid/conflicting utility requests invalidate retained pose before cadence reuse");
    }
    Check(bound.LoadAsset(original.string(),error) &&
          !bound.PrepareUtilityCandidatePose(animation,{},101,PlayerCpuSkinCadence::Hz30,updated,error),
          "reloading normal asset drops diagnostic bindings and retained sample");
    std::cout<<"utility_native_samples="<<samples<<" right_max_m="<<maximumRightError
             <<" left_max_component="<<maximumLeftDifference<<" failures="<<failures
             <<"; CPU-only, no arbitrary cancellation or RT joins admission\n";
    return failures?1:0;
}
