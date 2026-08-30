#pragma once

#include <string_view>

#include "gameplay/interactions/ChestRewardSequence.h"

namespace horde::platform::windows
{

struct WindowsChestPromptVisibility
{
    bool simulationPaused = false;
    bool pauseMenuVisible = false;
    bool settingsVisible = false;
    bool diagnosticsVisible = false;
    bool benchmarkReportVisible = false;
    bool rtLabVisible = false;
    bool deathOverlayVisible = false;
    bool endingOverlayVisible = false;
    bool benchmarkRunning = false;
    bool captureMode = false;
};

constexpr std::string_view WindowsChestPromptText(
    const horde::gameplay::interactions::ChestRewardPrompt prompt)
{
    using horde::gameplay::interactions::ChestRewardPrompt;
    switch (prompt)
    {
    case ChestRewardPrompt::Locked:
        return "LOCKED | DEFEAT THE LICH";
    case ChestRewardPrompt::OpenChest:
        return "PRESS E / A TO OPEN CHEST";
    case ChestRewardPrompt::Opening:
        return "OPENING...";
    case ChestRewardPrompt::ClaimLantern:
        return "PRESS E / A TO TAKE LANTERN";
    case ChestRewardPrompt::Unlocking:
        return "THE LICH'S SEAL IS BREAKING...";
    default:
        return {};
    }
}

constexpr bool ShouldShowWindowsChestPrompt(
    const horde::gameplay::interactions::ChestRewardPrompt prompt,
    const WindowsChestPromptVisibility& visibility)
{
    return !WindowsChestPromptText(prompt).empty() &&
           !visibility.simulationPaused && !visibility.pauseMenuVisible &&
           !visibility.settingsVisible && !visibility.diagnosticsVisible &&
           !visibility.benchmarkReportVisible && !visibility.rtLabVisible &&
           !visibility.deathOverlayVisible && !visibility.endingOverlayVisible &&
           !visibility.benchmarkRunning && !visibility.captureMode;
}

} // namespace horde::platform::windows
