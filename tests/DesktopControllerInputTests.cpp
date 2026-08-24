#include "platform/windows/DesktopControllerInput.h"
#include "platform/windows/WindowsRtLabState.h"

#include <cstdlib>
#include <filesystem>
#include <fstream>
#include <iostream>
#include <sstream>
#include <string>
#include <string_view>

namespace
{

using horde::platform::windows::LegacyAxis;
using horde::platform::windows::LegacyAxisSample;
using horde::platform::windows::LegacyRightStickAxes;
using horde::platform::windows::ControllerActionEdges;
using horde::platform::windows::ControllerTriggerLatch;
using horde::platform::windows::LegacyControllerIdentity;
using horde::platform::windows::MapLegacyControllerEdges;
using horde::platform::windows::MapLegacyControllerMenuEdges;
using horde::platform::windows::ApplyControllerLook;
using horde::platform::windows::StepControllerSlider;
using horde::platform::windows::UpdateXInputTriggerEdges;
using horde::platform::windows::SelectLegacyRightStickAxes;
using horde::platform::windows::RtLabControlRange;
using horde::platform::windows::RtLabUnlockContext;
using horde::platform::windows::CanPersistRtLabUnlock;
using horde::platform::windows::StepRtLabControl;
using horde::platform::windows::WrapRtLabFocus;
using horde::platform::windows::ShouldPlayControllerMenuSound;

void Require(const bool condition, const std::string_view message)
{
    if (!condition)
    {
        std::cerr << "Desktop controller input test failed: " << message << '\n';
        std::exit(EXIT_FAILURE);
    }
}

void RequireAxes(
    const LegacyRightStickAxes axes,
    const LegacyAxis horizontal,
    const LegacyAxis vertical,
    const std::string_view message)
{
    Require(axes.horizontal == horizontal && axes.vertical == vertical, message);
}

std::string ReadWindowsSource()
{
    std::filesystem::path candidate = std::filesystem::current_path();
    for (int depth = 0; depth < 8; ++depth)
    {
        const std::filesystem::path source =
            candidate / "src/platform/windows/DiagnosticWindow.cpp";
        if (std::filesystem::exists(source))
        {
            std::ifstream input(source, std::ios::binary);
            std::ostringstream text;
            text << input.rdbuf();
            return text.str();
        }
        if (!candidate.has_parent_path()) break;
        candidate = candidate.parent_path();
    }
    return {};
}

} // namespace

int main()
{
    Require(CanPersistRtLabUnlock({.finaleComplete = true}),
            "a genuine live finale completion must persist the RT Lab unlock");
    Require(!CanPersistRtLabUnlock({.finaleComplete = false}) &&
            !CanPersistRtLabUnlock({.finaleComplete = true, .capture = true}) &&
            !CanPersistRtLabUnlock({.finaleComplete = true, .checkpoint = true}) &&
            !CanPersistRtLabUnlock({.finaleComplete = true, .replay = true}) &&
            !CanPersistRtLabUnlock({.finaleComplete = true, .benchmark = true}) &&
            !CanPersistRtLabUnlock({.finaleComplete = true, .debugInjection = true}),
            "capture, checkpoint, replay, benchmark, and debug routes must never persist progress");

    Require(StepRtLabControl(100, true, RtLabControlRange::WaterfallPercent) == 105 &&
            StepRtLabControl(25, false, RtLabControlRange::WaterfallPercent) == 25 &&
            StepRtLabControl(200, true, RtLabControlRange::WaterfallPercent) == 200,
            "waterfall stepping must use five-point increments inside 25-200 percent");
    Require(StepRtLabControl(0, true, RtLabControlRange::HueDegrees) == 5 &&
            StepRtLabControl(-180, false, RtLabControlRange::HueDegrees) == -180 &&
            StepRtLabControl(180, true, RtLabControlRange::HueDegrees) == 180,
            "hue stepping must clamp to minus/plus 180 degrees");
    Require(StepRtLabControl(0, true, RtLabControlRange::UnitPercent) == 5 &&
            StepRtLabControl(200, true, RtLabControlRange::DoublePercent) == 200,
            "roof/dawn and fog/light controls must use their truthful bounds");
    Require(WrapRtLabFocus(0u, -1, 4u) == 3u && WrapRtLabFocus(3u, 1, 4u) == 0u,
            "keyboard/controller focus must wrap in both directions");
    Require(ShouldPlayControllerMenuSound(false) && !ShouldPlayControllerMenuSound(true),
            "ordinary menu navigation may retain feedback while every RT Lab interaction stays silent");

    constexpr LegacyControllerIdentity capturedBackbone{
        .vendorId = 0x358au,
        .productId = 0x0204u,
        .productName = "Microsoft PC-joystick driver",
    };

    // Owner-captured WinMM evidence for VID 358A / PID 0204: the physical
    // right stick moves Z/R while U/V remain fixed at zero. The generic
    // product string must not route this exact topology through the older
    // R/U assumption.
    RequireAxes(
        SelectLegacyRightStickAxes(
            LegacyAxisSample{.z = 0.0f, .r = 0.0f, .u = -1.0f, .v = -1.0f,
                             .hasZ = true, .hasR = true, .hasU = false, .hasV = false},
            capturedBackbone),
        LegacyAxis::Z,
        LegacyAxis::R,
        "captured Backbone PID 0204 must use its measured Z/R right stick");

    // Backbone's Windows HID mapping uses axes 3/4 for right X/Y. In WinMM
    // those are R/U; Z/V remain the two trigger axes.
    RequireAxes(
        SelectLegacyRightStickAxes(
            LegacyAxisSample{.z = 0.0f, .r = 0.0f, .u = 0.0f, .v = 0.0f,
                             .hasZ = true, .hasR = true, .hasU = true, .hasV = true},
            "Backbone One PlayStation Edition"),
        LegacyAxis::R,
        LegacyAxis::U,
        "Backbone must use its R/U right-stick axes");

    // Generic HID layouts are inferred from the two axes resting nearest their
    // midpoint, keeping trigger axes (which rest at an extreme) out of aiming.
    RequireAxes(
        SelectLegacyRightStickAxes(
            LegacyAxisSample{.z = -1.0f, .r = 0.0f, .u = 0.0f, .v = -1.0f,
                             .hasZ = true, .hasR = true, .hasU = true, .hasV = true},
            "Generic USB Gamepad"),
        LegacyAxis::R,
        LegacyAxis::U,
        "XInput-shaped legacy HID must use R/U and not its trigger axes");

    RequireAxes(
        SelectLegacyRightStickAxes(
            LegacyAxisSample{.z = 0.0f, .r = -1.0f, .u = -1.0f, .v = 0.0f,
                             .hasZ = true, .hasR = true, .hasU = true, .hasV = true},
            "Generic PlayStation Gamepad"),
        LegacyAxis::Z,
        LegacyAxis::V,
        "PS-shaped legacy HID must use Z/V and not its trigger axes");

    RequireAxes(
        SelectLegacyRightStickAxes(
            LegacyAxisSample{.u = 0.0f, .v = 0.0f, .hasU = true, .hasV = true},
            "Six-axis joystick"),
        LegacyAxis::U,
        LegacyAxis::V,
        "U/V-only look axes must remain supported");

    RequireAxes(
        SelectLegacyRightStickAxes(
            LegacyAxisSample{.z = 0.0f, .hasZ = true},
            "One-axis joystick"),
        LegacyAxis::None,
        LegacyAxis::None,
        "a single extra axis is not a right stick");

    // Owner-captured WinMM button masks: RT=0x200, LT=0x100, B/Circle=0x2.
    // Edges remain independent when another control is held.
    const ControllerActionEdges attack = MapLegacyControllerEdges(0x202u, 0x002u, capturedBackbone);
    Require(attack.attackPressed && !attack.parryPressed && !attack.dodgePressed,
            "captured Backbone RT must emit one attack edge while B is held");
    const ControllerActionEdges parry = MapLegacyControllerEdges(0x102u, 0x002u, capturedBackbone);
    Require(!parry.attackPressed && parry.parryPressed && !parry.dodgePressed,
            "captured Backbone LT must emit one parry edge while B is held");
    const ControllerActionEdges dodge = MapLegacyControllerEdges(0x002u, 0x000u, capturedBackbone);
    Require(!dodge.attackPressed && !dodge.parryPressed && dodge.dodgePressed,
            "captured Backbone B/Circle must emit a dodge edge");
    Require(!MapLegacyControllerEdges(0x302u, 0x302u, capturedBackbone).Any(),
            "held Backbone actions must not retrigger");

    // Current owner capture: Z/R reach the full 0..65535 range and rest at
    // 32767. Applying the sampled right-stick frame must change the persistent
    // platform view target before the fixed-step simulation consumes it.
    const auto turnedView = ApplyControllerLook(0.0f, 0.0f, 1.0f, 0.0f, 1.0f / 60.0f);
    Require(turnedView.yawRadians > 0.040f && turnedView.yawRadians < 0.043f,
            "full-right Backbone Z must visibly advance yaw at 60 Hz");
    const auto pitchedView = ApplyControllerLook(
        turnedView.yawRadians, 0.27f, 0.0f, -1.0f, 1.0f / 30.0f);
    Require(pitchedView.pitchRadians <= 0.28f && pitchedView.pitchRadians > 0.27f,
            "right-stick pitch must advance and retain the authored clamp");

    // Exact Backbone menu topology: D-pad is a WinMM POV hat and the standard
    // A/B/Menu fields occupy buttons 1/2/12. All are edge-triggered.
    const auto dpadDown = MapLegacyControllerMenuEdges(
        0u, 0u, 18000u, 65535u, capturedBackbone);
    Require(dpadDown.next && !dpadDown.previous,
            "Backbone POV down must navigate to the next visible menu control");
    const auto dpadUp = MapLegacyControllerMenuEdges(
        0u, 0u, 0u, 65535u, capturedBackbone);
    Require(dpadUp.previous && !dpadUp.next,
            "Backbone POV up must navigate to the previous visible menu control");
    const auto dpadRight = MapLegacyControllerMenuEdges(
        0u, 0u, 9000u, 65535u, capturedBackbone);
    Require(dpadRight.increase && !dpadRight.decrease,
            "Backbone POV right must increase a focused menu slider");
    const auto dpadLeft = MapLegacyControllerMenuEdges(
        0u, 0u, 27000u, 65535u, capturedBackbone);
    Require(dpadLeft.decrease && !dpadLeft.increase,
            "Backbone POV left must decrease a focused menu slider");
    Require(StepControllerSlider(75, false) == 70 &&
            StepControllerSlider(75, true) == 80,
            "controller slider steps must move exactly five percentage points");
    Require(StepControllerSlider(50, false) == 50 &&
            StepControllerSlider(100, true) == 100,
            "controller slider steps must respect the 50-100 percent bounds");
    const auto confirm = MapLegacyControllerMenuEdges(
        0x001u, 0u, 65535u, 65535u, capturedBackbone);
    Require(confirm.confirm && !confirm.cancel && !confirm.togglePause,
            "Backbone A must activate the focused menu control");
    const auto cancel = MapLegacyControllerMenuEdges(
        0x002u, 0u, 65535u, 65535u, capturedBackbone);
    Require(!cancel.confirm && cancel.cancel && !cancel.togglePause,
            "Backbone B/Circle must back out of a menu");
    const auto pause = MapLegacyControllerMenuEdges(
        0x0800u, 0u, 65535u, 65535u, capturedBackbone);
    Require(!pause.confirm && !pause.cancel && pause.togglePause,
            "Backbone menu/start must toggle pause");
    Require(!MapLegacyControllerMenuEdges(
                0x0800u, 0x0800u, 65535u, 65535u, capturedBackbone).Any(),
            "held menu/start must not rapidly pause and resume");

    const std::string windowsSource = ReadWindowsSource();
    Require(windowsSource.find("ControllerFocusOutlineSubclass") != std::string::npos &&
            windowsSource.find("SetWindowSubclass") != std::string::npos &&
            windowsSource.find("WM_SETFOCUS") != std::string::npos &&
            windowsSource.find("FrameRect") != std::string::npos,
            "controller-selectable menu controls must draw a persistent focus outline");
    Require(windowsSource.find("GetPrivateProfileIntA(\"progress\", \"rtLabUnlocked\"") != std::string::npos &&
            windowsSource.find("WritePrivateProfileStringA(\"progress\", \"rtLabUnlocked\"") != std::string::npos &&
            windowsSource.find("CanPersistRtLabUnlock(decision)") != std::string::npos,
            "Windows RT Lab progress must use its independent INI section and the genuine-finale decision");
    Require(windowsSource.find("simulation, ctx.outputExposure, ctx.waterQuality, ctx.rtSceneTuning") != std::string::npos &&
            windowsSource.find("context.rtSceneTuning = {};") != std::string::npos &&
            windowsSource.find("context.simulationPaused = pauseVisible || context.settingsVisible || context.rtLabVisible") != std::string::npos,
            "Windows RT Lab must pass route-local tuning to the renderer while pausing only simulation");
    Require(windowsSource.find("RT LAB UNLOCKED") != std::string::npos &&
            windowsSource.find("OPEN RT LAB") != std::string::npos &&
            windowsSource.find("RESTORE AUTHORED") != std::string::npos &&
            windowsSource.find("lastRtLabTelemetryTick < 250u") != std::string::npos,
            "Windows RT Lab must expose completion actions, authored reset, and four-Hz live telemetry");
    Require(windowsSource.find("WrapRtLabFocus(index, direction, controls.size())") != std::string::npos &&
            windowsSource.find("ShouldPlayControllerMenuSound(context.rtLabVisible)") != std::string::npos,
            "production RT Lab focus and silent navigation must use the behavior-tested seams");
    const std::size_t labCommandsBegin = windowsSource.find("case kRtLabButtonId:");
    const std::size_t labCommandsEnd = windowsSource.find("case kDiagnosticsButtonId:", labCommandsBegin);
    const std::size_t labFunctionsBegin = windowsSource.find("void OpenRtLab(");
    const std::size_t labFunctionsEnd = windowsSource.find("void ShowPauseMenu(", labFunctionsBegin);
    Require(labCommandsBegin != std::string::npos && labCommandsEnd != std::string::npos &&
            windowsSource.substr(labCommandsBegin, labCommandsEnd - labCommandsBegin).find("PlaySoundEffect") == std::string::npos &&
            labFunctionsBegin != std::string::npos && labFunctionsEnd != std::string::npos &&
            windowsSource.substr(labFunctionsBegin, labFunctionsEnd - labFunctionsBegin).find("PlaySoundEffect") == std::string::npos,
            "opening, adjusting, restoring, and closing the RT Lab must not add audio behavior");

    ControllerTriggerLatch triggerLatch{};
    const ControllerActionEdges firstTriggers = UpdateXInputTriggerEdges(0u, 255u, triggerLatch);
    Require(firstTriggers.attackPressed && !firstTriggers.parryPressed,
            "XInput RT threshold crossing must attack once");
    Require(!UpdateXInputTriggerEdges(0u, 255u, triggerLatch).Any(),
            "held XInput RT must not attack every frame");
    UpdateXInputTriggerEdges(0u, 0u, triggerLatch);
    const ControllerActionEdges leftTrigger = UpdateXInputTriggerEdges(255u, 0u, triggerLatch);
    Require(!leftTrigger.attackPressed && leftTrigger.parryPressed,
            "XInput LT threshold crossing must parry once");

    std::cout << "Desktop controller input tests passed\n";
    return EXIT_SUCCESS;
}
