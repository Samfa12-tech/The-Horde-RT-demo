#pragma once

#include <array>

namespace horde::gameplay::traversal
{
// Bellwether's connected-map stations share the registered terrain and B01
// shell transform. The final metres skirt the well and follow the south ramp.
// Support height always comes from the same imported geometry as rendering.
struct BellwetherStation { float x,z; };
inline constexpr BellwetherStation BellwetherMapStation(float east,float north) {
    return {east+78.3f,126.2f-north};
}
inline constexpr auto kBellwetherTavernRoute=[] {
    constexpr std::array<BellwetherStation,13> map{{
        {-65,90},{-56,76},{-59,58},{-47,44},{-30,34},{-19,18},
        {-4,4},{-3,-5.6f},{4,-5.5f},{4,1},{4,7},{4,13},{6,14.5f}
    }};
    auto stations=map;
    for(std::size_t i=0;i<map.size();++i)
        stations[i]=BellwetherMapStation(map[i].x,map[i].z);
    return stations;
}();
inline constexpr std::size_t kBellwetherChurchJunction=3;
inline constexpr std::size_t kBellwetherSouthApproachStation=9;
inline constexpr std::size_t kBellwetherSouthDoorStation=10;
inline constexpr std::size_t kBellwetherAnkleStation=5;
}
