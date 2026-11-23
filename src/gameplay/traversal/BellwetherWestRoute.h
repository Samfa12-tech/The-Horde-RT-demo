#pragma once

#include "gameplay/traversal/BellwetherTavernRoute.h"
#include "../../../assets/models/world/source/bellwether-relic-stall-v1/geometry.generated.h"
#include "../../../assets/models/world/source/bellwether-grate-v1/geometry.generated.h"

#include "../../../assets/models/world/source/bellwether-stock-v1/geometry.generated.h"

#include <cstdint>

namespace horde::gameplay::traversal {

inline constexpr auto kBellwetherWestJunction=kBellwetherSouthApproachStation-1;
inline constexpr BellwetherStation kBellwetherRelicDealerStand=BellwetherMapStation(-18.f,-3.f);
inline constexpr BellwetherStation kBellwetherRelicDealerApproach{62.1f,129.2f};
inline constexpr float kBellwetherRelicDealerApproachYaw=-1.57079632679f;
inline constexpr std::array kBellwetherWestApproach{
    BellwetherStation{73.f,131.7f},
    BellwetherStation{bellwether::kRelicStallApproachTurn[0],bellwether::kRelicStallApproachTurn[1]},
    kBellwetherRelicDealerApproach};
inline constexpr std::array kBellwetherGardenApproach{
    BellwetherStation{48.f,134.4f}, BellwetherStation{43.f,134.9f}, BellwetherStation{39.5f,136.23f}};
inline constexpr BellwetherStation kBellwetherGardenView{38.65f,138.48f};
inline constexpr auto kBellwetherStockApproach=[] {
    constexpr auto& points=bellwether::kBellwetherStockViewRoute;
    std::array<BellwetherStation,points.size()> route{};
    for(std::size_t i=0;i<points.size();++i)route[i]={points[i][0],points[i][1]};
    return route;
}();
inline constexpr BellwetherStation kBellwetherGardenAisle{34.f,136.23f};
inline constexpr BellwetherStation kBellwetherGardenGrateApproach{
    bellwether::kBellwetherGrateApproach[0],bellwether::kBellwetherGrateApproach[1]};
inline constexpr std::array kBellwetherServiceYardApproach{
    BellwetherStation{74.f,124.f}, BellwetherStation{75.5f,116.f}, BellwetherStation{74.8f,112.f}};
inline constexpr BellwetherStation kBellwetherServiceYardView{74.15f,112.65f};

} // namespace horde::gameplay::traversal

namespace horde::gameplay::bellwether {

inline constexpr std::uint32_t kRelicDealerActorId=0x42303644u;
inline constexpr float kRelicDealerRestYaw=1.57079632679f;

struct RelicDealerPose {
    std::array<float,3> foot;
    float yaw=kRelicDealerRestYaw;
};

} // namespace horde::gameplay::bellwether
