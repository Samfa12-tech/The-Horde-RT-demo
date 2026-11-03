#pragma once
#include <span>
#include <string>
#include <string_view>

namespace horde::platform::windows {
enum class ForestRouteCaptureMode { Disabled, BakedResident, SharedExisting64, SharedF01Sector76, SharedRetention64, NormalShared64, SharedF01Grouped76, BakedF01North68, BakedSurroundings480 };
struct ForestRouteProofLaunch {
    ForestRouteCaptureMode mode=ForestRouteCaptureMode::Disabled;
    bool landingOnly=false;
    bool gpuStages=false;
    std::string error;
};
// Representation A/B plus a separately selected frozen F01 composition proof.
// Normal captures, preferences, campaigns and Release remain unchanged.
inline ForestRouteProofLaunch ParseForestRouteProofLaunch(
    std::span<const std::wstring_view> arguments,bool debugBuild) {
    ForestRouteProofLaunch result;
    bool requested=false,hidden=false,capture=false,chapter=false,conflict=false,roundtrip=false,reunion=false,routeStages=false;
    for(std::size_t i=0;i<arguments.size();++i) {
        const auto a=arguments[i];
        if(a==L"--forest-route-proof") {
            if(requested || !debugBuild || i+1>=arguments.size()) {
                result.error="Forest route proof requires one Debug baked-resident/shared64/f01-sector/shared64-retention selection.";return result;
            }
            requested=true;const auto value=arguments[++i];
            if(value==L"baked-resident")result.mode=ForestRouteCaptureMode::BakedResident;
            else if(value==L"baked-surroundings480")result.mode=ForestRouteCaptureMode::BakedSurroundings480;
            else if(value==L"baked-f01-north68")result.mode=ForestRouteCaptureMode::BakedF01North68;
            else if(value==L"shared64")result.mode=ForestRouteCaptureMode::SharedExisting64;
            else if(value==L"f01-sector")result.mode=ForestRouteCaptureMode::SharedF01Sector76;
            else if(value==L"f01-grouped-r4")result.mode=ForestRouteCaptureMode::SharedF01Grouped76;
            else if(value==L"shared64-retention")result.mode=ForestRouteCaptureMode::SharedRetention64;
            else if(value==L"normal-shared64")result.mode=ForestRouteCaptureMode::NormalShared64;
            else {result.error="Unknown forest route proof mode.";return result;}
        }
        else if(a==L"--capture-chapter-landing") {
            if(result.landingOnly || !debugBuild || i+1>=arguments.size()) {
                result.error="Landing observation requires one Debug control/stages selection.";return result;
            }
            result.landingOnly=true;const auto value=arguments[++i];
            if(value==L"stages")result.gpuStages=true;
            else if(value!=L"control") {result.error="Unknown landing observation mode.";return result;}
        }
        else if(a==L"--capture-chapter-route-stages") {
            if(!debugBuild || routeStages) {result.error="Route GPU stages require one Debug selection.";return result;}
            routeStages=true;result.gpuStages=true;
        }
        else if(a==L"--capture-hidden")hidden=true;
        else if(a==L"--capture-showcase") {
            capture=i+1<arguments.size() && !arguments[i+1].starts_with(L"--");
            if(capture)++i;
        }
        else if(a==L"--development-checkpoint") {
            chapter=i+1<arguments.size() && arguments[i+1].starts_with(L"chapter-");
            reunion=i+1<arguments.size() && arguments[i+1]==L"chapter-kit-reunion";
            if(i+1<arguments.size())++i;
        }
        else if(a==L"--validate-chapter-roundtrip")roundtrip=true;
        else if(a.starts_with(L"--benchmark") || a.starts_with(L"--measure") || a.starts_with(L"--validate") ||
                a==L"--capture-graphics-preview" || a==L"--capture-hidden-preview-pair" ||
                a==L"--entry-menu-slice" || a==L"--inspect-static-world-source" || a==L"--anatomical-player-mount")conflict=true;
    }
    if(roundtrip && ((result.mode!=ForestRouteCaptureMode::NormalShared64 &&
       result.mode!=ForestRouteCaptureMode::Disabled) || !reunion))conflict=true;
    if(routeStages && (!roundtrip || result.landingOnly || !hidden || !capture || !reunion || conflict))
        result.error="Route GPU stages require only the complete hidden normal-owner reunion roundtrip.";
    if(result.landingOnly && (!hidden || !capture || !reunion || conflict || roundtrip ||
       (result.mode!=ForestRouteCaptureMode::Disabled && result.mode!=ForestRouteCaptureMode::NormalShared64)))
        result.error="Landing observation requires only a hidden normal-owner reunion capture, separate from full-route validation.";
    if(requested && (!hidden || !capture || !chapter || conflict))
        result.error="Forest route proof requires only a hidden authored-chapter showcase capture; no benchmark or cutover validation.";
    return result;
}
}
