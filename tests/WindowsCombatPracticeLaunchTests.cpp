#include "platform/windows/WindowsCombatPracticeLaunch.h"
#include "platform/windows/WindowsCombatTeachingPrompt.h"

#include <array>
#include <cmath>
#include <iostream>
#include <limits>
#include <string>
#include <vector>

int main()
{
    using namespace horde::platform::windows;
    bool passed = true;
    const auto check = [&](const bool value, const char* message)
    {
        if (!value) { passed = false; std::cerr << message << '\n'; }
    };
    const auto eased = ParseWindowsCombatPracticeLaunch({L"--development-rescue-journey", L"--development-teaching-slowdown"});
    check(eased.error.empty() && eased.teachingSlowdown && eased.practice == WindowsCombatPractice::None,
          "explicit eased journey keeps the ordinary route and opts into lesson slowdown");
    check(!ParseWindowsCombatPracticeLaunch({L"--development-teaching-slowdown"}, false).error.empty(),
          "eased teaching launch is Debug-only");
    check(!ParseWindowsCombatPracticeLaunch({L"--development-teaching-slowdown", L"--capture-showcase"}).error.empty(),
          "eased interactive teaching cannot contaminate a frozen capture");
    check(!ParseWindowsCombatPracticeLaunch({L"--development-teaching-slowdown", L"--development-teaching-slowdown"}).error.empty(),
          "duplicate teaching launch flags are rejected");
    const auto parry = ParseWindowsCombatPracticeLaunch({L"--development-combat-practice"});
    check(parry.error.empty() && parry.practice == WindowsCombatPractice::Parry,
          "parry practice launch is admitted in Debug");
    const auto keeper = ParseWindowsCombatPracticeLaunch({L"--development-keeper-practice"});
    check(keeper.error.empty() && keeper.practice == WindowsCombatPractice::Keeper,
          "Keeper practice launch is admitted in Debug");
    check(ParseWindowsCombatPracticeLaunch({L"--development-combat-practice"}, false).practice ==
              WindowsCombatPractice::None &&
          !ParseWindowsCombatPracticeLaunch({L"--development-combat-practice"}, false).error.empty(),
          "Release rejects the live practice switch");
    check(!ParseWindowsCombatPracticeLaunch({L"--development-combat-practice",
                                              L"--development-combat-practice"}).error.empty(),
          "duplicate launch rejected");
    check(!ParseWindowsCombatPracticeLaunch({L"--development-combat-practice",
                                              L"--development-keeper-practice"}).error.empty(),
          "conflicting practices rejected");
    for (const auto conflict : {L"--capture-showcase", L"--benchmark-showcase",
                                L"--development-checkpoint", L"--validate-output-resize",
                                L"--validate-native-motion", L"--debug-rt-lab",
                                L"--development-world-route", L"--development-vertical-proof",
                                L"--entry-menu-slice"})
    {
        check(!ParseWindowsCombatPracticeLaunch({L"--development-keeper-practice", conflict}).error.empty(),
              "practice cannot run alongside capture/benchmark work");
    }
    check(ParseWindowsCombatPracticeLaunch({L"--unrelated"}).error.empty() &&
              ParseWindowsCombatPracticeLaunch({L"--unrelated"}).practice == WindowsCombatPractice::None,
          "unrelated ordinary arguments remain ignored");
    using horde::gameplay::simulation::AttackReadability;
    horde::gameplay::simulation::CombatTeachingSnapshot persistent;
    persistent.enabled = false;
    persistent.stage = horde::gameplay::simulation::TutorialStage::Skipped;
    persistent.promptOpacity = 0.0f;
    persistent.attackReadability = AttackReadability::Parryable;
    check(CombatTeachingPromptText(persistent) == "<> YELLOW guard-break: parry/dodge" &&
              horde::platform::windows::HasAttackReadabilityCue(persistent) &&
              horde::platform::windows::AttackReadabilityColourRgb(persistent.attackReadability) == 0xFFD266u,
          "yellow parryable attack has a diamond/text/color cue after the lesson is skipped or disabled");
    persistent.attackReadability = AttackReadability::Unblockable;
    check(CombatTeachingPromptText(persistent) == "[X] RED: unblockable - DODGE" &&
              horde::platform::windows::AttackReadabilityColourRgb(persistent.attackReadability) == 0xFF6B6Bu,
          "red unblockable attack has a distinct cross/text/color dodge cue without lesson spam");
    persistent.enabled = true;
    persistent.stage = horde::gameplay::simulation::TutorialStage::Complete;
    check(CombatTeachingPromptText(persistent, true) == "[X] RED: unblockable - DODGE",
          "completed teaching does not suppress or append a lesson to current attack readability");
    persistent.attackReadability = AttackReadability::Neutral;
    check(CombatTeachingPromptText(persistent).empty() &&
              !horde::platform::windows::HasAttackReadabilityCue(persistent),
          "neutral returns to no persistent warning after the lesson");

    using horde::gameplay::simulation::CombatTeachingCue;
    using horde::gameplay::simulation::TutorialStage;
    horde::gameplay::simulation::CombatTeachingSnapshot teaching;
    teaching.enabled = true;
    teaching.stage = TutorialStage::Parry;
    teaching.promptOpacity = 1.0f;
    teaching.cue = CombatTeachingCue::ParryWindup;
    teaching.cueProgress = 0.5f;
    const auto windup = CombatTeachingPromptText(teaching);
    check(windup.find("Watch raised arm") != std::string::npos &&
              windup.find("COMBAT LESSON") == std::string::npos &&
              windup.find("###") == std::string::npos &&
              windup.find("[Q]") != std::string::npos &&
              windup.find("fresh [Q] press") != std::string::npos,
          "short in-world wind-up prompt shows current keyboard/controller bindings without diagnostic chrome");
    teaching.cue = CombatTeachingCue::ParryNow;
    const auto press = CombatTeachingPromptText(teaching);
    check(press.find("PARRY NOW") != std::string::npos &&
              press.find("fresh [Q] press") != std::string::npos &&
              press.find("Watch raised arm") == std::string::npos,
          "anticipatory press and live blade-window prompts stay distinct");
    teaching.cue = CombatTeachingCue::ParryActive;
    const auto active = CombatTeachingPromptText(teaching);
    check(active.find("Timed parry active") != std::string::npos &&
              active.find("face attacker") != std::string::npos &&
              active.find("PARRY NOW") == std::string::npos,
          "filled shape is reserved for the live blade window");
    teaching.stage = TutorialStage::Dodge;
    teaching.cue = CombatTeachingCue::DodgeNow;
    teaching.cueProgress = 4.0f;
    const auto dodge = CombatTeachingPromptText(teaching);
    check(dodge.find("DODGE NOW") != std::string::npos &&
              dodge.find("######") == std::string::npos &&
              dodge.find("[Space] + move") != std::string::npos,
          "dodge uses a short directional cue and current keyboard/controller bindings");
    const auto controllerDodge = CombatTeachingPromptText(teaching, true);
    check(controllerDodge.find("[B] + stick") != std::string::npos &&
          controllerDodge.find("[LT]") == std::string::npos &&
          controllerDodge.find("[Space]") == std::string::npos && dodge.find("[LT]") == std::string::npos,
          "active input glyphs select controller or keyboard without changing gameplay");
    teaching.promptOpacity = 0.0f;
    check(CombatTeachingPromptText(teaching).empty(), "zero-opacity cue hides the banner");
    check(CombatTeachingPromptAlpha(0.5f, false) == 128u &&
              CombatTeachingPromptAlpha(0.5f, true) == 255u &&
              CombatTeachingPromptAlpha(std::numeric_limits<float>::quiet_NaN(), false) == 0u,
          "prompt opacity is bounded and reduced motion removes fades");
    check(CombatTeachingPromptFadeAlpha(128u, 60u, false) == 64u &&
              CombatTeachingPromptFadeAlpha(255u, 120u, false) == 0u &&
              CombatTeachingPromptFadeAlpha(255u, 20u, true) == 0u,
          "completion fades over 120ms and reduced motion hides immediately");
    teaching.promptOpacity = 1.0f;
    teaching.cue = CombatTeachingCue::None;
    teaching.stage = TutorialStage::Skipped;
    check(CombatTeachingPromptText(teaching).empty(), "skip and completion do not leave a banner behind");
    teaching.stage=TutorialStage::Block;teaching.safePractice=true;teaching.promptOpacity=0.0f;
    teaching.attackReadability=AttackReadability::Parryable;
    const auto protectedBlock=CombatTeachingPromptText(teaching);
    check(protectedBlock.find("Practice: no damage") == 0 &&
          protectedBlock.find("block normal hits") != std::string::npos &&
          protectedBlock.find("YELLOW guard-break: parry/dodge") != std::string::npos &&
          protectedBlock.find("half damage") == std::string::npos,
          "actual protection and normal-block lesson survive yellow guard-break warning and zero lesson opacity");
    teaching.cue=CombatTeachingCue::Learned;
    check(CombatTeachingPromptText(teaching).find("Practice: no damage") == 0 &&
          CombatTeachingPromptText(teaching).find("Well parried")!=std::string::npos &&
          CombatTeachingPromptText(teaching).find("block normal hits")==std::string::npos,
          "actual parry success survives an out-of-order Block stage while protection stays truthful");
    teaching.cue=CombatTeachingCue::Block;
    check(CombatTeachingPromptText(teaching).find("Well parried")==std::string::npos,
          "an ordinary Hit without Learned cue cannot fabricate parry success");
    teaching.cue=CombatTeachingCue::None;
    teaching.stage=TutorialStage::AttackFromGuard;teaching.attackReadability=AttackReadability::Unblockable;
    check(CombatTeachingPromptText(teaching,true).find("Hold [LT] + [RT]") != std::string::npos &&
          CombatTeachingPromptText(teaching,true).find("RED: unblockable - DODGE") != std::string::npos,
          "guard attack teaching remains alongside authoritative red warning");
    for(const auto stage:{TutorialStage::Block,TutorialStage::Parry,TutorialStage::Dodge,TutorialStage::AttackFromGuard,TutorialStage::Ready})
    {
        teaching.stage=stage;
        check(std::string(CombatPracticeActionLabel(teaching)) == (stage==TutorialStage::Ready?"Finish practice":"Skip practice"),
              "only Ready exposes explicit Finish while other stages remain Skip");
        check(!teaching.blockLearned && !teaching.attackFromGuardLearned,
              "presenter never awards skill flags while labelling Ready or Skip");
    }
    teaching.stage=TutorialStage::Dodge;teaching.attackReadability=AttackReadability::Neutral;
    teaching.source=horde::gameplay::simulation::EntityId::SkeletonA;
    check(CombatTeachingPromptText(teaching).find("Skeleton attack")!=std::string::npos &&
          CombatTeachingPromptText(teaching).find("Keeper")==std::string::npos,
          "skeleton dodge practice cannot describe a future Keeper");
    teaching.source=horde::gameplay::simulation::EntityId::Lich;
    check(CombatTeachingPromptText(teaching).find("Keeper lightning")!=std::string::npos,
          "Keeper is named only from its published source");
    teaching.exitPending=true;
    check(CombatTeachingPromptText(teaching).find("Practice: no damage") == 0 &&
          CombatTeachingPromptText(teaching).find("Leaving practice safely")!=std::string::npos,
          "pending safe exit retains actual protection status");
    teaching.exitPending=false;teaching.safePractice=false;teaching.stage=TutorialStage::Skipped;
    check(CombatTeachingPromptText(teaching).empty(),"completed safe exit has no stale protected caption");
    return passed ? 0 : 1;
}
