#pragma once
#include <algorithm>
#include <array>
#include <cmath>
namespace horde::gameplay::dialogue {
enum class CompanionMotionMode { Idle, Turning, Starting, Walking };
struct CompanionMotionPose {
    bool diagnosticEnabled=false;
    CompanionMotionMode mode=CompanionMotionMode::Idle;
    float speed=0,turnPhase=0,turnAngleRadians=0,entryWeight=0,idleZeroWeight=0;
    bool operator==(const CompanionMotionPose&) const=default;
};
// Exact owned joint-only take's external asset-space yaw. Logical yaw is its
// negative under CompanionModelYaw. Source root-motion.json SHA256:
// 7a4928d4dfec5184071c96a4c8e1f25e07d7ce7c87545324450e75b0c29061d9
namespace companion_turn {
inline constexpr float duration=.75f,maxAngle=.7853981633974483f;
inline constexpr std::array<float,46> leftYaw{{0.0f,0.0f,5.48112221e-05f,0.00194132677f,0.00641787192f,0.0133516239f,0.0226097628f,0.0340594649f,0.0475679077f,0.063002266f,0.0802297294f,0.0991174653f,0.119532652f,0.141342476f,0.164414108f,0.188614711f,0.213811502f,0.239871621f,0.26666227f,0.294050604f,0.321903825f,0.350089103f,0.37847361f,0.406924546f,0.435309052f,0.463494331f,0.491347551f,0.518735886f,0.545526564f,0.571586668f,0.596783459f,0.620984077f,0.644055665f,0.665865541f,0.686280727f,0.705168426f,0.722395897f,0.737830281f,0.75133872f,0.762788415f,0.772046566f,0.778980315f,0.783456862f,0.785343349f,0.785398185f,0.785398185f}};
inline constexpr std::array<float,46> rightYaw{{-0.0f,-0.0f,-5.48112221e-05f,-0.00194132677f,-0.00641787192f,-0.0133516239f,-0.0226097628f,-0.0340594649f,-0.0475679077f,-0.063002266f,-0.0802297294f,-0.0991174653f,-0.119532652f,-0.141342476f,-0.164414108f,-0.188614711f,-0.213811502f,-0.239871621f,-0.26666227f,-0.294050604f,-0.321903825f,-0.350089103f,-0.37847361f,-0.406924546f,-0.435309052f,-0.463494331f,-0.491347551f,-0.518735886f,-0.545526564f,-0.571586668f,-0.596783459f,-0.620984077f,-0.644055665f,-0.665865541f,-0.686280727f,-0.705168426f,-0.722395897f,-0.737830281f,-0.75133872f,-0.762788415f,-0.772046566f,-0.778980315f,-0.783456862f,-0.785343349f,-0.785398185f,-0.785398185f}};
inline float ModelYaw(bool right,float phase) {
    const auto& keys=right?rightYaw:leftYaw;const float key=std::clamp(phase,0.f,1.f)*45.f;
    const auto i=std::min(static_cast<unsigned>(key),44u);
    return keys[i]+(keys[i+1]-keys[i])*(key-i);
}
}
}
