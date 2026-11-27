#pragma once

#include "gameplay/dialogue/ChapterDialogue.h"
#include "gameplay/traversal/ChapterTerrainMovement.h"
#include "../../../assets/models/world/source/bellwether-tavern-v1/staging.generated.h"

#include <algorithm>
#include <cmath>
#include <cstdint>
#include <optional>

namespace horde::gameplay::dialogue {

enum class TavernKeeperStage { AtDoor, Walking, AtHearth };
inline constexpr std::uint32_t kTavernKeeperActorId = 0x4230314bu;

inline bool TavernWelcomeAudible(const simulation::TriangleSupportSurface& terrain, Tuple source, Tuple listener) {
    const float height=std::min(source.y,listener.y);
    return std::abs(source.y-listener.y)<=1.4f && std::hypot(listener.x-source.x,listener.z-source.z)<=4.5f &&
        terrain.NonWalkableVolumeClear(source.x,source.z,listener.x,listener.z,height-.025f,height+.025f,.025f);
}

struct TavernKeeperPose {
    TavernKeeperStage stage;
    std::array<float,3> foot;
    float yaw;
    float travelled = 0.f;
    float walkSeconds = 0.f;
};

// Logical blocking is shared with the motor and can be consumed by a resident
// actor without deriving gameplay presence from a camera or a draw call.
class TavernKeeperPlacement {
public:
    static constexpr float kBodyRadius = .28f;
    static constexpr float kBodyHeight = 1.78f;
    const std::optional<TavernKeeperPose>& Pose() const { return pose_; }

    bool RestoreAtHearth(const simulation::TriangleSupportSurface& terrain) {
        *this = {};
        return Place(terrain, bellwether::kKeeperHearth, bellwether::kKeeperHearthFacingRadians,
                     TavernKeeperStage::AtHearth);
    }

    void Step(float seconds, bool kitSeated, const simulation::TriangleSupportSurface& terrain) {
        if(!std::isfinite(seconds) || seconds<=0.f)return;
        if(!pose_ && !Place(terrain, bellwether::kKeeperDoor, bellwether::kKeeperDoorFacingRadians,
                            TavernKeeperStage::AtDoor))return;
        auto& pose=*pose_;
        pose.travelled=0.f;
        if(!kitSeated || pose.stage==TavernKeeperStage::AtHearth)return;

        const float dt=std::min(seconds,.05f);
        const auto& target=approachingHearth_?bellwether::kKeeperHearth:bellwether::kKeeperAisle;
        const float dx=target[0]-pose.foot[0],dz=target[2]-pose.foot[2];
        const float distance=std::hypot(dx,dz);
        if(distance<=.0001f && !approachingHearth_) {
            approachingHearth_=true;
            return;
        }
        const float heading=distance>.0001f?std::atan2(dx,-dz):bellwether::kKeeperHearthFacingRadians;
        const float turn=std::remainder(heading-pose.yaw,6.283185307f);
        pose.yaw+=std::clamp(turn,-2.f*dt,2.f*dt);
        if(distance<=.0001f) {
            if(std::abs(turn)<.015f) {
                pose.yaw=bellwether::kKeeperHearthFacingRadians;
                pose.stage=TavernKeeperStage::AtHearth;
            }
            return;
        }
        if(std::abs(turn)>.3f)return;
        constexpr float walkMetresPerSecond = 1.08f;
        const float travel=std::min(distance,walkMetresPerSecond*dt);
        const float x=travel==distance?target[0]:pose.foot[0]+dx/distance*travel;
        const float z=travel==distance?target[2]:pose.foot[2]+dz/distance*travel;
        const auto moved=traversal::ResolveChapterStepMovement(terrain,pose.foot[0],pose.foot[2],x,z,
            pose.foot[1],kBodyRadius,kBodyHeight);
        if(moved.accepted)pose={TavernKeeperStage::Walking,{x,moved.support.worldY,z},pose.yaw,
            travel,pose.walkSeconds+travel/walkMetresPerSecond};
    }

private:
    bool Place(const simulation::TriangleSupportSurface& terrain, const std::array<float,3>& mark,
               float yaw, TavernKeeperStage stage) {
        const auto moved=traversal::ResolveChapterStepMovement(terrain,mark[0],mark[2],mark[0],mark[2],
            mark[1],kBodyRadius,kBodyHeight);
        if(!moved.accepted || std::abs(moved.support.worldY-mark[1])>.001f)return false;
        pose_=TavernKeeperPose{stage,{mark[0],moved.support.worldY,mark[2]},yaw};
        return true;
    }

    std::optional<TavernKeeperPose> pose_;
    bool approachingHearth_=false;
};

}
