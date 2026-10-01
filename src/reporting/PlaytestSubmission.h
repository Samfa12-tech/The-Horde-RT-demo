#pragma once

#include "reporting/PlaytestReport.h"

#include <span>

namespace horde::reporting
{
inline constexpr std::size_t kPlaytestSubmissionMaxBytes = 768u * 1024u;
inline constexpr std::size_t kPlaytestScreenshotMaxBytes = 512u * 1024u;
inline constexpr std::uint32_t kPlaytestScreenshotMaxLongEdge = 1280u;
inline constexpr std::uint32_t kPlaytestScreenshotMaxShortEdge = 720u;

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
