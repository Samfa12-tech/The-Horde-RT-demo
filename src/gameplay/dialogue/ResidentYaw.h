#pragma once

#include <algorithm>
#include <cmath>

namespace horde::gameplay::dialogue {

struct ResidentAttentionState {
    float remainingSeconds=0.f;
    bool latched=false;
};

inline bool StepResidentAttention(ResidentAttentionState& state,bool visible,float seconds,
    float duration) noexcept {
    if(!std::isfinite(seconds)||seconds<=0.f)return state.remainingSeconds>0.f;
    if(!visible) {
        state={};
        return false;
    }
    if(!state.latched) {
        state.latched=true;
        state.remainingSeconds=std::isfinite(duration)&&duration>0.f?duration:0.f;
    }
    const bool attending=state.remainingSeconds>0.f;
    state.remainingSeconds=std::max(0.f,state.remainingSeconds-seconds);
    return attending;
}

inline float TurnResidentYaw(float yaw,float target,float seconds,float radiansPerSecond) noexcept {
    if(!std::isfinite(yaw)||!std::isfinite(target)||!std::isfinite(seconds)||seconds<=0.f||
       !std::isfinite(radiansPerSecond)||radiansPerSecond<=0.f)return yaw;
    constexpr float fullTurn=6.28318530718f;
    const float delta=std::remainder(target-yaw,fullTurn);
    return std::remainder(yaw+std::clamp(delta,-radiansPerSecond*seconds,radiansPerSecond*seconds),fullTurn);
}

} // namespace horde::gameplay::dialogue
