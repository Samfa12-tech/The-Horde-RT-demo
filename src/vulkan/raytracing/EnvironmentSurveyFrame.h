#pragma once
#include "vulkan/raytracing/PresentableTinyRtScene.h"
#include <algorithm>
#include <array>
#include <cmath>
#include <string>

namespace horde::vulkan::raytracing {
struct EnvironmentSurveyPose {
    float x=0,z=0,supportY=0,yaw=0,pitch=0,timeSeconds=0;
};
namespace environment_survey_detail {
using Point=std::array<float,3>;
using Transform=gameplay::items::HeldItemTransform;
struct Basis { Point right,up,forward,eye; };
inline Basis View(float x,float z,float support,float yaw,float pitch,float crouch) {
    // Exact production held-item/renderer view policy, including its -0.05 bias.
    const float y=std::clamp(pitch,-.32f,.28f)-.05f;
    const float inverse=1.f/std::sqrt(1.f+y*y),s=std::sin(yaw),c=std::cos(yaw);
    return {{c,0,s},{-s*y*inverse,inverse,c*y*inverse},
        {s*inverse,y*inverse,-c*inverse},
        {x,gameplay::simulation::PlayerEyeWorldY(support)-crouch,z}};
}
inline bool Finite(const Point& p) {
    return std::all_of(p.begin(),p.end(),[](float value){return std::isfinite(value);});
}
inline float Dot(const Point& a,const Point& b) {
    return a[0]*b[0]+a[1]*b[1]+a[2]*b[2];
}
inline Point Vector(const Point& p,const Basis& from,const Basis& to) {
    const Point local{Dot(p,from.right),Dot(p,from.up),Dot(p,from.forward)};
    Point result{};
    for(unsigned i=0;i<3;++i)
        result[i]=to.right[i]*local[0]+to.up[i]*local[1]+to.forward[i]*local[2];
    return result;
}
inline Point Position(Point p,const Basis& from,const Basis& to) {
    for(unsigned i=0;i<3;++i)p[i]-=from.eye[i];
    auto result=Vector(p,from,to);
    for(unsigned i=0;i<3;++i)result[i]+=to.eye[i];
    return result;
}
inline bool Rebase(Transform& m,const Basis& from,const Basis& to) {
    for(float value:m)if(!std::isfinite(value))return false;
    for(unsigned column=0;column<4;++column) {
        Point p{m[column*4],m[column*4+1],m[column*4+2]};
        p=column==3?Position(p,from,to):Vector(p,from,to);
        for(unsigned axis=0;axis<3;++axis) {
            if(!std::isfinite(p[axis]))return false;
            m[column*4+axis]=p[axis];
        }
    }
    return true;
}
inline bool Attached(const gameplay::items::HeldItemState& item) {
    using gameplay::items::HeldItemParentMode;
    return !item.detached && (item.parentMode==HeldItemParentMode::HandSocket ||
                             item.parentMode==HeldItemParentMode::BodyStow);
}
}
// Pure render-input adaptation. No GameSimulation, input, progression, staging,
// event or camera-owner mutation; all ordinary renderer validators still run.
// Deliberately limited to a stationary settled pose: a moving/traversing or
// blended attachment cannot be represented by one rigid view/body rebase.
inline bool RebaseEnvironmentSurveyFrame(RtSceneFrameInputs& frame,
    const EnvironmentSurveyPose& pose,std::string& diagnostic) {
    using namespace environment_survey_detail;
    const auto fail=[&](const char* why){diagnostic=why;return false;};
    for(float value:{pose.x,pose.z,pose.supportY,pose.yaw,pose.pitch,pose.timeSeconds,
        frame.cameraX,frame.cameraZ,frame.playerSupportWorldY,frame.cameraYaw,
        frame.cameraPitch,frame.playerCrouchDropMetres,frame.walkAmount})
        if(!std::isfinite(value))return fail("Survey rebase requires finite camera and support inputs.");
    if(pose.pitch<-.32f || pose.pitch>.28f || pose.timeSeconds<0 ||
       frame.playerCrouchDropMetres<0 || frame.playerCrouchDropMetres>.6f)
        return fail("Survey rebase requires the existing bounded pitch/posture policy.");
    using gameplay::traversal::Phase;
    if(frame.walkAmount!=0 || frame.rescue.equipmentStowed || frame.rescue.ropeHandsActive ||
       (frame.rescue.phase!=Phase::LowerSafe && frame.rescue.phase!=Phase::UpperSafe) ||
       frame.playerAnimation.plantedPelvisDropMetres!=0 || frame.playerAnimation.dodge.weight!=0)
        return fail("Survey rebase requires stationary settled player ownership.");
    if(frame.fireEmitterCount>frame.fireEmitters.size())return fail("Survey fire emitter count is invalid.");
    for(const auto& item:frame.heldItems)
        if(item.transition.active || (item.visualStowBlend!=0 && item.visualStowBlend!=1))
            return fail("Survey rebase does not admit in-flight equipment transitions.");
    const auto from=View(frame.cameraX,frame.cameraZ,frame.playerSupportWorldY,
        frame.cameraYaw,frame.cameraPitch,frame.playerCrouchDropMetres);
    const auto to=View(pose.x,pose.z,pose.supportY,pose.yaw,pose.pitch,frame.playerCrouchDropMetres);
    const auto bodyFrom=View(frame.cameraX,frame.cameraZ,frame.playerSupportWorldY,
        frame.cameraYaw,.05f,frame.playerCrouchDropMetres);
    const auto bodyTo=View(pose.x,pose.z,pose.supportY,pose.yaw,.05f,frame.playerCrouchDropMetres);
    auto next=frame;
    bool movedLight=false,movedLantern=false;
    const Basis *lightFrom=&from,*lightTo=&to;
    for(auto& item:next.heldItems) {
        if(!Attached(item))continue;
        const bool body=item.parentMode==gameplay::items::HeldItemParentMode::BodyStow;
        if(!Rebase(item.worldFromItem,body?bodyFrom:from,body?bodyTo:to))
            return fail("Survey attached-item transform is invalid.");
        if(item.hand==gameplay::items::HeldHand::LeftHand) {
            movedLight=true;movedLantern=item.id==gameplay::items::HeldItemId::RewardLantern;
            lightFrom=body?&bodyFrom:&from;lightTo=body?&bodyTo:&to;
        }
        // worldFromDetach is a historical world trajectory anchor, not a hand.
    }
    if(movedLight && (!Rebase(next.heldLight.worldFromFlame,*lightFrom,*lightTo) ||
                      !Rebase(next.heldLight.worldFromLight,*lightFrom,*lightTo)))
        return fail("Survey held-light transform is invalid.");
    if(movedLantern) {
        if(!Rebase(next.rewardLanternWorldFromHinge,*lightFrom,*lightTo) ||
           !Rebase(next.lanternPendulum.worldFromBody,*lightFrom,*lightTo))
            return fail("Survey carried-lantern transform is invalid.");
        next.lanternPendulum.previousPivotPosition=Position(next.lanternPendulum.previousPivotPosition,*lightFrom,*lightTo);
        next.lanternPendulum.previousPivotVelocity=Vector(next.lanternPendulum.previousPivotVelocity,*lightFrom,*lightTo);
        next.lanternPendulum.previousHandForward=Vector(next.lanternPendulum.previousHandForward,*lightFrom,*lightTo);
        if(!Finite(next.lanternPendulum.previousPivotPosition) ||
           !Finite(next.lanternPendulum.previousPivotVelocity) || !Finite(next.lanternPendulum.previousHandForward))
            return fail("Survey carried-lantern history is invalid.");
    }
    for(std::size_t i=0;i<next.fireEmitterCount;++i) {
        auto& emitter=next.fireEmitters[i];bool playerOwned=false;
        const Basis *emitterFrom=&from,*emitterTo=&to;
        for(const auto& item:frame.heldItems) {
            using gameplay::effects::FireEmitterParentObject;
            using gameplay::items::HeldItemId;
            if(Attached(item) &&
                ((item.id==HeldItemId::OriginalTorch && emitter.parentObject==FireEmitterParentObject::OriginalTorch) ||
                 (item.id==HeldItemId::RewardLantern && emitter.parentObject==FireEmitterParentObject::RewardLantern))) {
                playerOwned=true;
                const bool body=item.parentMode==gameplay::items::HeldItemParentMode::BodyStow;
                emitterFrom=body?&bodyFrom:&from;emitterTo=body?&bodyTo:&to;
            }
        }
        if(!playerOwned)continue; // World lights and a dropped torch remain fixed.
        if(!Rebase(emitter.worldFromFlame,*emitterFrom,*emitterTo) || !Rebase(emitter.worldFromLight,*emitterFrom,*emitterTo))
            return fail("Survey player fire transform is invalid.");
        emitter.previousPivotPosition=Position(emitter.previousPivotPosition,*emitterFrom,*emitterTo);
        emitter.previousPivotVelocity=Vector(emitter.previousPivotVelocity,*emitterFrom,*emitterTo);
        if(!Finite(emitter.previousPivotPosition) || !Finite(emitter.previousPivotVelocity))
            return fail("Survey player fire history is invalid.");
        // leanX/Z are flame-local projections, not world-space vector fields.
    }
    // Local kinematics/IK, equipment semantics, world actors/lights, torch release
    // anchors, contact effects and progression remain exactly as published.
    next.cameraX=pose.x;next.cameraZ=pose.z;next.playerSupportWorldY=pose.supportY;
    next.cameraYaw=pose.yaw;next.cameraPitch=pose.pitch;next.walkTime=pose.timeSeconds;
    frame=next;diagnostic.clear();return true;
}
} // namespace horde::vulkan::raytracing
