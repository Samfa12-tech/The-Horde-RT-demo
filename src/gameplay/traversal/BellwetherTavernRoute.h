#pragma once

#include <array>
#include <cmath>
#include "../../../assets/models/world/source/bellwether-tavern-v1/staging.generated.h"

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
inline constexpr BellwetherStation kBellwetherLanternFittingApproach{86.75f,110.10f};

// The authored room starts at the inner face of the south wall. The ramp
// and sill remain exterior even where they share the physical floor height.
inline bool IsOnBellwetherTavernRoomFloor(float x, float y, float z) {
    const auto& low = bellwether::kRoomMin;
    const auto& high = bellwether::kRoomMax;
    return x >= low[0] && x <= high[0] && z >= low[2] && z <= high[2] && std::abs(y - low[1]) <= .02f;
}
}
