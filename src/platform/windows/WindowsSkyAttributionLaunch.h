#pragma once
#include <span>
#include <string>
#include <string_view>

namespace horde::platform::windows
{
enum class SkyAttributionMode { Production, DirectionalOnly, Disabled };
struct SkyAttributionLaunch {
    SkyAttributionMode mode=SkyAttributionMode::Production;
    bool requested=false;
    std::string error;
};
inline SkyAttributionLaunch ParseSkyAttributionLaunch(
    std::span<const std::wstring_view> arguments, bool debugBuild)
{
    SkyAttributionLaunch result;
    bool chapter=false,capture=false,checkpoint=false,incompatible=false;
    for(std::size_t i=0;i<arguments.size();++i) {
        const auto argument=arguments[i];
        const auto value=[&]() {return i+1<arguments.size() && !arguments[i+1].starts_with(L"--");};
        if(argument==L"--development-rescue-journey") chapter=true;
        else if(argument==L"--capture-showcase") {capture=value();if(capture) ++i;}
        else if(argument==L"--development-checkpoint") {
            checkpoint=value() && arguments[i+1].starts_with(L"chapter-");
            if(value()) ++i;
        }
        else if(argument==L"--validate-chapter-roundtrip" || argument==L"--validate-static-zone-cutover" ||
                argument==L"--validate-static-triangle-cutover" || argument==L"--benchmark-showcase" ||
                argument==L"--validate-native-motion" || argument==L"--validate-output-resize" ||
                argument==L"--capture-graphics-preview") incompatible=true;
        else if(argument==L"--capture-sky-attribution") {
            if(result.requested || !value()) {
                result.error="Sky attribution requires one explicit directional/off mode.";return result;
            }
            result.requested=true;
            const auto mode=arguments[++i];
            if(mode==L"directional") result.mode=SkyAttributionMode::DirectionalOnly;
            else if(mode==L"off") result.mode=SkyAttributionMode::Disabled;
            else {result.error="Unknown sky-attribution mode.";return result;}
        }
    }
    if(result.requested && (!debugBuild || !chapter || !capture || !checkpoint || incompatible))
        result.error="Sky attribution requires a frozen Debug chapter capture without another validation mode.";
    return result;
}
inline const char* SkyAttributionName(SkyAttributionMode mode) {
    switch(mode) {
    case SkyAttributionMode::DirectionalOnly:return "directional-only";
    case SkyAttributionMode::Disabled:return "sky-disabled";
    default:return "production";
    }
}
}
