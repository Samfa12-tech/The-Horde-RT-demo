#pragma once

#include <array>
#include <cstdint>

namespace horde::gameplay::bellwether {

// Preserve the order of the discriminants recorded in campaign saves.
enum class MillPreparation : std::uint8_t {
    Unexamined, BearingNeeded, BearingRecovered, FitChecked, RigTrialReady, RigTested
};
enum class MillAction : std::uint8_t { None, InspectGearing, RecoverBearing, CheckFit, StartLeakTest, ConfirmSeal };
enum class MillLeakTestPhase : std::uint8_t { Idle, Testing, ReturnToDry };

inline constexpr float kMillLeakTestSeconds=2.f;

// The current chapter profile has room for the tavern keeper and one
// specialist alongside Kit.
enum class Specialist { None, RecordKeeper, Artificer };
inline constexpr std::uint32_t kArtificerActorId=0x42303541u;
inline constexpr float kArtificerRestYaw=-1.69f;

struct ArtificerPose {
    std::array<float,3> foot;
    float yaw=kArtificerRestYaw;
};

} // namespace horde::gameplay::bellwether
