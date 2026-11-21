#pragma once

#include "gameplay/SpatialAudio.h"
#include "gameplay/simulation/GameplayEvent.h"

#include <array>

namespace horde::audio {

enum class KitAnkleCue : std::int32_t { None, BootAndGravel, SteadyBreath };

struct KitAnkleBeat {
    KitAnkleCue cue;
    float seconds, forward, right, height, gain;
};

// Contact of the rolling right boot and the quiet recovery beat of the take.
inline constexpr std::array<KitAnkleBeat,2> kKitAnkleBeats{{
    {KitAnkleCue::BootAndGravel,8.f/60.f,.145f,.14f,.035f,.74f},
    {KitAnkleCue::SteadyBreath,31.f/60.f,0,0,1.58f,.64f}
}};

inline constexpr KitAnkleCue KitAnkleCueForEvent(const gameplay::simulation::GameplayEvent& event) {
    using namespace gameplay::simulation;
    if(event.type!=GameplayEventType::KitAnkleFoley || event.source!=EntityId::Kit)return KitAnkleCue::None;
    const auto cue=static_cast<KitAnkleCue>(event.payload);
    return cue==KitAnkleCue::BootAndGravel || cue==KitAnkleCue::SteadyBreath ? cue : KitAnkleCue::None;
}

inline constexpr const char* KitAnkleClip(KitAnkleCue cue) {
    switch(cue) {
    case KitAnkleCue::BootAndGravel:return "audio/kit-ankle/runtime/boot_and_gravel.wav";
    case KitAnkleCue::SteadyBreath:return "audio/kit-ankle/runtime/steady_breath.wav";
    default:return nullptr;
    }
}

inline gameplay::SpatialAudioGains KitAnkleGains(const gameplay::simulation::GameplayEvent& event) {
    if(KitAnkleCueForEvent(event)==KitAnkleCue::None)return {};
    // The contact happens on the outdoor road, outside the tomb wall proxy.
    return gameplay::CalculateSpatialAudio({event.worldX,event.worldZ,event.intensity,1.6f,8.f,event.worldY},
        {event.listenerX,event.listenerZ,event.listenerYawRadians,event.listenerY},
        gameplay::SpatialAudioDomain::ExteriorRoute);
}

} // namespace horde::audio
