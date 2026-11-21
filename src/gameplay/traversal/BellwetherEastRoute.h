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
// The artificer stands beyond the south end of the bench, outside the approach.
inline constexpr BellwetherStation kBellwetherMillArtificerMark{117.45f,117.15f};
inline constexpr BellwetherStation kBellwetherMillArtificerView{115.16f,117.00f};
inline constexpr BellwetherStation kBellwetherMillWorkMark{bellwether::kMillWorkMark[0], bellwether::kMillWorkMark[2]};
// The existing cradle sits .43 m east and .56 m north of the old bearing.
inline constexpr std::array<float,3> kMillDriveMount{
    bellwether::kMillComparison[0]+.43f, bellwether::kMillComparison[1]-.16f,
    bellwether::kMillComparison[2]-.56f};
inline constexpr std::array<float,3> kMillDriveWork{
    kMillDriveMount[0], kMillDriveMount[1]+.18f, kMillDriveMount[2]};
inline constexpr BellwetherStation kBellwetherBearingBinApproach{
    bellwether::kMillBearingBin[0]-1.f, bellwether::kMillBearingBin[2]};

inline constexpr auto kBellwetherRiverLanding=[] {
    constexpr auto& centres=bellwether::kRiverLandingStations;
    std::array<BellwetherStation,centres.size()> route{};
    for(std::size_t i=0;i<route.size();++i)route[i]={centres[i][0],centres[i][1]};
    return route;
}();
inline constexpr BellwetherStation kBellwetherRiverDryMark{
    bellwether::kRiverLandingDryMark[0],bellwether::kRiverLandingDryMark[1]};
inline constexpr BellwetherStation kBellwetherRiverLandingView{
    bellwether::kRiverLandingView[0],bellwether::kRiverLandingView[1]};

} // namespace horde::gameplay::traversal
