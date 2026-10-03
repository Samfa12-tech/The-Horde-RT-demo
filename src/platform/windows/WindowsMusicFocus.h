#pragma once

namespace horde::platform::windows
{

// Sample the actual foreground game window at publication, not a latched
// WM_ACTIVATEAPP flag. A loss notification suspends immediately even if the
// foreground handover has not completed when that notification is dispatched.
[[nodiscard]] constexpr bool WindowsMusicShouldSuspend(
    const bool gameForeground,
    const bool controlsReady,
    const bool menuPaused,
    const bool focusLossNotification = false) noexcept
{
    return !gameForeground || !controlsReady || menuPaused || focusLossNotification;
}

} // namespace horde::platform::windows
