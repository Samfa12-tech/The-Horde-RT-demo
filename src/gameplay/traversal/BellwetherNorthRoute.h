#pragma once

#include "gameplay/traversal/BellwetherTavernRoute.h"

namespace horde::gameplay::traversal {

// The connected-map church path splits at the existing west-side approach.
// The church/wayside junction stays west of the church foundation.
inline constexpr std::array kBellwetherNorthApproach{
    kBellwetherTavernRoute[kBellwetherChurchJunction],
    BellwetherMapStation(-36.f,49.f),
    BellwetherMapStation(-25.f,59.f),
    BellwetherMapStation(-21.f,63.f),
    BellwetherMapStation(-16.f,61.f),
};
inline constexpr auto kBellwetherRecordKeeperMark=BellwetherMapStation(-7.f,51.f);
inline constexpr std::array kBellwetherChurchApproach{
    BellwetherMapStation(-14.f,51.f),
    kBellwetherRecordKeeperMark,
};
inline constexpr std::array kBellwetherWaysideApproach{
    BellwetherMapStation(-17.f,81.f),
    BellwetherMapStation(0.f,90.f),
    BellwetherMapStation(25.f,92.6f), // Inspection distance from the south-facing archive.
};

} // namespace horde::gameplay::traversal
