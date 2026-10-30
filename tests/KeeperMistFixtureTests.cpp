#include "gameplay/validation/KeeperMistFixture.h"
#include "gameplay/effects/KeeperCastPresentation.h"
#include <iostream>
#include "telemetry/CombatTargetObservation.h"
#include <memory>
int main() {
    using namespace horde::gameplay;
    auto a=std::make_unique<simulation::SimulationSnapshot>();
    auto b=std::make_unique<simulation::SimulationSnapshot>();unsigned at=0,bt=0;
    if(!validation::BuildKeeperMistFixture(*a,at) || !validation::BuildKeeperMistFixture(*b,bt)) {
        std::cerr<<"Fixture rejected: ticks="<<at<<" phase="<<int(a->lich.phase)
        <<" reveal="<<a->lich.revealComplete<<" readable="<<a->lich.readableCastPresentation
        <<" staff="<<a->lich.staffLightStrength<<" mist="<<a->lich.roomMistDensityScale
        <<" active="<<int(a->activeEnemyKind)<<" zone="<<int(a->zone)<<" torch="<<int(a->torchFailure.phase)<<" alive="<<a->playerAlive<<" tick="<<a->tickIndex<<" xyz="<<a->playerX<<","<<a->playerSupportWorldY<<","<<a->playerZ<<" lights="<<a->fireEmitterCount<<"\n";return 1;
    }
    if(at!=bt || a->tickIndex!=b->tickIndex || a->playerX!=b->playerX || a->playerZ!=b->playerZ ||
       a->playerYawRadians!=b->playerYawRadians || a->lich.x!=b->lich.x || a->lich.z!=b->lich.z ||
       effects::KeeperShaderPresentationKind(true,a->lich)!=1.f)return 2;
    for(unsigned i=1;i<3;++i)if(a->fireEmitters[i].worldFromLight!=b->fireEmitters[i].worldFromLight ||
       a->fireEmitters[i].phase!=b->fireEmitters[i].phase)return 3;
    const auto observed=horde::telemetry::ObserveCombatTarget(*a);
    if(!observed.hasTarget || observed.target!=simulation::EntityId::Lich ||
       std::abs(observed.centerDistance-3.5f)>.001f || !observed.hasBearing ||
       std::abs(observed.bearingDegrees)>.001f || observed.tick!=a->tickIndex)return 4;
    std::cout<<"Deterministic ordinary reveal with two production lights, full live mist, no player combat edges; setup ticks="<<at
      <<" camera="<<a->playerX<<","<<a->playerSupportWorldY<<","<<a->playerZ<<" keeper="<<a->lich.x<<","<<a->lich.z<<"\n";
}
