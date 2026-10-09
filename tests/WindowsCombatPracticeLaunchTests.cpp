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
    using horde::gameplay::simulation::CombatTeachingCue;
    using horde::gameplay::simulation::TutorialStage;
    horde::gameplay::simulation::CombatTeachingSnapshot teaching;
    teaching.enabled = true;
    teaching.promptOpacity = 1.0f;
    teaching.cue = CombatTeachingCue::ParryWindup;
    teaching.cueProgress = 0.5f;
    const auto windup = CombatTeachingPromptText(teaching);
    check(windup.find("O SKELETON A WIND-UP") != std::string::npos &&
              windup.find("###---") != std::string::npos,
          "wind-up uses hollow shape and bounded progress");
    teaching.cue = CombatTeachingCue::ParryNow;
    const auto press = CombatTeachingPromptText(teaching);
    check(press.find("o PRESS PARRY") != std::string::npos &&
              press.find("raise guard before the strike") != std::string::npos &&
              press.find("O SKELETON A WIND-UP") == std::string::npos,
          "anticipatory press and live blade-window prompts stay distinct");
    teaching.cue = CombatTeachingCue::ParryActive;
    const auto active = CombatTeachingPromptText(teaching);
    check(active.find("(*) PARRY WINDOW ACTIVE") != std::string::npos &&
              active.find("face the incoming blade") != std::string::npos &&
              active.find("PRESS PARRY") == std::string::npos,
          "filled shape is reserved for the live blade window");
    teaching.cue = CombatTeachingCue::DodgeNow;
    teaching.cueProgress = 4.0f;
    const auto dodge = CombatTeachingPromptText(teaching);
    check(dodge.find("<-- DODGE NOW -->") != std::string::npos &&
              dodge.find("######") != std::string::npos && dodge.find("MOVE/LOOK LIVE") != std::string::npos &&
              dodge.find("DODGE: SPACE + A/D") != std::string::npos,
          "dodge uses directional motion cue and bounded full progress");
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
    return passed ? 0 : 1;
}
