#pragma once

#include <array>
#include <cstdint>

namespace horde::gameplay::bellwether {

// Preserve the order of the discriminants recorded in campaign saves.
enum class MillPreparation : std::uint8_t {
    Unexamined, BearingNeeded, BearingRecovered, FitChecked, RigTrialReady, RigTested,
    BearingSeated, GearingVerified
};
enum class MillAction : std::uint8_t {
    None, InspectGearing, RecoverBearing, CheckFit, StartLeakTest, ConfirmSeal,
    SeatBearing=8, TurnDrive, TakeRig
};

// Save bytes 0–5 predate the bench repair. Use this order for progression.
inline constexpr unsigned MillProgress(MillPreparation stage) {
    switch(stage) {
    case MillPreparation::BearingSeated: return 4;
    case MillPreparation::GearingVerified: return 5;
    case MillPreparation::RigTrialReady: return 6;
    case MillPreparation::RigTested: return 7;
    default: return static_cast<unsigned>(stage);
    }
}

inline constexpr bool HasMillDrive(MillPreparation stage) {
    return MillProgress(stage)>=MillProgress(MillPreparation::BearingSeated);
}
enum class MillLeakTestPhase : std::uint8_t { Idle, Testing, ReturnToDry };

inline constexpr float kMillLeakTestSeconds=2.f;
inline constexpr float kMillDriveTestSeconds=1.f;
inline constexpr float kMillDriveTestRadians=1.82f;

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
