#pragma once

#include <algorithm>
#include <array>
#include <cmath>
#include <cstddef>
#include <cstdint>
#include <optional>
#include <span>

#include "gameplay/simulation/DevelopmentWorldRoute.h"

namespace horde::gameplay::dialogue {
enum class Line : std::uint32_t { None, KeeperSense, KeeperCloser, Grate, RescueFound,
    RescueRope, ReunionQuestion, ReunionHint, ReunionProof, ReunionFirstPiece,
    ReunionDepart, ForestNight, ForestWaystone, ForestClue, ForestWait, ForestVillage,
    FlaskEmpty, FlaskTopup, TavernSit, TavernLimit, TavernWork, RoadStop, RoadAnkle, RoadSlow,
    TavernIntroduce, TavernHordeArmy, TavernHoardAnswer, TavernWelcome, TavernDirections,
    LanternFirstHelp, LanternFirstIdentity, LanternKingName, LanternFirstRequest, LanternRegisterReturn,
    RecordKeeperInspect, RecordKeeperErrand, RecordKeeperDirections,
    RecordKeeperRegister, RecordKeeperTwoRoads, RecordKeeperRoadOrder, TavernRegisterDirections, TavernExpeditionDirections,
    TavernLanternIdentity, TavernLanternSeals, KitVoiceReaction, KitVoiceListen, KitRoyalClaim, KitRoadHelp };
struct LineSpec { Line id; const char* stableId; const char* speaker; const char* text;
    const char* audio; float duration; unsigned priority; };
inline constexpr char kExpeditionDirections[]=
    "You can take either road first. The Abbey lies downstream. The Foundry is east, across the gorge.";
// The original thirteen Kit cuts retain their 11 October distribution grant.
// The later two flask-guidance cuts are separately pinned in their manifest.
inline constexpr std::array<LineSpec,48> kLines{{
    {Line::None,"","","","",0,0},
    {Line::KeeperSense,"keeper.sense","Keeper","I sense you","assets/audio/pixabay/keeper_i_sense_you.wav",2.590000f,3},
    {Line::KeeperCloser,"keeper.closer","Keeper","Come closer","assets/audio/pixabay/keeper_come_closer.wav",2.868479f,3},
    {Line::Grate,"prologue.kit_grate","Kit","Mate, are you ok? I heard the collapse! The treasure should be just ahead. Be careful!","assets/audio/kit/runtime/prologue.kit_grate.wav",6.469042f,1},
    {Line::RescueFound,"rescue.found","Kit","There you are. Still in one piece?","assets/audio/kit/runtime/rescue.found.wav",2.430458f,2},
    {Line::RescueRope,"rescue.rope","Kit","Stay clear. Rope coming down.","assets/audio/kit/runtime/rescue.rope.wav",2.622917f,2},
    {Line::ReunionQuestion,"reunion.question","Kit","Steady. Catch your breath. Did you find it?","assets/audio/kit/runtime/reunion.question.wav",3.107667f,1},
    {Line::ReunionHint,"reunion.hint","Kit","Let me see. Raise it.","assets/audio/kit/runtime/reunion.hint.wav",2.139708f,1},
    {Line::ReunionProof,"reunion.proof","Kit","Then we're not chasing a story anymore.","assets/audio/kit/runtime/reunion.proof.wav",2.601125f,1},
    {Line::ReunionFirstPiece,"reunion.first_piece","Kit","A start, then. Let's see where it leads.","assets/audio/kit/runtime/reunion.first_piece.wav",3.333625f,1},
    {Line::ReunionDepart,"reunion.depart","Kit","Come on. There's a fire and a dry seat waiting in the village.","assets/audio/kit/runtime/reunion.depart.wav",4.408417f,1},
    {Line::ForestNight,"forest.night","Kit","I'd forgotten how big the sky was.","assets/audio/kit/runtime/forest.night.wav",2.436708f,1},
    {Line::ForestWaystone,"forest.waystone","Kit","Hold it there. There are marks under the moss.","assets/audio/kit/runtime/forest.waystone.wav",2.978125f,1},
    {Line::ForestClue,"forest.clue","Kit","A road to the treasury, perhaps. Someone in the village might read it.","assets/audio/kit/runtime/forest.clue.wav",4.216542f,1},
    {Line::ForestWait,"forest.wait","Kit","I'll wait here.","assets/audio/kit/runtime/forest.wait.wav",1.589208f,1},
    {Line::ForestVillage,"forest.village","Kit","There. Chimney smoke.","assets/audio/kit/runtime/forest.village.wav",2.173792f,1},
    {Line::FlaskEmpty,"flask.empty","Kit","Is your flask empty? Fill it here.","assets/audio/kit/runtime/flask.empty.wav",2.419333f,1},
    {Line::FlaskTopup,"flask.topup","Kit","Don't forget to top your flask up here if you need it.","assets/audio/kit/runtime/flask.topup.wav",2.939333f,1},
    {Line::TavernSit,"kit.accept_base.sit","Kit","Right. I'm sitting down before I make this worse.","",3.379333f,1},
    {Line::TavernLimit,"kit.accept_base.limit","Kit","I won't get far along those roads like this. You'll have to go without me.","",4.739333f,1},
    {Line::TavernWork,"kit.accept_base.work","Kit","Bring what you find back here. I can still make myself useful.","",4.259333f,1},
    {Line::RoadStop,"kit.approach_injury.stop","Kit","Ah. Wait a moment.","",1.6f,1},
    {Line::RoadAnkle,"kit.approach_injury.ankle","Kit","Loose ground. Turned my ankle. I can stand, but give me a moment.","",4.25f,1},
    {Line::RoadSlow,"kit.approach_injury.slow","Kit","Stay close a moment. I can manage if we go slowly.","",3.4f,1},
    {Line::TavernIntroduce,"hub.kit_introduce","Kit","The tomb was there. So was something worth the climb.","",4.099333f,1},
    {Line::TavernHordeArmy,"hub.horde_army","Tavern keeper","My grandfather called the Horde an army. You treasure hunters always hear the other word.","",6.2f,1},
    {Line::TavernHoardAnswer,"hub.hoard_answer","Kit","Armies need paying. I'm hoping we're both right.","",3.939333f,1},
    {Line::TavernWelcome,"hub.arrival_chair","Tavern keeper","Easy. There's a chair just inside.","",2.6f,1},
    {Line::TavernDirections,"hub.record_keeper_lead","","Ask the record keeper by the northern church about the wayside shrine.","",7.0f,1},
    {Line::LanternFirstHelp,"lantern.first_help","Lantern voice","The lower stone. Its hinge is hidden under the roots.","",4.6f,2},
    {Line::LanternFirstIdentity,"lantern.first_identity","Lantern voice","I was king here. My keepers sealed me away.","",4.5f,2},
    {Line::LanternKingName,"lantern.king_name","Lantern voice","Veyr. If the name still means anything.","",3.8f,2},
    {Line::LanternFirstRequest,"lantern.first_request","Lantern voice","Three seals guard the treasury. Find them, and I can put an end to this.","",6.1f,2},
    {Line::LanternRegisterReturn,"shrine.recap_register","Lantern voice","Take the register back to Bellwether. It will help you find the seals.","",5.8f,2},
    {Line::RecordKeeperInspect,"hub.record_inspect","Record keeper","Bring the light closer. This mark belongs to the old treasury road.","",5.8f,1},
    {Line::RecordKeeperErrand,"hub.first_errand","Record keeper","The wayside shrine kept a road register. If it survived, it may name the next stops.","",6.6f,1},
    {Line::RecordKeeperDirections,"hub.shrine_direction","Record keeper","Take the northern path out of Bellwether. The shrine is on a short spur beyond it.","",6.4f,1},
    {Line::RecordKeeperRegister,"shrine.register","Record keeper","The Abbey. The royal Foundry. Then the Glass Court.","",5.0f,1},
    {Line::RecordKeeperTwoRoads,"hub.two_routes","Record keeper","We can trace the first two roads. The Court's route is still sealed.","",5.5f,1},
    {Line::RecordKeeperRoadOrder,"hub.routes_independent","Record keeper",kExpeditionDirections,"",7.4f,1},
    {Line::TavernRegisterDirections,"hub.show_register_note","","Show the road register to the record keeper by the northern church.","",6.0f,1},
    {Line::TavernExpeditionDirections,"hub.expedition_routes_note","",kExpeditionDirections,"",7.4f,1},
    {Line::TavernLanternIdentity,"lantern.town_identity","Lantern voice","I am Veyr. Your friend has found the road to my treasury.","",5.8f,2},
    {Line::TavernLanternSeals,"lantern.town_seals","Lantern voice","Three seals. The Abbey, the Foundry, and the Glass Court. Bring them together, and the road opens.","",8.0f,2},
    {Line::KitVoiceReaction,"kit.voice_reaction","Kit","Next time, a little warning before the lamp starts talking.","",5.3f,2},
    {Line::KitVoiceListen,"kit.first_direct_voice.finish","Kit","Go on, then. I'm listening.","",3.3f,2},
    {Line::KitRoyalClaim,"kit.royal_claim.king","Kit","King Veyr. That's quite a claim to make from inside a lamp.","",5.5f,2},
    {Line::KitRoadHelp,"kit.royal_claim.help","Kit","If you know the roads, help us get there and back. We can start with that.","",6.8f,2}
}};
inline constexpr std::array kHearthLines{Line::TavernSit,Line::TavernLimit,Line::TavernWork};
inline constexpr float kHearthWorkPauseSeconds=.55f;
inline constexpr bool IsHearthLine(Line line) {return line>=Line::TavernSit && line<=Line::TavernWork;}
inline constexpr bool IsRoadLine(Line line) {return line>=Line::RoadStop && line<=Line::RoadSlow;}
inline constexpr std::array kRoadLines{Line::RoadStop,Line::RoadAnkle,Line::RoadSlow};
inline constexpr std::array kTavernConversation{Line::TavernIntroduce,Line::TavernHordeArmy,Line::TavernHoardAnswer};
inline constexpr bool IsTavernConversationLine(Line line) {
    return line>=kTavernConversation.front() && line<=kTavernConversation.back();
}
inline constexpr bool IsGuidanceLine(Line line) {
    return line==Line::TavernDirections || line==Line::TavernRegisterDirections || line==Line::TavernExpeditionDirections;
}
enum class TavernGuidance { Shrine, Register, Expeditions };
inline constexpr std::array kShrineReveal{Line::LanternFirstIdentity,Line::LanternKingName,Line::LanternFirstRequest};
enum class ShrineSpeech { Help, Reveal, Recap, RegisterReturn };
inline constexpr bool IsShrineLanternLine(Line line) {
    return line>=Line::LanternFirstHelp && line<=Line::LanternRegisterReturn;
}
inline constexpr std::array kRecordKeeperLead{Line::RecordKeeperInspect,Line::RecordKeeperErrand,Line::RecordKeeperDirections};
inline constexpr std::array kRecordKeeperReading{Line::RecordKeeperRegister,Line::RecordKeeperTwoRoads,Line::RecordKeeperRoadOrder};
enum class RecordKeeperAction : std::uint8_t { None, ShowRoadMark, ShowRegister, RecallRoutes };
inline constexpr bool IsRecordKeeperLine(Line line) {
    return line>=kRecordKeeperLead.front() && line<=kRecordKeeperReading.back();
}
inline constexpr std::array kTavernLanternExchange{Line::TavernLanternIdentity,Line::TavernLanternSeals,
    Line::KitVoiceReaction,Line::KitVoiceListen,Line::KitRoyalClaim,Line::KitRoadHelp};
inline constexpr bool IsTavernLanternLine(Line line) {
    return line>=kTavernLanternExchange.front() && line<=kTavernLanternExchange.back();
}
inline constexpr bool IsTavernLanternVoice(Line line) {
    return line==Line::TavernLanternIdentity || line==Line::TavernLanternSeals;
}
inline constexpr bool IsBellwetherLine(Line line) {
    return IsHearthLine(line) || IsRoadLine(line) || IsTavernConversationLine(line) ||
        line==Line::TavernWelcome || IsGuidanceLine(line) || IsShrineLanternLine(line) ||
        IsRecordKeeperLine(line) || IsTavernLanternLine(line);
}
inline const LineSpec& Spec(Line line) { const auto n=static_cast<unsigned>(line);
    return kLines[n<kLines.size()?n:0]; }
inline bool RequiresChapterCaption(Line line) {
    return IsBellwetherLine(line) && Spec(line).audio[0]=='\0';
}
struct Tuple { float x=0,y=0,z=0; };
inline constexpr float kCompanionSpeechHeight=1.4f;
struct Snapshot {
    Line line=Line::None; std::uint64_t generation=1; float elapsed=0,duration=0;
    Tuple source{},listener{}; float listenerYaw=0;
    bool paused=false,grateConsumed=false,rescueConsumed=false,reunionStarted=false;
    bool waitingForRaise=false,proofAccepted=false,reminderConsumed=false;
    bool forestNightConsumed=false,forestWaystoneConsumed=false;
    bool forestClueConsumed=false,forestVillageConsumed=false;
    unsigned hearthLinesResolved=0,tavernConversationLinesResolved=0;
    unsigned tavernLanternLinesStarted=0;
    // A labelled companion fixture contract, not a rendered/admitted Kit actor.
    bool companionVisualAdmitted=false;
};
struct Context {
    Tuple player{},listener{}; float yaw=0;
    // Current grounded companion root plus speech height, owned by simulation.
    // Absence retains the last valid source while the actor is unavailable.
    std::optional<Tuple> companionSource;
    std::optional<Tuple> tavernKeeperSource;
    std::optional<Tuple> shrineLanternSource;
    std::optional<Tuple> recordKeeperSource;
    std::optional<Tuple> tavernLanternSource;
    bool recordKeeperInReach=false;
    bool paused=false,alive=true,combat=false,claimed=false,exterior=false,traversing=false;
    bool lanternLow=false,lanternHigh=false;
    bool flaskGuideEnabled=false; unsigned flaskCharges=0;
    bool companionAtBasin=false,playerNearBasin=false;
    bool tavernInReach=false, kitAtChair=false, kitSeated=false;
    bool keeperAtHearth=false, tavernDoorInReach=false, kitAtSouthDoor=false;
    bool roadInReach=false, kitCaughtAnkle=false, kitReadyForProtectiveWalk=false;
    std::uint64_t manualRaiseSequence=0,visibleRaiseSequence=0;
};
// Logical story recovery only: active speech restarts, with no audio clock,
// callback identity, presentation receipt or renderer state crossing sessions.
struct Progress {
    Line activeLine=Line::None;
    std::array<Line,8> continuations{};
    std::size_t continuationCount=0;
    bool grateConsumed=false,rescueConsumed=false,reunionStarted=false;
    bool waitingForRaise=false,proofAccepted=false,reminderConsumed=false;
    bool forestNightConsumed=false,forestWaystoneConsumed=false;
    bool forestClueConsumed=false,forestVillageConsumed=false;
    unsigned hearthLinesResolved=0,tavernConversationLinesResolved=0;
    unsigned tavernLanternLinesStarted=0;
};
class Director {
public:
    const Snapshot& State() const { return state_; }
    bool CanOfferTavernLantern(const Context& c) const {
        return state_.hearthLinesResolved==kHearthLines.size() &&
            state_.tavernLanternLinesStarted<kTavernLanternExchange.size() &&
            state_.line==Line::None && continuationCount_==0 && !tavernConversationActive_ &&
            CanHearTavernLantern(c);
    }
    bool CanStartTavernConversation(const Context& c) const {
        return !tavernConversationActive_ && state_.line==Line::None && continuationCount_==0 &&
            state_.hearthLinesResolved==kHearthLines.size() &&
            state_.tavernConversationLinesResolved<kTavernConversation.size() && CanHearTavernConversation(c);
    }
    bool TryStartTavernConversation(const Context& c) {
        if(!CanStartTavernConversation(c))return false;
        tavernConversationActive_=true;
        StartNextTavernLine(c);
        return true;
    }
    bool CanShowTavernDirections(const Context& c) const {
        return state_.line==Line::None && continuationCount_==0 &&
            state_.tavernConversationLinesResolved==kTavernConversation.size() && AtTavernHearth(c);
    }
    void ShowTavernDirections(const Context& c,TavernGuidance guidance=TavernGuidance::Shrine) {
        if(!CanShowTavernDirections(c))return;
        constexpr std::array notes{Line::TavernDirections,Line::TavernRegisterDirections,Line::TavernExpeditionDirections};
        Start(notes[static_cast<std::size_t>(guidance)],*c.companionSource,c);
    }
    bool TryStartShrineSpeech(ShrineSpeech speech,const Context& c) {
        if(state_.line!=Line::None || continuationCount_!=0 || shrineRevealIndex_<kShrineReveal.size() ||
           !CanHearShrineLantern(c))return false;
        constexpr std::array starts{Line::LanternFirstHelp,Line::LanternFirstIdentity,Line::LanternFirstRequest,
            Line::LanternRegisterReturn};
        Start(starts[static_cast<std::size_t>(speech)],*c.shrineLanternSource,c);
        if(speech==ShrineSpeech::Reveal)shrineRevealIndex_=1;
        return true;
    }
    bool CanStartRecordKeeper(const Context& c) const {
        return c.recordKeeperInReach && CanHearRecordKeeper(c) && state_.forestClueConsumed &&
            state_.hearthLinesResolved==kHearthLines.size() && state_.line==Line::None &&
            continuationCount_==0 && recordKeeperPending_.empty();
    }
    bool TryStartRecordKeeper(RecordKeeperAction action,const Context& c) {
        if(action==RecordKeeperAction::None || !CanStartRecordKeeper(c))return false;
        recordKeeperPending_=action==RecordKeeperAction::ShowRoadMark?kRecordKeeperLead:kRecordKeeperReading;
        StartNextRecordKeeperLine(c);
        return true;
    }
    bool HoldingAnkleAssessment() const {
        return state_.line==Line::RoadStop || state_.line==Line::RoadAnkle ||
            (roadLinesResolved_==kRoadLines.size()-1 && state_.line!=Line::RoadSlow);
    }
    bool BasinGuidePending() const {
        if(state_.line==Line::FlaskEmpty || state_.line==Line::FlaskTopup)return true;
        for(std::size_t i=0;i<continuationCount_;++i)
            if(continuations_[i]==Line::FlaskEmpty || continuations_[i]==Line::FlaskTopup)return true;
        return false;
    }
    // Continuous Kit speech follows the same actor/listener snapshot as the
    // renderer. Event-time Keeper and unseen tomb/shaft calls stay unchanged.
    // Spatial refresh never advances the line, cursor, queue or callback ID.
    void RefreshCompanionSpatial(const Context& c) {
        if(!c.paused && c.alive) BindCompanionSpatial(c);
    }
    Progress ExportProgress() const {
        Progress p;
        p.activeLine=IsBellwetherLine(state_.line) ? Line::None : state_.line;
        p.continuations=continuations_;p.continuationCount=continuationCount_;
        p.grateConsumed=state_.grateConsumed;p.rescueConsumed=state_.rescueConsumed;
        p.reunionStarted=state_.reunionStarted;p.waitingForRaise=state_.waitingForRaise;
        p.proofAccepted=state_.proofAccepted;p.reminderConsumed=state_.reminderConsumed;
        p.forestNightConsumed=state_.forestNightConsumed;
        p.forestWaystoneConsumed=state_.forestWaystoneConsumed;
        p.forestClueConsumed=state_.forestClueConsumed;p.forestVillageConsumed=state_.forestVillageConsumed;
        p.hearthLinesResolved=state_.hearthLinesResolved;
        p.tavernConversationLinesResolved=state_.tavernConversationLinesResolved;
        p.tavernLanternLinesStarted=state_.tavernLanternLinesStarted;
        return p;
    }
    static bool ValidateProgress(const Progress& p) {
        const auto recoverable=[](Line line) {
            // Keeper speech belongs to a live combat emitter, not either safe
            // campaign boundary. ForestWait currently has no logical trigger.
            return static_cast<std::uint32_t>(line)<kLines.size() &&
                line!=Line::KeeperSense && line!=Line::KeeperCloser && line!=Line::ForestWait &&
                !IsBellwetherLine(line);
        };
        if(!recoverable(p.activeLine) || p.continuationCount>p.continuations.size() ||
           p.hearthLinesResolved>kHearthLines.size() || (p.hearthLinesResolved && !p.forestVillageConsumed) ||
           p.tavernConversationLinesResolved>kTavernConversation.size() ||
           p.tavernLanternLinesStarted>kTavernLanternExchange.size() ||
           (p.tavernLanternLinesStarted && p.hearthLinesResolved!=kHearthLines.size()) ||
           (p.tavernConversationLinesResolved && p.hearthLinesResolved!=kHearthLines.size()) ||
           (p.waitingForRaise && (!p.reunionStarted || p.proofAccepted)) ||
           (p.reunionStarted && !p.waitingForRaise && !p.proofAccepted) ||
           (p.proofAccepted && !p.reunionStarted) ||
           (p.reunionStarted && !p.rescueConsumed) ||
           (p.reminderConsumed && !p.reunionStarted) ||
           ((p.forestNightConsumed||p.forestWaystoneConsumed||p.forestClueConsumed||
             p.forestVillageConsumed) && !p.proofAccepted) ||
           (p.forestWaystoneConsumed && !p.forestNightConsumed) ||
           (p.forestClueConsumed && !p.forestWaystoneConsumed) ||
           (p.forestVillageConsumed && !p.forestClueConsumed)) return false;
        const auto validStory=[&](Line line,bool queued) {
            switch(line) {
            case Line::RescueFound: case Line::RescueRope: return p.rescueConsumed;
            case Line::ReunionQuestion: return p.reunionStarted;
            case Line::ReunionHint: return p.reunionStarted&&p.reminderConsumed;
            case Line::ReunionProof: case Line::ReunionFirstPiece: case Line::ReunionDepart:
                return p.proofAccepted;
            case Line::ForestNight: return p.proofAccepted && (!queued||!p.forestNightConsumed);
            case Line::ForestWaystone: return p.proofAccepted && (!queued||!p.forestWaystoneConsumed);
            case Line::ForestClue: return p.proofAccepted && (!queued||!p.forestClueConsumed);
            case Line::ForestVillage: return p.proofAccepted && (!queued||!p.forestVillageConsumed);
            case Line::FlaskEmpty: case Line::FlaskTopup: return p.proofAccepted;
            case Line::Grate: return !p.grateConsumed;
            default: return true;
            }
        };
        if(!validStory(p.activeLine,false)) return false;
        unsigned basinCues=(p.activeLine==Line::FlaskEmpty || p.activeLine==Line::FlaskTopup)?1u:0u;
        unsigned previous=static_cast<unsigned>(p.activeLine);
        for(std::size_t i=0;i<p.continuations.size();++i) {
            const auto line=p.continuations[i];
            if(i>=p.continuationCount) {if(line!=Line::None) return false;continue;}
            // Only generated continuation identities are admitted, in their
            // existing story order; duplicates cannot replay completed speech.
            const auto order=static_cast<unsigned>(line);
            if(!recoverable(line) || line==Line::None || line==Line::Grate ||
               line==Line::RescueFound || line==Line::ReunionHint || line==p.activeLine ||
               order<=previous || !validStory(line,true)) return false;
            if((line==Line::FlaskEmpty || line==Line::FlaskTopup) && ++basinCues>1u) return false;
            previous=order;
        }
        // These responses are committed together at proof acceptance. An
        // interrupted response must retain its remaining suffix; otherwise a
        // superficially valid record would silently drop required dialogue.
        const auto hasSuffix=[&](Line line,std::size_t next) {
            if(line==Line::ReunionProof) {
                if(next>=p.continuationCount || p.continuations[next]!=Line::ReunionFirstPiece) return false;
                ++next;
            }
            if(line==Line::ReunionProof || line==Line::ReunionFirstPiece)
                return next<p.continuationCount && p.continuations[next]==Line::ReunionDepart;
            return true;
        };
        if(!hasSuffix(p.activeLine,0)) return false;
        for(std::size_t i=0;i<p.continuationCount;++i)
            if(!hasSuffix(p.continuations[i],i+1)) return false;
        return true;
    }
    bool RestoreProgress(const Progress& p,const Context& c,
                         std::uint64_t generationFloor,std::uint64_t raiseFloor) {
        const auto finite=[](Tuple t) {return std::isfinite(t.x)&&std::isfinite(t.y)&&std::isfinite(t.z);};
        // Leave room for Start's second identity advance and reject saturation
        // instead of allowing an old completion to identify a restored line.
        const auto floor=std::max(state_.generation,generationFloor);
        if(!ValidateProgress(p) || floor>=UINT64_MAX-1 || !finite(c.player) ||
           !finite(c.listener) || !std::isfinite(c.yaw) ||
           (c.companionSource && !finite(*c.companionSource)) || !c.alive || c.traversing) return false;
        state_={};state_.generation=Next(floor);state_.paused=c.paused;
        state_.grateConsumed=p.grateConsumed;state_.rescueConsumed=p.rescueConsumed;
        state_.reunionStarted=p.reunionStarted;state_.waitingForRaise=p.waitingForRaise;
        state_.proofAccepted=p.proofAccepted;state_.reminderConsumed=p.reminderConsumed;
        state_.forestNightConsumed=p.forestNightConsumed;
        state_.forestWaystoneConsumed=p.forestWaystoneConsumed;
        state_.forestClueConsumed=p.forestClueConsumed;state_.forestVillageConsumed=p.forestVillageConsumed;
        state_.hearthLinesResolved=p.hearthLinesResolved;hearthPauseSeconds_=0;roadLinesResolved_=0;
        state_.tavernConversationLinesResolved=p.tavernConversationLinesResolved;
        state_.tavernLanternLinesStarted=p.tavernLanternLinesStarted;
        tavernLanternActive_=false;
        tavernLanternRaiseFloor_=c.manualRaiseSequence;
        tavernConversationActive_=tavernWelcomeStarted_=false;
        shrineRevealIndex_=kShrineReveal.size();
        recordKeeperPending_={};
        pending_=Line::None;continuations_=p.continuations;continuationCount_=p.continuationCount;
        raiseFloor_=std::max(raiseFloor,c.manualRaiseSequence);waitingSeconds_=0;
        if(p.activeLine!=Line::None) {
            const Tuple source=p.activeLine==Line::Grate?Tuple{2.6f,.30f,-8.6f}:
                (p.activeLine==Line::RescueFound||p.activeLine==Line::RescueRope?
                 Tuple{-33.7f,2.8f,-12.8f}:SourceFor(p.activeLine));
            Start(p.activeLine,source,c);
        }
        return true;
    }
    // Pause/surface reflow preserves the active line and its bounded queue;
    // reconstruction/reset still cancels and advances the generation.
    void Pause() { state_.paused=true; }
    void Reset(bool preserveOnce=false) {
        const auto old=state_;const auto oldRaiseFloor=raiseFloor_;
        const auto oldContinuations=continuations_;const auto oldContinuationCount=continuationCount_;
        state_={}; state_.generation=Next(old.generation);
        if(preserveOnce) { state_.grateConsumed=old.grateConsumed;state_.rescueConsumed=old.rescueConsumed;
            state_.proofAccepted=old.proofAccepted;state_.reminderConsumed=old.reminderConsumed;
            state_.reunionStarted=old.reunionStarted;state_.waitingForRaise=old.waitingForRaise;
            state_.forestNightConsumed=old.forestNightConsumed;
            state_.forestWaystoneConsumed=old.forestWaystoneConsumed;
            state_.forestClueConsumed=old.forestClueConsumed;
            state_.forestVillageConsumed=old.forestVillageConsumed;
            state_.hearthLinesResolved=old.hearthLinesResolved;
            state_.tavernConversationLinesResolved=old.tavernConversationLinesResolved;
            state_.tavernLanternLinesStarted=old.tavernLanternLinesStarted; }
        tavernLanternActive_=false;
        tavernLanternRaiseFloor_=0;
        tavernConversationActive_=false;
        shrineRevealIndex_=kShrineReveal.size();
        recordKeeperPending_={};
        if(!preserveOnce)tavernWelcomeStarted_=false;
        pending_=Line::None; continuations_={}; continuationCount_=0;
        if(preserveOnce) { continuations_=oldContinuations;continuationCount_=oldContinuationCount; }
        if(!preserveOnce)roadLinesResolved_=0;
        else if(IsRoadLine(old.line))roadLinesResolved_=kRoadLines.size();
        raiseFloor_=preserveOnce?oldRaiseFloor:0; waitingSeconds_=hearthPauseSeconds_=0;
    }
    // Lifecycle cancellation invalidates callbacks but retains logical once state.
    void Suspend() { if(state_.line==Line::Grate) state_.grateConsumed=true;
        if(IsRoadLine(state_.line))roadLinesResolved_=kRoadLines.size();
        Cancel(); pending_=Line::None; }
    bool Complete(Line line,std::uint64_t generation) {
        if(line==Line::None || state_.line!=line || state_.generation!=generation) return false;
        Finish(); return true;
    }
    void Skip() { if(state_.line!=Line::None) Finish(); }
    void Request(Line line,Tuple source,const Context& c) {
        if(line==Line::None || static_cast<std::uint32_t>(line)>=kLines.size() || IsBellwetherLine(line)) return;
        if(state_.line!=Line::None && Spec(line).priority<=Spec(state_.line).priority &&
            !(line==Line::ReunionProof && (state_.line==Line::ReunionQuestion||state_.line==Line::ReunionHint))) {
            if(line==Line::RescueRope) Queue(line);
            return;
        }
        if(state_.line!=Line::None) { if(state_.line==Line::Grate) state_.grateConsumed=true;Cancel(false); }
        Start(line,source,c);
    }
    void Step(float seconds,const Context& c) {
        state_.paused=c.paused; if(c.paused) return;
        const float dt=std::isfinite(seconds)?std::clamp(seconds,0.0f,0.05f):0;
        if(!c.alive) { Suspend(); return; }
        const bool hearsHearth=c.tavernInReach && c.exterior && !c.traversing && !c.combat && c.companionSource.has_value();
        const bool hearsRoad=c.roadInReach && c.exterior && !c.traversing && !c.combat && c.companionSource.has_value();
        const bool hearsWelcome=c.tavernDoorInReach && c.exterior && !c.traversing && !c.combat &&
            c.tavernKeeperSource && Finite(*c.tavernKeeperSource);
        if(state_.line==Line::TavernWelcome && !hearsWelcome)Cancel(false);
        if(IsGuidanceLine(state_.line) && !AtTavernHearth(c))Cancel(false);
        const bool hearsShrine=CanHearShrineLantern(c);
        if(!hearsShrine && (IsShrineLanternLine(state_.line) || shrineRevealIndex_<kShrineReveal.size()))
            Cancel(false);
        const bool hearsRecordKeeper=CanHearRecordKeeper(c);
        if(!hearsRecordKeeper && (IsRecordKeeperLine(state_.line) || !recordKeeperPending_.empty()))
            Cancel(false);
        if(tavernLanternActive_ && (!CanHearTavernLantern(c) || !c.lanternHigh))Cancel(false);
        if(tavernConversationActive_ && !CanHearTavernConversation(c)) {
            if(IsTavernConversationLine(state_.line))Cancel(false);
            tavernConversationActive_=false;
        }
        if(IsHearthLine(state_.line) && !hearsHearth) Cancel(false);
        if(!hearsRoad) {
            if(IsRoadLine(state_.line)) Cancel(false);
            if(roadLinesResolved_ || c.kitCaughtAnkle)roadLinesResolved_=kRoadLines.size();
        }
        const float dx=c.player.x-2.6f,dz=c.player.z+8.6f;
        const bool audible=dx*dx+dz*dz<=16.0f && !c.exterior;
        if((c.combat || !audible) && state_.line==Line::Grate) {
            // Passing out of range unheard permits return; combat consumes.
            if(c.combat) state_.grateConsumed=true;
            Cancel();
        }
        // Active Grate ticks can leave a pending local request. Completion or
        // skip consumes that call; discard its request before it can restart.
        if(c.claimed || state_.grateConsumed) pending_=Line::None;
        if(!state_.grateConsumed && !c.claimed && audible) pending_=Line::Grate;
        if(!audible) pending_=Line::None;
        if(pending_==Line::Grate && !c.combat && state_.line==Line::None) {
            Start(Line::Grate,{2.6f,.30f,-8.6f},c);pending_=Line::None;
        }
        if(c.claimed && !state_.rescueConsumed && state_.line==Line::None) {
            state_.rescueConsumed=true; Start(Line::RescueFound,{-33.7f,2.8f,-12.8f},c);
            Queue(Line::RescueRope);
        }
        if(c.exterior && !c.traversing && c.lanternLow && !state_.reunionStarted) {
            state_.reunionStarted=true;state_.waitingForRaise=true;raiseFloor_=c.manualRaiseSequence;
            // The simulation binds a grounded Kit actor before publication.
            Request(Line::ReunionQuestion,{-32.0f,3.6f,-10.0f},c);
            if(state_.line!=Line::ReunionQuestion) Queue(Line::ReunionQuestion);
        }
        if(state_.waitingForRaise && c.exterior && !c.traversing) {
            waitingSeconds_+=dt;
            if(c.lanternHigh && c.manualRaiseSequence>raiseFloor_ &&
                c.visibleRaiseSequence==c.manualRaiseSequence) {
                state_.proofAccepted=true;state_.waitingForRaise=false;
                Request(Line::ReunionProof,{-32.0f,3.6f,-10.0f},c);
                if(state_.line!=Line::ReunionProof) Queue(Line::ReunionProof);
                Queue(Line::ReunionFirstPiece);Queue(Line::ReunionDepart);
                if(c.flaskGuideEnabled) Queue(c.flaskCharges==0?Line::FlaskEmpty:Line::FlaskTopup);
            } else if(waitingSeconds_>=8 && !state_.reminderConsumed && state_.line==Line::None) {
                state_.reminderConsumed=true;Request(Line::ReunionHint,{-32.0f,3.6f,-10.0f},c);
            }
        }
        StepForestFixture(c);
        if(state_.line!=Line::None) {
            state_.elapsed+=dt;
            if(state_.elapsed>=state_.duration) Finish();
        }
        if(state_.line==Line::None && continuationCount_!=0) {
            const auto next=continuations_[0];
            if((next!=Line::FlaskEmpty && next!=Line::FlaskTopup) ||
               (c.companionAtBasin && c.playerNearBasin)) {
                auto line=PopContinuation();
                // Charges may change while Kit walks and waits. Choose the
                // spoken variant at delivery, before its identity is fixed.
                if(line==Line::FlaskEmpty || line==Line::FlaskTopup)
                    line=c.flaskCharges==0?Line::FlaskEmpty:Line::FlaskTopup;
                Start(line,SourceFor(line),c);
            }
        }
        StepRoad(c,hearsRoad);
        if(!tavernWelcomeStarted_ && state_.forestVillageConsumed && hearsWelcome && c.kitAtSouthDoor &&
           state_.line==Line::None && continuationCount_==0) {
            tavernWelcomeStarted_=true;
            Start(Line::TavernWelcome,*c.tavernKeeperSource,c);
        }
        StepHearth(c,hearsHearth,dt);
        if(tavernConversationActive_ && state_.line==Line::None)StartNextTavernLine(c);
        if(hearsShrine && state_.line==Line::None && continuationCount_==0 && shrineRevealIndex_<kShrineReveal.size())
            Start(kShrineReveal[shrineRevealIndex_++],*c.shrineLanternSource,c);
        if(hearsRecordKeeper && state_.line==Line::None && continuationCount_==0 && !recordKeeperPending_.empty())
            StartNextRecordKeeperLine(c);
        StepTavernLantern(c);
        RefreshCompanionSpatial(c);
    }
private:
    static bool Finite(Tuple t) {return std::isfinite(t.x)&&std::isfinite(t.y)&&std::isfinite(t.z);}
    static bool CanHearShrineLantern(const Context& c) {
        return c.alive && c.claimed && c.exterior && !c.paused && !c.combat && !c.traversing && c.kitSeated &&
            c.shrineLanternSource && Finite(*c.shrineLanternSource) && Finite(c.listener) && std::isfinite(c.yaw);
    }
    static bool CanHearRecordKeeper(const Context& c) {
        return c.alive && c.claimed && c.exterior && !c.paused && !c.combat && !c.traversing && c.kitSeated &&
            c.recordKeeperSource && Finite(*c.recordKeeperSource) && Finite(c.listener) && std::isfinite(c.yaw);
    }
    void StartNextRecordKeeperLine(const Context& c) {
        const auto line=recordKeeperPending_.front();
        recordKeeperPending_=recordKeeperPending_.subspan(1);
        Start(line,*c.recordKeeperSource,c);
    }
    static bool AtTavernHearth(const Context& c) {
        return c.alive && c.exterior && !c.paused && !c.combat && !c.traversing && c.tavernInReach &&
            c.kitSeated && c.companionSource && Finite(*c.companionSource) && Finite(c.listener) && std::isfinite(c.yaw);
    }
    static bool CanHearTavernLantern(const Context& c) {
        return c.claimed && AtTavernHearth(c) && c.tavernLanternSource && Finite(*c.tavernLanternSource);
    }
    void StepTavernLantern(const Context& c) {
        if(!tavernLanternActive_) {
            if(!CanOfferTavernLantern(c) || c.lanternLow) {
                tavernLanternRaiseFloor_=c.manualRaiseSequence;
                return;
            }
            if(!c.lanternHigh || c.manualRaiseSequence<=tavernLanternRaiseFloor_ ||
               c.visibleRaiseSequence!=c.manualRaiseSequence)return;
            tavernLanternRaiseFloor_=c.manualRaiseSequence;
            tavernLanternActive_=true;
        }
        if(state_.line!=Line::None)return;
        if(state_.tavernLanternLinesStarted==kTavernLanternExchange.size()) {
            tavernLanternActive_=false;
            return;
        }
        const auto line=kTavernLanternExchange[state_.tavernLanternLinesStarted++];
        Start(line,IsTavernLanternVoice(line)?*c.tavernLanternSource:*c.companionSource,c);
    }
    static bool CanHearTavernConversation(const Context& c) {
        return AtTavernHearth(c) && c.keeperAtHearth && c.tavernKeeperSource && Finite(*c.tavernKeeperSource);
    }
    void StartNextTavernLine(const Context& c) {
        const auto line=kTavernConversation[state_.tavernConversationLinesResolved];
        Start(line,line==Line::TavernHordeArmy?*c.tavernKeeperSource:*c.companionSource,c);
    }
    void BindCompanionSpatial(const Context& c) {
        if(state_.line<Line::ReunionQuestion || (state_.line>Line::FlaskTopup && !IsBellwetherLine(state_.line))) return;
        const bool fromKeeper=state_.line==Line::TavernHordeArmy || state_.line==Line::TavernWelcome;
        const auto& source=IsShrineLanternLine(state_.line)?c.shrineLanternSource:
            IsTavernLanternVoice(state_.line)?c.tavernLanternSource:
            IsRecordKeeperLine(state_.line)?c.recordKeeperSource:(fromKeeper?c.tavernKeeperSource:c.companionSource);
        if(!Finite(c.listener) || !std::isfinite(c.yaw) || (source && !Finite(*source))) return;
        state_.listener=c.listener;state_.listenerYaw=c.yaw;
        if(source) state_.source=*source;
    }
    static std::uint64_t Next(std::uint64_t x) { return x==UINT64_MAX?x:x+1; }
    static Tuple AtRoutePoint(std::size_t index,float heightOffset=1.4f) {
        const auto p=simulation::kWorldRoutePoints[index];return {p.x,p.y+heightOffset,p.z};
    }
    static Tuple SourceFor(Line line) {
        switch(line) {
        case Line::ForestNight: return AtRoutePoint(3);
        case Line::ForestWaystone: case Line::ForestClue: return AtRoutePoint(6);
        case Line::ForestVillage: return AtRoutePoint(7);
        default: return {-32.0f,3.6f,-10.0f};
        }
    }
    bool HasQueued(Line line) const {
        for(std::size_t i=0;i<continuationCount_;++i) if(continuations_[i]==line) return true;
        return false;
    }
    void Queue(Line line) {
        if(line==Line::None || HasQueued(line) || state_.line==line || continuationCount_==continuations_.size()) return;
        continuations_[continuationCount_++]=line;
    }
    Line PopContinuation() {
        const Line line=continuations_[0];
        for(std::size_t i=1;i<continuationCount_;++i) continuations_[i-1]=continuations_[i];
        continuations_[--continuationCount_]=Line::None;
        return line;
    }
    void StepForestFixture(const Context& c) {
        // Keep the refill introduction ahead of forest commentary even if the
        // player runs on before following Kit. It remains one ordered story
        // chain and can be resumed from a campaign save.
        if(!state_.proofAccepted || !c.exterior || c.traversing || BasinGuidePending()) return;
        if(!simulation::ResolveWorldRouteSupport(c.player.x,c.player.z).grounded) return;
        const auto& route=simulation::kWorldRoutePoints;
        const auto distance=[&](std::size_t index) {
            return std::hypot(c.player.x-route[index].x,c.player.z-route[index].z);
        };
        const float fromF02=distance(3),fromF03=distance(6),fromF04=distance(7);
        // F02/F03/F04 are the translated route points, never copied planning coordinates.
        if(!state_.forestNightConsumed && !HasQueued(Line::ForestNight) &&
           fromF02>3.0f && fromF03+.25f<fromF02)
            Queue(Line::ForestNight);
        const bool nightReached=state_.forestNightConsumed||HasQueued(Line::ForestNight)||state_.line==Line::ForestNight;
        if(nightReached && !state_.forestWaystoneConsumed && fromF03<=3.0f) {
            Queue(Line::ForestWaystone);Queue(Line::ForestClue);
        }
        const bool clueReached=state_.forestClueConsumed||HasQueued(Line::ForestClue)||state_.line==Line::ForestClue;
        if(clueReached && !state_.forestVillageConsumed && fromF04<=3.0f)
            Queue(Line::ForestVillage);
    }
    void StepRoad(const Context& c,bool hearsRoad) {
        if(!state_.forestVillageConsumed || !hearsRoad || !c.kitCaughtAnkle ||
           state_.line!=Line::None || continuationCount_ || roadLinesResolved_>=kRoadLines.size())return;
        const auto line=kRoadLines[roadLinesResolved_];
        if(line==Line::RoadSlow && !c.kitReadyForProtectiveWalk)return;
        Start(line,*c.companionSource,c);
    }
    void StepHearth(const Context& c,bool hearsHearth,float dt) {
        if(!state_.forestVillageConsumed) return;
        auto& resolved=state_.hearthLinesResolved;
        // Arriving after the chair is occupied still permits the two seated
        // lines. The earlier sit remark expires with the act of sitting.
        if(c.kitSeated && resolved==0 && state_.line!=Line::TavernSit)resolved=1;
        if(!hearsHearth || state_.line!=Line::None || continuationCount_ || resolved>=kHearthLines.size())return;
        if(resolved==0 ? !c.kitAtChair : !c.kitSeated)return;
        hearthPauseSeconds_=std::max(0.f,hearthPauseSeconds_-dt);
        if(hearthPauseSeconds_==0)Start(kHearthLines[resolved],*c.companionSource,c);
    }
    void MarkOnce(Line line) {
        switch(line) {
        case Line::ForestNight: state_.forestNightConsumed=true;break;
        case Line::ForestWaystone: state_.forestWaystoneConsumed=true;break;
        case Line::ForestClue: state_.forestClueConsumed=true;break;
        case Line::ForestVillage: state_.forestVillageConsumed=true;break;
        default: break;
        }
    }
    void Cancel(bool clearContinuations=true) { state_.line=Line::None;state_.elapsed=0;state_.duration=0;
        tavernConversationActive_=false;
        tavernLanternActive_=false;
        shrineRevealIndex_=kShrineReveal.size();
        recordKeeperPending_={};
        state_.generation=Next(state_.generation);
        // A priority interruption invalidates only the active callback. Preserve
        // the ordered once-only story responses; lifecycle/local cancellation
        // still discards its queue explicitly.
        if(clearContinuations) { continuations_={};continuationCount_=0; } }
    void Finish() { if(state_.line==Line::Grate) state_.grateConsumed=true;
        if(IsRoadLine(state_.line))++roadLinesResolved_;
        if(IsTavernConversationLine(state_.line) && ++state_.tavernConversationLinesResolved==kTavernConversation.size())
            tavernConversationActive_=false;
        if(IsHearthLine(state_.line)) {
            ++state_.hearthLinesResolved;
            if(state_.line==Line::TavernLimit)hearthPauseSeconds_=kHearthWorkPauseSeconds;
        }
        state_.line=Line::None;state_.generation=Next(state_.generation);state_.elapsed=0; }
    void Start(Line line,Tuple source,const Context& c) {
        state_.generation=Next(state_.generation);state_.line=line;state_.duration=Spec(line).duration;
        state_.elapsed=0;state_.source=source;state_.listener=c.listener;state_.listenerYaw=c.yaw;MarkOnce(line);
        // Restore may start paused, but must publish the restored actor anchor
        // immediately rather than the old authored fallback for its first frame.
        BindCompanionSpatial(c);
    }
    Snapshot state_{};Line pending_=Line::None;
    std::array<Line,8> continuations_{};std::size_t continuationCount_=0;
    std::size_t roadLinesResolved_=0;
    std::size_t shrineRevealIndex_=kShrineReveal.size();
    std::span<const Line> recordKeeperPending_;
    bool tavernConversationActive_=false;
    bool tavernWelcomeStarted_=false;
    bool tavernLanternActive_=false;
    std::uint64_t tavernLanternRaiseFloor_=0;
    std::uint64_t raiseFloor_=0;float waitingSeconds_=0,hearthPauseSeconds_=0;
};
} // namespace horde::gameplay::dialogue
