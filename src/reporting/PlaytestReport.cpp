#include "reporting/PlaytestReport.h"

#include <algorithm>
#include <charconv>
#include <cmath>
#include <limits>
#include <utility>

namespace horde::reporting
{
namespace
{

enum class TextStatus : std::uint8_t { Ok, Empty, TooLarge, InvalidUtf8, Control, Sensitive };

bool IsContinuation(const unsigned char value) noexcept { return (value & 0xc0u) == 0x80u; }

bool ValidUtf8(const std::string_view text) noexcept
{
    std::size_t i = 0u;
    while (i < text.size())
    {
        const auto first = static_cast<unsigned char>(text[i]);
        if (first <= 0x7fu) { ++i; continue; }
        std::size_t count = 0u;
        std::uint32_t codepoint = 0u;
        if (first >= 0xc2u && first <= 0xdfu) { count = 1u; codepoint = first & 0x1fu; }
        else if (first >= 0xe0u && first <= 0xefu) { count = 2u; codepoint = first & 0x0fu; }
        else if (first >= 0xf0u && first <= 0xf4u) { count = 3u; codepoint = first & 0x07u; }
        else return false;
        if (i + count >= text.size()) return false;
        for (std::size_t n = 1u; n <= count; ++n)
        {
            const auto next = static_cast<unsigned char>(text[i + n]);
            if (!IsContinuation(next)) return false;
            codepoint = (codepoint << 6u) | (next & 0x3fu);
        }
        if ((count == 2u && codepoint < 0x800u) ||
            (count == 3u && codepoint < 0x10000u) ||
            (codepoint >= 0xd800u && codepoint <= 0xdfffu) || codepoint > 0x10ffffu)
        {
            return false;
        }
        i += count + 1u;
    }
    return true;
}

bool ContainsControl(const std::string_view text) noexcept
{
    for (std::size_t i = 0u; i < text.size();)
    {
        const auto first = static_cast<unsigned char>(text[i]);
        if (first <= 0x7fu)
        {
            if (first < 0x20u || first == 0x7fu) return true;
            ++i;
            continue;
        }
        std::size_t count = first <= 0xdfu ? 1u : first <= 0xefu ? 2u : 3u;
        std::uint32_t codepoint = first & (count == 1u ? 0x1fu : count == 2u ? 0x0fu : 0x07u);
        for (std::size_t n = 1u; n <= count; ++n)
            codepoint = (codepoint << 6u) | (static_cast<unsigned char>(text[i + n]) & 0x3fu);
        if (codepoint >= 0x80u && codepoint <= 0x9fu) return true;
        i += count + 1u;
    }
    return false;
}

bool IsWhitespaceOnly(const std::string_view text) noexcept
{
    return std::all_of(text.begin(), text.end(), [](const char value) {
        return value == ' ' || value == '\t' || value == '\r' || value == '\n' || value == '\v' || value == '\f';
    });
}

char AsciiLower(const char value) noexcept
{
    return value >= 'A' && value <= 'Z' ? static_cast<char>(value + ('a' - 'A')) : value;
}

std::string LowerAscii(const std::string_view text)
{
    std::string result;
    result.reserve(text.size());
    for (const char value : text) result.push_back(AsciiLower(value));
    return result;
}

bool ContainsSensitivePattern(const std::string_view text)
{
    const std::string lower = LowerAscii(text);
    if (lower.find("://") != std::string::npos || lower.find("www.") != std::string::npos ||
        lower.find("mailto:") != std::string::npos || lower.find("@") != std::string::npos ||
        lower.find("\\\\") != std::string::npos ||
        lower.find("/users/") != std::string::npos || lower.find("/home/") != std::string::npos ||
        lower.find("/storage/emulated/") != std::string::npos || lower.find("/private/") != std::string::npos)
    {
        return true;
    }
    if (lower.size() >= 3u &&
        ((lower[0] >= 'a' && lower[0] <= 'z' && lower[1] == ':' &&
          (lower[2] == '\\' || lower[2] == '/')) ||
         (lower[0] == '/' && lower[1] == '/')))
    {
        return true;
    }
    for (std::size_t i = 0u; i + 2u < lower.size(); ++i)
    {
        const bool driveLetter = lower[i] >= 'a' && lower[i] <= 'z';
        const bool pathSeparator = lower[i + 2u] == '\\' || lower[i + 2u] == '/';
        const bool tokenBoundary = i == 0u || !((lower[i - 1u] >= 'a' && lower[i - 1u] <= 'z') ||
                                                (lower[i - 1u] >= '0' && lower[i - 1u] <= '9') ||
                                                lower[i - 1u] == '_');
        if (driveLetter && lower[i + 1u] == ':' && pathSeparator && tokenBoundary) return true;
    }

    constexpr std::string_view keys[] = {
        "token", "secret", "password", "authorization", "cookie", "api_key", "apikey", "credential", "bearer",
    };
    for (const std::string_view key : keys)
    {
        std::size_t at = lower.find(key);
        while (at != std::string::npos)
        {
            std::size_t next = at + key.size();
            while (next < lower.size() && (lower[next] == ' ' || lower[next] == '\t')) ++next;
            if (next < lower.size() && (lower[next] == '=' || lower[next] == ':')) return true;
            if (key == "bearer" && next < lower.size() && lower[next] != ',' && lower[next] != '.') return true;
            at = lower.find(key, at + 1u);
        }
    }

    // Reject likely phone numbers while avoiding long hexadecimal build IDs.
    for (std::size_t begin = 0u; begin < text.size(); ++begin)
    {
        if (!(text[begin] == '+' || text[begin] == '(' || (text[begin] >= '0' && text[begin] <= '9'))) continue;
        std::size_t digits = 0u;
        bool phoneMarker = text[begin] == '+' || text[begin] == '(';
        std::size_t end = begin;
        for (; end < text.size(); ++end)
        {
            const char ch = text[end];
            if (ch >= '0' && ch <= '9') { ++digits; continue; }
            if (ch == '+' || ch == '(' || ch == ')' || ch == '-' || ch == '.' || ch == ' ') { phoneMarker = true; continue; }
            break;
        }
        if ((digits >= 10u || (phoneMarker && digits >= 7u)) &&
            (begin == 0u || !((text[begin - 1u] >= 'A' && text[begin - 1u] <= 'Z') ||
                              (text[begin - 1u] >= 'a' && text[begin - 1u] <= 'z'))))
        {
            return true;
        }
        begin = std::max(begin, end == 0u ? begin : end - 1u);
    }
    return false;
}

TextStatus ValidateText(const std::string_view text, const std::size_t maximumBytes,
                        const bool mayBeEmpty = false)
{
    if (text.empty() && !mayBeEmpty) return TextStatus::Empty;
    if (!mayBeEmpty && IsWhitespaceOnly(text)) return TextStatus::Empty;
    if (text.size() > maximumBytes) return TextStatus::TooLarge;
    if (!ValidUtf8(text)) return TextStatus::InvalidUtf8;
    if (ContainsControl(text)) return TextStatus::Control;
    if (ContainsSensitivePattern(text)) return TextStatus::Sensitive;
    return TextStatus::Ok;
}

bool ValidReportId(const std::string_view id) noexcept
{
    if (id.size() < 8u || id.size() > 96u) return false;
    return std::all_of(id.begin(), id.end(), [](const char value) {
        return (value >= 'A' && value <= 'Z') || (value >= 'a' && value <= 'z') ||
               (value >= '0' && value <= '9') || value == '_' || value == '-';
    });
}

bool Digits(const std::string_view text, const std::size_t begin, const std::size_t count) noexcept
{
    if (begin + count > text.size()) return false;
    for (std::size_t i = begin; i < begin + count; ++i)
        if (text[i] < '0' || text[i] > '9') return false;
    return true;
}

bool ValidUtcTimestamp(const std::string_view value) noexcept
{
    if (value.size() < 20u || value[4] != '-' || value[7] != '-' || value[10] != 'T' ||
        value[13] != ':' || value[16] != ':') return false;
    if (!Digits(value, 0u, 4u) || !Digits(value, 5u, 2u) || !Digits(value, 8u, 2u) ||
        !Digits(value, 11u, 2u) || !Digits(value, 14u, 2u) || !Digits(value, 17u, 2u)) return false;
    const auto number = [&value](const std::size_t start, const std::size_t length) {
        int result = 0;
        for (std::size_t i = start; i < start + length; ++i) result = result * 10 + (value[i] - '0');
        return result;
    };
    const int year = number(0u, 4u), month = number(5u, 2u), day = number(8u, 2u);
    const int hour = number(11u, 2u), minute = number(14u, 2u), second = number(17u, 2u);
    if (year == 0 || month < 1 || month > 12 || hour > 23 || minute > 59 || second > 59) return false;
    constexpr int daysPerMonth[] = {31, 28, 31, 30, 31, 30, 31, 31, 30, 31, 30, 31};
    const bool leap = year % 4 == 0 && (year % 100 != 0 || year % 400 == 0);
    const int maxDay = daysPerMonth[month - 1] + (month == 2 && leap ? 1 : 0);
    if (day < 1 || day > maxDay) return false;
    if (value.size() == 20u) return value[19] == 'Z';
    if (value[19] != '.') return false;
    const std::size_t fractionDigits = value.size() - 21u;
    if (fractionDigits < 1u || fractionDigits > 9u || value.back() != 'Z') return false;
    return Digits(value, 20u, fractionDigits);
}

bool IsValidCategory(const PlaytestReportCategory value) noexcept
{
    return value >= PlaytestReportCategory::Gameplay && value <= PlaytestReportCategory::Other;
}

bool IsValidImpact(const PlaytestReportImpact value) noexcept
{
    return value >= PlaytestReportImpact::BlocksProgress && value <= PlaytestReportImpact::Polish;
}

std::string_view CategoryName(const PlaytestReportCategory value) noexcept
{
    switch (value)
    {
    case PlaytestReportCategory::Gameplay: return "gameplay";
    case PlaytestReportCategory::Visuals: return "visuals";
    case PlaytestReportCategory::Performance: return "performance";
    case PlaytestReportCategory::Controls: return "controls";
    case PlaytestReportCategory::Audio: return "audio";
    case PlaytestReportCategory::Other: return "other";
    }
    return {};
}

std::string_view ImpactName(const PlaytestReportImpact value) noexcept
{
    switch (value)
    {
    case PlaytestReportImpact::BlocksProgress: return "blocks-progress";
    case PlaytestReportImpact::MajorFriction: return "major-friction";
    case PlaytestReportImpact::MinorFriction: return "minor-friction";
    case PlaytestReportImpact::Polish: return "polish";
    }
    return {};
}

void AppendJsonString(std::string& output, const std::string_view value)
{
    output.push_back('"');
    for (const char ch : value)
    {
        switch (ch)
        {
        case '"': output += "\\\""; break;
        case '\\': output += "\\\\"; break;
        default: output.push_back(ch); break;
        }
    }
    output.push_back('"');
}

void AppendNumber(std::string& output, const double value)
{
    // Use a stable short decimal representation; values were checked finite.
    char buffer[64]{};
    const auto result = std::to_chars(buffer, buffer + sizeof(buffer), value,
                                      std::chars_format::general, 8);
    if (result.ec == std::errc{}) output.append(buffer, result.ptr);
}

PlaytestReportStatus ToReportStatus(const TextStatus status) noexcept
{
    switch (status)
    {
    case TextStatus::Ok: return PlaytestReportStatus::Ready;
    case TextStatus::Empty: return PlaytestReportStatus::EmptyNote;
    case TextStatus::TooLarge: return PlaytestReportStatus::NoteTooLarge;
    case TextStatus::InvalidUtf8: return PlaytestReportStatus::InvalidUtf8;
    case TextStatus::Control: return PlaytestReportStatus::ControlCharacter;
    case TextStatus::Sensitive: return PlaytestReportStatus::SensitiveContent;
    }
    return PlaytestReportStatus::InvalidContext;
}

} // namespace

PreparedPlaytestReport PreparePlaytestReport(const PlaytestReportInput& input)
{
    PreparedPlaytestReport result;
    if (!input.consentToSubmit) { result.status = PlaytestReportStatus::ConsentRequired; return result; }
    if (!ValidReportId(input.reportId)) { result.status = PlaytestReportStatus::InvalidReportId; return result; }
    if (!ValidUtcTimestamp(input.capturedAtUtc)) { result.status = PlaytestReportStatus::InvalidTimestamp; return result; }
    if (!IsValidCategory(input.category)) { result.status = PlaytestReportStatus::InvalidCategory; return result; }
    if (!IsValidImpact(input.impact)) { result.status = PlaytestReportStatus::InvalidImpact; return result; }
    const auto noteStatus = ValidateText(input.note, kPlaytestReportMaxNoteBytes);
    if (noteStatus != TextStatus::Ok) { result.status = ToReportStatus(noteStatus); return result; }

    if (input.includeBasicContext)
    {
        const std::string_view fields[] = {input.context.product, input.context.version, input.context.build,
            input.context.platform, input.context.rawModel, input.context.gpu, input.context.backend,
            input.context.quality};
        for (const auto field : fields)
        {
            if (ValidateText(field, kPlaytestReportMaxContextStringBytes) != TextStatus::Ok)
            {
                result.status = PlaytestReportStatus::InvalidContext;
                return result;
            }
        }
        if (!std::isfinite(input.context.renderScale) || input.context.renderScale < 0.0 ||
            input.context.renderScale > 2.0 || input.context.internalWidth == 0u ||
            input.context.internalHeight == 0u || input.context.internalWidth > 16384u ||
            input.context.internalHeight > 16384u)
        {
            result.status = PlaytestReportStatus::InvalidContext;
            return result;
        }
    }

    std::string json;
    json.reserve(1024u + input.note.size());
    json += "{\"schemaVersion\":1,\"reportId\":";
    AppendJsonString(json, input.reportId);
    json += ",\"capturedAtUtc\":";
    AppendJsonString(json, input.capturedAtUtc);
    json += ",\"category\":";
    AppendJsonString(json, CategoryName(input.category));
    json += ",\"impact\":";
    AppendJsonString(json, ImpactName(input.impact));
    json += ",\"note\":";
    AppendJsonString(json, input.note);
    if (input.includeBasicContext)
    {
        json += ",\"context\":{\"product\":"; AppendJsonString(json, input.context.product);
        json += ",\"version\":"; AppendJsonString(json, input.context.version);
        json += ",\"build\":"; AppendJsonString(json, input.context.build);
        json += ",\"platform\":"; AppendJsonString(json, input.context.platform);
        json += ",\"rawModel\":"; AppendJsonString(json, input.context.rawModel);
        json += ",\"gpu\":"; AppendJsonString(json, input.context.gpu);
        json += ",\"backend\":"; AppendJsonString(json, input.context.backend);
        json += ",\"quality\":"; AppendJsonString(json, input.context.quality);
        json += ",\"renderScale\":"; AppendNumber(json, input.context.renderScale);
        json += ",\"internalExtent\":[" + std::to_string(input.context.internalWidth) + "," +
                std::to_string(input.context.internalHeight) + "],\"rtPresented\":";
        json += input.context.rtPresented ? "true}" : "false}";
    }
    json.push_back('}');
    if (json.size() > kPlaytestReportMaxJsonBytes)
    {
        result.status = PlaytestReportStatus::JsonTooLarge;
        return result;
    }
    result.status = PlaytestReportStatus::Ready;
    result.reportId.assign(input.reportId);
    result.json = std::move(json);
    return result;
}

std::string_view PlaytestReportStatusName(const PlaytestReportStatus status) noexcept
{
    switch (status)
    {
    case PlaytestReportStatus::Ready: return "ready";
    case PlaytestReportStatus::ConsentRequired: return "explicit submission/export consent is required";
    case PlaytestReportStatus::InvalidReportId: return "report ID must be opaque URL-safe text";
    case PlaytestReportStatus::InvalidTimestamp: return "capture time must be a valid UTC timestamp";
    case PlaytestReportStatus::InvalidCategory: return "report category is invalid";
    case PlaytestReportStatus::InvalidImpact: return "report impact is invalid";
    case PlaytestReportStatus::EmptyNote: return "player note is required";
    case PlaytestReportStatus::NoteTooLarge: return "player note exceeds the byte limit";
    case PlaytestReportStatus::InvalidUtf8: return "text is not valid UTF-8";
    case PlaytestReportStatus::ControlCharacter: return "text contains a control character";
    case PlaytestReportStatus::SensitiveContent: return "text contains content that should not be shared";
    case PlaytestReportStatus::InvalidContext: return "opted-in basic context is invalid";
    case PlaytestReportStatus::JsonTooLarge: return "prepared report exceeds the JSON byte limit";
    case PlaytestReportStatus::InvalidPreparedReport: return "prepared report is invalid";
    }
    return "unknown report status";
}

bool PlaytestReportDelivery::Begin(const PreparedPlaytestReport& report,
                                   PlaytestReportAttempt& attempt)
{
    if (!report.IsReady() || !ValidReportId(report.reportId) || report.json.empty() ||
        report.json.size() > kPlaytestReportMaxJsonBytes ||
        state_ == PlaytestReportDeliveryState::InFlight) return false;
    reportId_ = report.reportId;
    json_ = report.json;
    if (!MakeAttempt(attempt)) return false;
    state_ = PlaytestReportDeliveryState::InFlight;
    return true;
}

bool PlaytestReportDelivery::Retry(PlaytestReportAttempt& attempt)
{
    if (state_ != PlaytestReportDeliveryState::RetryableFailure || !MakeAttempt(attempt)) return false;
    state_ = PlaytestReportDeliveryState::InFlight;
    return true;
}

bool PlaytestReportDelivery::MakeAttempt(PlaytestReportAttempt& attempt)
{
    if (token_ == std::numeric_limits<std::uint64_t>::max()) return false;
    ++token_;
    attempt = {token_, reportId_, json_};
    return true;
}

bool PlaytestReportDelivery::Complete(const std::uint64_t token,
                                     const PlaytestReportDeliveryResult result) noexcept
{
    if (state_ != PlaytestReportDeliveryState::InFlight || token != token_) return false;
    switch (result)
    {
    case PlaytestReportDeliveryResult::Accepted: state_ = PlaytestReportDeliveryState::Accepted; return true;
    case PlaytestReportDeliveryResult::RetryableFailure: state_ = PlaytestReportDeliveryState::RetryableFailure; return true;
    case PlaytestReportDeliveryResult::PermanentFailure: state_ = PlaytestReportDeliveryState::Failed; return true;
    }
    return false;
}

void PlaytestReportDelivery::Cancel() noexcept
{
    if (token_ != std::numeric_limits<std::uint64_t>::max()) ++token_;
    state_ = PlaytestReportDeliveryState::Cancelled;
    reportId_.clear();
    json_.clear();
}

} // namespace horde::reporting
