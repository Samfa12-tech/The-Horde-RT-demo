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
    const auto end=simulation::kPrivateChapterDemoEndpoint;
    const float dx=end.x-p.x,dz=end.z-p.z;
    const float t=std::clamp(((s.playerX-p.x)*dx+(s.playerZ-p.z)*dz)/(dx*dx+dz*dz),0.f,1.f);
    // The turnaround includes the visible timber stop beyond its centre.
    const bool onApproach=std::hypot(s.playerX-(p.x+dx*t),s.playerZ-(p.z+dz*t))<=2.f ||
        std::hypot(s.playerX-end.x,s.playerZ-end.z)<=3.8f;
    return boundary && boundary->contentRevision==2 && Validate(*boundary) &&
        s.developmentRescueJourney && s.playerAlive && s.playerGrounded &&
        s.rescue.exteriorSide && s.rescue.firstAscentCompleted && !s.rescue.equipmentStowed &&
        s.worldRoute.readiness[static_cast<std::size_t>(simulation::WorldZoneId::Lookout)]==simulation::ZoneReadiness::Ready &&
        onApproach &&
        s.chapterDialogue.line==dialogue::Line::None && s.chapterDialogue.proofAccepted &&
        s.chapterDialogue.forestVillageConsumed && s.chapterCompanion.grounded &&
        s.chapterCompanion.waypoint==simulation::kWorldRoutePoints.size();
}
} // namespace horde::gameplay::campaign
