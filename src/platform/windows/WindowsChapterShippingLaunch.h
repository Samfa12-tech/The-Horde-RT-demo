#pragma once
#include <span>
#include <string>
#include <string_view>
namespace horde::platform::windows {
struct WindowsChapterShippingLaunch {
    bool requested=false,foregroundRequested=false,bellwetherDoorway=false,bellwetherResidentRoundTrip=false;
    bool bellwetherWestRoundTrip=false;
    std::string error;
};
inline WindowsChapterShippingLaunch ParseWindowsChapterShippingLaunch(
    std::span<const std::wstring_view> args,bool debugSupported,bool optimizedMeasurementSupported=false) {
    WindowsChapterShippingLaunch result;
    unsigned requests=0,doorway=0,residentRoundTrips=0,westRoundTrips=0,captures=0,checkpoints=0,foreground=0,hidden=0;
    unsigned residentFixtures=0,campaignDirectories=0;
    bool reunion=false,repeatVisit=false,conflict=false;
    for(std::size_t i=0;i<args.size();++i) {
        const auto a=args[i];
        requests+=a==L"--measure-chapter-shipping-route";
        doorway+=a==L"--measure-bellwether-doorway";
        residentRoundTrips+=a==L"--measure-bellwether-resident-roundtrip";
        westRoundTrips+=a==L"--measure-bellwether-west-roundtrip";
        foreground+=a==L"--measure-chapter-foreground";
        captures+=a==L"--capture-showcase";
        hidden+=a==L"--capture-hidden";
        residentFixtures+=a==L"--chapter-resident-fixture";
        campaignDirectories+=a==L"--bellwether-campaign-dir";
        if(a==L"--development-checkpoint") {
            ++checkpoints;reunion=i+1<args.size() && args[i+1]==L"chapter-kit-reunion";
            repeatVisit=i+1<args.size() && args[i+1]==L"bellwether-repeat-visit";
        }
        if(a==L"--validate-chapter-roundtrip" || a==L"--validate-static-zone-cutover" ||
           a==L"--validate-static-triangle-cutover" || a==L"--capture-graphics-preview" ||
           a==L"--benchmark-showcase" || a==L"--validate-output-resize" ||
           a==L"--validate-native-motion" || a==L"--capture-sky-attribution" ||
           a==L"--mist-off" || a==L"--capture-dust" || a==L"--capture-portrait" ||
           a.starts_with(L"--rt-lab-")) conflict=true;
    }
    result.bellwetherDoorway=doorway!=0 || residentRoundTrips!=0 || westRoundTrips!=0;
    result.bellwetherResidentRoundTrip=residentRoundTrips!=0 || westRoundTrips!=0;
    result.bellwetherWestRoundTrip=westRoundTrips!=0;
    result.requested=requests!=0 || result.bellwetherDoorway;
    result.foregroundRequested=foreground!=0;
    if(foreground && (!result.requested || foreground!=1))
        result.error="--measure-chapter-foreground requires exactly one bounded chapter measurement.";
    const bool hiddenRepeatVisit=debugSupported && !requests && captures==1 && checkpoints==1 &&
        repeatVisit && hidden==1 && !foreground && !conflict;
    if(doorway && (!hiddenRepeatVisit || doorway!=1 || residentFixtures!=1 || campaignDirectories || result.bellwetherResidentRoundTrip))
        result.error="--measure-bellwether-doorway requires one hidden Debug bellwether-repeat-visit capture with a resident fixture and no tuning overrides.";
    if(result.bellwetherResidentRoundTrip && (!hiddenRepeatVisit || residentRoundTrips+westRoundTrips!=1 ||
       campaignDirectories!=1 || residentFixtures || doorway))
        result.error="Bellwether round trips require one hidden Debug bellwether-repeat-visit capture with the combined campaign directory.";
    if(requests && !result.bellwetherDoorway && (!(debugSupported || optimizedMeasurementSupported) || requests!=1 || captures!=1 ||
       checkpoints!=1 || !reunion || conflict))
        result.error="--measure-chapter-shipping-route requires a Debug or separately enabled optimized measurement build and one chapter-kit-reunion capture without tuning overrides.";
    return result;
}
}
