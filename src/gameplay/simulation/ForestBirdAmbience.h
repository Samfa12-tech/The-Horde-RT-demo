#pragma once
#include "gameplay/simulation/SimulationSnapshot.h"
#include "scene/ChapterRegionalWater.h"

namespace horde::gameplay::simulation {
// Finite owl/cricket/pond candidates, pending listening/mix acceptance.
// Historical class/event names preserve the existing transport contract.
// Runtime-only cadence: no random source, save fields, backlog or mixer.
class ForestBirdAmbience {
public:
    enum class Cue { None, Play, Stop, PlayCrickets, PlayPondFrogs };
    static constexpr unsigned kInitialQuietTicks=720, kIntervalTicks=2400;
    static constexpr float kPondProximityMetres=18.f;
    static bool ExteriorEligible(const SimulationSnapshot& s) {
        const auto zone=s.worldRoute.current;
        return s.developmentRescueJourney && s.rescue.exteriorSide && !s.rescue.equipmentStowed &&
            !s.paused && s.playerAlive && s.playerGrounded && s.chapterDialogue.proofAccepted &&
            s.chapterDialogue.line==dialogue::Line::None &&
            (zone==WorldZoneId::TombExterior || zone==WorldZoneId::ForestApproach || zone==WorldZoneId::Lookout) &&
            s.worldRoute.readiness[static_cast<unsigned>(zone)]==ZoneReadiness::Ready;
        // This authored campaign exterior is always the same night; legacy
        // dawn/showcase routes never satisfy developmentRescueJourney.
    }
    static bool Eligible(const SimulationSnapshot& s) {
        return ExteriorEligible(s) &&
            (s.worldRoute.current==WorldZoneId::ForestApproach || s.worldRoute.current==WorldZoneId::Lookout);
    }
    static bool PondEligible(const SimulationSnapshot& s) {
        return ExteriorEligible(s) && horde::scene::IsNearChapterRegionalWater(
            s.playerX,s.playerSupportWorldY,s.playerZ,kPondProximityMetres);
    }
    Cue Step(const SimulationSnapshot& s) {return Step(Eligible(s),PondEligible(s));}
    Cue Step(bool forestEligible,bool pondEligible=false) {
        if(!forestEligible && !pondEligible){const bool stop=eligible_;Reset();return stop?Cue::Stop:Cue::None;}
        // Stop the current finite voice as soon as its own habitat becomes
        // ineligible, even if another exterior ambience remains available.
        if((activeCue_==Cue::PlayPondFrogs && !pondEligible) ||
           ((activeCue_==Cue::Play || activeCue_==Cue::PlayCrickets) && !forestEligible)) {
            Reset();return Cue::Stop;
        }
        eligible_=true;
        if(++quietTicks_>=nextTicks_){quietTicks_=0;nextTicks_=kIntervalTicks;
            return activeCue_=forestEligible?Cue::Play:Cue::PlayPondFrogs;}
        // Eight-second cuts occupy one platform voice, twelve seconds apart.
        // Missing/late cues are dropped; they never queue or overlap.
        if(nextTicks_==kIntervalTicks && quietTicks_==kInitialQuietTicks)
            return activeCue_=pondEligible?Cue::PlayPondFrogs:Cue::PlayCrickets;
        return Cue::None;
    }
    void Reset(){eligible_=false;quietTicks_=0;nextTicks_=kInitialQuietTicks;activeCue_=Cue::None;}
private:
    unsigned quietTicks_=0,nextTicks_=kInitialQuietTicks;
    bool eligible_=false;
    Cue activeCue_=Cue::None;
};
} // namespace horde::gameplay::simulation
