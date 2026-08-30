#include "gameplay/interactions/ChestRewardSequence.h"

#include <algorithm>
#include <cmath>

namespace horde::gameplay::interactions
{

bool ChestRewardSequence::BeginUnlockCountdown()
{
    if (snapshot_.phase != ChestRewardPhase::Locked || snapshot_.unlockPending)
    {
        return false;
    }
    snapshot_.unlockPending = true;
    snapshot_.phaseTime = 0.0f;
    snapshot_.lidOpenProgress = 0.0f;
    return true;
}

bool ChestRewardSequence::Unlock()
{
    if (snapshot_.phase != ChestRewardPhase::Locked)
    {
        return false;
    }
    snapshot_.phase = ChestRewardPhase::ClosedUnlocked;
    snapshot_.phaseTime = 0.0f;
    snapshot_.lidOpenProgress = 0.0f;
    snapshot_.unlockPending = false;
    return true;
}

ChestRewardAction ChestRewardSequence::TryInteract(const InteractionQuery& query)
{
    if (!IsInteractionAvailable(query, kRewardChestInteractionPosition))
    {
        return ChestRewardAction::None;
    }
    if (snapshot_.phase == ChestRewardPhase::ClosedUnlocked)
    {
        snapshot_.phase = ChestRewardPhase::Opening;
        snapshot_.phaseTime = 0.0f;
        snapshot_.lidOpenProgress = 0.0f;
        return ChestRewardAction::OpeningStarted;
    }
    if (snapshot_.phase == ChestRewardPhase::LanternAvailable)
    {
        snapshot_.phase = ChestRewardPhase::LanternClaimed;
        snapshot_.phaseTime = 0.0f;
        snapshot_.lidOpenProgress = 1.0f;
        return ChestRewardAction::LanternClaimed;
    }
    return ChestRewardAction::None;
}

ChestRewardPrompt ChestRewardSequence::QueryPrompt(
    const InteractionQuery& query) const
{
    if (!IsInteractionAvailable(query, kRewardChestInteractionPosition))
    {
        return ChestRewardPrompt::None;
    }
    switch (snapshot_.phase)
    {
    case ChestRewardPhase::Locked:
        return snapshot_.unlockPending
            ? ChestRewardPrompt::Unlocking
            : ChestRewardPrompt::Locked;
    case ChestRewardPhase::ClosedUnlocked:
        return ChestRewardPrompt::OpenChest;
    case ChestRewardPhase::Opening:
        return ChestRewardPrompt::Opening;
    case ChestRewardPhase::LanternAvailable:
        return ChestRewardPrompt::ClaimLantern;
    default:
        return ChestRewardPrompt::None;
    }
}

const ChestRewardSnapshot& ChestRewardSequence::Update(float fixedDeltaSeconds)
{
    if (!std::isfinite(fixedDeltaSeconds) || fixedDeltaSeconds <= 0.0f)
    {
        return snapshot_;
    }
    if (snapshot_.phase == ChestRewardPhase::Locked && snapshot_.unlockPending)
    {
        snapshot_.phaseTime = std::min(
            kUnlockDelaySeconds, snapshot_.phaseTime + fixedDeltaSeconds);
        // Fixed 1/60 accumulation lands a few float ulps below exactly 2.0.
        // Use a sub-tick tolerance so the authored boundary remains 120 ticks,
        // rather than slipping audibly to tick 121 on one compiler.
        if (snapshot_.phaseTime + 0.0001f >= kUnlockDelaySeconds)
        {
            snapshot_.phase = ChestRewardPhase::ClosedUnlocked;
            snapshot_.phaseTime = 0.0f;
            snapshot_.unlockPending = false;
        }
        return snapshot_;
    }
    if (snapshot_.phase != ChestRewardPhase::Opening)
    {
        return snapshot_;
    }
    snapshot_.phaseTime = std::min(
        kOpeningDurationSeconds, snapshot_.phaseTime + fixedDeltaSeconds);
    snapshot_.lidOpenProgress = snapshot_.phaseTime / kOpeningDurationSeconds;
    if (snapshot_.lidOpenProgress + 0.000001f >= 1.0f)
    {
        snapshot_.phase = ChestRewardPhase::LanternAvailable;
        snapshot_.phaseTime = 0.0f;
        snapshot_.lidOpenProgress = 1.0f;
    }
    return snapshot_;
}

void ChestRewardSequence::Reset()
{
    snapshot_ = {};
}

void ChestRewardSequence::Import(const ChestRewardSnapshot& snapshot)
{
    snapshot_ = snapshot;
    if (!std::isfinite(snapshot_.phaseTime)) snapshot_.phaseTime = 0.0f;
    if (!std::isfinite(snapshot_.lidOpenProgress)) snapshot_.lidOpenProgress = 0.0f;
    snapshot_.phaseTime = std::max(0.0f, snapshot_.phaseTime);
    snapshot_.lidOpenProgress = std::clamp(snapshot_.lidOpenProgress, 0.0f, 1.0f);
    if (snapshot_.phase != ChestRewardPhase::Locked)
    {
        snapshot_.unlockPending = false;
    }
    else if (!snapshot_.unlockPending)
    {
        snapshot_.phaseTime = 0.0f;
    }
    else
    {
        snapshot_.phaseTime = std::min(snapshot_.phaseTime, kUnlockDelaySeconds);
    }
}

} // namespace horde::gameplay::interactions
