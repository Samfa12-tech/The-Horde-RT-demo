#pragma once
#include "platform/windows/WindowsGraphicsPreviewCapture.h"
#include <span>
#include <string>
#include <string_view>

namespace horde::platform::windows
{
struct WindowsMotionEvidenceLaunch
{
    bool requested = false;
    std::wstring outputDirectory;
    std::string scenario;
    std::string error;
};
inline WindowsMotionEvidenceLaunch ParseWindowsMotionEvidenceLaunch(
    std::span<const std::wstring_view> arguments)
{
    WindowsMotionEvidenceLaunch result;
    bool scenarioSeen = false;
    bool conflict = false;
    bool unknown = false;
    bool computeSeen = false;
    bool motionArgument = false;
    for (const auto argument : arguments)
        motionArgument |= argument == L"--validate-native-motion" || argument == L"--motion-scenario";
    if (motionArgument && arguments.size() > 16)
    { result.error = "Native motion accepts at most sixteen command-line entries."; return result; }
    for (std::size_t i = 0; i < arguments.size(); ++i)
    {
        const auto argument = arguments[i];
        if (argument == L"--validate-native-motion")
        {
            if (result.requested || i+1 == arguments.size() || arguments[i+1].starts_with(L"--"))
            { result.error = "--validate-native-motion requires one unique absolute fresh output directory."; return result; }
            result.requested = true; result.outputDirectory = arguments[++i];
            if (!AbsoluteWindowsCapturePath(result.outputDirectory) || result.outputDirectory.size() > 32700 ||
                result.outputDirectory.starts_with(L"\\\\?\\") || result.outputDirectory.starts_with(L"\\\\.\\"))
            { result.error = "Native motion requires an ordinary absolute Windows output directory."; return result; }
        }
        else if (argument == L"--motion-scenario")
        {
            if (scenarioSeen || i+1 == arguments.size())
            { result.error = "--motion-scenario requires one unique literal scenario."; return result; }
            scenarioSeen = true;
            const auto scenario = arguments[++i];
            for (const auto literal : {"torch-low-opening", "shaft-up", "keeper-first-entry", "keeper-retry-reward"})
                if (scenario == std::wstring(literal, literal + std::char_traits<char>::length(literal))) result.scenario = literal;
            if (result.scenario.empty())
            { result.error = "Unknown native motion scenario."; return result; }
        }
        else if (argument == L"--require-rayquery-compute") { unknown |= computeSeen; computeSeen = true; }
        else
        {
            unknown = true;
            if (argument.starts_with(L"--capture") || argument.starts_with(L"--benchmark") ||
                argument.starts_with(L"--rt-lab") || argument == L"--debug-rt-lab" ||
                argument == L"--development-checkpoint" || argument == L"--validate-output-resize" ||
                argument == L"--anatomical-player-mount" || argument.starts_with(L"--report")) conflict = true;
        }
    }
    if (!result.requested && scenarioSeen) result.error = "--motion-scenario requires --validate-native-motion.";
    else if (result.requested && !scenarioSeen) result.error = "Native motion requires --motion-scenario.";
    else if (result.requested && (conflict || unknown))
        result.error = "Native motion cannot share capture, benchmark, checkpoint, RT Lab, report or unknown options.";
    return result;
}
}
