#pragma once
#include "gameplay/SpatialAudio.h"
#include "gameplay/dialogue/ChapterDialogue.h"
namespace horde::audio {
// Rescue speech crosses the open shaft. Preserve its world anchor, panning,
// obstruction and range; compensate its short-reference falloff, not its PCM.
inline gameplay::SpatialAudioGains ChapterDialogueSpatial(
    const gameplay::dialogue::Snapshot& s, gameplay::SpatialAudioDomain domain) {
    using gameplay::dialogue::Line;
    const bool rescue=s.line==Line::RescueFound || s.line==Line::RescueRope;
    return gameplay::CalculateSpatialAudio(
        {s.source.x,s.source.z,rescue?.60f:.36f,rescue?3.f:1.f,14.f,s.source.y},
        {s.listener.x,s.listener.z,s.listenerYaw,s.listener.y},domain);
}
}
