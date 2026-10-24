#include "graphics/GraphicsPreviewSession.h"
#include "platform/windows/WindowsGraphicsPreviewCapture.h"
#include "platform/windows/WindowsPreviewMeshPairCapture.h"
#include "platform/windows/WindowsHiddenCaptureLaunch.h"
#include "platform/windows/WindowsForestRouteProofLaunch.h"

#include <algorithm>
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
    const std::array<std::wstring_view,7> forestArgs{L"--forest-route-proof",L"shared64",L"--capture-hidden",
        L"--capture-showcase",L"C:\\captures\\forest",L"--development-checkpoint",L"chapter-landing-north"};
    Check(ParseForestRouteProofLaunch(forestArgs,true).mode==ForestRouteCaptureMode::SharedExisting64 &&
          ParseForestRouteProofLaunch(forestArgs,true).error.empty(),"shared64 is limited to an explicit hidden chapter capture");
    Check(!ParseForestRouteProofLaunch(forestArgs,false).error.empty(),"forest proof is rejected by Release");
    auto bakedForest=forestArgs;bakedForest[1]=L"baked-resident";
    Check(ParseForestRouteProofLaunch(bakedForest,true).mode==ForestRouteCaptureMode::BakedResident &&
          ParseForestRouteProofLaunch(bakedForest,true).error.empty(),"existing baked control remains independently selectable");
    auto sectorArgs=forestArgs;sectorArgs[1]=L"f01-sector";
    Check(ParseForestRouteProofLaunch(sectorArgs,true).mode==ForestRouteCaptureMode::SharedF01Sector76 &&
          ParseForestRouteProofLaunch(sectorArgs,true).error.empty(),"F01 sector is a separate explicit hidden composition proof");
    Check(!ParseForestRouteProofLaunch(sectorArgs,false).error.empty(),"Release rejects F01 sector composition");
    auto retentionArgs=forestArgs;retentionArgs[1]=L"shared64-retention";
    Check(ParseForestRouteProofLaunch(retentionArgs,true).mode==ForestRouteCaptureMode::SharedRetention64 &&
          ParseForestRouteProofLaunch(retentionArgs,true).error.empty(),"whole-instance retention requires its own explicit proof selection");
    Check(!ParseForestRouteProofLaunch(retentionArgs,false).error.empty(),"Release rejects whole-instance retention proof");
    auto normalForestArgs=forestArgs;normalForestArgs[1]=L"normal-shared64";
    Check(ParseForestRouteProofLaunch(normalForestArgs,true).mode==ForestRouteCaptureMode::NormalShared64 &&
          ParseForestRouteProofLaunch(normalForestArgs,true).error.empty(),"normal shared ownership uses a distinct explicit validation selection");
    Check(!ParseForestRouteProofLaunch(normalForestArgs,false).error.empty(),"normal-owner validation launcher remains Debug-only");
    std::array<std::wstring_view,8> normalRouteArgs{};
    std::copy(normalForestArgs.begin(),normalForestArgs.end(),normalRouteArgs.begin());
    normalRouteArgs[6]=L"chapter-kit-reunion";normalRouteArgs[7]=L"--validate-chapter-roundtrip";
    Check(ParseForestRouteProofLaunch(normalRouteArgs,true).error.empty(),"normal shared ownership may use the existing reunion roundtrip gate");
    normalRouteArgs[1]=L"shared64";
    Check(!ParseForestRouteProofLaunch(normalRouteArgs,true).error.empty(),"isolated proof mode cannot bypass normal route ownership");
    const std::array<std::wstring_view,8> routeStages{L"--capture-showcase",L"C:\\captures\\route",L"--capture-hidden",
        L"--development-checkpoint",L"chapter-kit-reunion",L"--validate-chapter-roundtrip",
        L"--capture-chapter-route-stages",L"--development-rescue-journey"};
    const auto observedRoute=ParseForestRouteProofLaunch(routeStages,true);
    Check(observedRoute.error.empty() && observedRoute.gpuStages && !observedRoute.landingOnly &&
        observedRoute.mode==ForestRouteCaptureMode::Disabled,"GPU observation preserves the complete ordinary baked route gate");
    Check(!ParseForestRouteProofLaunch(routeStages,false).error.empty(),"Release rejects full-route GPU observation");
    auto invalidRouteStages=routeStages;invalidRouteStages[5]=L"--other";
    Check(!ParseForestRouteProofLaunch(invalidRouteStages,true).error.empty(),"GPU observation cannot replace full-route validation");
    invalidRouteStages=routeStages;invalidRouteStages[2]=L"--other";
    Check(!ParseForestRouteProofLaunch(invalidRouteStages,true).error.empty(),"full-route GPU observation requires hidden operation");
    const std::array<std::wstring_view,7> landingArgs{L"--capture-chapter-landing",L"stages",L"--capture-hidden",
        L"--capture-showcase",L"C:\\captures\\landing",L"--development-checkpoint",L"chapter-kit-reunion"};
    const auto landing=ParseForestRouteProofLaunch(landingArgs,true);
    Check(landing.error.empty() && landing.landingOnly && landing.gpuStages && landing.mode==ForestRouteCaptureMode::Disabled,
          "landing attribution selects the ordinary baked owner and explicit stage capture");
    auto landingControl=landingArgs;landingControl[1]=L"control";
    Check(ParseForestRouteProofLaunch(landingControl,true).error.empty() &&
          !ParseForestRouteProofLaunch(landingControl,true).gpuStages,"uninstrumented landing control is explicit");
    Check(!ParseForestRouteProofLaunch(landingArgs,false).error.empty(),"Release rejects partial landing observation");
    std::array<std::wstring_view,8> mixedLanding{};
    std::copy(landingArgs.begin(),landingArgs.end(),mixedLanding.begin());mixedLanding[7]=L"--validate-chapter-roundtrip";
    Check(!ParseForestRouteProofLaunch(mixedLanding,true).error.empty(),"partial attribution cannot replace full route validation");
    std::array<std::wstring_view,9> sharedLanding{};
    std::copy(landingArgs.begin(),landingArgs.end(),sharedLanding.begin());
    sharedLanding[7]=L"--forest-route-proof";sharedLanding[8]=L"normal-shared64";
    Check(ParseForestRouteProofLaunch(sharedLanding,true).error.empty(),"normal shared owner supports the same bounded landing workload");
    sharedLanding[8]=L"shared64";
    Check(!ParseForestRouteProofLaunch(sharedLanding,true).error.empty(),"isolated shared proof cannot substitute for normal-owner attribution");
    auto badLanding=landingArgs;badLanding[2]=L"--validate-chapter-roundtrip";
    Check(!ParseForestRouteProofLaunch(badLanding,true).error.empty(),"landing observation cannot acquire foreground");
    badLanding=landingArgs;badLanding[6]=L"chapter-forest-lookout";
    Check(!ParseForestRouteProofLaunch(badLanding,true).error.empty(),"partial observation is restricted to its original prefix start");
    auto mixedRetention=retentionArgs;mixedRetention[2]=L"--validate-static-zone-cutover";
    Check(!ParseForestRouteProofLaunch(mixedRetention,true).error.empty(),"whole-instance proof cannot reuse common-owner cutover");
    auto unsafeSector=sectorArgs;unsafeSector[2]=L"--validate-static-zone-cutover";
    Check(!ParseForestRouteProofLaunch(unsafeSector,true).error.empty(),"F01 sector cannot invoke static cutover");
    auto badForest=forestArgs;badForest[1]=L"other";
    Check(!ParseForestRouteProofLaunch(badForest,true).error.empty(),"unknown forest representation rejected");
    badForest=forestArgs;badForest[2]=L"--validate-static-zone-cutover";
    Check(!ParseForestRouteProofLaunch(badForest,true).error.empty(),"fully resident forest proof cannot invoke static cutover");
    badForest=forestArgs;badForest[6]=L"ordinary";
    Check(!ParseForestRouteProofLaunch(badForest,true).error.empty(),"forest proof cannot select unrelated scenes");
    const std::array<std::wstring_view,3> pairArgs{L"--capture-hidden-preview-pair",L"shared",L"C:\\captures\\pair"};
    Check(ParsePreviewMeshPairCaptureLaunch(pairArgs,true).mode==PreviewMeshPairCaptureMode::Shared &&
          ParsePreviewMeshPairCaptureLaunch(pairArgs,true).error.empty(),"explicit Debug hidden shared pair parses");
    Check(!ParsePreviewMeshPairCaptureLaunch(pairArgs,false).error.empty(),"Release rejects hidden pair");
    auto invalidPair=pairArgs;invalidPair[1]=L"other";
    Check(!ParsePreviewMeshPairCaptureLaunch(invalidPair,true).error.empty(),"unknown pair mode rejected");
    invalidPair=pairArgs;invalidPair[2]=L"relative";
    Check(!ParsePreviewMeshPairCaptureLaunch(invalidPair,true).error.empty(),"relative pair output rejected");
    const std::array<std::wstring_view,4> mixedPair{pairArgs[0],pairArgs[1],pairArgs[2],L"--capture-hidden"};
    Check(!ParsePreviewMeshPairCaptureLaunch(mixedPair,true).error.empty(),"pair cannot weaken generic hidden guard");
    const std::array<std::wstring_view,3> oldHiddenPreview{L"--capture-hidden",L"--capture-graphics-preview",L"C:\\captures\\preview"};
    Check(!ParseWindowsHiddenCaptureLaunch(oldHiddenPreview,true).error.empty(),"ordinary hidden-preview incompatibility remains");
    const std::array<std::wstring_view,6> repeatedPair{pairArgs[0],pairArgs[1],pairArgs[2],pairArgs[0],L"baked",pairArgs[2]};
    Check(!ParsePreviewMeshPairCaptureLaunch(repeatedPair,true).error.empty(),"duplicate pair request rejected");
    Check(!ParsePreviewMeshPairCaptureLaunch(std::span(pairArgs).first(2),true).error.empty(),"missing pair output rejected");
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
