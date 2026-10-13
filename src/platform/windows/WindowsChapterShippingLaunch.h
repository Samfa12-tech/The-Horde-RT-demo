#pragma once
#include <span>
#include <string>
#include <string_view>
namespace horde::platform::windows {
struct WindowsChapterShippingLaunch { bool requested=false;std::string error; };
inline WindowsChapterShippingLaunch ParseWindowsChapterShippingLaunch(
    std::span<const std::wstring_view> args,bool debugSupported) {
    WindowsChapterShippingLaunch result;
    unsigned requests=0,captures=0,checkpoints=0;
    bool reunion=false,conflict=false;
    for(std::size_t i=0;i<args.size();++i) {
        const auto a=args[i];
        requests+=a==L"--measure-chapter-shipping-route";
        captures+=a==L"--capture-showcase";
        if(a==L"--development-checkpoint") {
            ++checkpoints;reunion=i+1<args.size() && args[i+1]==L"chapter-kit-reunion";
        }
        if(a==L"--validate-chapter-roundtrip" || a==L"--validate-static-zone-cutover" ||
           a==L"--validate-static-triangle-cutover" || a==L"--capture-graphics-preview" ||
           a==L"--benchmark-showcase" || a==L"--validate-output-resize" ||
           a==L"--validate-native-motion" || a==L"--capture-sky-attribution" ||
           a==L"--mist-off" || a==L"--capture-dust" || a==L"--capture-portrait" ||
           a.starts_with(L"--rt-lab-")) conflict=true;
    }
    result.requested=requests!=0;
    if(result.requested && (!debugSupported || requests!=1 || captures!=1 ||
       checkpoints!=1 || !reunion || conflict))
        result.error="--measure-chapter-shipping-route requires only one Debug chapter-kit-reunion capture without tuning overrides.";
    return result;
}
}
