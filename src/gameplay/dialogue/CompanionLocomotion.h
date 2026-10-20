#pragma once
#include <algorithm>
#include <cmath>

namespace horde::gameplay::dialogue {
struct CompanionLocomotionPose {
    float walkPhase=0,walkWeight=0;
    bool operator==(const CompanionLocomotionPose&) const=default;
};
class CompanionLocomotion {
public:
    const CompanionLocomotionPose& State() const {return state_;}
    void Reset() {state_={};}
    void Step(float seconds,float distance,bool walking) {
        if(!std::isfinite(seconds) || seconds<=0 || !std::isfinite(distance) || distance<0)return;
        seconds=std::min(seconds,.05f);
        const float target=walking?1.f:0.f;
        state_.walkWeight+=std::clamp(target-state_.walkWeight,-seconds/.18f,seconds/.18f);
        state_.walkPhase=std::fmod(state_.walkPhase+distance/kWalkCycleDistanceMetres,1.f);
    }
    // Pinned to measured stance travel of the admitted Warden walk, rather than
    // changing the route motor to fit an arbitrary one-second clip playback.
    static constexpr float kWalkCycleDistanceMetres=1.56f;
private:
    CompanionLocomotionPose state_{};
};
}
