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

inline constexpr std::array kBellwetherSalvagerAuditionCuts{
    BellwetherVoiceCut{gameplay::dialogue::Line::SalvagerShoreTask,126704,"8fe30ce30ccef9f9692f6d0f82a08bd618880038434b00336727cc70aad79ca9"},
    BellwetherVoiceCut{gameplay::dialogue::Line::SalvagerShoreReminder,113264,"294501da575c700e3b4ed1e518f8dbfc6cb13e71c13c85a0a060e3c6ce993543"},
    BellwetherVoiceCut{gameplay::dialogue::Line::SalvagerShoreReturn,111344,"01af397a57165188e559aaebe18edc5e76deeb4e2dedb91ab19a041cd11d1971"}
};

// The November 4 Kit and record-keeper review takes and October 29 dealer
// audition match the existing playable scripts. Keep the caption reading time.
inline constexpr std::array kBellwetherFollowOnAuditionCuts{
    BellwetherVoiceCut{gameplay::dialogue::Line::RecordKeeperTwoRoads, 106064, "dc6602591b41f3b42a598f41f2113dc01a613f2b6ec62dbf94337b72a2c28d5d"},
    BellwetherVoiceCut{gameplay::dialogue::Line::RecordKeeperRoadOrder, 135824, "3506524546c1506b8183c2268f3a46c09421ffe79419ba25ae3646f826b6bcf0"},
    BellwetherVoiceCut{gameplay::dialogue::Line::KitVoiceReaction, 92624, "305ea7a23ee19fa7c92db51b5b1a515139209a6dd6ffaf9e490f73864ea1f0ed"},
    BellwetherVoiceCut{gameplay::dialogue::Line::KitVoiceListen, 45584, "fe8f68d761ad49bcdbf56e54c4f268a0584536a5c291f5051e6343391abb6048"},
    BellwetherVoiceCut{gameplay::dialogue::Line::KitRoadHelp, 122384, "3bf0390321c7abf02f5faf13f02773f3b560fd74f2aef1b42f9f14f753e6e817"},
    BellwetherVoiceCut{gameplay::dialogue::Line::RelicDealerOrdinarySale, 78704, "7f045c4859e64d7dd79d9d813cffb5cb49f66e7d999dc2a70107beac683e05b7"}
};

// The quiet Kit hearth conversation remains a private audition overlay.
inline constexpr std::array kBellwetherHearthAuditionCuts{
    BellwetherVoiceCut{gameplay::dialogue::Line::KitTreasureRoof, 136784, "9f7c15c99489614579fc6f7e69845a2d3d83d46f5d75845d55464d3795c26b82"},
    BellwetherVoiceCut{gameplay::dialogue::Line::KitTreasureKey, 113744, "d82e1198d35124ffb23c25633141597f1c2bb4ee431a1ee300923b124d6caeb0"}
};

// Private village takes can be admitted one line at a time as they are reviewed.
inline constexpr std::array kBellwetherOptionalVoiceCuts{
    BellwetherVoiceCut{gameplay::dialogue::Line::RecordKeeperErrand, 127979, "7ddf64e532156ee4d46f50858f18c9362a748b7c98c67d8c564c9fcbe66291d8"},
    BellwetherVoiceCut{gameplay::dialogue::Line::RecordKeeperDirections, 110730, "25f310416c92fd6011550960bda57fd4c2070ba92beb18cfd0dc6e17cecb4e44"}
};

inline constexpr bool HasBellwetherVoiceCut(gameplay::dialogue::Line line, bool arrivalAudition = false,
                                            bool salvagerAudition = false, bool followOnAudition = false,
                                            bool hearthAudition = false) {
    const auto matches = [line](const BellwetherVoiceCut& cut) { return cut.line == line; };
    return std::any_of(kBellwetherVoiceCuts.begin(), kBellwetherVoiceCuts.end(), matches) ||
        (arrivalAudition && std::any_of(kBellwetherArrivalAuditionCuts.begin(),
                                      kBellwetherArrivalAuditionCuts.end(), matches)) ||
        (salvagerAudition && std::any_of(kBellwetherSalvagerAuditionCuts.begin(),
                                       kBellwetherSalvagerAuditionCuts.end(), matches)) ||
        (followOnAudition && std::any_of(kBellwetherFollowOnAuditionCuts.begin(),
                                       kBellwetherFollowOnAuditionCuts.end(), matches)) ||
        (hearthAudition && std::any_of(kBellwetherHearthAuditionCuts.begin(),
                                     kBellwetherHearthAuditionCuts.end(), matches));
}

} // namespace horde::audio
