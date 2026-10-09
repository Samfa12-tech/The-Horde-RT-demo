#pragma once

#include <array>
#include <string>
#include <string_view>
#include <vector>

namespace horde::platform::windows
{
enum class WindowsCombatPractice : unsigned char { None, Parry, Keeper };

struct WindowsCombatPracticeLaunch
{
    WindowsCombatPractice practice = WindowsCombatPractice::None;
    std::string error;
};

inline WindowsCombatPracticeLaunch ParseWindowsCombatPracticeLaunch(
    const std::vector<std::wstring_view>& arguments, const bool debugBuild = true)
{
    WindowsCombatPracticeLaunch result;
    bool sawParry = false;
    bool sawKeeper = false;
    for (const std::wstring_view argument : arguments)
    {
        if (argument == L"--development-combat-practice")
        {
            if (sawParry) result.error = "--development-combat-practice may only be specified once.";
            sawParry = true;
        }
        else if (argument == L"--development-keeper-practice")
        {
            if (sawKeeper) result.error = "--development-keeper-practice may only be specified once.";
            sawKeeper = true;
        }
    }
    if (!result.error.empty()) return result;
    if (!sawParry && !sawKeeper) return result;
    if (!debugBuild)
    {
        result.error = "Development combat practice is Debug-only.";
        return result;
    }
    if (sawParry && sawKeeper)
    {
        result.error = "Choose only one development combat practice.";
        return result;
    }
    constexpr std::array<std::wstring_view, 16> conflicting{
        L"--capture-showcase", L"--capture-graphics-preview", L"--benchmark-showcase",
        L"--development-checkpoint", L"--validate-output-resize",
        L"--validate-native-motion", L"--benchmark-workload", L"--debug-rt-lab",
        L"--capture-dust", L"--capture-portrait", L"--development-world-route",
        L"--development-world-route-staged", L"--motion-scenario", L"--motion-rt-workload",
        L"--development-vertical-proof", L"--entry-menu-slice"};
    for (const std::wstring_view argument : arguments)
    {
        for (const auto conflict : conflicting)
        {
            if (argument == conflict)
            {
                result.error = "Development combat practice cannot be combined with captures, checkpoints, or benchmarks.";
                return result;
            }
        }
    }
    result.practice = sawKeeper ? WindowsCombatPractice::Keeper : WindowsCombatPractice::Parry;
    return result;
}
} // namespace horde::platform::windows
