#pragma once

#include "gameplay/traversal/BellwetherTavernRoute.h"

#include <cstdint>

namespace horde::gameplay::traversal {

inline constexpr auto kBellwetherWestJunction=kBellwetherSouthApproachStation-1;
inline constexpr BellwetherStation kBellwetherRelicDealerStand=BellwetherMapStation(-18.f,-3.f);
inline constexpr BellwetherStation kBellwetherRelicDealerApproach{62.1f,129.2f};
inline constexpr float kBellwetherRelicDealerApproachYaw=-1.57079632679f;
inline constexpr std::array kBellwetherWestApproach{
    BellwetherStation{73.f,131.7f},kBellwetherRelicDealerApproach};

} // namespace horde::gameplay::traversal

namespace horde::gameplay::bellwether {

inline constexpr std::uint32_t kRelicDealerActorId=0x42303644u;
inline constexpr float kRelicDealerRestYaw=1.57079632679f;

struct RelicDealerPose {
    std::array<float,3> foot;
    float yaw=kRelicDealerRestYaw;
};

} // namespace horde::gameplay::bellwether
