#pragma once
#include <span>
#include <string>
#include <string_view>

namespace horde::platform::windows {
enum class ForestRouteCaptureMode { Disabled, BakedResident, SharedExisting64, SharedF01Sector76 };
struct ForestRouteProofLaunch {
    ForestRouteCaptureMode mode=ForestRouteCaptureMode::Disabled;
    std::string error;
};
// Representation A/B plus a separately selected frozen F01 composition proof.
// Normal captures, preferences, campaigns and Release remain unchanged.
inline ForestRouteProofLaunch ParseForestRouteProofLaunch(
    std::span<const std::wstring_view> arguments,bool debugBuild) {
    ForestRouteProofLaunch result;
    bool requested=false,hidden=false,capture=false,chapter=false,conflict=false;
    for(std::size_t i=0;i<arguments.size();++i) {
        const auto a=arguments[i];
        if(a==L"--forest-route-proof") {
            if(requested || !debugBuild || i+1>=arguments.size()) {
                result.error="Forest route proof requires one Debug baked-resident/shared64/f01-sector selection.";return result;
            }
            requested=true;const auto value=arguments[++i];
            if(value==L"baked-resident")result.mode=ForestRouteCaptureMode::BakedResident;
            else if(value==L"shared64")result.mode=ForestRouteCaptureMode::SharedExisting64;
            else if(value==L"f01-sector")result.mode=ForestRouteCaptureMode::SharedF01Sector76;
            else {result.error="Forest route proof mode must be baked-resident, shared64 or f01-sector.";return result;}
        }
        else if(a==L"--capture-hidden")hidden=true;
        else if(a==L"--capture-showcase") {
            capture=i+1<arguments.size() && !arguments[i+1].starts_with(L"--");
            if(capture)++i;
        }
        else if(a==L"--development-checkpoint") {
            chapter=i+1<arguments.size() && arguments[i+1].starts_with(L"chapter-");
            if(i+1<arguments.size())++i;
        }
        else if(a.starts_with(L"--benchmark") || a.starts_with(L"--measure") || a.starts_with(L"--validate") ||
                a==L"--capture-graphics-preview" || a==L"--capture-hidden-preview-pair" ||
                a==L"--entry-menu-slice" || a==L"--inspect-static-world-source" || a==L"--anatomical-player-mount")conflict=true;
    }
    if(requested && (!hidden || !capture || !chapter || conflict))
        result.error="Forest route proof requires only a hidden authored-chapter showcase capture; no benchmark or cutover validation.";
    return result;
}
}
