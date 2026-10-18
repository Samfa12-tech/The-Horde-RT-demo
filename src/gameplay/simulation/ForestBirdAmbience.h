#pragma once
#include "gameplay/simulation/SimulationSnapshot.h"

namespace horde::gameplay::simulation {
// Finite owl/evening-cricket candidates, pending listening/mix acceptance.
// Historical class/event names preserve the existing transport contract.
// Runtime-only cadence: no random source, save fields, backlog or mixer.
class ForestBirdAmbience {
public:
    enum class Cue { None, Play, Stop, PlayCrickets };
    static constexpr unsigned kInitialQuietTicks=720, kIntervalTicks=2400;
    static bool Eligible(const SimulationSnapshot& s) {
        const auto zone=s.worldRoute.current;
        return s.developmentRescueJourney && s.rescue.exteriorSide && !s.rescue.equipmentStowed &&
            !s.paused && s.playerAlive && s.playerGrounded && s.chapterDialogue.proofAccepted &&
            s.chapterDialogue.line==dialogue::Line::None &&
            (zone==WorldZoneId::ForestApproach || zone==WorldZoneId::Lookout) &&
            s.worldRoute.readiness[static_cast<unsigned>(zone)]==ZoneReadiness::Ready;
        // This authored campaign exterior is always the same night; legacy
        // dawn/showcase routes never satisfy developmentRescueJourney.
    }
    Cue Step(bool eligible) {
        if(!eligible){const bool stop=eligible_;Reset();return stop?Cue::Stop:Cue::None;}
        eligible_=true;
        if(++quietTicks_>=nextTicks_){quietTicks_=0;nextTicks_=kIntervalTicks;return Cue::Play;}
        // Eight-second cuts occupy one platform voice, twelve seconds apart.
        // Missing/late cues are dropped; they never queue or overlap.
        if(nextTicks_==kIntervalTicks && quietTicks_==kInitialQuietTicks)return Cue::PlayCrickets;
        return Cue::None;
    }
    void Reset(){eligible_=false;quietTicks_=0;nextTicks_=kInitialQuietTicks;}
private:
    unsigned quietTicks_=0,nextTicks_=kInitialQuietTicks;
    bool eligible_=false;
};
} // namespace horde::gameplay::simulation
