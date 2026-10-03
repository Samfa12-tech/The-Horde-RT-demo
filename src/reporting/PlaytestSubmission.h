#pragma once

#include "reporting/PlaytestReport.h"

#include <span>
#include <vector>

namespace horde::reporting
{
inline constexpr std::size_t kPlaytestSubmissionMaxBytes = 768u * 1024u;
inline constexpr std::size_t kPlaytestScreenshotMaxBytes = 512u * 1024u;
inline constexpr std::uint32_t kPlaytestScreenshotMaxLongEdge = 1280u;
inline constexpr std::uint32_t kPlaytestScreenshotMaxShortEdge = 720u;
// Default report thumbnail, disclosed by native UI. The schema admits larger
// future callers, but noisy RT images must normally fit the 512KiB PNG cap.
// This affects only the attachment, never gameplay resolution/materials/pixels.
inline constexpr std::uint32_t kPlaytestScreenshotCaptureLongEdge = 768u;
inline constexpr std::uint32_t kPlaytestScreenshotCaptureShortEdge = 432u;
// Capture readback is opt-in and transient. Reject oversized RT targets BEFORE
// allocating the GPU/CPU readback; one 4K RGBA8 image fits this 32MiB source cap.
inline constexpr std::uint64_t kPlaytestScreenshotMaxSourcePixels = 8u * 1024u * 1024u;

struct PlaytestScreenshotPixels
{
    std::uint32_t width = 0u, height = 0u;
    std::vector<std::uint8_t> rgba;
};
// Proportional bounded screenshot, not a renderer-resolution change. Input must
// be the consented, presented RT image in normal RGB channel order. No metadata.
[[nodiscard]] bool ResizePlaytestScreenshotRgba(std::uint32_t width, std::uint32_t height,
    std::span<const std::uint8_t> rgba, PlaytestScreenshotPixels& output);

// Only the game RT target may supply these pixels. The platform capture owner
// must obtain explicit screenshot consent BEFORE readback/encoding. No OS screen,
// UI overlay, file import, log, or save capture belongs in this interface.
struct PlaytestReportScreenshot
{
    std::span<const std::uint8_t> png;
    std::uint32_t width = 0u, height = 0u;
};

enum class PlaytestSubmissionStatus : std::uint8_t
{
    Ready, ReportRejected, ScreenshotNotConsented, InvalidScreenshot, TooLarge,
};

struct PreparedPlaytestSubmission
{
    PlaytestSubmissionStatus status = PlaytestSubmissionStatus::ReportRejected;
    PlaytestReportStatus reportStatus = PlaytestReportStatus::InvalidPreparedReport;
    std::string reportId;
    // Frozen consented envelope, WITHOUT the one-time anti-spam token. Retain
    // only in memory for explicit retries; never persist submission consent.
    std::string json;
    [[nodiscard]] bool IsReady() const noexcept { return status == PlaytestSubmissionStatus::Ready; }
};

// PNG framing, CRC, dimensions and metadata allowlist only, not pixel decoding.
// Platform encoders/decoders remain responsible for valid deflate/pixel content.
// No metadata chunks (text, EXIF, ICC or locations) or trailing bytes accepted.
[[nodiscard]] bool ValidatePlaytestScreenshot(const PlaytestReportScreenshot& screenshot) noexcept;
[[nodiscard]] PreparedPlaytestSubmission PreparePlaytestSubmission(
    const PlaytestReportInput& input, bool includeScreenshot = false,
    const PlaytestReportScreenshot& screenshot = {});

// One-time token exists only in this bounded attempt. It is not appended to the
// frozen bytes, exported JSON, diagnostics, or report identity. Caller must use
// HTTPS with no redirects, bounded response/timeouts and cancellation.
[[nodiscard]] std::string BuildPlaytestSubmissionRequest(
    const PreparedPlaytestSubmission& submission, std::string_view turnstileToken);
} // namespace horde::reporting
