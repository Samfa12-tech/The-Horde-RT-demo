#pragma once
#include "gameplay/simulation/GameSimulation.h"
#include "gameplay/effects/KeeperTorchLighting.h"
#include <cmath>
#include <memory>

namespace horde::gameplay::validation {
inline constexpr unsigned kKeeperMistWarmupFrames=60, kKeeperMistSampleFrames=120;
inline constexpr unsigned kKeeperMistFrames=kKeeperMistWarmupFrames+kKeeperMistSampleFrames;
// Isolated diagnostic setup only. Ordinary shared simulation performs the reveal;
// no player attack/parry/dodge/retry/interact edges, campaign IO or owner mutation.
// The resulting value is frozen for rendering, not a moving-fight/CPU benchmark.
inline bool BuildKeeperMistFixture(simulation::SimulationSnapshot& result,unsigned& setupTicks) {
    auto config=simulation::ProductionGameSimulationConfig();
    config.playerStartX=-1.8f;config.playerStartZ=-15.2f;config.playerStartYawRadians=-1.57079632679f;
    auto simulation=std::make_unique<simulation::GameSimulation>(config);
    simulation->StartLiveDevelopmentRescueJourney(true);
    simulation::InputSnapshot input{};
    input.paused=false;input.damageEnabled=false;input.tutorialEnabled=false;
    input.hasAuthoritativePlayerPose=true;
    input.authoritativePlayerX=config.playerStartX;input.authoritativePlayerZ=config.playerStartZ;
    input.yawRadians=-1.57079632679f;input.pitchRadians=0.f;
    setupTicks=0;
    while(setupTicks<180 && simulation->Snapshot().torchFailure.phase!=TorchFailurePhase::Settled)
        simulation->StepFixed(input,1.f/60.f,++setupTicks);
    if(simulation->Snapshot().torchFailure.phase!=TorchFailurePhase::Settled)return false;
    input.authoritativePlayerX=kKeeperRetryPosition.x;input.authoritativePlayerZ=kKeeperRetryPosition.z;
    while(setupTicks<600 && !simulation->Snapshot().lich.revealComplete) {
        simulation->StepFixed(input,1.f/60.f,++setupTicks);
    }
    result=simulation->Snapshot();
    if(!result.developmentRescueJourney || !result.playerAlive || result.lich.health!=3 ||
       !result.lich.revealComplete || !result.lich.readableCastPresentation ||
       result.lich.deathAnimationComplete || result.lich.staffLightStrength!=0 ||
       result.lich.roomMistDensityScale!=1 || result.fireEmitterCount!=3 ||
       result.lastConsumedAttackSequence || result.lastConsumedParrySequence ||
       result.lastConsumedDodgeSequence || result.lastConsumedRetrySequence ||
       result.lastConsumedInteractSequence || result.lastConsumedRouteResetSequence ||
       std::abs(result.playerX-kKeeperRetryPosition.x)>.0001f ||
       std::abs(result.playerZ-kKeeperRetryPosition.z)>.0001f)return false;
    for(unsigned i=0;i<effects::kKeeperTorchAnchors.size();++i) {
        const auto& light=result.fireEmitters[i+1];const auto& anchor=effects::kKeeperTorchAnchors[i];
        if(light.stableId!=anchor.stableId || light.strength!=1 || light.fuel!=1)return false;
    }
    return setupTicks>0 && setupTicks<600;
}
}
