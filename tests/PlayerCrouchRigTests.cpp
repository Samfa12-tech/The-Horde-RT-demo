#include "scene/assets/SkinnedMeshAsset.h"
#include "vulkan/raytracing/PlayerRenderSlot.h"

#include <algorithm>
#include <cmath>
#include <filesystem>
#include <iostream>
#include <limits>

namespace
{
using namespace horde::scene;
using namespace horde::vulkan::raytracing;
using namespace horde::gameplay::animation;
int failures = 0;
void Check(bool value, const char* message)
{
    if (!value) { ++failures; std::cerr << "FAIL: " << message << '\n'; }
}
bool Near(float a, float b) { return std::abs(a-b) < .0001f; }
float Distance(const SkinnedNodeTransform& a, const SkinnedNodeTransform& b)
{
    return std::hypot(std::hypot(a[12]-b[12], a[13]-b[13]), a[14]-b[14]);
}
}

int main(int argc, char** argv)
{
    if (argc != 2) return 2;
    const std::filesystem::path root(argv[1]);
    const auto path=root/"assets/models/player/runtime/gothic-traveller-lod0.runtime.glb";
    SkinnedMeshAsset world, view;
    PlayerRenderSlot slot;
    std::string diagnostic;
    if (!world.LoadClips(path.string(),PlayerLocomotionClipSet(),diagnostic) ||
        !view.LoadClips((root/"assets/models/player/viewmodel/runtime/gothic-traveller-viewmodel.runtime.glb").string(),
                        PlayerLocomotionClipSet(),diagnostic) || !slot.LoadAsset(path.string(),diagnostic))
    { std::cerr<<diagnostic<<'\n'; return 1; }
    SkinnedArmIkTarget arm;
    arm.poseWeight=0;
    std::size_t samples=0;
    for (auto clip : {SkinnedClip::Idle,SkinnedClip::Walking})
    for (int phase=0;phase<24;++phase)
    {
        const float time=world.ClipDuration(clip)*phase/24.0f;
        SkinnedPlayerPose standing;
        if (!world.EvaluatePlayerPose(clip,time,arm,arm,standing,diagnostic))
        { std::cerr<<diagnostic<<'\n';return 1; }
        std::array<SkinnedNodeTransform,3> originalLeft{},originalRight{};
        const std::array<std::string_view,3> leftNames{"LeftUpLeg","LeftLeg","LeftFoot"};
        const std::array<std::string_view,3> rightNames{"RightUpLeg","RightLeg","RightFoot"};
        if (!world.NodeTransforms(clip,time,leftNames,originalLeft,diagnostic) ||
            !world.NodeTransforms(clip,time,rightNames,originalRight,diagnostic)) return 1;
        for (float drop : {.001f,.05f,.15f,.3f,.45f,kMaximumPlayerCrouchPelvisDropMetres})
        {
            SkinnedPlayerPose crouched, independent;
            if (!world.EvaluatePlayerPose(clip,time,arm,arm,crouched,diagnostic,0,drop) ||
                !view.EvaluatePlayerPose(clip,time,arm,arm,independent,diagnostic,0,drop))
            { std::cerr<<"clip="<<int(clip)<<" phase="<<phase<<" drop="<<drop<<' '<<diagnostic<<'\n';return 1; }
            const auto& sockets=crouched.Sockets();
            Check(Near(sockets.hips[13],standing.Sockets().hips[13]-drop) &&
                  Near(sockets.hips[12],standing.Sockets().hips[12]) &&
                  Near(sockets.hips[14],standing.Sockets().hips[14]),"crouch must lower hips vertically, without root sinking/setback");
            Check(Near(sockets.leftHand[13],standing.Sockets().leftHand[13]-drop) &&
                  Near(sockets.rightHand[13],standing.Sockets().rightHand[13]-drop),
                  "released hands must lower with torso in the shared lowered-eye frame");
            for (int side=0;side<2;++side)
            {
                const auto& original=side?originalRight:originalLeft;
                const auto& solved=side?sockets.rightLeg:sockets.leftLeg;
                Check(solved[2]==original[2],"animated ankle transform must remain exact in Idle and Walking");
                Check(Near(Distance(solved[0],solved[1]),Distance(original[0],original[1])) &&
                      Near(Distance(solved[1],solved[2]),Distance(original[1],original[2])),
                      "crouch IK must preserve both leg lengths");
                if(drop==.001f) Check(Distance(solved[1],original[1])<.02f,
                                     "crouch onset must retain animated knee continuity");
            }
            std::vector<TexturedSkinnedRtVertex> body,primary,reference;
            std::vector<SkinnedPbrTangent> bodyT,primaryT,referenceT;
            if (!world.SkinPlayerPoseUniqueTextured(crouched,body,bodyT,diagnostic) ||
                !view.SkinPlayerPoseUniqueTextured(crouched,primary,primaryT,diagnostic) ||
                !view.SkinPlayerPoseUniqueTextured(independent,reference,referenceT,diagnostic))
            { std::cerr<<diagnostic<<'\n';return 1; }
            Check(primary.size()==reference.size() && primaryT.size()==referenceT.size(),
                  "shared primary palette must bind to the native viewmodel");
            for(std::size_t i=0;i<primary.size();++i)
                for(int axis=0;axis<3;++axis)
                    if(!Near(primary[i].position[axis],reference[i].position[axis]))
                    { Check(false,"primary and secondary must consume the same solved crouch pose");i=primary.size()-1;break; }
            float minimumY=0;
            if(!world.BootGroundingMinimumY(clip,time,minimumY,diagnostic)) return 1;
            const auto min=std::min_element(body.begin(),body.end(),[](const auto&a,const auto&b){return a.position[1]<b.position[1];});
            if(min!=body.end() && min->position[1]<minimumY-.001f)
                std::cerr<<"grounding clip="<<int(clip)<<" time="<<time<<" drop="<<drop
                         <<" vertex="<<std::distance(body.begin(),min)<<" xyz="<<min->position[0]<<','<<min->position[1]<<','<<min->position[2]
                         <<" bootY="<<minimumY<<" penetration="<<minimumY-min->position[1]<<'\n';
            Check(min!=body.end() && min->position[1]>=minimumY-.001f,"grounded crouch mesh must not sink below authored boot contacts");
            ++samples;
        }
    }
    // Renderer input targets already include the lowered eye; solve them once.
    for(auto locomotion : {PlayerLocomotionClip::Idle,PlayerLocomotionClip::Walk})
    for(float drop : {0.0f,.15f,.3f,.6f})
    {
        PlayerAnimationSnapshot animation;
        animation.locomotionClip=locomotion;
        animation.locomotionTime=.21f;
        animation.crouchPelvisDropMetres=drop;
        for(int hand=0;hand<2;++hand)
        {
            auto& target=hand?animation.rightIk:animation.leftIk;
            const float side=hand?-1.f:1.f;
            target.target={{side*.16f,1.10f-drop,.35f}};
            target.pole={{side,0,1}};
            target.gripX={{1,0,0}};target.gripY={{0,1,0}};target.gripZ={{0,0,1}};
        }
        bool updated=false;
        std::array<float,3> shoulder{};
        Check(slot.ShoulderCenter(animation,shoulder,diagnostic),"crouched shoulder composition must succeed");
        auto standing=animation;standing.crouchPelvisDropMetres=0;
        std::array<float,3> standingShoulder{};
        Check(slot.ShoulderCenter(standing,standingShoulder,diagnostic) && Near(shoulder[1],standingShoulder[1]-drop),
              "pre-solve shoulder must match lowered torso");
        Check(Near(slot.BootGroundingOffsetMetres(animation),slot.BootGroundingOffsetMetres(standing)),
              "crouch must not change grounded root offset");
        auto mount=horde::gameplay::items::IdentityHeldItemTransform();
        const auto basis=BuildPlayerModelWorldBasis({{1,0,0}},{{0,0,-1}});
        Check(slot.AnimatedHipsWorldTransform(animation,basis,{{0,0,0}},mount,diagnostic),"crouched equipment mount must succeed");
        if(!slot.PreparePose(animation,123,PlayerCpuSkinCadence::Hz30,updated,diagnostic))
        {std::cerr<<diagnostic<<'\n';return 1;}
        Check(updated,"same-tick crouch changes must invalidate cadence reuse");
        Check(Near(mount[13],slot.BoneSockets().hips[13]),"pre-solve equipment hips must match solved crouched hips");
        Check(slot.LeftSocketErrorMetres()<=kPlayerGripSocketToleranceMetres && slot.RightSocketErrorMetres()<=kPlayerGripSocketToleranceMetres,
              "crouched hands must retain existing 15mm target admission");
    }
    for(float invalid : {-1.f,.601f,std::numeric_limits<float>::infinity(),std::numeric_limits<float>::quiet_NaN()})
    {
        PlayerAnimationSnapshot animation;animation.crouchPelvisDropMetres=invalid;
        bool updated=true;
        Check(!slot.PreparePose(animation,123,PlayerCpuSkinCadence::Hz30,updated,diagnostic) && !updated && !slot.SolvedPose().IsValid(),
              "invalid crouch must invalidate old palette before cadence reuse");
        SkinnedPlayerPose bad;
        Check(!world.EvaluatePlayerPose(SkinnedClip::Idle,0,arm,arm,bad,diagnostic,0,invalid) && !bad.IsValid(),"native rig must reject malformed crouch");
    }
    SkinnedPlayerPose bad;
    Check(!world.EvaluatePlayerPose(SkinnedClip::Idle,0,arm,arm,bad,diagnostic,.1f,.1f),"rescue and crouch poses must not combine");
    Check(!world.EvaluatePlayerPose(SkinnedClip::Walking,0,arm,arm,bad,diagnostic,.1f),"rescue stance must remain Idle-only");
    PlayerAnimationState state;
    PlayerAnimationSnapshot imported;imported.crouchPelvisDropMetres=.6f;
    state.Import(imported);state.StepFixed({},1.f/60);
    Check(state.Snapshot().crouchPelvisDropMetres==0,"ordinary animation step must clear snapshot-owned crouch");
    state.Import(imported);state.Reset();
    Check(state.Snapshot().crouchPelvisDropMetres==0,"animation reset must clear crouch");
    std::cout<<"crouch_pose_samples="<<samples<<" failures="<<failures<<"; native CPU rig only, GPU not measured\n";
    return failures?1:0;
}
