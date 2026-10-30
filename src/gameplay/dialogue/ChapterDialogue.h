#pragma once

#include <algorithm>
#include <array>
#include <cmath>
#include <cstddef>
#include <cstdint>
#include <optional>

#include "gameplay/simulation/DevelopmentWorldRoute.h"

namespace horde::gameplay::dialogue {
enum class Line : std::uint32_t { None, KeeperSense, KeeperCloser, Grate, RescueFound,
    RescueRope, ReunionQuestion, ReunionHint, ReunionProof, ReunionFirstPiece,
    ReunionDepart, ForestNight, ForestWaystone, ForestClue, ForestWait, ForestVillage };
struct LineSpec { Line id; const char* stableId; const char* speaker; const char* text;
    const char* audio; float duration; unsigned priority; };
// Kit text/timing and PCM match the thirteen approved cuts. Public game/repository
// distribution was explicitly confirmed by the owner on 11 October 2026.
inline constexpr std::array<LineSpec,16> kLines{{
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
    {Line::ForestVillage,"forest.village","Kit","There. Chimney smoke.","assets/audio/kit/runtime/forest.village.wav",2.173792f,1}
}};
inline const LineSpec& Spec(Line line) { const auto n=static_cast<unsigned>(line);
    return kLines[n<kLines.size()?n:0]; }
struct Tuple { float x=0,y=0,z=0; };
inline constexpr float kCompanionSpeechHeight=1.4f;
struct Snapshot {
    Line line=Line::None; std::uint64_t generation=1; float elapsed=0,duration=0;
    Tuple source{},listener{}; float listenerYaw=0;
    bool paused=false,grateConsumed=false,rescueConsumed=false,reunionStarted=false;
    bool waitingForRaise=false,proofAccepted=false,reminderConsumed=false;
    bool forestNightConsumed=false,forestWaystoneConsumed=false;
    bool forestClueConsumed=false,forestVillageConsumed=false;
    // A labelled companion fixture contract, not a rendered/admitted Kit actor.
    bool companionVisualAdmitted=false;
};
struct Context {
    Tuple player{},listener{}; float yaw=0;
    // Current grounded companion root plus speech height, owned by simulation.
    // Absence retains the last valid source while the actor is unavailable.
    std::optional<Tuple> companionSource;
    bool paused=false,alive=true,combat=false,claimed=false,exterior=false,traversing=false;
    bool lanternLow=false,lanternHigh=false;
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
};
class Director {
public:
    const Snapshot& State() const { return state_; }
    // Continuous Kit speech follows the same actor/listener snapshot as the
    // renderer. Event-time Keeper and unseen tomb/shaft calls stay unchanged.
    // Spatial refresh never advances the line, cursor, queue or callback ID.
    void RefreshCompanionSpatial(const Context& c) {
        if(!c.paused && c.alive) BindCompanionSpatial(c);
    }
    Progress ExportProgress() const {
        Progress p;
        p.activeLine=state_.line;p.continuations=continuations_;p.continuationCount=continuationCount_;
        p.grateConsumed=state_.grateConsumed;p.rescueConsumed=state_.rescueConsumed;
        p.reunionStarted=state_.reunionStarted;p.waitingForRaise=state_.waitingForRaise;
        p.proofAccepted=state_.proofAccepted;p.reminderConsumed=state_.reminderConsumed;
        p.forestNightConsumed=state_.forestNightConsumed;
        p.forestWaystoneConsumed=state_.forestWaystoneConsumed;
        p.forestClueConsumed=state_.forestClueConsumed;p.forestVillageConsumed=state_.forestVillageConsumed;
        return p;
    }
    static bool ValidateProgress(const Progress& p) {
        const auto recoverable=[](Line line) {
            // Keeper speech belongs to a live combat emitter, not either safe
            // campaign boundary. ForestWait currently has no logical trigger.
            return static_cast<std::uint32_t>(line)<kLines.size() &&
                line!=Line::KeeperSense && line!=Line::KeeperCloser && line!=Line::ForestWait;
        };
        if(!recoverable(p.activeLine) || p.continuationCount>p.continuations.size() ||
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
            case Line::Grate: return !p.grateConsumed;
            default: return true;
            }
        };
        if(!validStory(p.activeLine,false)) return false;
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
            state_.forestVillageConsumed=old.forestVillageConsumed; }
        pending_=Line::None; continuations_={}; continuationCount_=0;
        if(preserveOnce) { continuations_=oldContinuations;continuationCount_=oldContinuationCount; }
        raiseFloor_=preserveOnce?oldRaiseFloor:0; waitingSeconds_=0;
    }
    // Lifecycle cancellation invalidates callbacks but retains logical once state.
    void Suspend() { if(state_.line==Line::Grate) state_.grateConsumed=true;
        Cancel(); pending_=Line::None; }
    bool Complete(Line line,std::uint64_t generation) {
        if(line==Line::None || state_.line!=line || state_.generation!=generation) return false;
        Finish(); return true;
    }
    void Skip() { if(state_.line!=Line::None) Finish(); }
    void Request(Line line,Tuple source,const Context& c) {
        if(line==Line::None || static_cast<std::uint32_t>(line)>=kLines.size()) return;
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
            const auto line=PopContinuation();
            Start(line,SourceFor(line),c);
        }
        RefreshCompanionSpatial(c);
    }
private:
    void BindCompanionSpatial(const Context& c) {
        if(state_.line<Line::ReunionQuestion || state_.line>Line::ForestVillage) return;
        const auto finite=[](Tuple t) {return std::isfinite(t.x)&&std::isfinite(t.y)&&std::isfinite(t.z);};
        if(!finite(c.listener) || !std::isfinite(c.yaw) ||
           (c.companionSource && !finite(*c.companionSource))) return;
        state_.listener=c.listener;state_.listenerYaw=c.yaw;
        if(c.companionSource) state_.source=*c.companionSource;
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
        if(!state_.proofAccepted || !c.exterior || c.traversing) return;
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
        state_.generation=Next(state_.generation);
        // A priority interruption invalidates only the active callback. Preserve
        // the ordered once-only story responses; lifecycle/local cancellation
        // still discards its queue explicitly.
        if(clearContinuations) { continuations_={};continuationCount_=0; } }
    void Finish() { if(state_.line==Line::Grate) state_.grateConsumed=true;
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
    std::uint64_t raiseFloor_=0;float waitingSeconds_=0;
};
} // namespace horde::gameplay::dialogue
