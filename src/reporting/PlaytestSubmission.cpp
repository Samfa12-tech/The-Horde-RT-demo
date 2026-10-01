#include "reporting/PlaytestSubmission.h"

#include <algorithm>
#include <array>

namespace horde::reporting
{
namespace
{
std::uint32_t ReadBigEndian(const std::span<const std::uint8_t> bytes, const std::size_t at) noexcept
{
    return (static_cast<std::uint32_t>(bytes[at]) << 24u) |
        (static_cast<std::uint32_t>(bytes[at + 1u]) << 16u) |
        (static_cast<std::uint32_t>(bytes[at + 2u]) << 8u) | bytes[at + 3u];
}

std::uint32_t Crc32(const std::span<const std::uint8_t> bytes) noexcept
{
    std::uint32_t crc = 0xffffffffu;
    for (const auto byte : bytes)
    {
        crc ^= byte;
        for (unsigned bit = 0u; bit < 8u; ++bit)
            crc = (crc >> 1u) ^ ((crc & 1u) != 0u ? 0xedb88320u : 0u);
    }
    return crc ^ 0xffffffffu;
}

std::string Base64(const std::span<const std::uint8_t> bytes)
{
    constexpr std::string_view alphabet = "ABCDEFGHIJKLMNOPQRSTUVWXYZabcdefghijklmnopqrstuvwxyz0123456789+/";
    std::string output;
    output.reserve(((bytes.size() + 2u) / 3u) * 4u);
    for (std::size_t at = 0u; at < bytes.size(); at += 3u)
    {
        const auto left = bytes.size() - at;
        const std::uint32_t word = (static_cast<std::uint32_t>(bytes[at]) << 16u) |
            (left > 1u ? static_cast<std::uint32_t>(bytes[at + 1u]) << 8u : 0u) |
            (left > 2u ? bytes[at + 2u] : 0u);
        output += alphabet[(word >> 18u) & 63u]; output += alphabet[(word >> 12u) & 63u];
        output += left > 1u ? alphabet[(word >> 6u) & 63u] : '=';
        output += left > 2u ? alphabet[word & 63u] : '=';
    }
    return output;
}
} // namespace

bool ValidatePlaytestScreenshot(const PlaytestReportScreenshot& screenshot) noexcept
{
    const auto bytes = screenshot.png;
    constexpr std::array<std::uint8_t, 8u> signature{137u, 80u, 78u, 71u, 13u, 10u, 26u, 10u};
    if (bytes.size() < 57u || bytes.size() > kPlaytestScreenshotMaxBytes ||
        screenshot.width == 0u || screenshot.height == 0u ||
        std::max(screenshot.width, screenshot.height) > kPlaytestScreenshotMaxLongEdge ||
        std::min(screenshot.width, screenshot.height) > kPlaytestScreenshotMaxShortEdge ||
        !std::equal(signature.begin(), signature.end(), bytes.begin())) return false;
    bool header = false, data = false;
    std::size_t at = signature.size();
    while (at < bytes.size())
    {
        if (bytes.size() - at < 12u) return false;
        const auto length = ReadBigEndian(bytes, at);
        if (length > bytes.size() - at - 12u) return false;
        const auto type = ReadBigEndian(bytes, at + 4u);
        if (Crc32(bytes.subspan(at + 4u, static_cast<std::size_t>(length) + 4u)) !=
            ReadBigEndian(bytes, at + 8u + length)) return false;
        if (!header)
        {
            if (type != 0x49484452u || length != 13u ||
                ReadBigEndian(bytes, at + 8u) != screenshot.width ||
                ReadBigEndian(bytes, at + 12u) != screenshot.height ||
                bytes[at + 16u] != 8u || (bytes[at + 17u] != 6u && bytes[at + 17u] != 2u) ||
                bytes[at + 18u] != 0u || bytes[at + 19u] != 0u || bytes[at + 20u] != 0u) return false;
            header = true;
        }
        else if (type == 0x49444154u) { if (length == 0u) return false; data = true; }
        else if (type == 0x49454e44u) return data && length == 0u && at + 12u == bytes.size();
        else return false;
        at += static_cast<std::size_t>(length) + 12u;
    }
    return false;
}

PreparedPlaytestSubmission PreparePlaytestSubmission(const PlaytestReportInput& input,
    const bool includeScreenshot, const PlaytestReportScreenshot& screenshot)
{
    PreparedPlaytestSubmission result;
    const auto report = PreparePlaytestReport(input);
    result.reportStatus = report.status;
    if (!report.IsReady()) return result;
    if (!includeScreenshot && (!screenshot.png.empty() || screenshot.width != 0u || screenshot.height != 0u))
    { result.status = PlaytestSubmissionStatus::ScreenshotNotConsented; return result; }
    if (includeScreenshot && !ValidatePlaytestScreenshot(screenshot))
    { result.status = PlaytestSubmissionStatus::InvalidScreenshot; return result; }
    result.json = "{\"schemaVersion\":1,\"product\":\"horde-lantern-rt\",\"report\":" + report.json +
        ",\"consentToSubmit\":true,\"includeDiagnostics\":" + (input.includeBasicContext ? "true" : "false") +
        ",\"includeScreenshot\":" + (includeScreenshot ? "true" : "false");
    if (includeScreenshot)
        result.json += ",\"screenshot\":{\"dataUrl\":\"data:image/png;base64," + Base64(screenshot.png) +
            "\",\"width\":" + std::to_string(screenshot.width) + ",\"height\":" +
            std::to_string(screenshot.height) + "}";
    result.json += '}';
    // Reserve for the maximum JSON-escaped token (2048 printable ASCII bytes).
    if (result.json.size() + 2u * 2048u + 32u > kPlaytestSubmissionMaxBytes)
    { result.json.clear(); result.status = PlaytestSubmissionStatus::TooLarge; return result; }
    result.reportId = report.reportId;
    result.status = PlaytestSubmissionStatus::Ready;
    return result;
}

std::string BuildPlaytestSubmissionRequest(const PreparedPlaytestSubmission& submission,
    const std::string_view turnstileToken)
{
    if (!submission.IsReady() || submission.json.size() < 2u || submission.json.front() != '{' ||
        submission.json.back() != '}' || submission.json.size() > kPlaytestSubmissionMaxBytes ||
        turnstileToken.empty() || turnstileToken.size() > 2048u) return {};
    std::string token;
    token.reserve(turnstileToken.size() + 2u);
    for (const unsigned char byte : turnstileToken)
    {
        if (byte < 0x20u || byte > 0x7eu) return {};
        if (byte == '"' || byte == '\\') token += '\\';
        token += static_cast<char>(byte);
    }
    if (submission.json.size() + token.size() + 20u > kPlaytestSubmissionMaxBytes) return {};
    return submission.json.substr(0u, submission.json.size() - 1u) + ",\"turnstileToken\":\"" + token + "\"}";
}
} // namespace horde::reporting
