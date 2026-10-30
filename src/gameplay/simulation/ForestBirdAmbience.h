#pragma once
#include "gameplay/simulation/SimulationSnapshot.h"
#include "gameplay/simulation/ForestTreePlacementContract.h"
#include "scene/ChapterRegionalWater.h"
#include <algorithm>
#include <cmath>

namespace horde::gameplay::simulation {
// Finite owl/cricket/pond/foliage candidates, pending listening/mix acceptance.
// Historical class/event names preserve the existing transport contract.
// Runtime-only cadence: no random source, save fields, backlog or mixer.
class ForestBirdAmbience {
public:
    enum class Cue { None, Play, Stop, PlayCrickets, PlayPondFrogs, PlayFoliage };
    static constexpr unsigned kInitialQuietTicks=720, kIntervalTicks=2400;
    // Spare slot: first at 44 s, then every 40 s, clear of the eight-second
    // owl/pond/cricket cuts. A habitat/menu reset cannot cause an earlier repeat.
    static constexpr unsigned kFoliageSlotTicks=1920;
    static constexpr float kPondProximityMetres=18.f;
    static constexpr float kFoliageMaximumDistance=12.f, kFoliageReferenceDistance=4.f;
    static constexpr float kFoliageGain=.4f;
    // Existing route pine's lower crown. Y remains planar-mixer metadata;
    // source geometry, root/contact and collision are not altered.
    static constexpr traversal::Vec3 kFoliagePosition{
        kForestTreePlacementContract[7].x,4.55f,kForestTreePlacementContract[7].z};
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
    static bool FoliageEligible(const SimulationSnapshot& s) {
        return Eligible(s) && std::isfinite(s.playerX) && std::isfinite(s.playerZ) &&
            std::isfinite(s.playerSupportWorldY) &&
            std::hypot(s.playerX-kFoliagePosition.x,s.playerZ-kFoliagePosition.z)<kFoliageMaximumDistance;
    }
    static float FoliageRangeGain(const SimulationSnapshot& s) {
        if(!FoliageEligible(s))return 0;
        const auto distance=std::hypot(s.playerX-kFoliagePosition.x,s.playerZ-kFoliagePosition.z);
        return std::clamp((kFoliageMaximumDistance-distance)/4.f,0.f,1.f);
    }
    Cue Step(const SimulationSnapshot& s) {return Step(Eligible(s),PondEligible(s),FoliageEligible(s));}
    Cue Step(bool forestEligible,bool pondEligible=false,bool foliageEligible=false) {
        if(!forestEligible && !pondEligible){const bool stop=eligible_;Reset();return stop?Cue::Stop:Cue::None;}
        // Stop the current finite voice as soon as its own habitat becomes
        // ineligible, even if another exterior ambience remains available.
        if((activeCue_==Cue::PlayPondFrogs && !pondEligible) ||
           ((activeCue_==Cue::Play || activeCue_==Cue::PlayCrickets) && !forestEligible)) {
            Reset();return Cue::Stop;
        }
        if(activeCue_==Cue::PlayFoliage && !foliageEligible) {
            // Drop the finite voice without restarting the other nature slots.
            activeCue_=Cue::None;return Cue::Stop;
        }
        eligible_=true;
        if(++quietTicks_>=nextTicks_){quietTicks_=0;nextTicks_=kIntervalTicks;
            return activeCue_=forestEligible?Cue::Play:Cue::PlayPondFrogs;}
        // Eight-second cuts occupy one platform voice, twelve seconds apart.
        // Missing/late cues are dropped; they never queue or overlap.
        if(nextTicks_==kIntervalTicks && quietTicks_==kInitialQuietTicks)
            return activeCue_=pondEligible?Cue::PlayPondFrogs:Cue::PlayCrickets;
        if(nextTicks_==kIntervalTicks && quietTicks_==kFoliageSlotTicks && forestEligible && foliageEligible)
            return activeCue_=Cue::PlayFoliage;
        return Cue::None;
    }
    void Reset(){eligible_=false;quietTicks_=0;nextTicks_=kInitialQuietTicks;activeCue_=Cue::None;}
private:
    unsigned quietTicks_=0,nextTicks_=kInitialQuietTicks;
    bool eligible_=false;
    Cue activeCue_=Cue::None;
};
} // namespace horde::gameplay::simulation
