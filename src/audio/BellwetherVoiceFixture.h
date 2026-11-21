#pragma once

#include <algorithm>
#include <array>
#include <string_view>

#include "gameplay/dialogue/ChapterDialogue.h"

namespace horde::audio {

struct BellwetherVoiceCut {
    gameplay::dialogue::Line line;
    unsigned frames;
    std::string_view sha256;
};

// Exact, locally approved Kit takes from the November 4 intake.
// The shared line catalogue controls script, stable ID and timing.
inline constexpr std::array kBellwetherVoiceCuts{
    BellwetherVoiceCut{gameplay::dialogue::Line::TavernSit, 81104, "e088a2419814e9e055d553c11ab43cfc4459371aca9d4687279e93235593986a"},
    BellwetherVoiceCut{gameplay::dialogue::Line::TavernLimit, 113744, "f008e256ac6301240931dd98865cf5f30332f65cc64d48845d06acd64a20e22a"},
    BellwetherVoiceCut{gameplay::dialogue::Line::TavernWork, 102224, "9888430432f1972e100366a66b4aa932e757819bdb67cb1c8ead8ba989014e1e"},
    BellwetherVoiceCut{gameplay::dialogue::Line::TavernIntroduce, 98384, "64646e97b066c9dc8fd04d99a2a0a68dfa3f0cfab55e4ec2aa133ec8ab1ea2b9"},
    BellwetherVoiceCut{gameplay::dialogue::Line::TavernHoardAnswer, 94544, "a91d048e340a2f8c9adbba0046393fc1db847e48331dc84ba5d467c5e5dfa5ee"}
};

inline constexpr bool HasBellwetherVoiceCut(gameplay::dialogue::Line line) {
    return std::any_of(kBellwetherVoiceCuts.begin(), kBellwetherVoiceCuts.end(),
        [line](const BellwetherVoiceCut& cut) { return cut.line == line; });
}

} // namespace horde::audio
