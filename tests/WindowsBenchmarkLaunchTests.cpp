#include <array>
#include <iostream>
#include <string_view>

#include "platform/windows/WindowsBenchmarkLaunch.h"

int main()
{
    using horde::platform::windows::ParseWindowsBenchmarkLaunch;
    bool passed = true;
    const auto check = [&passed](bool condition, const char* message) {
        if (!condition)
        {
            std::cerr << message << '\n';
            passed = false;
        }
    };
    const std::array<std::wstring_view, 0> ordinary{};
    const auto absent = ParseWindowsBenchmarkLaunch(ordinary);
    check(!absent.requested && absent.error.empty(), "ordinary launch must remain interactive");
    const std::array<std::wstring_view, 3> benchmark{
        L"--benchmark-showcase", L"C:\\reports\\Horde α", L"--require-rayquery-compute"};
    const auto selected = ParseWindowsBenchmarkLaunch(benchmark);
    check(selected.requested && selected.error.empty() &&
              selected.outputDirectory == L"C:\\reports\\Horde α",
          "benchmark launch must preserve its exact Unicode output directory");
    const std::array<std::wstring_view, 1> missing{L"--benchmark-showcase"};
    check(!ParseWindowsBenchmarkLaunch(missing).error.empty(), "missing output directory must fail");
    const std::array<std::wstring_view, 2> empty{L"--benchmark-showcase", L""};
    check(!ParseWindowsBenchmarkLaunch(empty).error.empty(), "empty output directory must fail");
    const std::array<std::wstring_view, 2> option{L"--benchmark-showcase", L"--other-option"};
    check(!ParseWindowsBenchmarkLaunch(option).error.empty(), "another option is not an output directory");
    const std::array<std::wstring_view, 4> duplicate{
        L"--benchmark-showcase", L"first", L"--benchmark-showcase", L"second"};
    check(!ParseWindowsBenchmarkLaunch(duplicate).error.empty(), "ambiguous duplicate launch must fail");
    for (const auto conflicting : {L"--capture-showcase", L"--development-checkpoint"})
    {
        const std::array<std::wstring_view, 4> before{
            conflicting, L"value", L"--benchmark-showcase", L"reports"};
        const std::array<std::wstring_view, 4> after{
            L"--benchmark-showcase", L"reports", conflicting, L"value"};
        check(!ParseWindowsBenchmarkLaunch(before).error.empty() &&
                  !ParseWindowsBenchmarkLaunch(after).error.empty(),
              "benchmark cannot be combined with checkpoint/capture mutation in either order");
    }
    const std::array<std::wstring_view, 2> capture{L"--capture-showcase", L"reports"};
    check(!ParseWindowsBenchmarkLaunch(capture).requested &&
              ParseWindowsBenchmarkLaunch(capture).error.empty(),
          "existing capture-only launch remains the capture parser's responsibility");
    return passed ? 0 : 1;
}
