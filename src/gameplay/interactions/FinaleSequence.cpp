#include "gameplay/interactions/FinaleSequence.h"

#include <algorithm>
#include <cmath>

namespace horde::gameplay::interactions
{

bool FinaleSequence::NotifyLichDefeated()
{
    if (snapshot_.lichDefeated)
    {
        return false;
    }
    snapshot_.lichDefeated = true;
    snapshot_.phase = FinaleSequencePhase::LichFalling;
    snapshot_.endingPhase = FinaleEndingPhase::LichFalling;
    snapshot_.phaseTime = 0.0f;
    return true;
}

bool FinaleSequence::NotifyLanternClaimed()
{
    if (!snapshot_.lichDefeated || snapshot_.lanternClaimed)
    {
        return false;
    }
    snapshot_.lanternClaimed = true;
    snapshot_.phase = FinaleSequencePhase::RaisingLantern;
    snapshot_.endingPhase = FinaleEndingPhase::LichFalling;
    snapshot_.phaseTime = 0.0f;
    return true;
}

const FinaleSequenceSnapshot& FinaleSequence::Update(float fixedDeltaSeconds)
{
    if (!std::isfinite(fixedDeltaSeconds) || fixedDeltaSeconds <= 0.0f)
    {
        return snapshot_;
    }
    float remaining = fixedDeltaSeconds;
    while (remaining > 0.000001f)
    {
        float duration = 0.0f;
        switch (snapshot_.phase)
        {
        case FinaleSequencePhase::RaisingLantern:
            duration = kFinaleLanternRaiseSeconds;
            break;
        case FinaleSequencePhase::RevealingLantern:
            duration = kFinaleLanternRevealSeconds;
            break;
        case FinaleSequencePhase::SkylightOpening:
            duration = kFinaleSkylightOpenSeconds;
            break;
        case FinaleSequencePhase::DawnRevealed:
            duration = kFinaleDawnRevealSeconds;
            break;
        default:
            return snapshot_;
        }
        const float step = std::min(remaining, std::max(0.0f, duration - snapshot_.phaseTime));
        snapshot_.phaseTime = std::min(duration, snapshot_.phaseTime + step);
        remaining -= step;
        if (snapshot_.phase == FinaleSequencePhase::SkylightOpening)
        {
            snapshot_.skylightOpenProgress = snapshot_.phaseTime / duration;
        }
        else if (snapshot_.phase == FinaleSequencePhase::DawnRevealed)
        {
            snapshot_.dawnRevealProgress = snapshot_.phaseTime / duration;
        }
        if (snapshot_.phaseTime + 0.000001f < duration)
        {
            break;
        }
        snapshot_.phaseTime = 0.0f;
        switch (snapshot_.phase)
        {
        case FinaleSequencePhase::RaisingLantern:
            snapshot_.phase = FinaleSequencePhase::RevealingLantern;
            break;
        case FinaleSequencePhase::RevealingLantern:
            snapshot_.phase = FinaleSequencePhase::SkylightOpening;
            snapshot_.endingPhase = FinaleEndingPhase::SkylightOpening;
            break;
        case FinaleSequencePhase::SkylightOpening:
            snapshot_.skylightOpenProgress = 1.0f;
            snapshot_.phase = FinaleSequencePhase::DawnRevealed;
            snapshot_.endingPhase = FinaleEndingPhase::DawnRevealed;
            break;
        case FinaleSequencePhase::DawnRevealed:
            snapshot_.dawnRevealProgress = 1.0f;
            snapshot_.phase = FinaleSequencePhase::Complete;
            snapshot_.endingPhase = FinaleEndingPhase::Complete;
            break;
        default:
            break;
        }
    }
    return snapshot_;
}

void FinaleSequence::Reset()
{
    snapshot_ = {};
}

void FinaleSequence::Import(const FinaleSequenceSnapshot& snapshot)
{
    snapshot_ = snapshot;
    if (!std::isfinite(snapshot_.phaseTime)) snapshot_.phaseTime = 0.0f;
    if (!std::isfinite(snapshot_.skylightOpenProgress)) snapshot_.skylightOpenProgress = 0.0f;
    if (!std::isfinite(snapshot_.dawnRevealProgress)) snapshot_.dawnRevealProgress = 0.0f;
    snapshot_.phaseTime = std::max(0.0f, snapshot_.phaseTime);
    snapshot_.skylightOpenProgress = std::clamp(snapshot_.skylightOpenProgress, 0.0f, 1.0f);
    snapshot_.dawnRevealProgress = std::clamp(snapshot_.dawnRevealProgress, 0.0f, 1.0f);
}

} // namespace horde::gameplay::interactions
