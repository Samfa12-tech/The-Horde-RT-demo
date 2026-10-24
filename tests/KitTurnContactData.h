#pragma once
#include "gameplay/dialogue/CompanionTurnProfile.h"
#include <algorithm>
#include <array>
#include <cmath>
#include <cstdint>

// Test sole rows and shared exact owned take metadata; production remains opt-in.
// Sole rows: unchanged imported mesh, pure leg/boot weights, lowest 6 mm.
// Source root-motion.json SHA256 7a4928d4dfec5184071c96a4c8e1f25e07d7ce7c87545324450e75b0c29061d9
namespace kit_turn_contact {
inline constexpr std::array<std::uint32_t,39> leftSole{{18998,19003,19004,19039,19040,19041,19215,19216,19395,19396,19625,19626,19753,19754,19901,19902,20014,20015,20445,20446,20530,20531,20532,20762,20763,21193,21194,21195,21196,21220,21221,21492,21493,21511,21512,22176,22177,23746,23747}};
inline constexpr std::array<std::uint32_t,38> rightSole{{3094,3095,3097,3098,3133,3134,3205,3206,3361,3362,4008,4009,4136,4137,4138,4844,4845,4874,4875,4891,4892,5204,5205,6009,6010,6585,6586,6689,6690,6691,7611,7612,7670,7671,7723,7724,8401,8402}};
using horde::gameplay::dialogue::companion_turn::leftYaw;
using horde::gameplay::dialogue::companion_turn::rightYaw;
inline float Yaw(bool right,float phase) {return horde::gameplay::dialogue::companion_turn::ModelYaw(right,phase);}
}
