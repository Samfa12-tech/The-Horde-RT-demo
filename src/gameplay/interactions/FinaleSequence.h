#pragma once

#include <cstdint>

namespace horde::gameplay::interactions
{

enum class FinaleEndingPhase : std::uint8_t
{
    Inactive,
    LichFalling,
    SkylightOpening,
    DawnRevealed,
    Complete,
};

enum class FinaleSequencePhase : std::uint8_t
{
    Inactive,
    LichFalling,
    RaisingLantern,
    RevealingLantern,
    SkylightOpening,
    DawnRevealed,
    Complete,
};

struct FinaleSequenceSnapshot
{
    FinaleSequencePhase phase = FinaleSequencePhase::Inactive;
    FinaleEndingPhase endingPhase = FinaleEndingPhase::Inactive;
    float phaseTime = 0.0f;
    float skylightOpenProgress = 0.0f;
    float dawnRevealProgress = 0.0f;
    bool lichDefeated = false;
    bool lanternClaimed = false;

    bool operator==(const FinaleSequenceSnapshot&) const = default;
};

inline constexpr float kFinaleLanternRaiseSeconds = 0.65f;
inline constexpr float kFinaleLanternRevealSeconds = 1.25f;
inline constexpr float kFinaleSkylightOpenSeconds = 4.50f;
inline constexpr float kFinaleDawnRevealSeconds = 1.75f;

class FinaleSequence
{
public:
    bool NotifyLichDefeated();
    bool NotifyLanternClaimed();
    const FinaleSequenceSnapshot& Update(float fixedDeltaSeconds);
    void Reset();
    void Import(const FinaleSequenceSnapshot& snapshot);
    const FinaleSequenceSnapshot& Snapshot() const { return snapshot_; }

private:
    FinaleSequenceSnapshot snapshot_{};
};

} // namespace horde::gameplay::interactions
