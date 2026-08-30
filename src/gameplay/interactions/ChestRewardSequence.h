#pragma once

#include "gameplay/interactions/InteractionState.h"

#include <cstdint>

namespace horde::gameplay::interactions
{

enum class ChestRewardPhase : std::uint8_t
{
    Locked,
    ClosedUnlocked,
    Opening,
    LanternAvailable,
    LanternClaimed,
};

enum class ChestRewardAction : std::uint8_t
{
    None,
    OpeningStarted,
    LanternClaimed,
};

enum class ChestRewardPrompt : std::uint8_t
{
    None,
    Locked,
    OpenChest,
    Opening,
    ClaimLantern,
    Unlocking,
};

struct ChestRewardSnapshot
{
    ChestRewardPhase phase = ChestRewardPhase::Locked;
    float phaseTime = 0.0f;
    float lidOpenProgress = 0.0f;
    bool unlockPending = false;

    bool operator==(const ChestRewardSnapshot&) const = default;
};

class ChestRewardSequence
{
public:
    static constexpr float kUnlockDelaySeconds = 2.0f;
    static constexpr float kOpeningDurationSeconds = 1.20f;

    bool BeginUnlockCountdown();
    bool Unlock();
    ChestRewardAction TryInteract(const InteractionQuery& query);
    ChestRewardPrompt QueryPrompt(const InteractionQuery& query) const;
    const ChestRewardSnapshot& Update(float fixedDeltaSeconds);
    void Reset();
    void Import(const ChestRewardSnapshot& snapshot);
    const ChestRewardSnapshot& Snapshot() const { return snapshot_; }

private:
    ChestRewardSnapshot snapshot_{};
};

} // namespace horde::gameplay::interactions
