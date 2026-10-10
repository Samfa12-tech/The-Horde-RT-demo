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
    const gameplay::simulation::CombatTeachingSnapshot& teaching, const bool controller = false)
{
    using gameplay::simulation::CombatTeachingCue;
    using gameplay::simulation::TutorialStage;
    if (!teaching.enabled || !std::isfinite(teaching.promptOpacity) || teaching.promptOpacity <= 0.0f)
        return {};
    const char* lesson = "O Watch the attacker";
    switch (teaching.cue)
    {
    case CombatTeachingCue::ParryWindup:
        lesson = "O Watch the raised blade";
        break;
    case CombatTeachingCue::ParryNow: lesson = "o PARRY NOW - raise your guard"; break;
    case CombatTeachingCue::ParryActive: lesson = "(*) Guard up - face the blade"; break;
    case CombatTeachingCue::ParryRecovery: lesson = "-> Ready for the next strike"; break;
    case CombatTeachingCue::DodgeWindup: lesson = "O The Keeper gathers lightning - watch the staff"; break;
    case CombatTeachingCue::DodgeNow: lesson = "<-- DODGE NOW --> - step sideways"; break;
    case CombatTeachingCue::Recovery: lesson = "-> Close in before the next charge"; break;
    case CombatTeachingCue::Learned: lesson = "(*) Well parried - strike back"; break;
    case CombatTeachingCue::None:
        if (teaching.stage == TutorialStage::Complete || teaching.stage == TutorialStage::Skipped)
            return {};
        if (teaching.parryLearned) lesson = "O Dodge the Keeper's lightning";
        break;
    }
    std::string text = lesson;
    text += controller ? "\r\n[LT] Parry  |  [B] + left stick: Dodge"
                       : "\r\n[Q] Parry  |  [Space] + A/D: Dodge";
    return text;
}
} // namespace horde::platform::windows
