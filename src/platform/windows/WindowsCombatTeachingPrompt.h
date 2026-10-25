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

inline bool HasAttackReadabilityCue(const gameplay::simulation::CombatTeachingSnapshot& teaching)
{
    using gameplay::simulation::AttackReadability;
    return teaching.attackReadability == AttackReadability::Parryable ||
           teaching.attackReadability == AttackReadability::Unblockable;
}

// RGB channels only; the native presenter preserves OS high-contrast colors.
inline constexpr std::uint32_t AttackReadabilityColourRgb(
    const gameplay::simulation::AttackReadability value)
{
    using gameplay::simulation::AttackReadability;
    return value == AttackReadability::Parryable ? 0xFFD266u :
           value == AttackReadability::Unblockable ? 0xFF6B6Bu : 0xF2E9D8u;
}

inline const char* CombatPracticeActionLabel(const gameplay::simulation::CombatTeachingSnapshot& teaching)
{
    return teaching.stage == gameplay::simulation::TutorialStage::Ready ? "Finish practice" : "Skip practice";
}

inline std::string CombatTeachingLessonText(
    const gameplay::simulation::CombatTeachingSnapshot& teaching, const bool controller)
{
    using gameplay::simulation::CombatTeachingCue;
    using gameplay::simulation::TutorialStage;
    const std::string guard=controller?"[LT]":"[Q]", swing=controller?"[RT]":"[LMB]";
    const std::string dodge=controller?"[B] + stick":"[Space] + move";
    if(teaching.exitPending || teaching.cue==CombatTeachingCue::ExitPending)return "Leaving practice safely...";
    if(teaching.cue==CombatTeachingCue::Replay)return "Menu: Replay opening practice or Skip to continue";
    if(teaching.stage==TutorialStage::Complete || teaching.stage==TutorialStage::Skipped)return {};
    if(teaching.stage==TutorialStage::Ready || teaching.cue==CombatTeachingCue::Ready)
        return "Menu: Finish practice - enemies can hurt you";
    if(teaching.cue==CombatTeachingCue::Learned)return "Well parried - strike back";
    if(teaching.stage==TutorialStage::Block || teaching.cue==CombatTeachingCue::Block)return "Hold "+guard+": block normal hits";
    if(teaching.stage==TutorialStage::AttackFromGuard || teaching.cue==CombatTeachingCue::AttackFromGuard)return "Hold "+guard+" + "+swing+"; keep held";
    if(teaching.stage==TutorialStage::Dodge)
    {
        if(teaching.cue==CombatTeachingCue::DodgeNow)return "DODGE NOW - "+dodge;
        return (teaching.source==gameplay::simulation::EntityId::Lich?"Keeper lightning - ":
            teaching.source==gameplay::simulation::EntityId::SkeletonA || teaching.source==gameplay::simulation::EntityId::SkeletonB?
                "Skeleton attack - ":"Attacker - ")+dodge;
    }
    switch(teaching.cue)
    {
    case CombatTeachingCue::ParryWindup:return "Watch raised arm - fresh "+guard+" press";
    case CombatTeachingCue::ParryNow:return "PARRY NOW - fresh "+guard+" press";
    case CombatTeachingCue::ParryActive:return "Timed parry active - face attacker";
    case CombatTeachingCue::ParryRecovery:return "Release, then press "+guard+" to parry";
    default:return "Fresh "+guard+" press: timed parry";
    }
}

inline std::string CombatTeachingPromptText(
    const gameplay::simulation::CombatTeachingSnapshot& teaching, const bool controller = false,
    const bool waitingForRevivalClearance = false)
{
    using gameplay::simulation::AttackReadability;
    std::string text=teaching.safePractice?"Practice: no damage":"";
    if(teaching.safePractice || (teaching.enabled && std::isfinite(teaching.promptOpacity) && teaching.promptOpacity>0.0f))
    {
        const auto lesson=CombatTeachingLessonText(teaching,controller);
        if(!lesson.empty())text+=(text.empty()?"":"\r\n")+lesson;
    }
    if(waitingForRevivalClearance)
        text+=(text.empty()?"":"\r\n")+std::string("Step back from the skeletons.");
    if(teaching.attackReadability==AttackReadability::Parryable)
        text+=(text.empty()?"":"\r\n")+std::string("<> YELLOW guard-break: parry/dodge");
    else if(teaching.attackReadability==AttackReadability::Unblockable)
        text+=(text.empty()?"":"\r\n")+std::string("[X] RED: unblockable - DODGE");
    return text;
}
} // namespace horde::platform::windows
