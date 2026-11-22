#pragma once

#include <array>
#include <cstdint>

namespace horde::gameplay::bellwether {

inline constexpr std::uint32_t kSalvagerActorId=0x42303553u;
inline constexpr float kSalvagerRestYaw=3.14159265f;

struct SalvagerPose {
    std::array<float,3> foot;
    float yaw=kSalvagerRestYaw;
    float idleSeconds=0.f;
};

enum class ShorePracticeStage : std::uint8_t { Idle, FindDryMark, EnterShallows, Retreat, ReportBack };
enum class SalvagerAction : std::uint8_t { None, StartPractice, ReportPractice, RecallPractice };

} // namespace horde::gameplay::bellwether
