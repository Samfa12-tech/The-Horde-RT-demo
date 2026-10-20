#pragma once
#include "gameplay/simulation/SimulationSnapshot.h"
#include <algorithm>
#include <cmath>

namespace horde::gameplay::simulation {
// One finite incidental voice is shared with exterior nature cues on each
// platform. Only the fixed-step simulation schedules these local contacts.
class ChapterLocalAmbience {
public:
    enum class Cue { None, Stop, Drip, Rope };
    struct Result { Cue cue=Cue::None; traversal::Vec3 position{}; float gain=0; };
    static constexpr unsigned kDripInitialTicks=360, kRopeMinimumTicks=72;
    static constexpr float kRopeTravelMetres=.35f;
    // Within the existing waterfall wet footprint, at the west-side wet floor.
    static constexpr traversal::Vec3 kDripPosition{-3.4f,-.928f,-15.25f};
    static bool CommonEligible(const SimulationSnapshot& s) {
        return s.developmentRescueJourney && s.playerAlive && !s.paused &&
            s.chapterDialogue.line==dialogue::Line::None &&
            std::isfinite(s.playerX) && std::isfinite(s.playerZ) &&
            std::isfinite(s.playerSupportWorldY);
    }
    static bool DripEligible(const SimulationSnapshot& s) {
        return CommonEligible(s) && !s.rescue.exteriorSide && !s.rescue.equipmentStowed &&
            s.worldRoute.current==WorldZoneId::Dungeon && s.playerGrounded &&
            s.activeSkeletonCount==0 && IsWaterfallSkeletonRoom(s.playerX,s.playerZ) &&
            std::abs(s.playerSupportWorldY-kRouteFloorWorldY)<.25f &&
            std::hypot(s.playerX-kDripPosition.x,s.playerZ-kDripPosition.z)<=4.f;
    }
    static bool RopeEligible(const SimulationSnapshot& s) {
        return CommonEligible(s) && s.rescue.lanternClaimed && s.rescue.ropeReady &&
            s.rescue.equipmentStowed && s.rescue.ropeHandsActive &&
            (s.rescue.phase==traversal::Phase::Ascent || s.rescue.phase==traversal::Phase::PullUp ||
             s.rescue.phase==traversal::Phase::Descent || s.rescue.phase==traversal::Phase::Landing) &&
            std::all_of(s.rescue.grippingRopeNodeIndices.begin(),s.rescue.grippingRopeNodeIndices.end(),
                [](float v){return std::isfinite(v) && v>=0 && v<traversal::kRopeNodeCount;}) &&
            std::all_of(s.rescue.grippingHandTargets.begin(),s.rescue.grippingHandTargets.end(),
                [](const auto& v){return std::isfinite(v.x)&&std::isfinite(v.y)&&std::isfinite(v.z);});
    }
    static bool Eligible(const SimulationSnapshot& s) { return DripEligible(s)||RopeEligible(s); }
    Result Step(const SimulationSnapshot& s) {
        const bool identityChanged=seen_ && (retry_!=s.retryGeneration ||
            routeReset_!=s.lastConsumedRouteResetSequence || generation_!=s.rescue.generation || s.tickIndex<tick_);
        const bool stop=identityChanged && mode_!=Cue::None;
        if(identityChanged) Reset();
        if(seen_ && s.tickIndex==tick_) return {};
        seen_=true;tick_=s.tickIndex;retry_=s.retryGeneration;
        routeReset_=s.lastConsumedRouteResetSequence;generation_=s.rescue.generation;
        const auto wanted=RopeEligible(s)?Cue::Rope:DripEligible(s)?Cue::Drip:Cue::None;
        if(stop) { ClearCadence(); return {Cue::Stop}; }
        if(wanted!=mode_) {
            const bool leaving=mode_!=Cue::None;
            ClearCadence();
            if(leaving) return {Cue::Stop};
            mode_=wanted;
        }
        if(mode_==Cue::None) return {};
        if(mode_==Cue::Drip) {
            if(++ticks_<nextDrip_) return {};
            ticks_=0;
            constexpr unsigned intervals[]{1080,1440,1260};
            nextDrip_=intervals[dripVariant_++%3];
            return {Cue::Drip,kDripPosition,.25f};
        }
        const auto& indices=s.rescue.grippingRopeNodeIndices;
        if(!gripsSeen_) { previousGrips_=indices;gripsSeen_=true;return {}; }
        const float delta=(std::abs(indices[0]-previousGrips_[0])+
            std::abs(indices[1]-previousGrips_[1]))*traversal::kRopeSegmentRestLength;
        previousGrips_=indices;
        ticks_=std::min(ticks_+1,kRopeMinimumTicks);
        // Rebase a discontinuous hand import instead of converting it to debt.
        if(delta>.5f) { travel_=0;ticks_=0;return {}; }
        travel_=std::min(travel_+delta,kRopeTravelMetres);
        if(delta<=.00001f || travel_<kRopeTravelMetres || ticks_<kRopeMinimumTicks) return {};
        ticks_=0;travel_=0;
        const auto& a=s.rescue.grippingHandTargets[0];const auto& b=s.rescue.grippingHandTargets[1];
        return {Cue::Rope,{(a.x+b.x)*.5f,(a.y+b.y)*.5f,(a.z+b.z)*.5f},.18f};
    }
    void Reset() { seen_=false;ClearCadence(); }
private:
    void ClearCadence() { mode_=Cue::None;ticks_=0;nextDrip_=kDripInitialTicks;
        dripVariant_=0;gripsSeen_=false;travel_=0; }
    Cue mode_=Cue::None;
    unsigned ticks_=0,nextDrip_=kDripInitialTicks,dripVariant_=0;
    bool seen_=false,gripsSeen_=false;
    std::uint64_t tick_=0,routeReset_=0,generation_=0;
    std::uint32_t retry_=0;
    float travel_=0;
    std::array<float,2> previousGrips_{};
};
inline bool IsChapterAmbienceEvent(GameplayEventType type) {
    return type==GameplayEventType::ForestBird || type==GameplayEventType::ForestCrickets ||
        type==GameplayEventType::PondFrogs || type==GameplayEventType::ForestBirdStopped ||
        type==GameplayEventType::TombDrip || type==GameplayEventType::RopeContact ||
        type==GameplayEventType::ChapterLocalAmbienceStopped;
}
} // namespace horde::gameplay::simulation
