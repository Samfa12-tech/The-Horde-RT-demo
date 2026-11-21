#pragma once

#include "gameplay/simulation/InputSnapshot.h"
#include "gameplay/simulation/SimulationSnapshot.h"
#include "gameplay/traversal/BellwetherTavernRoute.h"

#include <array>
#include <cmath>
#include <string_view>

namespace horde::gameplay::validation {

// Ordinary movement after the completed tavern visit. Both platforms can use
// this input schedule without a renderer, save writer or pose override.
class BellwetherDoorwayProbe {
public:
    bool Start(const simulation::SimulationSnapshot& state) {
        *this = {};
        if (!Valid(state) || Distance(state, kPath.back()) > .1f) return false;
        generation_ = state.worldRoute.generation;
        phaseTick_ = state.tickIndex;
        station_ = kPath.size() - 1;
        phase_ = Phase::Room;
        return true;
    }

    simulation::InputSnapshot Next(const simulation::SimulationSnapshot& state) {
        simulation::InputSnapshot input;
        input.damageEnabled = false;
        input.tutorialEnabled = false;
        input.yawRadians = state.playerYawRadians;
        input.pitchRadians = -.11f;
        if (!Valid(state) || generation_ != state.worldRoute.generation || state.tickIndex < phaseTick_ ||
            state.tickIndex - phaseTick_ > 2400) phase_ = Phase::Failed;
        switch (phase_) {
        case Phase::Room:
            input.yawRadians = -.14f;
            if (state.tickIndex - phaseTick_ >= 600) {
                if (returns_ == 2) Enter(Phase::Complete, state);
                else {
                    --station_;
                    Enter(Phase::Outbound, state);
                }
            }
            break;
        case Phase::Outbound:
            if (Walk(state, input)) {
                if (station_ == 0) Enter(Phase::Square, state);
                else --station_;
            }
            break;
        case Phase::Square:
            input.yawRadians = 0.f;
            if (state.tickIndex - phaseTick_ >= 420) {
                ++station_;
                Enter(Phase::Inbound, state);
            }
            break;
        case Phase::Inbound:
            if (Walk(state, input)) {
                if (station_ + 1 == kPath.size()) {
                    ++returns_;
                    Enter(Phase::Room, state);
                } else ++station_;
            }
            break;
        default:
            input.paused = true;
            break;
        }
        return input;
    }

    bool Complete() const { return phase_ == Phase::Complete; }
    bool Failed() const { return phase_ == Phase::Failed; }
    unsigned Returns() const { return returns_; }
    std::size_t Station() const { return station_; }
    std::string_view PhaseName() const {
        switch (phase_) {
        case Phase::Room: return "room";
        case Phase::Outbound: return "doorway-outbound";
        case Phase::Square: return "square";
        case Phase::Inbound: return "doorway-inbound";
        case Phase::Complete: return "complete";
        default: return "failed";
        }
    }

private:
    enum class Phase { Room, Outbound, Square, Inbound, Complete, Failed };
    static constexpr auto kDoor = traversal::kBellwetherSouthDoorStation;
    static constexpr auto kApproach = traversal::kBellwetherSouthApproachStation;
    static constexpr auto kRoute = traversal::kBellwetherTavernRoute;
    static constexpr std::array kPath{kRoute[kApproach - 1], kRoute[kApproach],
        kRoute[kDoor], kRoute[kDoor + 1], traversal::BellwetherStation{83.97f,114.04f}};

    static bool Valid(const simulation::SimulationSnapshot& state) {
        using namespace dialogue;
        return state.developmentRescueJourney && state.rescue.exteriorSide && !state.rescue.equipmentStowed &&
            state.playerGrounded && !state.worldRoute.blocked && state.worldRoute.generation &&
            std::isfinite(state.playerX) && std::isfinite(state.playerZ) &&
            state.interaction.heldLightKind == interactions::HeldLightKind::RewardLantern &&
            state.chapterCompanion.grounded && state.chapterCompanion.bellwetherArrival == BellwetherArrivalStage::Seated &&
            state.tavernKeeper && state.tavernKeeper->stage == TavernKeeperStage::AtHearth &&
            state.chapterDialogue.line == Line::None &&
            state.chapterDialogue.tavernConversationLinesResolved == kTavernConversation.size();
    }
    static float Distance(const simulation::SimulationSnapshot& state, traversal::BellwetherStation target) {
        return std::hypot(target.x - state.playerX, target.z - state.playerZ);
    }
    bool Walk(const simulation::SimulationSnapshot& state, simulation::InputSnapshot& input) const {
        const auto target = kPath[station_];
        const auto distance = Distance(state, target);
        if (distance < .045f) return true;
        input.yawRadians = std::atan2(target.x - state.playerX, state.playerZ - target.z);
        input.moveForward = std::min(distance / .12f, 1.f);
        return false;
    }
    void Enter(Phase phase, const simulation::SimulationSnapshot& state) {
        phase_ = phase;
        phaseTick_ = state.tickIndex;
    }
    Phase phase_ = Phase::Failed;
    std::uint64_t generation_ = 0, phaseTick_ = 0;
    std::size_t station_ = 0;
    unsigned returns_ = 0;
};

} // namespace horde::gameplay::validation
