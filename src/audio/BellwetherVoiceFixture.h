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

// The arrival auditions use the exact private review WAVs. Listening approval
// and shipping admission are tracked separately from the Debug path.
inline constexpr std::array kBellwetherArrivalAuditionCuts{
    BellwetherVoiceCut{gameplay::dialogue::Line::RoadStop, 53264, "817f8f5527b1d9764977b9c255f119d9cc1ad9f2903f6283f640440fe96a9391"},
    BellwetherVoiceCut{gameplay::dialogue::Line::RoadAnkle, 122384, "ddb89b45e4bef7205c983c9fedf3e0cceca18db8479f0ac326cd0e6dc218c2e4"},
    BellwetherVoiceCut{gameplay::dialogue::Line::RoadSlow, 85904, "8c16e87f66d1514735c33d18a8d678ec2d1fea88993eb9800394cab7267826f4"},
    BellwetherVoiceCut{gameplay::dialogue::Line::TavernWelcome, 88784, "927ca0dc61a138a704328079a5fa300a3b4131fabce204b9be7fc850fe1a8a0b"},
    BellwetherVoiceCut{gameplay::dialogue::Line::TavernHordeArmy, 133904, "a088307c4969f436888eb16eddf22ec23ceab59940adbf34ba7a2ba4f7401d5d"}
};

inline constexpr bool HasBellwetherVoiceCut(gameplay::dialogue::Line line, bool arrivalAudition = false) {
    const auto matches = [line](const BellwetherVoiceCut& cut) { return cut.line == line; };
    return std::any_of(kBellwetherVoiceCuts.begin(), kBellwetherVoiceCuts.end(), matches) ||
        (arrivalAudition && std::any_of(kBellwetherArrivalAuditionCuts.begin(),
                                      kBellwetherArrivalAuditionCuts.end(), matches));
}

} // namespace horde::audio
