#pragma once
#include <cmath>
#include <span>
#include <string>
#include <string_view>
#include "scene/ChapterRegionalWater.h"

namespace horde::platform::windows
{
struct WindowsEnvironmentSurveyLaunch
{
    bool requested = false, waterDisabled = false;
    std::string profile, error;
};
inline WindowsEnvironmentSurveyLaunch ParseWindowsEnvironmentSurveyLaunch(
    std::span<const std::wstring_view> arguments, bool debugBuild)
{
    WindowsEnvironmentSurveyLaunch result;
    bool capture=false, hidden=false, chapter=false, conflict=false;
    for(std::size_t i=0;i<arguments.size();++i)
    {
        const auto a=arguments[i];
        if(a==L"--capture-environment-survey")
        {
            if(result.requested || i+1==arguments.size() || arguments[i+1]!=L"river-bend")
            {result.error="Environment survey requires exactly one river-bend profile.";return result;}
            result.requested=true;result.profile="river-bend";++i;
        }
        else if(a==L"--survey-water-disabled")
        {
            if(result.waterDisabled){result.error="Duplicate survey water baseline modifier.";return result;}
            result.waterDisabled=true;
        }
        else if(a==L"--capture-showcase")
        {if(capture)conflict=true;capture=i+1<arguments.size() && !arguments[i+1].starts_with(L"--");if(capture)++i;}
        else if(a==L"--capture-hidden")hidden=true;
        else if(a==L"--development-rescue-journey")chapter=true;
        else if(a==L"--require-rayquery-compute"){} // Same scene, explicit real RT backend.
        else conflict=true; // No gameplay seed, camera, tuning or another validation path.
    }
    if(result.waterDisabled && !result.requested)
        result.error="--survey-water-disabled requires an explicit environment survey.";
    if(result.requested && (!debugBuild || !capture || !hidden || !chapter || conflict))
        result.error="Environment survey requires a hidden Debug chapter showcase capture without gameplay staging or another validation mode.";
    return result;
}

struct EnvironmentSurveyCamera
{
    float x=0,z=0,supportY=0,yaw=0,pitch=-.18f;
};
inline EnvironmentSurveyCamera RiverBendSurveyCamera()
{
    // Original atlas shore west of the E58/N-25 bend. Elevation is never taken
    // from this plan: the capture must query the actual imported terrain.
    const auto shore=horde::scene::ChapterWaterFromAtlas(56,-5,0);
    const auto target=horde::scene::ChapterWaterFromAtlas(58,-25,6);
    return {shore[0],shore[2],0,std::atan2(target[0]-shore[0],shore[2]-target[2]),-.18f};
}
inline constexpr unsigned kEnvironmentSurveyFrames=120;
inline constexpr unsigned kEnvironmentSurveyCaptureStride=10;
inline constexpr float EnvironmentSurveyTime(unsigned frame){return float(frame)/60.f;}
}
