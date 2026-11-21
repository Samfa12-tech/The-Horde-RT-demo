#pragma once

#include <array>
#include <cstdint>

namespace horde::gameplay::dialogue {

inline constexpr std::uint32_t kRecordKeeperActorId=0x42303252u;

struct RecordKeeperPose {
    std::array<float,3> foot;
    float yaw;
    float idleSeconds=0;
};

} // namespace horde::gameplay::dialogue
