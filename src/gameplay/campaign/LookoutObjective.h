#pragma once

#include "gameplay/campaign/CampaignCheckpoint.h"
#include "gameplay/simulation/SimulationSnapshot.h"
#include <optional>

namespace horde::gameplay::campaign {
// Read-only presentation eligibility. Earning progress remains the simulation's
// checkpoint transaction; opening a panel never completes or saves the chapter.
inline bool LookoutObjectiveAvailable(const simulation::SimulationSnapshot& s,
    const std::optional<Checkpoint>& boundary) {
    const auto p=simulation::kWorldRoutePoints.back();
    return boundary && boundary->contentRevision==2 && Validate(*boundary) &&
        s.developmentRescueJourney && s.playerAlive && s.playerGrounded &&
        s.rescue.exteriorSide && s.rescue.firstAscentCompleted && !s.rescue.equipmentStowed &&
        s.worldRoute.current==simulation::WorldZoneId::Lookout &&
        s.worldRoute.readiness[static_cast<std::size_t>(simulation::WorldZoneId::Lookout)]==simulation::ZoneReadiness::Ready &&
        std::hypot(s.playerX-p.x,s.playerZ-p.z)<=2.0f &&
        s.chapterDialogue.line==dialogue::Line::None && s.chapterDialogue.proofAccepted &&
        s.chapterDialogue.forestVillageConsumed && s.chapterCompanion.grounded &&
        s.chapterCompanion.waypoint==simulation::kWorldRoutePoints.size();
}
} // namespace horde::gameplay::campaign
