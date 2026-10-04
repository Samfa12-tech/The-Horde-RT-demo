#pragma once

#include "graphics/GraphicsPreviewSession.h"
#include <array>
#include <span>
#include <string>
#include <string_view>

namespace horde::platform::windows
{
struct GraphicsPreviewCaptureLaunch
{
    bool requested = false;
    std::wstring outputDirectory;
    std::string error;
};
inline bool AbsoluteWindowsCapturePath(const std::wstring_view path) noexcept
{
    const auto separator = [](const wchar_t ch) { return ch == L'/' || ch == L'\\'; };
    if (path.size() >= 3u && ((path[0] >= L'A' && path[0] <= L'Z') ||
        (path[0] >= L'a' && path[0] <= L'z')) && path[1] == L':' && separator(path[2])) return true;
    if (path.size() < 5u || !separator(path[0]) || !separator(path[1])) return false;
    const auto share = path.find_first_of(L"/\\", 2u);
    return share != std::wstring_view::npos && share > 2u && share + 1u < path.size();
}
// Parsing and authored capture poses are portable CPU contracts. Neither opens
// preferences nor admits hardware; the native owner must prove real RT output.
inline GraphicsPreviewCaptureLaunch ParseGraphicsPreviewCaptureLaunch(
    const std::span<const std::wstring_view> arguments)
{
    GraphicsPreviewCaptureLaunch result;
    bool conflictingMode = false;
    for (std::size_t index = 0u; index < arguments.size(); ++index)
    {
        const auto argument = arguments[index];
        if (argument == L"--capture-showcase" || argument == L"--development-checkpoint" ||
            argument == L"--benchmark-showcase" || argument == L"--benchmark-workload" ||
            argument == L"--anatomical-player-mount") conflictingMode = true;
        if (argument != L"--capture-graphics-preview") continue;
        if (result.requested || index + 1u == arguments.size() || arguments[index + 1u].starts_with(L"--"))
        { result.error = "--capture-graphics-preview requires one unique absolute output directory."; return result; }
        result.requested = true;
        result.outputDirectory = arguments[++index];
        if (!AbsoluteWindowsCapturePath(result.outputDirectory))
        { result.error = "--capture-graphics-preview requires an absolute Windows output directory."; return result; }
    }
    if (result.requested && conflictingMode)
        result.error = "Graphics preview capture cannot share a showcase, checkpoint or benchmark run.";
    return result;
}
struct GraphicsPreviewCapturePose
{
    std::string_view name;
    horde::graphics::GraphicsPreviewCamera camera;
    std::uint64_t tick;
    bool motion;
    bool testPausedAdvance;
};
inline constexpr std::array<GraphicsPreviewCapturePose, 9u> kGraphicsPreviewCapturePoses{{
    {"overview", horde::graphics::GraphicsPreviewCamera::Overview, 120u, false, false},
    {"materials", horde::graphics::GraphicsPreviewCamera::Materials, 120u, false, false},
    {"glass", horde::graphics::GraphicsPreviewCamera::Glass, 120u, false, false},
    {"water", horde::graphics::GraphicsPreviewCamera::Water, 120u, false, false},
    {"skeleton", horde::graphics::GraphicsPreviewCamera::Skeleton, 120u, false, false},
    {"mirror", horde::graphics::GraphicsPreviewCamera::Mirror, 120u, false, false},
    {"paused-overview", horde::graphics::GraphicsPreviewCamera::Overview, 120u, false, true},
    {"reset-overview", horde::graphics::GraphicsPreviewCamera::Overview, 0u, false, false},
    {"motion-overview", horde::graphics::GraphicsPreviewCamera::Overview, 120u, true, false},
}};
inline bool StageGraphicsPreviewCapturePose(horde::graphics::GraphicsPreviewSession& session,
                                          const GraphicsPreviewCapturePose& pose)
{
    session.Pause(false); session.SetMotion(pose.motion); session.SelectCamera(pose.camera); session.Reset();
    for (std::uint64_t tick = 0u; tick < pose.tick; ++tick) session.Advance(1.0 / 60.0);
    session.Pause(true);
    const auto before = session.Snapshot();
    if (pose.testPausedAdvance) session.Advance(0.25);
    const auto after = session.Snapshot();
    return after.tick == pose.tick && before.timelineEpoch == after.timelineEpoch &&
        before.fireEmitters[0].phase == after.fireEmitters[0].phase &&
        before.fireEmitters[1].phase == after.fireEmitters[1].phase;
}
} // namespace horde::platform::windows
