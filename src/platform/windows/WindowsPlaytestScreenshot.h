#pragma once

#include "reporting/PlaytestSubmission.h"

namespace horde::platform::windows
{
// Encodes an already-consented, RGB-normalised game RT thumbnail in memory.
// No OS capture, file import, metadata, silent resize or disk write. WIC output
// storage is bounded at the attachment cap, including unsuccessful encodes.
[[nodiscard]] bool EncodeWindowsPlaytestScreenshot(
    const reporting::PlaytestScreenshotPixels& pixels, std::vector<std::uint8_t>& png) noexcept;
}
