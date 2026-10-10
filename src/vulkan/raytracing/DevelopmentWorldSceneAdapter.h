#pragma once
#include "gameplay/simulation/GameSimulation.h"
#include "vulkan/raytracing/PresentableTinyRtScene.h"

namespace horde::vulkan::raytracing
{
inline void PublishDevelopmentWorldReadiness(
    horde::gameplay::simulation::GameSimulation& simulation,
    const PresentableTinyRtScene& scene)
{
    using namespace horde::gameplay::simulation;
    const auto snapshot=simulation.Snapshot();
    if(!snapshot.developmentWorldRoute && !snapshot.developmentRescueJourney) return;
    for(std::size_t i=1;i<kWorldZones.size();++i)
    {
        if(snapshot.worldRoute.readiness[i]!=ZoneReadiness::Unprepared &&
           snapshot.worldRoute.readiness[i]!=ZoneReadiness::Preparing) continue;
        const WorldZoneToken token{snapshot.worldRoute.generation,static_cast<WorldZoneId>(i)};
        simulation.PublishWorldZoneReadiness(token,ZoneReadiness::Preparing);
        const auto actual=scene.WorldZoneReadiness(token);
        if(actual==ZoneReadiness::Ready) simulation.PublishWorldZoneReadiness(token,actual);
    }
}
} // namespace horde::vulkan::raytracing
