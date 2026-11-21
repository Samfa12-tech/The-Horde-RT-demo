#pragma once

#include <cstdint>

namespace horde::gameplay::bellwether {

enum class MillPreparation : std::uint8_t { Unexamined, BearingNeeded, BearingRecovered, FitChecked };
enum class MillAction : std::uint8_t { None, InspectGearing, RecoverBearing, CheckFit };

} // namespace horde::gameplay::bellwether
