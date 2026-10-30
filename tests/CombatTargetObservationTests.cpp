#include "telemetry/CombatTargetObservation.h"
#include <cmath>
#include <iostream>
#include <limits>
#include <memory>
#include <sstream>
#include "telemetry/CombatTimingTrace.h"

int main()
{
    using namespace horde::gameplay;
    using namespace horde::gameplay::simulation;
    const auto require=[](bool pass,const char* message){if(!pass)std::cerr<<"FAIL: "<<message<<'\n';return pass;};
    bool pass=true;
    auto state=std::make_unique<SimulationSnapshot>();
    state->tickIndex=500;state->inputPublicationSequence=77;
    state->activeEnemyKind=EnemyKind::Lich;state->activeEnemyId=EntityId::Lich;
    state->enemyRoster.encounters[1].status=EncounterStatus::Active;
    state->lich.revealComplete=true;state->lich.health=3;
    state->playerX=2;state->playerZ=3;state->playerYawRadians=0;
    state->lich.x=2;state->lich.z=1;
    auto current=horde::telemetry::ObserveCombatTarget(*state);
    pass &= require(current.hasTarget && current.target==EntityId::Lich && current.centerDistance==2 &&
        current.hasBearing && std::abs(current.bearingDegrees)<.001f && current.tick==500 &&
        current.inputPublication==77 && !current.hasContact,"current live target from same snapshot, no invented contact");
    state->lich.x=4;state->lich.z=3;
    current=horde::telemetry::ObserveCombatTarget(*state);
    pass &= require(std::abs(current.bearingDegrees-90)<.001f,"right target has positive bearing");
    state->playerYawRadians=100*6.28318530718f+1.57079632679f;
    current=horde::telemetry::ObserveCombatTarget(*state);
    pass &= require(std::abs(current.bearingDegrees)<.005f,"unbounded look yaw wraps only the diagnostic bearing");
    const auto frozen=current;
    state->paused=true;state->tickIndex=501;state->lich.x=5;
    current=horde::telemetry::ObserveCombatTarget(*state);
    pass &= require(current.paused && current.centerDistance==3 && current.tick==501 &&
        frozen.centerDistance==2 && frozen.tick==500,"paused/current observation is a value, independent from old snapshots");
    CombatContactSample cut;
    cut.tick=480;cut.cut=PlayerAttackCut::DownwardCut;cut.phase=PlayerCombatAction::SwingActive;
    cut.target=EntityId::SkeletonA;cut.outcome=CombatContactOutcome::AcceptedSwordContact;
    for(unsigned i=0;i<100;++i)state->combatContactTrace.Push(cut);
    auto damage=cut;damage.tick=499;damage.cut=PlayerAttackCut::None;damage.outcome=CombatContactOutcome::DamageAccepted;
    state->combatContactTrace.Push(damage);
    current=horde::telemetry::ObserveCombatTarget(*state);
    pass &= require(current.hasContact && current.contactSequence==100 && current.contactTick==480 &&
        current.contactAgeTicks==21 && current.contactTarget==EntityId::SkeletonA &&
        current.target==EntityId::Lich && current.outcome==CombatContactOutcome::AcceptedSwordContact,
        "wrapped bounded history retains sword sample identity/age separate from current target and later damage");
    cut.tick=999;state->combatContactTrace.Push(cut);
    pass &= require(horde::telemetry::ObserveCombatTarget(*state).contactSequence==100,"future sample cannot be called current history");
    state->combatContactTrace.Clear();
    pass &= require(!horde::telemetry::ObserveCombatTarget(*state).hasContact,"reset clears historical observations");
    horde::telemetry::CombatTimingTrace exhausted;
    horde::telemetry::RtSubmittedFrameIdentity submitted{};
    submitted.submissionSerial=1;submitted.frame.recordSerial=1;submitted.frame.simulationTick=state->tickIndex;
    std::ostringstream exported;
    for(unsigned i=0;i<160;++i)
    {
        cut.tick=state->tickIndex;state->combatContactTrace.Push(cut);
        exhausted.WriteAcceptedPresent(exported,*state,submitted,1000+i,true);
    }
    state->lich.x=6;state->tickIndex=502;
    current=horde::telemetry::ObserveCombatTarget(*state);
    pass &= require(!exhausted.HasReportable(*state) && current.hasTarget && current.centerDistance==4 &&
        current.tick==502 && current.contactTick==501 && current.contactAgeTicks==1,
        "current observation remains fresh after the bounded presentation exporter is exhausted");
    state->lich.health=0;
    pass &= require(!horde::telemetry::ObserveCombatTarget(*state).hasTarget,"dead target is unavailable");
    state->lich.health=3;state->lich.revealComplete=false;
    pass &= require(!horde::telemetry::ObserveCombatTarget(*state).hasTarget,"dormant Keeper is unavailable");
    state->lich.revealComplete=true;state->lich.x=state->playerX;state->lich.z=state->playerZ;
    current=horde::telemetry::ObserveCombatTarget(*state);
    pass &= require(current.hasTarget && current.centerDistance==0 && !current.hasBearing,"overlapping centers have no invented bearing or mesh gap");
    state->lich.x=std::numeric_limits<float>::quiet_NaN();
    pass &= require(!horde::telemetry::ObserveCombatTarget(*state).hasTarget,"nonfinite target is unavailable");
    state->activeEnemyKind=EnemyKind::Skeleton;state->activeEnemyId=EntityId::SkeletonB;
    state->enemyRoster.encounters[0].status=EncounterStatus::Active;
    state->skeletonEnemies[0].id=EntityId::SkeletonA;state->skeletonEnemies[0].x=1;
    state->skeletonEnemies[1].id=EntityId::SkeletonB;state->skeletonEnemies[1].x=6;state->skeletonEnemies[1].z=3;
    current=horde::telemetry::ObserveCombatTarget(*state);
    pass &= require(current.hasTarget && current.target==EntityId::SkeletonB && current.centerDistance==4,
        "selected guard identity is preserved rather than silently replacing it with a nearer guard");
    state->skeletonEnemies[1].dead=true;
    pass &= require(!horde::telemetry::ObserveCombatTarget(*state).hasTarget,"dead selected guard is not replaced");
    state->skeletonEnemies[1].dead=false;state->playerYawRadians=std::numeric_limits<float>::infinity();
    pass &= require(!horde::telemetry::ObserveCombatTarget(*state).hasTarget,"nonfinite player pose is unavailable");
    std::cout<<"Current snapshot target/center/bearing and bounded historical sword sample checks "<<(pass?"passed":"failed")<<'\n';
    return pass?0:1;
}
