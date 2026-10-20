#pragma once
#include <algorithm>
#include <cmath>

namespace horde::gameplay::dialogue {
struct CompanionLocomotionPose {
    float walkPhase=0,walkWeight=0;
    float leftTurnStep=0,rightTurnStep=0;
    bool operator==(const CompanionLocomotionPose&) const=default;
};
class CompanionLocomotion {
public:
    const CompanionLocomotionPose& State() const {return state_;}
    void Reset() {state_={};turnSeconds_=-1;leftStep_=false;}
    void Step(float seconds,float distance,bool walking,float yawDelta=0) {
        if(!std::isfinite(seconds) || seconds<=0 || !std::isfinite(distance) || distance<0 || !std::isfinite(yawDelta))return;
        seconds=std::min(seconds,.05f);
        const float target=walking?1.f:0.f;
        state_.walkWeight+=std::clamp(target-state_.walkWeight,-seconds/.18f,seconds/.18f);
        state_.walkPhase=std::fmod(state_.walkPhase+distance/kWalkCycleDistanceMetres,1.f);
        // A stationary turn uses a small alternating leg fold, never the walk
        // clip's uncompensated rearward stance sweep. Finish an admitted step
        // after facing settles; walking fades it out instead of snapping.
        const bool turning=!walking && state_.walkWeight==0 && std::abs(yawDelta)>.005f;
        if(walking)turnSeconds_=-1;
        else if(turnSeconds_<0 && turning){turnSeconds_=0;leftStep_=yawDelta>0;}
        float left=0,right=0;
        if(turnSeconds_>=0) {
            turnSeconds_+=seconds;
            if(turnSeconds_>=kTurnStepSeconds) {
                if(turning){turnSeconds_-=kTurnStepSeconds;leftStep_=!leftStep_;}
                else turnSeconds_=-1;
            }
            if(turnSeconds_>=0) {
                const float wave=std::sin(3.14159265359f*turnSeconds_/kTurnStepSeconds);
                (leftStep_?left:right)=wave*wave;
            }
        }
        const auto approach=[seconds](float from,float target){
            return from+std::clamp(target-from,-seconds/.12f,seconds/.12f);
        };
        state_.leftTurnStep=approach(state_.leftTurnStep,left);
        state_.rightTurnStep=approach(state_.rightTurnStep,right);
    }
    // Pinned to measured stance travel of the admitted Warden walk, rather than
    // changing the route motor to fit an arbitrary one-second clip playback.
    static constexpr float kWalkCycleDistanceMetres=1.56f;
    static constexpr float kTurnStepSeconds=.36f;
private:
    CompanionLocomotionPose state_{};
    float turnSeconds_=-1;
    bool leftStep_=false;
};
}
