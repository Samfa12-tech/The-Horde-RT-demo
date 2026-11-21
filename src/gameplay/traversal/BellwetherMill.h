#pragma once

#include <cstdint>

namespace horde::gameplay::bellwether {

// Preserve the order of the discriminants recorded in campaign saves.
enum class MillPreparation : std::uint8_t {
    Unexamined, BearingNeeded, BearingRecovered, FitChecked, RigTrialReady, RigTested
};
enum class MillAction : std::uint8_t { None, InspectGearing, RecoverBearing, CheckFit, StartLeakTest, ConfirmSeal };
enum class MillLeakTestPhase : std::uint8_t { Idle, Testing, ReturnToDry };

inline constexpr float kMillLeakTestSeconds=2.f;

} // namespace horde::gameplay::bellwether
