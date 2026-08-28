#include "gameplay/interactions/ChestRewardSequence.h"

#include <algorithm>
#include <cmath>

namespace horde::gameplay::interactions
{

bool ChestRewardSequence::Unlock()
{
    if (snapshot_.phase != ChestRewardPhase::Locked)
    {
        return false;
    }
    snapshot_.phase = ChestRewardPhase::ClosedUnlocked;
    snapshot_.phaseTime = 0.0f;
    snapshot_.lidOpenProgress = 0.0f;
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

const ChestRewardSnapshot& ChestRewardSequence::Update(float fixedDeltaSeconds)
{
    if (snapshot_.phase != ChestRewardPhase::Opening ||
        !std::isfinite(fixedDeltaSeconds) || fixedDeltaSeconds <= 0.0f)
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
}

} // namespace horde::gameplay::interactions
