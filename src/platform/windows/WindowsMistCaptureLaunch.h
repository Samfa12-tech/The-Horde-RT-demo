#pragma once
#include <span>
#include <string>
#include <string_view>

namespace horde::platform::windows
{
struct WindowsMistCaptureLaunch
{
    bool off = false;
    std::string error;
};
inline WindowsMistCaptureLaunch ParseWindowsMistCaptureLaunch(
    const std::span<const std::wstring_view> arguments, const bool debugSupported)
{
    WindowsMistCaptureLaunch result;
    unsigned showcases = 0u;
    bool otherMode = false;
    for (const auto argument : arguments)
    {
        if (argument == L"--capture-showcase") ++showcases;
        if (argument == L"--capture-graphics-preview" || argument == L"--benchmark-showcase" ||
            argument == L"--validate-native-motion" || argument == L"--validate-output-resize") otherMode = true;
        if (argument != L"--mist-off") continue;
        if (result.off) { result.error = "--mist-off may only be specified once."; return result; }
        result.off = true;
    }
    if (result.off && (!debugSupported || showcases != 1u || otherMode))
        result.error = "--mist-off requires exactly one Debug --capture-showcase run.";
    return result;
}
}
