#pragma once

#include <cstdint>

namespace horde::gameplay::traversal {

using ResidentMask=std::uint8_t;
inline constexpr ResidentMask kTavernKeeperResident=1u;
inline constexpr ResidentMask kRecordKeeperResident=2u;
inline constexpr ResidentMask kArtificerResident=4u;
inline constexpr ResidentMask kSalvagerResident=8u;
inline constexpr ResidentMask kTownResidentMask=kTavernKeeperResident|kRecordKeeperResident;
inline constexpr ResidentMask kEastResidentMask=kArtificerResident|kSalvagerResident;
inline constexpr ResidentMask kAllResidentMask=kTownResidentMask|kEastResidentMask;

} // namespace horde::gameplay::traversal
