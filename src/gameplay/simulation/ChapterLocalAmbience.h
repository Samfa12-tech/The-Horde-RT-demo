#pragma once
#include "gameplay/simulation/SimulationSnapshot.h"
#include "gameplay/simulation/ForestBirdAmbience.h"
#include "gameplay/effects/BellwetherHearth.h"
#include "gameplay/SpatialAudio.h"
#include "audio/LichRevivalAudio.h"
#include <algorithm>
#include <cmath>

namespace horde::gameplay::simulation {
// One finite incidental voice is shared with exterior nature cues on each
// platform. Only the fixed-step simulation schedules these local contacts.
class ChapterLocalAmbience {
public:
    enum class Cue { None, Stop, Drip, Rope, Hearth, Pig, Hen, Mill };
    struct Result { Cue cue=Cue::None; traversal::Vec3 position{}; float gain=0; };
    static constexpr unsigned kDripInitialTicks=360, kRopeMinimumTicks=72;
    static constexpr unsigned kHearthInitialTicks=105, kHearthIntervalTicks=540;
    static constexpr float kHearthRange=9.f;
    static constexpr unsigned kStockInitialTicks=30, kStockMinimumTicks=180;
    static constexpr float kStockRange=9.f;
    static constexpr float kMillRange=10.f;
    static constexpr traversal::Vec3 kMillAxle{
        bellwether::kMillWaterwheelAxle[0], bellwether::kMillWaterwheelAxle[1], bellwether::kMillWaterwheelAxle[2]};
    static constexpr traversal::Vec3 kHearthPosition{
        effects::kBellwetherHearthFlame[0], effects::kBellwetherHearthFlame[1], effects::kBellwetherHearthFlame[2]};
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
    static bool HearthEligible(const SimulationSnapshot& s) {
        return CommonEligible(s) && s.rescue.exteriorSide && !s.rescue.equipmentStowed && s.playerGrounded &&
            traversal::IsOnBellwetherTavernRoomFloor(s.playerX, s.playerSupportWorldY, s.playerZ) &&
            std::hypot(s.playerX - kHearthPosition.x, s.playerZ - kHearthPosition.z) < kHearthRange &&
            std::any_of(s.fireEmitters.begin(), s.fireEmitters.end(), [](const auto& emitter) {
                return emitter.stableId == effects::kBellwetherHearthEmitterId && emitter.strength > 0.f;
            });
    }
    static bool StockEligible(const SimulationSnapshot& s) {
        return GroundedExteriorEligible(s) && std::any_of(bellwether::kBellwetherStockHeadMounts.begin(),
            bellwether::kBellwetherStockHeadMounts.end(), [&](const auto& mount) {
                return StockSourceInRange(s,{mount[0],mount[1],mount[2]});
            });
    }
    static bool StockEventEligible(const SimulationSnapshot& s, const GameplayEvent& event) {
        return GroundedExteriorEligible(s) && StockSourceInRange(s,{event.worldX,event.worldY,event.worldZ});
    }
    static bool MillEligible(const SimulationSnapshot& s) {
        return GroundedExteriorEligible(s) && bellwether::IsMillWaterwheelOperating(s.millPreparation) &&
            std::abs(s.playerSupportWorldY-kMillAxle.y)<3.f &&
            std::hypot(s.playerX-kMillAxle.x,s.playerZ-kMillAxle.z)<kMillRange &&
            s.millWaterwheelAngleRadians>=0.f && s.millWaterwheelAngleRadians<bellwether::kMillWaterwheelTwoPi;
    }
    static bool Eligible(const SimulationSnapshot& s) {
        return DripEligible(s)||RopeEligible(s)||HearthEligible(s)||StockEligible(s)||MillEligible(s);
    }
    static constexpr GameplayEventType EventType(Cue cue) {
        switch(cue) {
        case Cue::Drip: return GameplayEventType::TombDrip;
        case Cue::Rope: return GameplayEventType::RopeContact;
        case Cue::Hearth: return GameplayEventType::BellwetherHearth;
        case Cue::Pig: return GameplayEventType::BellwetherPigGrunt;
        case Cue::Hen: return GameplayEventType::BellwetherHenCluck;
        case Cue::Mill: return GameplayEventType::BellwetherWaterwheelCreak;
        default: return GameplayEventType::ChapterLocalAmbienceStopped;
        }
    }
    Result Step(const SimulationSnapshot& s) {
        const bool identityChanged=seen_ && (retry_!=s.retryGeneration ||
            routeReset_!=s.lastConsumedRouteResetSequence || generation_!=s.rescue.generation || s.tickIndex<tick_);
        const bool stop=identityChanged && mode_!=Habitat::None;
        if(identityChanged) Reset();
        if(seen_ && s.tickIndex==tick_) return {};
        seen_=true;tick_=s.tickIndex;retry_=s.retryGeneration;
        routeReset_=s.lastConsumedRouteResetSequence;generation_=s.rescue.generation;
        const auto wanted=RopeEligible(s)?Habitat::Rope:DripEligible(s)?Habitat::Drip:HearthEligible(s)?Habitat::Hearth:
            StockEligible(s)?Habitat::Stock:MillEligible(s)?Habitat::Mill:Habitat::None;
        if(stop) { ClearCadence(); return {Cue::Stop}; }
        if(wanted!=mode_) {
            const bool leaving=mode_!=Habitat::None;
            ClearCadence();
            if(leaving) return {Cue::Stop};
            mode_=wanted;
        }
        if(mode_==Habitat::None) return {};
        if(mode_==Habitat::Drip) {
            if(++ticks_<nextDrip_) return {};
            ticks_=0;
            constexpr unsigned intervals[]{1080,1440,1260};
            nextDrip_=intervals[dripVariant_++%3];
            return {Cue::Drip,kDripPosition,.25f};
        }
        if(mode_==Habitat::Hearth) {
            if(++ticks_<nextHearth_) return {};
            ticks_=0;
            nextHearth_=kHearthIntervalTicks;
            return {Cue::Hearth,kHearthPosition,.42f};
        }
        if(mode_==Habitat::Stock) return StepStock(s);
        if(mode_==Habitat::Mill) return StepMill(s.millWaterwheelAngleRadians);
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
    enum class Habitat { None, Drip, Rope, Hearth, Stock, Mill };
    static bool GroundedExteriorEligible(const SimulationSnapshot& s) {
        return CommonEligible(s) && s.rescue.exteriorSide && !s.rescue.equipmentStowed && s.playerGrounded;
    }
    Result StepMill(float angle) {
        const float previous = millAngle_;
        millAngle_ = angle;
        // Entry or a discontinuous phase import rebases the contact clock.
        // Both loaded journal contacts lie away from the 2π wrap.
        if(previous<0.f || angle-previous>.02f) return {};
        constexpr float firstContact=.24f;
        constexpr float secondContact=firstContact+.5f*bellwether::kMillWaterwheelTwoPi;
        if(previous<firstContact && angle>=firstContact) return {Cue::Mill,kMillAxle,.65f};
        if(previous<secondContact && angle>=secondContact) return {Cue::Mill,kMillAxle,.49f};
        return {};
    }
    static bool StockSourceInRange(const SimulationSnapshot& s, traversal::Vec3 position) {
        const float x=s.playerX-position.x, z=s.playerZ-position.z;
        return std::abs(s.playerSupportWorldY-position.y)<2.f && x*x+z*z<kStockRange*kStockRange;
    }
    Result StepStock(const SimulationSnapshot& s) {
        stockQuietTicks_=std::min(stockQuietTicks_+1,kStockMinimumTicks);
        Result result;
        for(std::size_t i=0;i<s.stockHeadPoses.size();++i) {
            // Use the same fixed-step gesture already rendered at this mount.
            // Missed or suppressed onsets are never queued.
            const bool turning=std::abs(s.stockHeadPoses[i].yawRadians)>.06f;
            const bool started=turning && !stockTurning_[i];
            stockTurning_[i]=turning;
            if(!started || stockQuietTicks_<kStockMinimumTicks) continue;
            const auto& mount=bellwether::kBellwetherStockHeadMounts[i];
            const traversal::Vec3 position{mount[0],mount[1],mount[2]};
            if(!StockSourceInRange(s,position)) continue;
            stockQuietTicks_=0;
            result=i==0 ? Result{Cue::Pig,position,.72f} : Result{Cue::Hen,position,.82f};
        }
        return result;
    }
    void ClearCadence() { mode_=Habitat::None;ticks_=0;nextDrip_=kDripInitialTicks;nextHearth_=kHearthInitialTicks;
        dripVariant_=0;gripsSeen_=false;travel_=0;stockTurning_={};millAngle_=-1.f;
        stockQuietTicks_=kStockMinimumTicks-kStockInitialTicks; }
    Habitat mode_=Habitat::None;
    unsigned ticks_=0,nextDrip_=kDripInitialTicks,dripVariant_=0,nextHearth_=kHearthInitialTicks;
    unsigned stockQuietTicks_=kStockMinimumTicks-kStockInitialTicks;
    std::array<bool,bellwether::kStockHeadKinds.size()> stockTurning_{};
    bool seen_=false,gripsSeen_=false;
    std::uint64_t tick_=0,routeReset_=0,generation_=0;
    std::uint32_t retry_=0;
    float travel_=0;
    float millAngle_=-1.f;
    std::array<float,2> previousGrips_{};
};
inline bool IsStockAmbienceEvent(GameplayEventType type) {
    return type==GameplayEventType::BellwetherPigGrunt || type==GameplayEventType::BellwetherHenCluck;
}
inline bool IsBellwetherExteriorAmbienceEvent(GameplayEventType type) {
    return IsStockAmbienceEvent(type) || type==GameplayEventType::BellwetherWaterwheelCreak;
}
inline bool IsChapterAmbienceEvent(GameplayEventType type) {
    return type==GameplayEventType::ForestBird || type==GameplayEventType::ForestCrickets ||
        type==GameplayEventType::PondFrogs || type==GameplayEventType::ForestBirdStopped ||
        type==GameplayEventType::TombDrip || type==GameplayEventType::RopeContact ||
        type==GameplayEventType::ChapterLocalAmbienceStopped || type==GameplayEventType::ForestFoliage ||
        type==GameplayEventType::BellwetherHearth || IsBellwetherExteriorAmbienceEvent(type);
}
// Both platform transports use one event-time source/gain calculation. Other
// semantic events retain their existing 1/14 metre emitter contract.
inline SpatialAudioEmitter ChapterAmbienceEmitter(const GameplayEvent& event) {
    if(event.type==GameplayEventType::LichRevivalCry)return audio::LichRevivalEmitter(event);
    if(event.type==GameplayEventType::BellwetherHearth)
        return {event.worldX,event.worldZ,std::max(0.f,event.intensity),2.f,ChapterLocalAmbience::kHearthRange,event.worldY};
    if(IsBellwetherExteriorAmbienceEvent(event.type)) {
        const float range=IsStockAmbienceEvent(event.type)?ChapterLocalAmbience::kStockRange:ChapterLocalAmbience::kMillRange;
        const float distance=std::hypot(event.worldX-event.listenerX,event.worldZ-event.listenerZ);
        const float edgeGain=std::clamp((range-distance)/2.5f,0.f,1.f);
        return {event.worldX,event.worldZ,std::max(0.f,event.intensity)*edgeGain,4.f,
            range,event.worldY};
    }
    const bool foliage=event.type==GameplayEventType::ForestFoliage;
    return {event.worldX,event.worldZ,std::max(0.f,event.intensity),
        foliage?ForestBirdAmbience::kFoliageReferenceDistance:1.f,
        foliage?ForestBirdAmbience::kFoliageMaximumDistance:14.f,event.worldY};
}
inline SpatialAudioGains ChapterAmbienceGains(const GameplayEvent& event) {
    return CalculateSpatialAudio(ChapterAmbienceEmitter(event),
        {event.listenerX,event.listenerZ,event.listenerYawRadians,event.listenerY},
        IsBellwetherExteriorAmbienceEvent(event.type)?SpatialAudioDomain::ExteriorRoute:SpatialAudioDomain::DungeonRoute);
}
} // namespace horde::gameplay::simulation
