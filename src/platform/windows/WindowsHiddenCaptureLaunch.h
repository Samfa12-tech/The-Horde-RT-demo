#pragma once
#include <span>
#include <string>
#include <string_view>

namespace horde::platform::windows
{
struct WindowsHiddenCaptureLaunch
{
    bool hidden = false;
    std::string error;
};

// A visibility modifier for the existing Debug showcase capture, never an
// alternate renderer, camera route, benchmark or permission to acquire input.
inline WindowsHiddenCaptureLaunch ParseWindowsHiddenCaptureLaunch(
    std::span<const std::wstring_view> arguments, bool debugBuild)
{
    WindowsHiddenCaptureLaunch result;
    bool capture = false, incompatible = false;
    for (std::size_t i = 0; i < arguments.size(); ++i)
    {
        const auto argument = arguments[i];
        if (argument == L"--capture-hidden")
        {
            if (result.hidden)
            {
                result.error = "--capture-hidden may only be specified once.";
                return result;
            }
            result.hidden = true;
        }
        else if (argument == L"--capture-showcase")
        {
            capture = i + 1 < arguments.size() && !arguments[i + 1].starts_with(L"--");
            if (capture) ++i;
        }
        else if (argument == L"--benchmark-showcase" ||
                 argument == L"--measure-chapter-shipping-route" ||
                 argument == L"--measure-chapter-foreground" ||
                 argument == L"--validate-native-motion" ||
                 argument == L"--validate-output-resize" ||
                 argument == L"--capture-graphics-preview") incompatible = true;
    }
    if (result.hidden && (!debugBuild || !capture || incompatible))
        result.error = "--capture-hidden requires a Debug showcase capture without foreground or other validation modes.";
    return result;
}
}
