#pragma once

#include <array>
#include <cstdint>

namespace horde::gameplay::dialogue {

inline constexpr std::uint32_t kRecordKeeperActorId=0x42303252u;
inline constexpr float kRecordKeeperRestYaw=-1.57079632679f;

struct RecordKeeperPose {
    std::array<float,3> foot;
    float yaw=kRecordKeeperRestYaw;
    float idleSeconds=0;
};

} // namespace horde::gameplay::dialogue
