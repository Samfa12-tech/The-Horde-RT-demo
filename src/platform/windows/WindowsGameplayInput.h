#pragma once

#include "platform/windows/WindowsInteractionPrompt.h"
#include <algorithm>
#include <cmath>

namespace horde::platform::windows
{
struct DesktopMovementInput {
    float forward = 0, strafe = 0;
    bool analogue = false;
};
// A keyboard movement hold owns its walk/Shift contract. A connected or
// tilted controller cannot silently turn that digital movement into running.
inline DesktopMovementInput ResolveDesktopMovementInput(bool forward, bool backward,
    bool left, bool right, float controllerForward, float controllerStrafe) noexcept {
    if (forward || backward || left || right)
        return {float(forward)-float(backward),float(right)-float(left),false};
    const auto bounded=[](float value){return std::isfinite(value)?std::clamp(value,-1.f,1.f):0.f;};
    const float f=bounded(controllerForward),s=bounded(controllerStrafe);
    return {f,s,f!=0 || s!=0};
}
enum class DesktopClickAction { Ignore, AcquireCapture, Attack, Interact };

// One click chooses one intent. Simulation revalidates Interact at consumption;
// a stale/failed interaction is never retried as an attack. Keep a displayed
// interaction intent even if eligibility has changed since that HUD publication.
constexpr DesktopClickAction ResolveDesktopLeftClick(
    const bool gameplayAvailable, const bool capturedAndFocused,
    const gameplay::interactions::ChestRewardPrompt prompt,
    const bool interactionPromptPresented = false)
{
    using gameplay::interactions::ChestRewardPrompt;
    if (!gameplayAvailable) return DesktopClickAction::Ignore;
    if (!capturedAndFocused) return DesktopClickAction::AcquireCapture;
    if (interactionPromptPresented) return DesktopClickAction::Interact;
    if (prompt == ChestRewardPrompt::None || prompt == ChestRewardPrompt::Locked)
        return DesktopClickAction::Attack;
    return DesktopClickAction::Ignore;
}

enum class DesktopKeyAction { None, Dodge, Parry, ToggleLantern, DrinkFlask, Rest };
constexpr DesktopKeyAction ResolveDesktopGameplayKey(
    const unsigned key, const bool gameplayAvailable, const bool repeated, const bool lanternClaimed,
    const bool healingFlaskEnabled=false)
{
    if (!gameplayAvailable || repeated) return DesktopKeyAction::None;
    if (key == 0x20u) return DesktopKeyAction::Dodge; // VK_SPACE
    if (key == 'Q') return DesktopKeyAction::Parry;
    if (key == 'E' && lanternClaimed) return DesktopKeyAction::ToggleLantern;
    if (healingFlaskEnabled && key == 'H') return DesktopKeyAction::DrinkFlask;
    if (healingFlaskEnabled && key == 'J') return DesktopKeyAction::Rest;
    return DesktopKeyAction::None;
}
} // namespace horde::platform::windows
