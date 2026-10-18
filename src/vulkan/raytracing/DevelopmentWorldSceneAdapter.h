#pragma once
#include "gameplay/simulation/GameSimulation.h"
#include "vulkan/raytracing/PresentableTinyRtScene.h"

namespace horde::vulkan::raytracing
{
// Scene supplies current renderer-owned resource readiness. The template also
// permits explicit CPU protocol fixtures; it never substitutes their evidence
// for an actual initialized/presented RT scene.
template<class Scene>
inline void PublishDevelopmentWorldReadiness(
    horde::gameplay::simulation::GameSimulation& simulation,
    const Scene& scene)
{
    using namespace horde::gameplay::simulation;
    const auto snapshot=simulation.Snapshot();
    if(!snapshot.developmentWorldRoute && !snapshot.developmentRescueJourney) return;
    // A loaded exterior checkpoint starts with only its destination ready.
    // The resident dungeon must be admitted from the real scene too, or the
    // return rope remains Preparing even after all native resources exist.
    for(std::size_t i=0;i<kWorldZones.size();++i)
    {
        if(snapshot.worldRoute.readiness[i]!=ZoneReadiness::Unprepared &&
           snapshot.worldRoute.readiness[i]!=ZoneReadiness::Preparing) continue;
        const WorldZoneToken token{snapshot.worldRoute.generation,static_cast<WorldZoneId>(i)};
        simulation.PublishWorldZoneReadiness(token,ZoneReadiness::Preparing);
        const auto actual=scene.WorldZoneReadiness(token);
        if(actual==ZoneReadiness::Ready) {
            if(snapshot.developmentRescueJourney && token.zone!=WorldZoneId::Dungeon) {
                const auto terrain=scene.ChapterTerrainSurface();
                if(terrain)simulation.PublishChapterTerrain(token,terrain);
            }
            else simulation.PublishWorldZoneReadiness(token,actual);
        }
    }
}
} // namespace horde::vulkan::raytracing
