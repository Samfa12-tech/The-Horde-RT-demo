#pragma once

#include "gameplay/simulation/CharacterPosture.h"
#include "gameplay/simulation/InputSnapshot.h"
#include "gameplay/simulation/SimulationSnapshot.h"

#include <algorithm>
#include <array>
#include <cmath>

namespace horde::gameplay::validation {

// The caller owns ticks, presentation and campaign operations. These actions
// observe the live snapshot and publish the same axes and edges as a player.
template<class Step>
struct CampaignTombInput {
    const simulation::SimulationSnapshot& state;
    simulation::InputSnapshot& input;
    Step step;

    float AimMove(float x,float z,float stop=.03f) const {
        const float dx=x-state.playerX,dz=z-state.playerZ,distance=std::hypot(dx,dz);
        input.yawRadians=std::atan2(dx,-dz);
        input.pitchRadians=0;
        input.moveStrafe=0;
        const float reach=std::min(distance,.30f);
        const float scale=distance>.0001f?reach/distance:0.f;
        input.crouchHeld=!simulation::AuthoredBodySweepClear(state.playerX,state.playerZ,
            state.playerX+dx*scale,state.playerZ+dz*scale,state.playerSupportWorldY,
            simulation::kStandingBodyHeight,kPlayerCollisionRadius);
        input.moveForward=distance>stop?std::min(1.f,distance/.15f):0.f;
        return distance;
    }

    bool WalkTo(float x,float z) const {
        for(unsigned tick=0;tick<2400;++tick) {
            const bool reached=AimMove(x,z)<.035f;
            if(reached)input.moveForward=0;
            if(!step())return false;
            if(reached)return true;
        }
        return false;
    }

    bool ReachGuards() const {
        constexpr std::array<std::array<float,2>,8> route{{
            {0,-1},{0,-4.8f},{0,-9.4f},{4.2f,-9.4f},{4.2f,-14.6f},{1,-14.6f},
            {-1.8f,-15.2f},{-3.2f,-15.2f}}};
        for(const auto& mark:route)
            if(!WalkTo(mark[0],mark[1]))return false;
        return true;
    }

    bool DefeatGuards() const {
        for(unsigned tick=0;tick<2400;++tick) {
            const simulation::SkeletonEnemySnapshot* target=nullptr;
            float nearest=100.f;
            for(unsigned i=0;i<state.skeletonEnemyCount;++i) {
                const auto& enemy=state.skeletonEnemies[i];
                const float distance=std::hypot(enemy.x-state.playerX,enemy.z-state.playerZ);
                if(!enemy.dead && distance<nearest){target=&enemy;nearest=distance;}
            }
            if(!target)break;
            AimMove(target->x,target->z,1.05f);
            if(state.playerCombat.action==PlayerCombatAction::Idle) {
                const bool incoming=input.damageEnabled && nearest<2.f &&
                    target->action==EnemyCombatAction::AttackWindup &&
                    target->actionTime>=CombatTimeline::kSkeletonAttackWindupSeconds-.15f;
                if(incoming) {
                    input.moveForward=0;
                    if(target->attackKind==EnemyAttackKind::Normal)++input.commands.parry;
                    else {input.moveStrafe=1.f;++input.commands.dodge;}
                } else if(nearest<1.55f)++input.commands.attack;
            }
            if(!step())return false;
        }
        return state.skeletonEnemyCount==2 && state.skeletonEnemies[0].dead && state.skeletonEnemies[1].dead;
    }

    bool DefeatKeeper() const {
        bool dodgedCharge=false;
        for(unsigned tick=0;tick<2400 && state.lich.phase!=LichPhase::Dead;++tick) {
            const float distance=AimMove(state.lich.x,state.lich.z,1.15f);
            if(state.lich.phase!=LichPhase::Charging)dodgedCharge=false;
            const bool dodge=input.damageEnabled && !dodgedCharge && state.lich.phase==LichPhase::Charging &&
                state.lich.phaseTime>=LichEncounter::kChargeDuration-.15f;
            if(dodge) {
                input.moveForward=0;
                input.moveStrafe=1.f;
                ++input.commands.dodge;
                dodgedCharge=true;
            } else if(state.lich.revealComplete && state.lich.hitCooldownRemaining<=0 && distance<1.8f &&
                state.playerCombat.action==PlayerCombatAction::Idle)++input.commands.attack;
            if(!step())return false;
        }
        return state.lich.phase==LichPhase::Dead;
    }

    bool ClaimLantern() const {
        input.moveForward=input.moveStrafe=0;
        input.yawRadians=-1.57079632679f;
        for(unsigned tick=0;tick<600 && state.chestReward.phase!=interactions::ChestRewardPhase::LanternClaimed;++tick) {
            if(state.chestPrompt==interactions::ChestRewardPrompt::OpenChest ||
               state.chestPrompt==interactions::ChestRewardPrompt::ClaimLantern)++input.commands.interact;
            if(!step())return false;
        }
        return state.chestReward.phase==interactions::ChestRewardPhase::LanternClaimed;
    }
};

} // namespace horde::gameplay::validation
