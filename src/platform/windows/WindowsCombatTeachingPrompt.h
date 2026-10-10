#pragma once

#include "gameplay/simulation/CombatTeaching.h"

#include <algorithm>
#include <cmath>
#include <cstdint>
#include <string>

namespace horde::platform::windows
{
inline constexpr std::uint64_t kCombatTeachingPromptFadeMilliseconds = 120u;

inline unsigned char CombatTeachingPromptAlpha(const float opacity, const bool reducedMotion)
{
    if (!std::isfinite(opacity) || opacity <= 0.0f) return 0u;
    if (reducedMotion) return 255u;
    return static_cast<unsigned char>(std::lround(std::clamp(opacity, 0.0f, 1.0f) * 255.0f));
}

inline unsigned char CombatTeachingPromptFadeAlpha(
    const unsigned char previousAlpha, const std::uint64_t elapsedMilliseconds, const bool reducedMotion)
{
    if (reducedMotion || elapsedMilliseconds >= kCombatTeachingPromptFadeMilliseconds) return 0u;
    const auto remaining = kCombatTeachingPromptFadeMilliseconds - elapsedMilliseconds;
    return static_cast<unsigned char>(
        (static_cast<std::uint64_t>(previousAlpha) * remaining +
         kCombatTeachingPromptFadeMilliseconds / 2u) /
        kCombatTeachingPromptFadeMilliseconds);
}

inline std::string CombatTeachingPromptText(
    const gameplay::simulation::CombatTeachingSnapshot& teaching)
{
    using gameplay::simulation::CombatTeachingCue;
    using gameplay::simulation::TutorialStage;
    if (!teaching.enabled || !std::isfinite(teaching.promptOpacity) || teaching.promptOpacity <= 0.0f)
        return {};
    const char* lesson = "O WATCH THE ATTACKER - read the motion";
    switch (teaching.cue)
    {
    case CombatTeachingCue::ParryWindup:
        lesson = teaching.source == gameplay::simulation::EntityId::SkeletonB
            ? "O SKELETON B WIND-UP - watch its raised blade"
            : "O SKELETON A WIND-UP - watch its raised blade";
        break;
    case CombatTeachingCue::ParryNow: lesson = "o PRESS PARRY - raise guard before the strike"; break;
    case CombatTeachingCue::ParryActive: lesson = "(*) PARRY WINDOW ACTIVE - face the incoming blade"; break;
    case CombatTeachingCue::ParryRecovery: lesson = "-> RESET - ready for the next strike"; break;
    case CombatTeachingCue::DodgeWindup: lesson = "O KEEPER CHARGING - watch the staff glow build"; break;
    case CombatTeachingCue::DodgeNow: lesson = "<-- DODGE NOW --> - move sideways"; break;
    case CombatTeachingCue::Recovery: lesson = "-> RECOVER - face the Keeper again"; break;
    case CombatTeachingCue::Learned: lesson = "(*) PARRY LANDED - the attacker is staggered"; break;
    case CombatTeachingCue::None:
        if (teaching.stage == TutorialStage::Complete || teaching.stage == TutorialStage::Skipped)
            return {};
        if (teaching.parryLearned) lesson = "O NEXT: DODGE THE KEEPER'S CHARGE";
        break;
    }
    const float progress = std::isfinite(teaching.cueProgress)
        ? std::clamp(teaching.cueProgress, 0.0f, 1.0f) : 0.0f;
    const int filled = std::clamp(static_cast<int>(std::lround(progress * 6.0f)), 0, 6);
    std::string bar;
    bar.reserve(6u);
    for (int index = 0; index < 6; ++index) bar.push_back(index < filled ? '#' : '-');
    std::string text = "COMBAT LESSON [" + bar + "]\r\n" + lesson;
    if (teaching.slowdownActive) text += "  |  TIME EASED";
    text += "\r\nPARRY: Q  |  DODGE: SPACE + A/D  |  MOVE/LOOK LIVE  |  RETRY SAFE";
    return text;
}
} // namespace horde::platform::windows
