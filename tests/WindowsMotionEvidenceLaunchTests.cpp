#include "platform/windows/WindowsMotionEvidenceLaunch.h"
#include <array>
#include <iostream>
#include <vector>
int main()
{
    using horde::platform::windows::ParseWindowsMotionEvidenceLaunch;
    int failures=0;
    const auto check=[&](bool ok){ if(!ok) ++failures; };
    for(const auto scenario:{L"torch-low-opening",L"shaft-up",L"keeper-first-entry",L"keeper-retry-reward"})
    {
        const std::array<std::wstring_view,4> args{L"--validate-native-motion",L"C:\\fresh motion",L"--motion-scenario",scenario};
        const auto parsed=ParseWindowsMotionEvidenceLaunch(args);
        check(parsed.requested && parsed.error.empty() && !parsed.scenario.empty());
    }
    for(const auto conflict:{L"--capture-showcase",L"--capture-graphics-preview",L"--benchmark-showcase",
        L"--development-checkpoint",L"--debug-rt-lab",L"--rt-lab-hue",L"--validate-output-resize",L"--report-preview",L"--unknown"})
    {
        const std::array<std::wstring_view,5> args{L"--validate-native-motion",L"C:\\fresh",L"--motion-scenario",L"shaft-up",conflict};
        check(!ParseWindowsMotionEvidenceLaunch(args).error.empty());
    }
    for(const auto path:{L"relative",L"C:relative",L"\\root",L"\\\\?\\C:\\device",L"\\\\.\\PhysicalDrive0"})
    {
        const std::array<std::wstring_view,4> args{L"--validate-native-motion",path,L"--motion-scenario",L"shaft-up"};
        check(!ParseWindowsMotionEvidenceLaunch(args).error.empty());
    }
    for(const auto args:{std::vector<std::wstring_view>{L"--motion-scenario",L"shaft-up"},
        {L"--validate-native-motion",L"C:\\fresh"}, {L"--validate-native-motion"},
        {L"--validate-native-motion",L"C:\\fresh",L"--motion-scenario",L"bogus"},
        {L"--validate-native-motion",L"C:\\fresh",L"--motion-scenario",L"shaft-up",L"--motion-scenario",L"shaft-up"},
        {L"--validate-native-motion",L"C:\\fresh",L"--motion-scenario",L"shaft-up",L"--validate-native-motion",L"C:\\other"}})
        check(!ParseWindowsMotionEvidenceLaunch(args).error.empty());
    std::vector<std::wstring_view> tooMany(17,L"x"); tooMany[0]=L"--validate-native-motion";
    check(!ParseWindowsMotionEvidenceLaunch(tooMany).error.empty());
    check(ParseWindowsMotionEvidenceLaunch(std::vector<std::wstring_view>(17,L"unrelated")).error.empty());
    const std::array<std::wstring_view,5> compute{L"--motion-scenario",L"shaft-up",L"--require-rayquery-compute",L"--validate-native-motion",L"C:\\fresh"};
    check(ParseWindowsMotionEvidenceLaunch(compute).error.empty());
    std::cout<<"Native motion launch failures="<<failures<<'\n';
    return failures ? 1 : 0;
}
