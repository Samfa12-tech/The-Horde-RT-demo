#pragma once

#include "gameplay/dialogue/BellwetherRoadStone.generated.h"
#include "gameplay/dialogue/CompanionRouteFixture.h"

namespace horde::gameplay::dialogue {

// This prop shares the companion's fixed-step contact clock. The seated
// checkpoint already restores the story state that keeps the stone settled.
inline const std::array<float, 12>& BellwetherRoadStoneWorldFromModel(const CompanionSnapshot& companion)
{
    using Stage = BellwetherArrivalStage;
    switch (companion.bellwetherArrival)
    {
        case Stage::AtLookout:
        case Stage::OnRoad:
            return kBellwetherRoadStoneTrack.front();
        case Stage::AnkleAssessment:
        {
            const float lastTick = static_cast<float>(kBellwetherRoadStoneTrack.size() - 1u);
            const float tick = std::clamp(companion.motion.anklePlaybackSeconds * 60.f, 0.f, lastTick);
            return kBellwetherRoadStoneTrack[static_cast<std::size_t>(std::round(tick))];
        }
        case Stage::ProtectiveApproach:
        case Stage::ChairTurn:
        case Stage::Lowering:
        case Stage::Seated:
            return kBellwetherRoadStoneTrack.back();
    }
    return kBellwetherRoadStoneTrack.front();
}

}
