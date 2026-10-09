#include "platform/windows/WindowsOutputResizeLaunch.h"
#include <iostream>

int main()
{
    using namespace horde::platform::windows;
    bool passed = true;
    const auto check = [&](const bool value, const char* message) {
        if (!value) { passed = false; std::cerr << message << '\n'; }
    };
    const std::array<std::wstring_view, 2> valid{L"--validate-output-resize", L"C:\\validation\\resize"};
    check(ParseOutputResizeValidationLaunch(valid).requested && ParseOutputResizeValidationLaunch(valid).error.empty(),
          "absolute isolated resize mode parses");
    for (const auto path : {L"relative", L"C:relative", L"\\rooted", L""})
    { const std::array<std::wstring_view, 2> args{valid[0], path}; check(!ParseOutputResizeValidationLaunch(args).error.empty(), "partial/empty output rejected"); }
    const std::array<std::wstring_view, 1> missing{valid[0]};
    check(!ParseOutputResizeValidationLaunch(missing).error.empty(), "missing destination rejected");
    const std::array<std::wstring_view, 4> duplicate{valid[0], valid[1], valid[0], valid[1]};
    check(!ParseOutputResizeValidationLaunch(duplicate).error.empty(), "duplicate request rejected");
    for (const auto other : {L"--capture-showcase", L"--capture-graphics-preview", L"--development-checkpoint",
                            L"--benchmark-showcase", L"--benchmark-workload", L"--anatomical-player-mount",
                            L"--debug-rt-lab", L"--rt-lab-roof"})
    { const std::array<std::wstring_view, 3> args{valid[0], valid[1], other}; check(!ParseOutputResizeValidationLaunch(args).error.empty(), "other workloads/tuning rejected"); }
    const std::array<std::wstring_view, 4> compute{valid[0], valid[1], L"--require-rayquery-compute", L"--capture-portrait"};
    check(ParseOutputResizeValidationLaunch(compute).error.empty(), "real compute and capture aspect remain independent");
    const std::array<std::wstring_view, 2> unc{valid[0], L"\\\\server\\share\\resize"};
    check(ParseOutputResizeValidationLaunch(unc).error.empty(), "fully qualified UNC accepted");
    check(kOutputResizeWorkloads.size() == 2 && kOutputResizeScaleSequence == std::array{100, 75, 50, 100}, "fixed workloads and output sequence retain accepted quality");
    const OutputResizeOwnedFrame submitted{1, 2, 30, 60, 28, 29, 0};
    check(IsCurrentOutputResizeCompletion(submitted, submitted, 31), "actual matching owning completion accepted");
    for (const auto stale : {OutputResizeOwnedFrame{2, 2, 30, 60, 28, 29, 0}, OutputResizeOwnedFrame{1, 3, 30, 60, 28, 29, 0},
                            OutputResizeOwnedFrame{1, 2, 29, 60, 28, 29, 0}, OutputResizeOwnedFrame{1, 2, 30, 61, 28, 29, 0},
                            OutputResizeOwnedFrame{1, 2, 30, 60, 27, 29, 0}, OutputResizeOwnedFrame{1, 2, 30, 60, 28, 27, 0},
                            OutputResizeOwnedFrame{1, 2, 30, 60, 28, 29, 1}})
        check(!IsCurrentOutputResizeCompletion(submitted, stale, 31), "stale epoch/generation/submission/tick rejected");
    check(!IsCurrentOutputResizeCompletion(submitted, submitted, 0), "incomplete drain rejected");
    check(!IsCurrentOutputResizeCompletion({}, {}, 31), "tokenless frame rejected");
    std::array<std::string, 8> filenames{};
    std::size_t fileIndex = 0;
    for (const auto workload : kOutputResizeWorkloads)
        for (std::size_t scaleIndex = 0; scaleIndex < kOutputResizeScaleSequence.size(); ++scaleIndex)
        {
            const auto filename = OutputResizeCaptureFilename(workload, kOutputResizeScaleSequence[scaleIndex], scaleIndex == 0);
            for (std::size_t previous = 0; previous < fileIndex; ++previous)
                check(filenames[previous] != filename, "baseline and return-to-100 cannot overwrite any capture");
            filenames[fileIndex++] = filename;
        }
    return passed ? 0 : 1;
}
