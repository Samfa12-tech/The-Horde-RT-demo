#pragma once
#include "gameplay/dialogue/ChapterDialogue.h"
#include <algorithm>
#include <cmath>

namespace horde::gameplay::dialogue {
struct CompanionActingPose {
    float headPitch=0,headYaw=0,chestPitch=0;
    bool recognitionActive=false;
    bool operator==(const CompanionActingPose&) const=default;
};

// Transient 60 Hz presentation state. Story, route, speech completion and saves
// remain authoritative elsewhere; an animation can never accept lantern proof.
class CompanionActing {
public:
    const CompanionActingPose& State() const {return state_;}
    void Reset() {state_={};recognitionSeconds_=-1;}
    void Step(float seconds,const Snapshot& speech,bool exterior,bool alive,
              bool grounded,bool walking,bool freshVisibleProof) {
        if(speech.paused || !std::isfinite(seconds) || seconds<=0)return;
        const float dt=std::min(seconds,.05f);
        const bool body=exterior && alive && grounded;
        if(!body)recognitionSeconds_=-1;
        // Only the live director's false->true proof transaction supplies this
        // edge. Restoring an accepted story/active line never supplies it.
        else if(freshVisibleProof && speech.proofAccepted)recognitionSeconds_=0;
        float recognition=0;
        if(recognitionSeconds_>=0) {
            recognitionSeconds_+=dt;
            if(recognitionSeconds_>=1.2f)recognitionSeconds_=-1;
            else recognition=std::sin(3.14159265359f*recognitionSeconds_/1.2f);
        }
        state_.recognitionActive=recognitionSeconds_>=0;
        float headPitch=0,headYaw=0,chestPitch=0;
        // Grate/rescue calls remain unseen; Keeper speech never animates Kit.
        const bool talking=body && speech.line>=Line::ReunionQuestion && speech.line<=Line::ForestVillage &&
            std::isfinite(speech.elapsed) && std::isfinite(speech.duration) && speech.duration>0;
        if(talking) {
            const float t=std::clamp(speech.elapsed,0.0f,speech.duration);
            const float edge=std::clamp(std::min(t,speech.duration-t)/.2f,0.0f,1.0f);
            const float envelope=edge*edge*(3-2*edge)*(walking?.4f:1.0f);
            headPitch=static_cast<float>(.035*std::sin(double(t)*5)*envelope);
            headYaw=static_cast<float>(.025*std::sin(double(t)*2.3)*envelope);
            chestPitch=walking?0:static_cast<float>(.012*std::sin(double(t)*3.1)*envelope);
        }
        headPitch+=.14f*recognition;chestPitch+=.035f*recognition;
        const auto approach=[dt](float from,float to,float speed){return from+std::clamp(to-from,-speed*dt,speed*dt);};
        state_.headPitch=approach(state_.headPitch,headPitch,.65f);
        state_.headYaw=approach(state_.headYaw,headYaw,.35f);
        state_.chestPitch=approach(state_.chestPitch,chestPitch,.25f);
    }
private:
    CompanionActingPose state_{};
    float recognitionSeconds_=-1;
};
} // namespace horde::gameplay::dialogue
