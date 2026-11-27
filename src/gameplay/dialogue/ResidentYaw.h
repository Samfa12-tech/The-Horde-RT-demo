#pragma once

#include <algorithm>
#include <cmath>

namespace horde::gameplay::dialogue {

inline float TurnResidentYaw(float yaw,float target,float seconds,float radiansPerSecond) noexcept {
    if(!std::isfinite(yaw)||!std::isfinite(target)||!std::isfinite(seconds)||seconds<=0.f||
       !std::isfinite(radiansPerSecond)||radiansPerSecond<=0.f)return yaw;
    constexpr float fullTurn=6.28318530718f;
    const float delta=std::remainder(target-yaw,fullTurn);
    return std::remainder(yaw+std::clamp(delta,-radiansPerSecond*seconds,radiansPerSecond*seconds),fullTurn);
}

} // namespace horde::gameplay::dialogue
