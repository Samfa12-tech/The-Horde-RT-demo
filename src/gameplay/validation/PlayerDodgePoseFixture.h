#pragma once
#include "gameplay/simulation/GameSimulation.h"
#include <string>
#include <vector>

namespace horde::gameplay::validation {
struct PlayerDodgePoseFrame {
    std::string name;
    simulation::SimulationSnapshot snapshot;
    bool mesh = false;
};
// Debug/host fixture only: independently stepped noncombat production snapshots,
// with explicit imported lantern equipment where named. No storage/input/device
// authority, earned-progression claim or mutation of the application's simulation.
inline std::vector<PlayerDodgePoseFrame> BuildPlayerDodgePoseFixture() {
    using namespace simulation;
    std::vector<PlayerDodgePoseFrame> frames;
    constexpr std::array<std::array<float,2>,4> directions{{{1,0},{-1,0},{0,-1},{0,1}}};
    constexpr std::array<const char*,4> names{{"forward","back","left","right"}};
    for(unsigned equipment=0;equipment<2;++equipment)for(unsigned axis=0;axis<4;++axis) {
        auto config=ProductionGameSimulationConfig();
        config.playerStartX=-5.5f;config.playerStartZ=-13.5f;config.playerStartYawRadians=.4f;
        config.playerStartPitchRadians=-.32f;config.waterfallSkeletonEncounter=false;
        config.swordStartsStowed=false;
        GameSimulation sim(config);
        if(equipment) {
            interactions::ChestRewardSnapshot chest;chest.phase=interactions::ChestRewardPhase::LanternClaimed;
            chest.lidOpenProgress=1;
            interactions::InteractionState held;held.heldLightKind=interactions::HeldLightKind::RewardLantern;
            held.heldLightPose=interactions::HeldLightPose::High;
            sim.ImportRewardCheckpoint(chest,held,{});
        }
        InputSnapshot input{};input.damageEnabled=false;input.tutorialEnabled=false;
        input.yawRadians=.4f;input.pitchRadians=-.32f;
        const std::string prefix=std::string(equipment?"lantern-":"torch-")+names[axis];
        const auto sample=[&](const std::string& stage,bool mesh=false) {
            frames.push_back({prefix+"-"+stage,sim.Snapshot(),mesh});
        };
        sample("before",true);
        input.moveForward=directions[axis][0];input.moveStrafe=directions[axis][1];input.commands.dodge=1;
        for(unsigned tick=1;tick<=18;++tick) {
            sim.StepFixed(input);input.moveForward=0;input.moveStrafe=0;
            if(tick==1 || tick==3 || tick==6 || tick==9 || tick==12 || tick==18)
                sample(std::to_string(tick),tick==6);
        }
        // One interruption per equipment family, using actual existing pause
        // cancellation followed by neutral resume and a fresh dodge edge.
        if(axis==3) {
            for(unsigned tick=0;tick<24;++tick)sim.StepFixed(input);
            input.commands.dodge=2;
            for(unsigned tick=0;tick<6;++tick)sim.StepFixed(input);
            sample("pause-before");
            input.paused=true;sim.AdvanceFrame(input,0);sample("paused");
            input.paused=false;sim.StepFixed(input);sample("resumed");
            sim.ResetRoute();sample("reset");
        }
    }
    return frames;
}
}
