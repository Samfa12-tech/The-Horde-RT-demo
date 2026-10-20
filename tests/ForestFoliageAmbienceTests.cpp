#include "gameplay/simulation/ChapterLocalAmbience.h"
#include "gameplay/simulation/FixedStepRunner.h"
#include "platform/android/GameplayEventMetadata.h"

#include <algorithm>
#include <array>
#include <bit>
#include <cmath>
#include <cstdint>
#include <iostream>
#include <limits>
#include <utility>
#include <vector>

namespace {
using namespace horde::gameplay;
using namespace horde::gameplay::simulation;
using Ambience=ForestBirdAmbience;
using Cue=Ambience::Cue;
int checks=0,failures=0;
void Check(bool value,const char* message) {
    ++checks;
    if(!value) {++failures;std::cerr<<"FAIL: "<<message<<'\n';}
}
bool Near(float a,float b,float tolerance=.00001f) {return std::abs(a-b)<=tolerance;}
SimulationSnapshot Forest(float distance=0.f) {
    SimulationSnapshot s;
    s.developmentRescueJourney=true;s.playerAlive=s.playerGrounded=true;
    s.rescue.exteriorSide=true;s.rescue.equipmentStowed=false;
    s.chapterDialogue.proofAccepted=true;s.chapterDialogue.line=dialogue::Line::None;
    s.worldRoute.current=WorldZoneId::ForestApproach;
    s.worldRoute.readiness[static_cast<unsigned>(WorldZoneId::ForestApproach)]=ZoneReadiness::Ready;
    s.worldRoute.readiness[static_cast<unsigned>(WorldZoneId::Lookout)]=ZoneReadiness::Ready;
    s.playerX=Ambience::kFoliagePosition.x+distance;
    s.playerZ=Ambience::kFoliagePosition.z;s.playerSupportWorldY=1.f;
    return s;
}
SimulationSnapshot Suppressed(unsigned mode) {
    auto s=Forest();
    switch(mode) {
    case 0:s.developmentRescueJourney=false;break;
    case 1:s.rescue.exteriorSide=false;break;
    case 2:s.rescue.equipmentStowed=true;break;
    case 3:s.paused=true;break;
    case 4:s.playerAlive=false;break;
    case 5:s.playerGrounded=false;break;
    case 6:s.chapterDialogue.proofAccepted=false;break;
    case 7:s.chapterDialogue.line=dialogue::Line::ForestNight;break;
    case 8:s.worldRoute.current=WorldZoneId::Dungeon;break;
    case 9:s.worldRoute.readiness[static_cast<unsigned>(WorldZoneId::ForestApproach)]=ZoneReadiness::Preparing;break;
    case 10:s.playerX=std::numeric_limits<float>::quiet_NaN();break;
    case 11:s.playerZ=std::numeric_limits<float>::infinity();break;
    case 12:s.playerSupportWorldY=std::numeric_limits<float>::quiet_NaN();break;
    case 13:s.playerX=Ambience::kFoliagePosition.x+12.f;break;
    case 14:s.playerZ=Ambience::kFoliagePosition.z+12.01f;break;
    case 15:s.worldRoute.current=WorldZoneId::TombExterior;break;
    }
    return s;
}
void RangeAndEligibility() {
    Check(Ambience::FoliageEligible(Forest()),"ready same-night forest at the authored crown admits the finite rustle");
    Check(Ambience::kFoliagePosition.x==kForestTreePlacementContract[7].x &&
          Ambience::kFoliagePosition.z==kForestTreePlacementContract[7].z &&
          kForestTreePlacementContract[7].species==ForestTreeSpecies::Pine,
          "source XZ retains the existing route pine contract rather than following the listener");
    for(unsigned mode=0;mode<16;++mode) {
        const auto s=Suppressed(mode);
        Check(!Ambience::FoliageEligible(s),"legacy/life/dialogue/traversal/readiness/finite/range guards reject foliage");
        Check(Ambience::FoliageRangeGain(s)==0.f,"ineligible foliage always has zero source range gain");
    }
    for(const auto& pair:std::array<std::array<float,2>,7>{{
            {{0.f,1.f}},{{4.f,1.f}},{{8.f,1.f}},{{10.f,.5f}},
            {{11.f,.25f}},{{11.99f,.0025f}},{{12.f,0.f}}}}) {
        const auto s=Forest(pair[0]);const auto gain=Ambience::FoliageRangeGain(s);
        Check(Near(gain,pair[1]) && std::isfinite(gain) && gain>=0 && gain<=1,
              "outer four metres fade smoothly to silence at the strict twelve-metre boundary");
    }
    auto s=Forest(5.f);s.worldRoute.current=WorldZoneId::Lookout;
    Check(Ambience::FoliageEligible(s),"a ready exterior zone change does not remove an otherwise valid nearby source");
    s.playerSupportWorldY=500.f;
    Check(Ambience::FoliageEligible(s),"current source range is explicitly planar; metadata must not imply elevation attenuation");
}
void CadenceAndMissingSlots() {
    for(const unsigned rate:{15u,24u,30u,60u,120u,144u}) {
        Ambience ambience;FixedStepRunner runner;unsigned tick=0;
        std::vector<unsigned> owl,crickets,foliage;std::vector<std::pair<unsigned,Cue>> starts;
        for(unsigned frame=0;frame<rate*130;++frame)runner.Advance(1.0/rate,false,[&](float) {
            ++tick;const auto cue=ambience.Step(true,false,true);
            if(cue==Cue::Play)owl.push_back(tick);
            if(cue==Cue::PlayCrickets)crickets.push_back(tick);
            if(cue==Cue::PlayFoliage)foliage.push_back(tick);
            if(cue==Cue::Play || cue==Cue::PlayCrickets || cue==Cue::PlayFoliage)starts.push_back({tick,cue});
        });
        Check(tick==7800 && runner.OverrunCount()==0,"ordinary render groupings retain exactly 130 seconds of fixed ticks");
        Check(owl==std::vector<unsigned>({720,3120,5520}),"adding foliage leaves owl cadence unchanged");
        Check(crickets==std::vector<unsigned>({1440,3840,6240}),"adding foliage leaves cricket cadence unchanged");
        Check(foliage==std::vector<unsigned>({2640,5040,7440}),"foliage begins at 44 quiet seconds and repeats only every 40 seconds");
        bool separated=true;
        for(std::size_t i=1;i<starts.size();++i) {
            const unsigned previousDuration=starts[i-1].second==Cue::PlayFoliage?60u:480u;
            separated&=starts[i].first>starts[i-1].first+previousDuration;
        }
        Check(separated,"one-second foliage and eight-second species cuts occupy distinct finite voice slots");
    }
    Ambience missed;unsigned leaves=0;
    for(unsigned tick=1;tick<=2640;++tick)leaves+=missed.Step(true,false,false)==Cue::PlayFoliage;
    Check(leaves==0,"a source outside range consumes its slot without emitting");
    for(unsigned tick=2641;tick<5040;++tick)leaves+=missed.Step(true,false,true)==Cue::PlayFoliage;
    Check(leaves==0 && missed.Step(true,false,true)==Cue::PlayFoliage,
          "source reentry drops missed debt instead of playing or restarting immediately");

    Ambience sourceExit;
    for(unsigned tick=0;tick<2640;++tick)sourceExit.Step(true,false,true);
    Check(sourceExit.Step(true,false,false)==Cue::Stop,"leaving the active foliage source stops the shared finite voice");
    Check(sourceExit.Step(true,false,false)==Cue::None,"source-only loss emits one stop rather than a stop stream");
    unsigned nextOwl=0,newLeaves=0;
    for(unsigned tick=0;tick<479;++tick) {
        const auto cue=sourceExit.Step(true,false,true);
        nextOwl+=cue==Cue::Play;newLeaves+=cue==Cue::PlayFoliage;
    }
    Check(nextOwl==1 && newLeaves==0,"source-only loss preserves the existing owl cycle rather than restarting its twelve-second timer");

    Ambience zoneChange;auto s=Forest();
    for(unsigned tick=0;tick<2000;++tick)zoneChange.Step(s);
    s.worldRoute.current=WorldZoneId::Lookout;unsigned beforeSlot=0;
    for(unsigned tick=2000;tick<2639;++tick)beforeSlot+=zoneChange.Step(s)==Cue::PlayFoliage;
    Check(beforeSlot==0 && zoneChange.Step(s)==Cue::PlayFoliage,
          "ready forest/lookout transition retains cadence without an immediate enter cue");

    Ambience boundedCatchup;FixedStepRunner runner;unsigned catchupLeaves=0;
    for(unsigned tick=0;tick<2639;++tick)boundedCatchup.Step(true,false,true);
    runner.Advance(1000,false,[&](float){catchupLeaves+=boundedCatchup.Step(true,false,true)==Cue::PlayFoliage;});
    Check(catchupLeaves==1 && runner.TotalTicks()<=8 && runner.OverrunCount()==1,
          "a long render stall cannot turn skipped time into a backlog of foliage requests");
}
void LifecycleQuietRestart() {
    // Pure policy snapshots, not a claim of physically earned progress or output.
    for(const unsigned mode:{0u,1u,2u,3u,4u,5u,6u,7u,8u,9u,15u}) {
        Ambience ambience;const auto good=Forest();
        for(unsigned tick=0;tick<2640;++tick)ambience.Step(good);
        const auto invalid=Suppressed(mode);
        Check(ambience.Step(invalid)==Cue::Stop && ambience.Step(invalid)==Cue::None,
              "life/dialogue/pause/ownership/habitat loss stops once and clears transient cadence");
        unsigned premature=0;
        for(unsigned tick=0;tick<2639;++tick)premature+=ambience.Step(good)==Cue::PlayFoliage;
        Check(premature==0 && ambience.Step(good)==Cue::PlayFoliage,
              "resume or reentry requires a complete new quiet interval rather than replaying the last rustle");
    }
    Ambience explicitReset;
    for(unsigned tick=0;tick<2639;++tick)explicitReset.Step(true,false,true);
    explicitReset.Reset();unsigned replay=0;
    for(unsigned tick=0;tick<2639;++tick)replay+=explicitReset.Step(true,false,true)==Cue::PlayFoliage;
    Check(replay==0 && explicitReset.Step(true,false,true)==Cue::PlayFoliage,
          "explicit reset/load-like replacement cannot retain almost-due foliage debt");
    Ambience fresh;Check(fresh.Step(true,false,true)==Cue::None,"new session cannot start an incidental voice on its first tick");
}
void GainAndTransport() {
    static_assert(static_cast<unsigned>(GameplayEventType::ForestBird)==28);
    static_assert(static_cast<unsigned>(GameplayEventType::ForestBirdStopped)==29);
    static_assert(static_cast<unsigned>(GameplayEventType::ForestCrickets)==30);
    static_assert(static_cast<unsigned>(GameplayEventType::PondFrogs)==31);
    static_assert(static_cast<unsigned>(GameplayEventType::TombDrip)==32);
    static_assert(static_cast<unsigned>(GameplayEventType::RopeContact)==33);
    static_assert(static_cast<unsigned>(GameplayEventType::ChapterLocalAmbienceStopped)==34);
    static_assert(static_cast<unsigned>(GameplayEventType::ForestFoliage)==35 && kGameplayEventTypeCount==36);
    GameplayEvent event;event.type=GameplayEventType::ForestFoliage;
    event.sequence=0x12345678u;event.tickIndex=98765;
    event.worldX=Ambience::kFoliagePosition.x;event.worldZ=Ambience::kFoliagePosition.z;
    event.worldY=Ambience::kFoliagePosition.y;event.listenerY=2.125f;event.intensity=.4f;
    const auto metadata=horde::platform::android::PackGameplayEventMetadata(event);
    const auto vertical=horde::platform::android::PackGameplayEventVerticalMetadata(event);
    Check((metadata&255u)==35u && (metadata>>32u)==event.sequence && ((metadata>>24u)&255u)==0,
          "append-only foliage ID retains ordered compact transport without a fabricated equipment cue");
    Check(std::bit_cast<float>(static_cast<std::uint32_t>(vertical))==event.worldY &&
          std::bit_cast<float>(static_cast<std::uint32_t>(vertical>>32u))==event.listenerY,
          "foliage retains exact event-time source and listener height metadata");
    Check(IsChapterAmbienceEvent(event.type) && IsChapterAmbienceEvent(GameplayEventType::ForestBirdStopped) &&
          !IsChapterAmbienceEvent(GameplayEventType::PlayerDamaged),
          "the combined finite family includes foliage/stop while combat feedback stays separate");
    auto emitter=ChapterAmbienceEmitter(event);
    Check(emitter.referenceDistance==4.f && emitter.maximumDistance==12.f && emitter.gain==event.intensity &&
          emitter.x==event.worldX && emitter.z==event.worldZ && emitter.worldY==event.worldY,
          "both transports receive the source intensity once with the authored four/twelve metre spatial contract");
    const auto right=CalculateSpatialAudio(emitter,{event.worldX-4.f,event.worldZ,0.f,event.listenerY});
    const auto left=CalculateSpatialAudio(emitter,{event.worldX+4.f,event.worldZ,0.f,event.listenerY});
    Check(right.obstructed && left.obstructed && Near(right.left,left.right) && Near(right.right,left.left) &&
          Near(right.right,.4f*kObstructedAudioGain) && right.left<.00001f && left.right<.00001f,
          "planar pan preserves existing exterior obstruction attenuation without squaring the source gain");
    event.intensity=.2f;
    const auto half=CalculateSpatialAudio(ChapterAmbienceEmitter(event),{event.worldX-4.f,event.worldZ,0.f,event.listenerY});
    Check(Near(half.right,right.right*.5f),"halving an event intensity halves output rather than applying a second source multiplier");
    event.intensity=.4f;
    const auto farther=CalculateSpatialAudio(ChapterAmbienceEmitter(event),{event.worldX-8.f,event.worldZ,0.f,event.listenerY});
    const auto outside=CalculateSpatialAudio(ChapterAmbienceEmitter(event),{event.worldX-12.f,event.worldZ,0.f,event.listenerY});
    Check(Near(farther.right,right.right*.5f) && outside.left==0 && outside.right==0,
          "existing inverse-distance spatial attenuation is finite and becomes zero at maximum range");
    auto high=emitter;high.worldY=500.f;
    const auto samePlanar=CalculateSpatialAudio(high,{event.worldX-4.f,event.worldZ,0.f,-500.f});
    Check(samePlanar.left==right.left && samePlanar.right==right.right,
          "height remains metadata; this change cannot claim a new elevation mixer");
    for(unsigned type=0;type<kGameplayEventTypeCount;++type) {
        if(type==35)continue;
        event.type=static_cast<GameplayEventType>(type);
        const auto prior=ChapterAmbienceEmitter(event);
        Check(prior.referenceDistance==1.f && prior.maximumDistance==14.f && prior.gain==event.intensity,
              "every prior semantic event retains its original spatial distances and single source intensity");
    }
    event.type=GameplayEventType::ForestFoliage;event.intensity=-1.f;
    Check(ChapterAmbienceEmitter(event).gain==0,"negative incidental intensity cannot create output");
}
} // namespace

int main() {
    RangeAndEligibility();CadenceAndMissingSlots();LifecycleQuietRestart();GainAndTransport();
    std::cout<<checks<<" forest foliage policy/spatial/transport checks, "<<failures<<" failures\n";
    std::cout<<"Host scheduling and event contracts only; no actual listening, platform output or device acceptance.\n";
    return failures?1:0;
}
