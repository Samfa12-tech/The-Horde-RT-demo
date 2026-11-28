#pragma once
#include "gameplay/SpatialAudio.h"
#include "gameplay/dialogue/ChapterDialogue.h"
namespace horde::audio {
inline gameplay::SpatialAudioGains ChapterDialogueSpatial(
    const gameplay::dialogue::Snapshot& s, gameplay::SpatialAudioDomain domain) {
    namespace dialogue = gameplay::dialogue;
    const bool rescue = s.line == dialogue::Line::RescueFound || s.line == dialogue::Line::RescueRope;
    const bool hearth = dialogue::IsHearthLine(s.line) || dialogue::IsTavernConversationLine(s.line) ||
        dialogue::IsTavernLanternLine(s.line) || dialogue::IsKitTreasureTalkLine(s.line);
    // Keep conversation across the hearth clear at the table's listening distance.
    // The common spatial calculation still owns pan, obstruction and range.
    const float sourceGain = rescue ? .60f : (hearth ? .50f : .36f);
    const float referenceDistance = rescue ? 3.f : (hearth ? 2.5f : 1.f);
    return gameplay::CalculateSpatialAudio(
        {s.source.x,s.source.z,sourceGain,referenceDistance,14.f,s.source.y},
        {s.listener.x,s.listener.z,s.listenerYaw,s.listener.y},domain);
}
}
