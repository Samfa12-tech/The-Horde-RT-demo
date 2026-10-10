#include "platform/windows/DesktopControllerInput.h"
#include "platform/windows/GraphicsMenuNavigation.h"
#include "platform/windows/WindowsCaptureContracts.h"
#include "platform/windows/WindowsInteractionPrompt.h"
#include "platform/windows/WindowsGameplayInput.h"
#include "gameplay/simulation/GameSimulation.h"
#include "platform/windows/WindowsRtLabState.h"

#include <cstdlib>
#include <filesystem>
#include <fstream>
#include <iostream>
#include <memory>
#include <sstream>
#include <string>
#include <string_view>
#include <vector>

namespace
{

using horde::platform::windows::LegacyAxis;
using horde::platform::windows::LegacyAxisSample;
using horde::platform::windows::LegacyRightStickAxes;
using horde::platform::windows::ControllerActionEdges;
using horde::platform::windows::ControllerTriggerLatch;
using horde::platform::windows::ControllerPollDisposition;
using horde::platform::windows::ControllerFocusLatch;
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
using horde::platform::windows::StepRtLabScroll;
using horde::platform::windows::RtLabScrollAction;
using horde::platform::windows::WrapRtLabFocus;
using horde::platform::windows::ShouldPlayControllerMenuSound;
using horde::platform::windows::WindowsChestPromptText;
using horde::platform::windows::ClaimedRewardCapturePolicy;
using horde::platform::windows::FindGraphicsMenuNeighbor;
using horde::platform::windows::GraphicsMenuDirection;
using horde::platform::windows::GraphicsMenuRect;

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
    const std::vector<GraphicsMenuRect> graphicsRows{
        {0, 0, 600, 36},       // full-width preset
        {0, 44, 104, 80},      // glass
        {112, 44, 216, 80},    // mist
        {224, 44, 368, 80},    // dust
        {0, 88, 600, 126},     // render-scale trackbar
        {0, 134, 190, 170},    // water
        {198, 134, 388, 170},  // fire
        {396, 134, 600, 170},  // shadow
        {0, 178, 190, 214},    // apply
        {198, 178, 388, 214},  // keep
        {396, 178, 600, 214},  // revert
        {0, 222, 296, 258},    // defaults
        {304, 222, 600, 258},  // back
        {620, 600, 900, 636},  // distant preview control column
    };
    Require(FindGraphicsMenuNeighbor(graphicsRows, 0, GraphicsMenuDirection::Down) == 3u,
            "graphics down must enter the right-aligned toggle nearest the full-width preset center");
    Require(FindGraphicsMenuNeighbor(graphicsRows, 1, GraphicsMenuDirection::Right) == 2u &&
            FindGraphicsMenuNeighbor(graphicsRows, 3, GraphicsMenuDirection::Down) == 4u,
            "graphics horizontal and vertical navigation must follow adjacent control rectangles");
    Require(FindGraphicsMenuNeighbor(graphicsRows, 9, GraphicsMenuDirection::Right) == 10u,
            "right from Keep must select the same-row Revert control");
    Require(FindGraphicsMenuNeighbor(graphicsRows, 12, GraphicsMenuDirection::Right) == 10u,
            "right from Back must select the nearby Revert diagonal instead of the distant preview column");
    Require(FindGraphicsMenuNeighbor(graphicsRows, 3, GraphicsMenuDirection::Right) == 7u,
            "right from Dust must reach the nearest in-cone diagonal Shadow control, not the preset above");
    const std::vector<GraphicsMenuRect> previewOffsetRows{
        {620, 300, 700, 340},  // current preview control
        {640, 440, 720, 476},  // lower preview control, slightly offset but same column
        {705, 390, 785, 426},  // closer vertically, but a diagonal beside the column
    };
    Require(FindGraphicsMenuNeighbor(previewOffsetRows, 0, GraphicsMenuDirection::Down) == 1u,
            "down must prefer the slightly offset same-column preview control over a nearer diagonal row");
    const std::vector<GraphicsMenuRect> filteredControls{
        {0, 0, 100, 36},      // focused, enabled, visible control
        {112, 0, 212, 36},    // enabled, visible next control
    };
    Require(FindGraphicsMenuNeighbor(filteredControls, 0, GraphicsMenuDirection::Right) == 1u,
            "the prefiltered focus list must navigate to its next enabled and visible control");
    const std::vector<GraphicsMenuRect> diagonalOutsideCone{
        {0, 0, 20, 20},
        {-100, 21, -80, 41},
    };
    Require(FindGraphicsMenuNeighbor(diagonalOutsideCone, 0, GraphicsMenuDirection::Down) == std::nullopt &&
            FindGraphicsMenuNeighbor(diagonalOutsideCone, 0, GraphicsMenuDirection::Right) == std::nullopt,
            "spatial navigation must stay put at boundaries and reject unrelated diagonal controls");

    constexpr auto ordinaryLanternCapture =
        ClaimedRewardCapturePolicy("lantern-held-high");
    constexpr auto maximumWallCapture =
        ClaimedRewardCapturePolicy("lantern-wall-high");
    constexpr auto chestClearanceCapture =
        ClaimedRewardCapturePolicy("lantern-chest-held-high");
    constexpr auto finaleRewardCapture =
        ClaimedRewardCapturePolicy("finale-roof");
    static_assert(ordinaryLanternCapture.requirePlayerPixels &&
                  ordinaryLanternCapture.requireRewardBodyPixels &&
                  !ordinaryLanternCapture.requireRewardRingPixels &&
                  !ordinaryLanternCapture.requireSwordPixels &&
                  !ordinaryLanternCapture.permitsCompleteWallRetraction);
    static_assert(!maximumWallCapture.requirePlayerPixels &&
                  !maximumWallCapture.requireRewardBodyPixels &&
                  !maximumWallCapture.requireRewardRingPixels &&
                  maximumWallCapture.requireSwordPixels &&
                  maximumWallCapture.permitsCompleteWallRetraction);
    static_assert(ClaimedRewardCapturePolicy("lantern-wall-low")
                      .permitsCompleteWallRetraction);
    static_assert(chestClearanceCapture.requirePlayerPixels &&
                  chestClearanceCapture.requireRewardBodyPixels &&
                  chestClearanceCapture.requireRewardRingPixels &&
                  chestClearanceCapture.requireSwordPixels &&
                  !chestClearanceCapture.permitsCompleteWallRetraction);
    static_assert(finaleRewardCapture.requirePlayerPixels &&
                  finaleRewardCapture.requireRewardBodyPixels &&
                  finaleRewardCapture.requireRewardRingPixels &&
                  finaleRewardCapture.requireSwordPixels &&
                  !finaleRewardCapture.permitsCompleteWallRetraction);

    using namespace horde::gameplay::items;
    HeldItemState stableStow;
    stableStow.id = HeldItemId::Sword;
    stableStow.hand = HeldHand::RightHand;
    stableStow.parentMode = HeldItemParentMode::BodyStow;
    stableStow.visualStowBlend = 1.0f;
    stableStow.visualGripBlend = 0.0f;
    const auto stowPolicy = ClaimedRewardCapturePolicy("finale-roof",
        horde::platform::windows::IsCaptureSwordFullyStowed(stableStow));
    Require(stowPolicy.requirePlayerPixels && stowPolicy.requireRewardBodyPixels &&
            stowPolicy.requireRewardRingPixels && !stowPolicy.requireSwordPixels &&
            !stowPolicy.permitsCompleteWallRetraction,
            "fully stowed sword only changes sword primary visibility, preserving reward proof");
    for (unsigned invalid = 0; invalid < 7; ++invalid)
    {
        auto other = stableStow;
        if (invalid == 0) other.transition.active = true;
        if (invalid == 1) other.visualStowBlend = 0.9f;
        if (invalid == 2) other.visualGripBlend = 0.1f;
        if (invalid == 3) other.parentMode = HeldItemParentMode::HandSocket;
        if (invalid == 4) other.detached = true;
        if (invalid == 5) other.id = HeldItemId::OriginalTorch;
        if (invalid == 6) other.hand = HeldHand::LeftHand;
        Require(ClaimedRewardCapturePolicy("finale-roof",
                    horde::platform::windows::IsCaptureSwordFullyStowed(other)).requireSwordPixels,
                "held, partial, moving or invalid stow must not waive held-sword visibility");
    }

    using horde::gameplay::interactions::ChestRewardPrompt;
    Require(WindowsChestPromptText(ChestRewardPrompt::Locked) ==
                "LOCKED | DEFEAT THE LICH" &&
            WindowsChestPromptText(ChestRewardPrompt::OpenChest) ==
                "LEFT-CLICK / A TO OPEN CHEST" &&
            WindowsChestPromptText(ChestRewardPrompt::Opening) == "OPENING..." &&
            WindowsChestPromptText(ChestRewardPrompt::ClaimLantern) ==
                "LEFT-CLICK / A TO TAKE LANTERN" &&
            WindowsChestPromptText(ChestRewardPrompt::Unlocking) ==
                "THE LICH'S SEAL IS BREAKING..." &&
            WindowsChestPromptText(ChestRewardPrompt::None).empty(),
            "Windows labels must map every shared chest prompt without owning gameplay state");
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
    Require(StepRtLabScroll(120, 600, 36, 240, RtLabScrollAction::LineDown) == 156 &&
            StepRtLabScroll(120, 600, 36, 240, RtLabScrollAction::LineUp) == 84 &&
            StepRtLabScroll(120, 600, 36, 240, RtLabScrollAction::PageDown) == 360 &&
            StepRtLabScroll(120, 600, 36, 240, RtLabScrollAction::PageUp) == 0 &&
            StepRtLabScroll(120, 600, 36, 240, RtLabScrollAction::Top) == 0 &&
            StepRtLabScroll(120, 600, 36, 240, RtLabScrollAction::Bottom) == 600 &&
            StepRtLabScroll(120, 600, 36, 240, RtLabScrollAction::Thumb, 345) == 345,
            "RT Lab line, page, thumb, and boundary scrolling must remain reachable and clamped");
    Require(ShouldPlayControllerMenuSound(false) && !ShouldPlayControllerMenuSound(true),
            "ordinary menu navigation may retain feedback while every RT Lab interaction stays silent");

    constexpr LegacyControllerIdentity capturedBackbone{
        .vendorId = 0x358au,
        .productId = 0x0204u,
        .productName = "Microsoft PC-joystick driver",
    };

    ControllerFocusLatch focusLatch{};
    Require(focusLatch.Observe(true) == ControllerPollDisposition::Reseed,
            "first focused controller poll must seed held inputs without delivering them");
    focusLatch.CompleteReseed();
    Require(focusLatch.Observe(true) == ControllerPollDisposition::Deliver,
            "controller input must deliver after its initial focused baseline");
    Require(focusLatch.Observe(false) == ControllerPollDisposition::Suppress &&
                focusLatch.Observe(false) == ControllerPollDisposition::Suppress,
            "unfocused controller polls must suppress delivery");
    Require(focusLatch.Observe(true) == ControllerPollDisposition::Reseed,
            "focus return must reseed held controller buttons before delivery");
    focusLatch.CompleteReseed();
    Require(focusLatch.Observe(true) == ControllerPollDisposition::Deliver,
            "controller delivery must resume after the focus-return baseline is seeded");
    const std::uint32_t heldButtons = 0x0803u;
    Require(!MapLegacyControllerEdges(heldButtons, heldButtons, capturedBackbone).Any() &&
                !MapLegacyControllerMenuEdges(
                    heldButtons, heldButtons, 18000u, 18000u, capturedBackbone).Any(),
            "buttons held through focus return must not become gameplay or menu presses");
    Require(!MapLegacyControllerEdges(0u, heldButtons, capturedBackbone).Any() &&
                MapLegacyControllerEdges(heldButtons, 0u, capturedBackbone).Any() &&
                MapLegacyControllerMenuEdges(
                    heldButtons, 0u, 65535u, 65535u, capturedBackbone).Any(),
            "release then repress after focus return must produce fresh gameplay and menu edges");
    ControllerTriggerLatch focusTriggerLatch{};
    horde::platform::windows::SeedXInputTriggerLatch(0u, 255u, focusTriggerLatch);
    Require(!UpdateXInputTriggerEdges(0u, 255u, focusTriggerLatch).Any(),
            "a trigger held through focus return must be seeded without a gameplay action");
    UpdateXInputTriggerEdges(0u, 0u, focusTriggerLatch);
    Require(UpdateXInputTriggerEdges(0u, 255u, focusTriggerLatch).attackPressed,
            "a trigger released then pressed after focus return must produce a fresh action");

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
    const ControllerActionEdges interact =
        MapLegacyControllerEdges(0x001u, 0x000u, capturedBackbone);
    Require(interact.interactPressed && !interact.attackPressed &&
                !interact.toggleHeldLightPosePressed,
            "captured Backbone A must interact during unpaused gameplay");
    const ControllerActionEdges toggle =
        MapLegacyControllerEdges(0x008u, 0x000u, capturedBackbone);
    Require(toggle.toggleHeldLightPosePressed && !toggle.interactPressed,
            "captured Backbone Y must raise/lower the claimed lantern");
    Require(!MapLegacyControllerEdges(0x302u, 0x302u, capturedBackbone).Any(),
            "held Backbone actions must not retrigger");

    const LegacyControllerIdentity genericController{};
    const ControllerActionEdges genericInteract =
        MapLegacyControllerEdges(0x001u, 0x000u, genericController);
    Require(genericInteract.interactPressed && !genericInteract.attackPressed,
            "generic controller A must interact without overloading attack");
    const ControllerActionEdges genericAttack =
        MapLegacyControllerEdges(0x004u, 0x000u, genericController);
    Require(genericAttack.attackPressed && !genericAttack.interactPressed,
            "generic controller X must retain an independent attack edge");

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

    // Owner's read-only paused L3 capture: button 14, mask 0x2000.
    Require(horde::platform::windows::LegacyRunTogglePressed(0x2000u, 0u, capturedBackbone) &&
            !horde::platform::windows::LegacyRunTogglePressed(0x2000u, 0x2000u, capturedBackbone) &&
            !horde::platform::windows::LegacyRunTogglePressed(0u, 0x2000u, capturedBackbone) &&
            !horde::platform::windows::LegacyRunTogglePressed(0x0800u, 0u, capturedBackbone) &&
            !horde::platform::windows::LegacyRunTogglePressed(0x2000u, 0u, LegacyControllerIdentity{}),
            "captured L3 toggles once; hold/release/menu/unknown layouts cannot toggle run");
    Require(horde::platform::windows::XInputRunTogglePressed(0x0040u, 0u) &&
            !horde::platform::windows::XInputRunTogglePressed(0x0040u, 0x0040u) &&
            !horde::platform::windows::XInputRunTogglePressed(0x0080u, 0u),
            "XInput L3 uses a single edge and never maps right-stick click to run");
    const std::string windowsSource = ReadWindowsSource();
    const auto legacyAcquisition = windowsSource.find("if (context.legacyJoystickId != joystick ||");
    const auto legacyAcquisitionEnd = windowsSource.find("context.legacyJoystickId = joystick;", legacyAcquisition);
    const auto xinputAcquisition = windowsSource.find("if (context.xInputUserIndex != xinputUser)");
    const auto xinputAcquisitionEnd = windowsSource.find("context.xInputUserIndex = xinputUser;", xinputAcquisition);
    Require(legacyAcquisition != std::string::npos && legacyAcquisitionEnd != std::string::npos &&
            xinputAcquisition != std::string::npos && xinputAcquisitionEnd != std::string::npos,
            "both actual native device-acquisition paths must remain inspectable");
    Require(windowsSource.substr(legacyAcquisition, legacyAcquisitionEnd - legacyAcquisition).find(
                "context.previousLegacyControllerButtons = legacy.dwButtons;") != std::string::npos &&
            windowsSource.substr(xinputAcquisition, xinputAcquisitionEnd - xinputAcquisition).find(
                "context.previousControllerButtons = state.Gamepad.wButtons;") != std::string::npos &&
            windowsSource.substr(xinputAcquisition, xinputAcquisitionEnd - xinputAcquisition).find(
                "SeedXInputTriggerLatch(") != std::string::npos,
            "native acquisition must seed held buttons/triggers before mapping gameplay edges");

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
    Require(horde::platform::windows::StepControllerAudioVolume(40, false) == 30 &&
            horde::platform::windows::StepControllerAudioVolume(40, true) == 50 &&
            horde::platform::windows::StepControllerAudioVolume(10, false) == 0 &&
            horde::platform::windows::StepControllerAudioVolume(0, false) == 0 &&
            horde::platform::windows::StepControllerAudioVolume(100, true) == 100,
            "both audio sliders must step by ten and reach mute, not inherit render-scale bounds");
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

    Require(windowsSource.find("ControllerFocusOutlineSubclass") != std::string::npos &&
            windowsSource.find("SetWindowSubclass") != std::string::npos &&
            windowsSource.find("WM_SETFOCUS") != std::string::npos &&
            windowsSource.find("FrameRect") != std::string::npos,
            "controller-selectable menu controls must draw a persistent focus outline");
    Require(windowsSource.find("GetPrivateProfileIntA(\"progress\", \"rtLabUnlocked\"") != std::string::npos &&
            windowsSource.find("WritePrivateProfileStringA(\"progress\", \"rtLabUnlocked\"") != std::string::npos &&
            windowsSource.find("CanPersistRtLabUnlock(decision)") != std::string::npos,
            "Windows RT Lab progress must use its independent INI section and the genuine-finale decision");
    const std::size_t measurementPauseBegin = windowsSource.find("bool MeasurementPausedByUi(");
    const std::size_t measurementPauseEnd = windowsSource.find(
        "void ApplyOverlayState(", measurementPauseBegin);
    Require(windowsSource.find("simulation, ctx.outputExposure, ctx.waterQuality, ctx.rtSceneTuning") != std::string::npos &&
            windowsSource.find("context.rtSceneTuning = {};") != std::string::npos &&
            measurementPauseBegin != std::string::npos && measurementPauseEnd != std::string::npos &&
            windowsSource.substr(measurementPauseBegin, measurementPauseEnd - measurementPauseBegin)
                    .find("return pauseVisible || context.settingsVisible || context.rtLabVisible") != std::string::npos &&
            windowsSource.substr(measurementPauseBegin, measurementPauseEnd - measurementPauseBegin)
                    .find("context.diagnosticsVisible || context.benchmarkReportVisible;") != std::string::npos &&
            windowsSource.find("context.simulationPaused = MeasurementPausedByUi(context);") != std::string::npos &&
            windowsSource.find("context.simulationInput.paused = context.simulationPaused;") != std::string::npos,
            "Windows RT Lab must pass route-local tuning to the renderer while pausing simulation through the shared UI helper");
    Require(windowsSource.find("RT LAB UNLOCKED") != std::string::npos &&
            windowsSource.find("OPEN RT LAB") != std::string::npos &&
            windowsSource.find("RESTORE AUTHORED") != std::string::npos &&
            windowsSource.find("lastRtLabTelemetryTick < 250u") != std::string::npos,
            "Windows RT Lab must expose completion actions, authored reset, and four-Hz live telemetry");
    const std::size_t endingMenuBegin = windowsSource.find("void ShowEndingMenu(");
    const std::size_t endingMenuEnd = windowsSource.find("bool ApplyPlayerRetryCheckpoint(", endingMenuBegin);
    Require(endingMenuBegin != std::string::npos && endingMenuEnd != std::string::npos &&
            windowsSource.substr(endingMenuBegin, endingMenuEnd - endingMenuBegin)
                    .find("context.rtLabVisible") != std::string::npos,
            "Windows finale polling must not replace or mutate an open RT Lab");
    Require(windowsSource.find("WrapRtLabFocus(index, direction, controls.size())") != std::string::npos &&
            windowsSource.find("ShouldPlayControllerMenuSound(context.rtLabVisible)") != std::string::npos,
            "production RT Lab focus and silent navigation must use the behavior-tested seams");
    const std::size_t graphicsNavigationBegin = windowsSource.find("void NavigateGraphicsMenu(");
    const std::size_t graphicsNavigationEnd = windowsSource.find("void CancelControllerMenu(", graphicsNavigationBegin);
    const std::size_t visibleControlsBegin = windowsSource.find("std::vector<HWND> VisibleControllerMenuControls(");
    const std::size_t visibleControlsEnd = windowsSource.find("void NavigateControllerMenu(", visibleControlsBegin);
    const std::size_t keyboardTabBegin = windowsSource.find("sceneContext->simulationPaused && wParam == VK_TAB");
    const std::size_t keyboardTabEnd = windowsSource.find("wParam == VK_UP || wParam == VK_DOWN", keyboardTabBegin);
    const std::size_t keyboardHorizontalBegin = windowsSource.find(
        "if (sceneContext->simulationPaused && (wParam == VK_LEFT || wParam == VK_RIGHT)");
    const std::size_t keyboardHorizontalEnd = windowsSource.find(
        "if (sceneContext->simulationPaused && (wParam == VK_RETURN || wParam == VK_SPACE)",
        keyboardHorizontalBegin);
    Require(graphicsNavigationBegin != std::string::npos && graphicsNavigationEnd != std::string::npos &&
            windowsSource.substr(graphicsNavigationBegin, graphicsNavigationEnd - graphicsNavigationBegin)
                    .find("FindGraphicsMenuNeighbor(") != std::string::npos &&
            windowsSource.find("NavigateGraphicsMenu(*sceneContext") != std::string::npos &&
            visibleControlsBegin != std::string::npos && visibleControlsEnd != std::string::npos &&
            windowsSource.substr(visibleControlsBegin, visibleControlsEnd - visibleControlsBegin)
                    .find("IsWindowEnabled(control)") != std::string::npos &&
            windowsSource.substr(visibleControlsBegin, visibleControlsEnd - visibleControlsBegin)
                    .find("IsWindowVisible(control)") != std::string::npos &&
            keyboardHorizontalBegin != std::string::npos && keyboardHorizontalEnd != std::string::npos &&
            windowsSource.substr(keyboardHorizontalBegin, keyboardHorizontalEnd - keyboardHorizontalBegin)
                    .find("AdjustFocusedControllerSlider") <
            windowsSource.substr(keyboardHorizontalBegin, keyboardHorizontalEnd - keyboardHorizontalBegin)
                    .find("NavigateGraphicsMenu") &&
            keyboardTabBegin != std::string::npos && keyboardTabEnd != std::string::npos &&
            windowsSource.substr(keyboardTabBegin, keyboardTabEnd - keyboardTabBegin)
                    .find("NavigateControllerMenu(*sceneContext,") != std::string::npos &&
            windowsSource.substr(keyboardTabBegin, keyboardTabEnd - keyboardTabBegin)
                    .find("(GetKeyState(VK_SHIFT) & 0x8000) != 0 ? -1 : 1") != std::string::npos &&
            windowsSource.find("WrapRtLabFocus(index, direction, controls.size())") != std::string::npos,
            "graphics arrow navigation must use spatial control bounds while Tab retains the cyclic menu path");
    const std::size_t labCommandsBegin = windowsSource.find("case kRtLabButtonId:");
    const std::size_t labCommandsEnd = windowsSource.find("case kDiagnosticsButtonId:", labCommandsBegin);
    const std::size_t labFunctionsBegin = windowsSource.find("void OpenRtLab(");
    const std::size_t labFunctionsEnd = windowsSource.find("void ShowPauseMenu(", labFunctionsBegin);
    Require(labCommandsBegin != std::string::npos && labCommandsEnd != std::string::npos &&
            windowsSource.substr(labCommandsBegin, labCommandsEnd - labCommandsBegin).find("PlaySoundEffect") == std::string::npos &&
            labFunctionsBegin != std::string::npos && labFunctionsEnd != std::string::npos &&
            windowsSource.substr(labFunctionsBegin, labFunctionsEnd - labFunctionsBegin).find("PlaySoundEffect") == std::string::npos,
            "opening, adjusting, restoring, and closing the RT Lab must not add audio behavior");

    using horde::gameplay::interactions::ChestRewardPrompt;
    using horde::platform::windows::ShouldShowWindowsChestPrompt;
    using horde::platform::windows::WindowsChestPromptVisibility;
    Require(ShouldShowWindowsChestPrompt(ChestRewardPrompt::Locked, {}) &&
                !ShouldShowWindowsChestPrompt(ChestRewardPrompt::None, {}),
            "Windows chest prompt visibility requires a shared non-empty gameplay prompt");
    for (const WindowsChestPromptVisibility suppressed : {
             WindowsChestPromptVisibility{.simulationPaused = true},
             WindowsChestPromptVisibility{.pauseMenuVisible = true},
             WindowsChestPromptVisibility{.settingsVisible = true},
             WindowsChestPromptVisibility{.diagnosticsVisible = true},
             WindowsChestPromptVisibility{.benchmarkReportVisible = true},
             WindowsChestPromptVisibility{.rtLabVisible = true},
             WindowsChestPromptVisibility{.deathOverlayVisible = true},
             WindowsChestPromptVisibility{.endingOverlayVisible = true},
             WindowsChestPromptVisibility{.benchmarkRunning = true},
             WindowsChestPromptVisibility{.captureMode = true}})
    {
        Require(!ShouldShowWindowsChestPrompt(ChestRewardPrompt::OpenChest, suppressed),
                "pause, death, finale, lab, diagnostics, benchmark, settings, and capture UI must suppress the Windows chest prompt");
    }
    const std::size_t overlayStateBegin = windowsSource.find("void ApplyOverlayState(");
    const std::size_t overlayStateEnd = windowsSource.find("void ShowPauseMenu(", overlayStateBegin);
    Require(overlayStateBegin != std::string::npos && overlayStateEnd != std::string::npos &&
                windowsSource.substr(overlayStateBegin, overlayStateEnd - overlayStateBegin)
                        .find("UpdateChestPrompt(context);") != std::string::npos,
            "synchronous Windows overlay transitions must hide the chest prompt without waiting for another rendered frame");

    using horde::platform::windows::DesktopClickAction;
    using horde::platform::windows::DesktopKeyAction;
    using horde::platform::windows::ResolveDesktopLeftClick;
    using horde::platform::windows::ResolveDesktopGameplayKey;
    Require(ResolveDesktopLeftClick(true, false, ChestRewardPrompt::OpenChest) == DesktopClickAction::AcquireCapture &&
            ResolveDesktopLeftClick(true, false, ChestRewardPrompt::None) == DesktopClickAction::AcquireCapture,
            "initial focus/capture click never publishes interaction or attack");
    Require(ResolveDesktopLeftClick(false, true, ChestRewardPrompt::OpenChest) == DesktopClickAction::Ignore &&
            ResolveDesktopLeftClick(false, false, ChestRewardPrompt::None) == DesktopClickAction::Ignore,
            "menu, pause and lost gameplay ownership clicks are ignored");
    Require(ResolveDesktopLeftClick(true, true, ChestRewardPrompt::None) == DesktopClickAction::Attack &&
            ResolveDesktopLeftClick(true, true, ChestRewardPrompt::OpenChest, true) == DesktopClickAction::Interact &&
            ResolveDesktopLeftClick(true, true, ChestRewardPrompt::ClaimLantern, true) == DesktopClickAction::Interact &&
            ResolveDesktopLeftClick(true, true, ChestRewardPrompt::Opening) == DesktopClickAction::Ignore &&
            ResolveDesktopLeftClick(true, true, ChestRewardPrompt::Locked) == DesktopClickAction::Attack &&
            ResolveDesktopLeftClick(true, true, ChestRewardPrompt::Unlocking) == DesktopClickAction::Ignore,
            "captured click chooses one action; locked hint preserves attack and busy chest phases consume clicks");
    Require(ResolveDesktopLeftClick(true,true,ChestRewardPrompt::None,true)==DesktopClickAction::Interact &&
            ResolveDesktopLeftClick(true,true,ChestRewardPrompt::OpenChest,false)==DesktopClickAction::Ignore &&
            ResolveDesktopLeftClick(true,true,ChestRewardPrompt::Locked,true)==DesktopClickAction::Interact,
            "stale displayed prompt stays interaction even after reset locks chest; unseen eligibility never swings");
    Require(ResolveDesktopGameplayKey(0x20u,true,false,false)==DesktopKeyAction::Dodge &&
            ResolveDesktopGameplayKey('Q',true,false,false)==DesktopKeyAction::Parry &&
            ResolveDesktopGameplayKey('E',true,false,true)==DesktopKeyAction::ToggleLantern &&
            ResolveDesktopGameplayKey('E',true,false,false)==DesktopKeyAction::None &&
            ResolveDesktopGameplayKey('C',true,false,true)==DesktopKeyAction::None &&
            ResolveDesktopGameplayKey('F',true,false,true)==DesktopKeyAction::None,
            "Space/Q/E have new Windows mapping, claimed-only lantern and retired C/F mappings");
    for (const unsigned key : {0x20u, unsigned('Q'), unsigned('E')})
        Require(ResolveDesktopGameplayKey(key,false,false,true)==DesktopKeyAction::None &&
                ResolveDesktopGameplayKey(key,true,true,true)==DesktopKeyAction::None,
                "menu Space and auto-repeated gameplay keys do not generate edges");

    // Route the native click decision into the real authoritative consumer.
    // Eligibility can change after the snapshot used to select the intent.
    using namespace horde::gameplay::simulation;
    using namespace horde::gameplay::interactions;
    GameSimulation staleClick;
    ChestRewardSnapshot unlocked;
    unlocked.phase=ChestRewardPhase::ClosedUnlocked;
    staleClick.ImportRewardCheckpoint(unlocked, {}, {});
    InputSnapshot staleInput;
    staleInput.damageEnabled=false;
    const auto intent=ResolveDesktopLeftClick(true,true,staleClick.Snapshot().chestPrompt,true);
    if(intent==DesktopClickAction::Interact) ++staleInput.commands.interact;
    if(intent==DesktopClickAction::Attack) ++staleInput.commands.attack;
    staleClick.StepFixed(staleInput); // actual spawn is out of reward range
    Require(staleClick.Snapshot().lastConsumedInteractSequence==1u &&
            staleClick.Snapshot().lastConsumedAttackSequence==0u &&
            staleClick.Snapshot().chestReward.phase==ChestRewardPhase::ClosedUnlocked,
            "stale contextual intent is authoritatively rejected without fallback swing");
    staleClick.StepFixed(staleInput);
    Require(staleClick.Snapshot().lastConsumedInteractSequence==1u &&
            staleClick.Snapshot().lastConsumedAttackSequence==0u,
            "one mouse edge is not repeated on subsequent fixed ticks");
    staleInput.commands.attack=2; staleInput.commands.parry=2; staleInput.commands.dodge=2;
    staleInput.commands.interact=2; staleInput.commands.toggleHeldLightPose=2;
    staleClick.SynchronizePausedInput(staleInput);
    staleClick.StepFixed(staleInput);
    Require(staleClick.Snapshot().lastConsumedAttackSequence==2u &&
            staleClick.Snapshot().lastConsumedDodgeSequence==2u &&
            !staleClick.Snapshot().swordCombat.playerAttackPulse &&
            staleClick.Snapshot().chestReward.phase==ChestRewardPhase::ClosedUnlocked,
            "capture/focus/pause cancellation discards queued edges before resume");
    const auto mouseBegin=windowsSource.rfind("    case WM_LBUTTONDOWN:");
    const auto mouseEnd=windowsSource.find("    case WM_MOUSEMOVE:",mouseBegin);
    const auto mouseSection=windowsSource.substr(mouseBegin,mouseEnd-mouseBegin);
    const auto rightBegin=mouseSection.find("    case WM_RBUTTONDOWN:");
    Require(mouseSection.find("ResolveDesktopLeftClick(")!=std::string::npos &&
            mouseSection.find("sceneContext->chestInteractionPromptPresented")!=std::string::npos &&
            rightBegin!=std::string::npos &&
            mouseSection.substr(rightBegin).find("PublishDesktopCombatEdge")==std::string::npos &&
            windowsSource.find("ResolveDesktopGameplayKey(")!=std::string::npos &&
            windowsSource.find("DiscardDesktopPendingCommands(*sceneContext)")!=std::string::npos &&
            windowsSource.find("(wParam == VK_RETURN || wParam == VK_SPACE)")!=std::string::npos,
            "native owner wires tested admission, reserved right mouse, cancellation and menu Space");

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

    // Deliver the measured physical L3 edge through the shared fixed-step
    // command path, including held/reseed and lifecycle intent cancellation.
    auto runSimulation=std::make_unique<horde::gameplay::simulation::GameSimulation>();
    horde::gameplay::simulation::InputSnapshot runInput;
    runInput.damageEnabled=false;runInput.moveForward=1;
    std::uint32_t previousL3=0;
    for(int tick=0;tick<12;++tick) {
        if(horde::platform::windows::LegacyRunTogglePressed(0x2000u,previousL3,capturedBackbone))
            ++runInput.commands.runToggle;
        previousL3=0x2000u;runSimulation->StepFixed(runInput);
    }
    Require(runInput.commands.runToggle==1 && runSimulation->Snapshot().runToggleActive &&
            runSimulation->Snapshot().runActive,"L3 held over twelve ticks creates one running intent");
    runInput.paused=true;runSimulation->StepFixed(runInput);
    Require(!runSimulation->Snapshot().runToggleActive&&!runSimulation->Snapshot().runActive,
            "pause cancels controller run and keeps gameplay frozen");
    runInput.paused=false;runSimulation->StepFixed(runInput);
    Require(!runSimulation->Snapshot().runToggleActive,
            "same held/reseed L3 counter cannot restore running after pause");
    runInput.commands.runToggle=2;runSimulation->StepFixed(runInput);
    Require(runSimulation->Snapshot().runToggleActive,"a fresh deliberate L3 edge can run again");
    ++runInput.commands.clearRunIntent;runSimulation->StepFixed(runInput);
    Require(!runSimulation->Snapshot().runToggleActive,"focus/traversal intent cancellation consumes the existing run toggle");

    std::cout << "Desktop controller input tests passed\n";
    return EXIT_SUCCESS;
}
