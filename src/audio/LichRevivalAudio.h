#pragma once
#include "gameplay/SpatialAudio.h"
#include "gameplay/simulation/GameplayEvent.h"
#include <algorithm>
#include <cmath>
#include <cstdint>

namespace horde::audio {
// Provisional private audition preset. This does not admit a distributed asset.
inline constexpr float kLichRevivalMixGain=.55f,kLichRevivalReferenceMetres=24.f,kLichRevivalMaximumMetres=48.f;
inline constexpr std::uint64_t kLichRevivalWaveBytes=350252u,kLichRevivalPcmBytes=350208u;
inline constexpr const char* kLichRevivalWaveSha256="1e1891010f4c89e6f20ec5fba90191aa0f0999c0bd95a4df3fe36659beb9e521";
inline gameplay::SpatialAudioEmitter LichRevivalEmitter(const gameplay::simulation::GameplayEvent& event) {
    return {event.worldX,event.worldZ,std::isfinite(event.intensity)?std::clamp(event.intensity,0.f,1.f):0.f,
        kLichRevivalReferenceMetres,kLichRevivalMaximumMetres,event.worldY};
}
inline bool IsLichRevivalCry(const gameplay::simulation::GameplayEvent& event) {
    using namespace gameplay::simulation;
    return event.type==GameplayEventType::LichRevivalCry && event.source==EntityId::Lich && event.target==EntityId::Player;
}
inline float OwnedCueGain(float user,float duck) {
    return std::isfinite(user) && std::isfinite(duck)?std::clamp(user,0.f,1.f)*std::clamp(duck,0.f,1.f):0.f;
}
// One finite voice: original event pan is latched; only source volume changes.
// Cancellation retains the sequence floor, so a stale packet cannot restart it.
struct OwnedSpatialOneShot {
    std::uint64_t lifetime=0,lastSequence=0;
    bool seenLifetime=false,active=false;
    float left=0,right=0;
    bool ObserveLifetime(std::uint64_t next) {
        if(seenLifetime && next<=lifetime)return false;
        const bool stopped=active;seenLifetime=true;lifetime=next;lastSequence=0;active=false;return stopped;
    }
    void Cancel(){active=false;}
    bool Begin(std::uint64_t sequence,float l,float r,bool eligible) {
        if(sequence==0 || sequence<=lastSequence)return false;
        lastSequence=sequence;
        if(!eligible || !std::isfinite(l) || !std::isfinite(r))return false;
        left=std::clamp(l,0.f,1.f);right=std::clamp(r,0.f,1.f);
        active=left>0.f || right>0.f;return active;
    }
};
}
