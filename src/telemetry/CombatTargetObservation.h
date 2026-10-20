#pragma once
#include <algorithm>
#include <cmath>
#include <cstdint>
#include <limits>
#include "gameplay/simulation/SimulationSnapshot.h"

namespace horde::telemetry
{
// Read-only diagnostic of one immutable simulation snapshot. Center XZ distance
// and bearing do not establish sword reach, obstruction or blade/mesh separation.
struct CombatTargetObservation
{
    std::uint64_t tick=0, inputPublication=0, contactTick=0, contactAgeTicks=0;
    std::uint64_t contactSequence=0, contactGeneration=0;
    gameplay::simulation::EntityId target=gameplay::simulation::EntityId::Invalid;
    gameplay::simulation::EntityId contactTarget=gameplay::simulation::EntityId::Invalid;
    gameplay::simulation::CombatContactOutcome outcome=gameplay::simulation::CombatContactOutcome::NoContact;
    float playerX=0, playerZ=0, targetX=0, targetZ=0, centerDistance=0, bearingDegrees=0;
    bool paused=false, hasTarget=false, hasBearing=false, hasContact=false;
};
inline CombatTargetObservation ObserveCombatTarget(
    const gameplay::simulation::SimulationSnapshot& state)
{
    using namespace gameplay;
    using namespace gameplay::simulation;
    CombatTargetObservation result;
    result.tick=state.tickIndex;result.inputPublication=state.inputPublicationSequence;
    result.paused=state.paused;result.playerX=state.playerX;result.playerZ=state.playerZ;
    const bool active=std::any_of(state.enemyRoster.encounters.begin(),state.enemyRoster.encounters.end(),
        [&](const auto& entry){return entry.kind==state.activeEnemyKind && entry.status==EncounterStatus::Active;});
    if(active && state.activeEnemyKind==EnemyKind::Lich && state.activeEnemyId==EntityId::Lich &&
       state.lich.health>0 && state.lich.revealComplete)
    {
        result.target=EntityId::Lich;result.targetX=state.lich.x;result.targetZ=state.lich.z;
    }
    else if(active && state.activeEnemyKind==EnemyKind::Skeleton)
    {
        for(std::size_t i=0;i<std::min(state.skeletonEnemyCount,state.skeletonEnemies.size());++i)
        {
            const auto& enemy=state.skeletonEnemies[i];
            if(enemy.id==state.activeEnemyId && !enemy.dead && enemy.health>0 &&
               (enemy.id==EntityId::SkeletonA || enemy.id==EntityId::SkeletonB))
            {result.target=enemy.id;result.targetX=enemy.x;result.targetZ=enemy.z;break;}
        }
    }
    if(result.target!=EntityId::Invalid && std::isfinite(state.playerX) && std::isfinite(state.playerZ) &&
       std::isfinite(state.playerYawRadians) && std::isfinite(result.targetX) && std::isfinite(result.targetZ))
    {
        const double dx=double(result.targetX)-state.playerX,dz=double(result.targetZ)-state.playerZ;
        const double distance=std::hypot(dx,dz);
        if(std::isfinite(distance) && distance<=std::numeric_limits<float>::max())
        {
            result.hasTarget=true;result.centerDistance=float(distance);
            if(distance>1e-6)
            {
                constexpr double pi=3.14159265358979323846;
                result.bearingDegrees=float(std::remainder(std::atan2(dx,-dz)-state.playerYawRadians,2*pi)*180/pi);
                result.hasBearing=std::isfinite(result.bearingDegrees);
            }
        }
    }
    if(!result.hasTarget)result.target=EntityId::Invalid;
    const auto& trace=state.combatContactTrace;
    // Keep current target separate from historical contact target/result. A
    // generation clear removes the old history; no trace reporter quota is read.
    if(trace.count<=trace.kCapacity && trace.nextIndex<trace.kCapacity && trace.samples.size()==trace.kCapacity)
    {
        for(std::uint32_t i=0;i<trace.count;++i)
        {
            const auto& sample=trace.samples[(trace.nextIndex+trace.kCapacity-trace.count+i)%trace.kCapacity];
            if(sample.sequence && sample.sequence>result.contactSequence && sample.tick<=state.tickIndex &&
               sample.cut!=PlayerAttackCut::None &&
               (sample.phase==PlayerCombatAction::SwingActive || sample.phase==PlayerCombatAction::UpwardSliceActive))
            {
                result.hasContact=true;result.contactSequence=sample.sequence;
                result.contactTick=sample.tick;result.contactAgeTicks=state.tickIndex-sample.tick;
                result.contactGeneration=trace.generation;result.contactTarget=sample.target;
                result.outcome=sample.outcome;
            }
        }
    }
    return result;
}
} // namespace horde::telemetry
