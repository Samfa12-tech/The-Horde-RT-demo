#pragma once
#include "graphics/DustQuality.h"
#include <cstddef>
#include <optional>
#include <span>
#include <string>
#include <string_view>

namespace horde::platform::windows
{
struct WindowsMistCaptureLaunch
{
    bool off = false;
    std::optional<horde::graphics::DustQuality> dustQuality;
    std::string error;
};
inline WindowsMistCaptureLaunch ParseWindowsMistCaptureLaunch(
    const std::span<const std::wstring_view> arguments, const bool debugSupported)
{
    WindowsMistCaptureLaunch result;
    unsigned showcases = 0u;
    bool otherMode = false;
    bool nativeMotion = false;
    bool incompatibleDustMode = false;
    for (std::size_t index = 0u; index < arguments.size(); ++index)
    {
        const auto argument = arguments[index];
        if (argument == L"--capture-showcase") ++showcases;
        if (argument == L"--capture-graphics-preview" || argument == L"--benchmark-showcase" ||
            argument == L"--validate-native-motion" || argument == L"--validate-output-resize") otherMode = true;
        if (argument == L"--capture-graphics-preview" || argument == L"--benchmark-showcase" ||
            argument == L"--validate-output-resize") incompatibleDustMode = true;
        if (argument == L"--validate-native-motion") nativeMotion = true;
        if (argument == L"--mist-off")
        {
            if (result.off) { result.error = "--mist-off may only be specified once."; return result; }
            result.off = true;
        }
        if (argument != L"--capture-dust") continue;
        if (result.dustQuality) { result.error = "--capture-dust may only be specified once."; return result; }
        if (index + 1u >= arguments.size() || arguments[index + 1u].starts_with(L"--"))
        { result.error = "--capture-dust requires low, standard, or off."; return result; }
        const auto value = arguments[++index];
        if (value == L"low") result.dustQuality = horde::graphics::DustQuality::Low;
        else if (value == L"standard") result.dustQuality = horde::graphics::DustQuality::Standard;
        else if (value == L"off") result.dustQuality = horde::graphics::DustQuality::Off;
        else { result.error = "--capture-dust requires low, standard, or off."; return result; }
    }
    if (result.off && (!debugSupported || showcases != 1u || otherMode))
        result.error = "--mist-off requires exactly one Debug --capture-showcase run.";
    if (result.dustQuality && (!debugSupported || incompatibleDustMode || (showcases != 1u && !nativeMotion) ||
        (showcases != 0u && nativeMotion)))
        result.error = "--capture-dust requires one Debug --capture-showcase or --validate-native-motion run.";
    return result;
}
}
