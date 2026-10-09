#pragma once

#include "platform/windows/WindowsInteractionPrompt.h"

namespace horde::platform::windows
{
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

enum class DesktopKeyAction { None, Dodge, Parry, ToggleLantern };
constexpr DesktopKeyAction ResolveDesktopGameplayKey(
    const unsigned key, const bool gameplayAvailable, const bool repeated, const bool lanternClaimed)
{
    if (!gameplayAvailable || repeated) return DesktopKeyAction::None;
    if (key == 0x20u) return DesktopKeyAction::Dodge; // VK_SPACE
    if (key == 'Q') return DesktopKeyAction::Parry;
    if (key == 'E' && lanternClaimed) return DesktopKeyAction::ToggleLantern;
    return DesktopKeyAction::None;
}
} // namespace horde::platform::windows
