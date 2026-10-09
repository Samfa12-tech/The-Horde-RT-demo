#pragma once

#include "platform/windows/WindowsGraphicsPreviewCapture.h"
#include <array>
#include <span>
#include <string>
#include <string_view>

namespace horde::platform::windows
{
struct OutputResizeValidationLaunch
{
    bool requested = false;
    std::wstring outputDirectory;
    std::string error;
};
inline OutputResizeValidationLaunch ParseOutputResizeValidationLaunch(
    const std::span<const std::wstring_view> arguments)
{
    OutputResizeValidationLaunch result;
    bool conflict = false;
    for (std::size_t index = 0; index < arguments.size(); ++index)
    {
        const auto argument = arguments[index];
        if (argument == L"--capture-showcase" || argument == L"--capture-graphics-preview" ||
            argument == L"--development-checkpoint" || argument == L"--benchmark-showcase" ||
            argument == L"--benchmark-workload" || argument == L"--anatomical-player-mount" ||
            argument == L"--debug-rt-lab" || argument.starts_with(L"--rt-lab-")) conflict = true;
        if (argument != L"--validate-output-resize") continue;
        if (result.requested || index + 1 == arguments.size() || arguments[index + 1].starts_with(L"--"))
        { result.error = "--validate-output-resize requires one unique absolute output directory."; return result; }
        result.requested = true;
        result.outputDirectory = arguments[++index];
        if (!AbsoluteWindowsCapturePath(result.outputDirectory))
        { result.error = "--validate-output-resize requires an absolute Windows output directory."; return result; }
    }
    if (result.requested && conflict)
        result.error = "Output resize validation cannot share capture, checkpoint, benchmark or RT Lab tuning modes.";
    return result;
}
// Fixed production workloads and sequence, not a product quality or arbitrary checkpoint control.
inline constexpr std::array<std::string_view, 2> kOutputResizeWorkloads{"opening", "lantern-held-high-v1"};
inline constexpr std::array<int, 4> kOutputResizeScaleSequence{100, 75, 50, 100};
struct OutputResizeOwnedFrame
{
    std::uint64_t sceneEpoch = 0, measurementGeneration = 0, submissionSerial = 0, simulationTick = 0;
    std::uint64_t recordAttemptSerial = 0, recordSerial = 0;
    std::uint32_t frameSlot = 0;
};
inline bool IsCurrentOutputResizeCompletion(const OutputResizeOwnedFrame& submitted,
                                            const OutputResizeOwnedFrame& completed,
                                            const std::uint64_t completionSerial) noexcept
{
    return submitted.sceneEpoch != 0 && submitted.measurementGeneration != 0 &&
        submitted.submissionSerial != 0 && completionSerial != 0 &&
        submitted.recordAttemptSerial != 0 && submitted.recordSerial != 0 &&
        submitted.sceneEpoch == completed.sceneEpoch &&
        submitted.measurementGeneration == completed.measurementGeneration &&
        submitted.submissionSerial == completed.submissionSerial &&
        submitted.simulationTick == completed.simulationTick &&
        submitted.recordAttemptSerial == completed.recordAttemptSerial &&
        submitted.recordSerial == completed.recordSerial && submitted.frameSlot == completed.frameSlot;
}
inline std::string OutputResizeCaptureFilename(const std::string_view workload, const int scale, const bool baseline)
{
    return std::string(workload) + (baseline ? "-baseline-" : "-apply-") + std::to_string(scale) + ".png";
}
} // namespace horde::platform::windows
