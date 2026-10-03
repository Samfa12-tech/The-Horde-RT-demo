#include "graphics/GraphicsPreviewSession.h"
#include "platform/windows/WindowsGraphicsPreviewCapture.h"

#include <cmath>
#include <iostream>
#include <limits>

namespace
{
using namespace horde::graphics;
bool passed = true;
void Check(const bool condition, const char* message)
{
    if (!condition) { passed = false; std::cerr << "Graphics preview: " << message << '\n'; }
}
}
int main()
{
    using namespace horde::platform::windows;
    const std::array<std::wstring_view, 2u> captureArguments{L"--capture-graphics-preview", L"C:\\captures\\preview"};
    Check(ParseGraphicsPreviewCaptureLaunch(captureArguments).requested &&
          ParseGraphicsPreviewCaptureLaunch(captureArguments).error.empty(), "absolute isolated Debug capture arguments parse");
    const std::array<std::wstring_view, 2u> relativeCapture{L"--capture-graphics-preview", L"preview"};
    Check(!ParseGraphicsPreviewCaptureLaunch(relativeCapture).error.empty(), "capture cannot reinterpret a relative output path");
    const std::array<std::wstring_view, 3u> conflictingCapture{captureArguments[0], captureArguments[1], L"--benchmark-showcase"};
    Check(!ParseGraphicsPreviewCaptureLaunch(conflictingCapture).error.empty(), "capture cannot run gameplay benchmark simultaneously");
    const std::array<std::wstring_view, 4u> duplicateCapture{captureArguments[0], captureArguments[1], captureArguments[0], captureArguments[1]};
    Check(!ParseGraphicsPreviewCaptureLaunch(duplicateCapture).error.empty(), "duplicate capture destination rejected");
    Check(AbsoluteWindowsCapturePath(L"\\\\server\\share\\preview") && !AbsoluteWindowsCapturePath(L"C:preview") &&
          !AbsoluteWindowsCapturePath(L"\\preview"), "fully qualified drive and UNC paths distinguish rooted or drive-relative paths");
    GraphicsPreviewSession captureSession;
    std::uint64_t captureEpoch = captureSession.Snapshot().timelineEpoch;
    for (const auto& pose : kGraphicsPreviewCapturePoses)
    {
        Check(StageGraphicsPreviewCapturePose(captureSession, pose), "capture stages exact tick and pause without advancing gameplay");
        const auto snapshot = captureSession.Snapshot();
        Check(snapshot.tick == pose.tick && snapshot.timelineEpoch == ++captureEpoch && snapshot.paused &&
              snapshot.motionTest == pose.motion, "each capture begins a fresh deterministic epoch and frozen shared pose");
        if (pose.motion) Check(snapshot.camera.x != GraphicsPreviewCameraPose(pose.camera).x, "motion capture records actual shared camera motion");
    }
    const auto scene = MakeGraphicsPreviewDescription();
    Check(scene.worldQuads.size() == 10u && scene.waterQuads.size() == 1u, "bounded compact geometry includes native materials and separate water film");
    Check(scene.waterOrigin == PreviewPoint{-2.32f, 0.0f, -15.26f} && scene.waterStreams[0].radiusX == 0.006f,
          "preview retains actual shader water center/radius instead of unrelated relocated sheet");
    Check(scene.woodPlinthMinimum[1] == horde::gameplay::kRouteFloorWorldY, "actual PBR wood plinth is grounded on shared floor");
    bool mirror = false, metal = false, wet = false, ground = false;
    for (const auto& quad : scene.worldQuads)
    {
        mirror |= quad.material == 8u; metal |= quad.material == 4u;
        wet |= quad.material == 1u; ground |= quad.material == 3u;
        Check(quad.material <= 10u && quad.normal <= 5u, "preview uses shared production world material ABI");
        for (const auto& point : quad.vertices) Check(std::isfinite(point[0]) && std::isfinite(point[1]) && std::isfinite(point[2]), "finite authored geometry");
    }
    Check(mirror && metal && wet && ground, "representative material and mirror surfaces are actual geometry");
    GraphicsPreviewSession one, two;
    horde::gameplay::effects::FireEmitterFixedStepInput sockets;
    sockets.worldFromFlame[12] = -3.65f; sockets.worldFromFlame[13] = 0.70f; sockets.worldFromFlame[14] = -15.95f;
    sockets.worldFromLight = sockets.worldFromFlame;
    sockets.strength = 0.64f; sockets.fuel = 0.8f;
    one.SetFireSockets(0u, sockets); two.SetFireSockets(0u, sockets);
    for (int tick = 0; tick < 60; ++tick) one.Advance(1.0 / 60.0);
    for (int batch = 0; batch < 10; ++batch) two.Advance(0.10);
    const auto a = one.Snapshot(), b = two.Snapshot();
    Check(a.tick == 60u && a.tick == b.tick && a.skeleton.animationTime == b.skeleton.animationTime &&
          a.fireEmitters[0].phase == b.fireEmitters[0].phase, "equivalent fixed-step timelines produce deterministic actor and fire poses");
    Check(a.fireEmitters[0].worldFromFlame == sockets.worldFromFlame && a.fireEmitters[0].worldFromLight == sockets.worldFromLight,
          "fire timeline preserves production socket transform coherence");
    Check(a.fireEmitters[0].stableId != a.fireEmitters[1].stableId, "preview fire IDs are stable and distinct");
    GraphicsPreviewSession repeatedPause;
    for (int frame = 0; frame < 37; ++frame) { repeatedPause.Pause(false); repeatedPause.Advance(1.0 / 37.0); }
    Check(repeatedPause.Snapshot().tick == 60u,
          "repeated unchanged pause publication preserves fractional 60Hz carry at a non-divisor frame cap");
    one.Pause(true); one.Advance(8.0);
    Check(one.Snapshot().tick == a.tick, "preview pause prevents simulation animation advancement");
    one.Pause(false); one.Advance(std::numeric_limits<double>::quiet_NaN());
    Check(one.Snapshot().tick == a.tick, "invalid elapsed time cannot damage timeline");
    one.SelectCamera(GraphicsPreviewCamera::Water);
    Check(one.Snapshot().camera.x == GraphicsPreviewCameraPose(GraphicsPreviewCamera::Water).x, "fixed camera selection is explicit");
    one.SetMotion(true); one.Advance(0.10);
    Check(one.Snapshot().camera.x != GraphicsPreviewCameraPose(GraphicsPreviewCamera::Water).x, "repeatable motion test actually moves production camera");
    const auto oldEpoch = one.Snapshot().timelineEpoch;
    one.Reset();
    Check(one.Snapshot().tick == 0u && one.Snapshot().timelineEpoch == oldEpoch + 1u &&
          one.Snapshot().fireEmitters[0].worldFromFlame == sockets.worldFromFlame,
          "A/B reset begins new timeline and retains actual authored sockets");
    Check(one.Snapshot().fireEmitters[0].strength == sockets.strength &&
          one.Snapshot().fireEmitters[0].fuel == sockets.fuel,
          "tick-zero A/B reset retains actual admitted emitter strength and fuel");
    return passed ? 0 : 1;
}
