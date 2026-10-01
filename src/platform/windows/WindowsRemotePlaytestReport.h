#pragma once

#ifndef WIN32_LEAN_AND_MEAN
#define WIN32_LEAN_AND_MEAN
#endif
#ifndef NOMINMAX
#define NOMINMAX
#endif
#include <windows.h>

#include "platform/windows/WindowsPlaytestReport.h"
#include "platform/windows/WindowsPlaytestSubmission.h"
#include "platform/windows/WindowsPlaytestVerification.h"
#include "reporting/PlaytestReport.h"
#include "reporting/PlaytestSubmission.h"

#include <functional>
#include <cstdint>
#include <string>
#include <string_view>

namespace horde::platform::windows
{
struct WindowsRemotePlaytestServices
{
    // Must synchronously return only a downsampled game RT frame. This callback
    // is called only after screenshot opt-in and report validation; never pass
    // desktop/window pixels, UI, logs, saves, or file contents here.
    std::function<bool(HWND, WindowsPlaytestReportContext&,
        reporting::PlaytestScreenshotPixels&, std::wstring&)> captureGameFrame;
    std::function<WindowsPlaytestVerificationResult(HWND)> verifyForeground;
    WindowsPlaytestExchange exchange;
    std::function<bool(HWND, const std::string&, std::wstring&)> exportOfflineJson;
};

namespace remote_playtest_control
{
inline constexpr int Category = 202;
inline constexpr int Impact = 204;
inline constexpr int Note = 206;
inline constexpr int SubmitConsent = 207;
inline constexpr int IncludeDiagnostics = 208;
inline constexpr int IncludeScreenshot = 209;
inline constexpr int Preview = 210;
inline constexpr int Status = 211;
inline constexpr int Send = 212;
inline constexpr int Retry = 213;
inline constexpr int OfflineExport = 214;
inline constexpr int NewReport = 215;
inline constexpr int Close = 216;
}

// Aspect-preserving centered fit used by the game-frame preview only; it does
// not alter the submitted screenshot dimensions or pixels.
[[nodiscard]] RECT FitWindowsPlaytestPreviewRect(const RECT& bounds,
    std::uint32_t imageWidth, std::uint32_t imageHeight) noexcept;

// Foreground, owner-modal native form. It never persists consent or queues
// retries in the background. Missing verification/exchange services use the
// fixed production adapters; screenshot capture stays unavailable unless the
// application injects its render-owner callback.
void ShowWindowsRemotePlaytestReport(HWND owner,
    WindowsPlaytestReportContext context,
    WindowsRemotePlaytestServices services = {});
} // namespace horde::platform::windows
