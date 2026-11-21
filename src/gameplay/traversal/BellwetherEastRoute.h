#pragma once

#include "gameplay/traversal/BellwetherTavernRoute.h"

namespace horde::gameplay::traversal {

// From the south ramp, follow the tavern's east flank, then descend to
// the mill yard above the steep break in the terrain. B05 stays closed.
inline constexpr std::size_t kBellwetherMillJunction = kBellwetherSouthApproachStation - 1;
inline constexpr std::array kBellwetherMillApproach{
    kBellwetherTavernRoute[kBellwetherMillJunction],
    BellwetherMapStation(25.f, 1.f),
    BellwetherMapStation(26.f, 13.f),
    BellwetherMapStation(26.5f, 13.75f),
    BellwetherMapStation(38.5f, 13.f),
};
inline constexpr auto kBellwetherSalvagerMark = BellwetherMapStation(36.f, 0.f);
inline constexpr auto kBellwetherMillYardView = BellwetherMapStation(35.45f, 13.22f);
inline constexpr BellwetherStation kBellwetherMillWorkMark{bellwether::kMillWorkMark[0], bellwether::kMillWorkMark[2]};
inline constexpr BellwetherStation kBellwetherBearingBinApproach{
    bellwether::kMillBearingBin[0]-1.f, bellwether::kMillBearingBin[2]};

} // namespace horde::gameplay::traversal
