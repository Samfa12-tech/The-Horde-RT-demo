#include "update/GitHubReleaseUpdater.h"

#include <algorithm>
#include <charconv>
#include <cctype>
#include <limits>
#include <map>
#include <utility>

namespace horde::update
{
namespace
{
constexpr std::string_view kReleaseApiUrl =
    "https://api.github.com/repos/Samfa12-tech/The-Horde-RT-demo/releases?per_page=10";
constexpr std::string_view kReleasePagePrefix =
    "https://github.com/Samfa12-tech/The-Horde-RT-demo/releases/tag/";
constexpr std::size_t kMaximumResponseBytes = 256u * 1024u;
constexpr std::size_t kMaximumReleaseNotesBytes = 4096u;
constexpr std::size_t kMaximumJsonDepth = 32u;
constexpr std::size_t kMaximumJsonNodes = 12000u;

bool IsIdentifierCharacter(char value)
{
    const unsigned char byte = static_cast<unsigned char>(value);
    return std::isalnum(byte) != 0 || value == '-';
}

bool ParseCoreNumber(std::string_view text, std::uint32_t& value)
{
    if (text.empty() || (text.size() > 1u && text.front() == '0')) return false;
    std::uint64_t parsed = 0;
    const auto result = std::from_chars(text.data(), text.data() + text.size(), parsed);
    if (result.ec != std::errc{} || result.ptr != text.data() + text.size() ||
        parsed > std::numeric_limits<std::uint32_t>::max())
    {
        return false;
    }
    value = static_cast<std::uint32_t>(parsed);
    return true;
}

bool ParsePrerelease(std::string_view text, std::vector<SemanticVersionIdentifier>& identifiers)
{
    std::size_t cursor = 0;
    while (cursor <= text.size())
    {
        const std::size_t separator = text.find('.', cursor);
        const std::size_t end = separator == std::string_view::npos ? text.size() : separator;
        const std::string_view identifier = text.substr(cursor, end - cursor);
        if (identifier.empty() ||
            !std::all_of(identifier.begin(), identifier.end(), IsIdentifierCharacter))
        {
            return false;
        }

        SemanticVersionIdentifier parsed;
        parsed.text.assign(identifier);
        parsed.numeric = std::all_of(identifier.begin(), identifier.end(), [](char value) {
            return value >= '0' && value <= '9';
        });
        if (parsed.numeric)
        {
            if (identifier.size() > 1u && identifier.front() == '0') return false;
            const auto result = std::from_chars(
                identifier.data(), identifier.data() + identifier.size(), parsed.numericValue);
            if (result.ec != std::errc{} || result.ptr != identifier.data() + identifier.size()) return false;
        }
        identifiers.push_back(std::move(parsed));
        if (separator == std::string_view::npos) break;
        cursor = separator + 1u;
    }
    return !identifiers.empty();
}

bool ValidateBuildMetadata(std::string_view text)
{
    if (text.empty()) return false;
    std::size_t cursor = 0;
    while (cursor <= text.size())
    {
        const std::size_t separator = text.find('.', cursor);
        const std::size_t end = separator == std::string_view::npos ? text.size() : separator;
        const std::string_view identifier = text.substr(cursor, end - cursor);
        if (identifier.empty() ||
            !std::all_of(identifier.begin(), identifier.end(), IsIdentifierCharacter))
        {
            return false;
        }
        if (separator == std::string_view::npos) break;
        cursor = separator + 1u;
    }
    return true;
}

enum class JsonType { Null, Boolean, Number, String, Array, Object };

struct JsonValue
{
    JsonType type = JsonType::Null;
    bool boolean = false;
    std::string text;
    std::vector<JsonValue> array;
    std::map<std::string, JsonValue, std::less<>> object;

    const JsonValue* Find(std::string_view key) const
    {
        const auto found = object.find(key);
        return found == object.end() ? nullptr : &found->second;
    }
};

class JsonParser
{
public:
    explicit JsonParser(std::string_view source)
        : cursor_(source.data()), end_(source.data() + source.size())
    {
    }

    bool Parse(JsonValue& value)
    {
        SkipWhitespace();
        if (!ParseValue(value, 0u)) return false;
        SkipWhitespace();
        return cursor_ == end_;
    }

private:
    bool CountNode()
    {
        ++nodes_;
        return nodes_ <= kMaximumJsonNodes;
    }

    void SkipWhitespace()
    {
        while (cursor_ != end_ &&
               (*cursor_ == ' ' || *cursor_ == '\t' || *cursor_ == '\r' || *cursor_ == '\n'))
        {
            ++cursor_;
        }
    }

    bool ParseValue(JsonValue& value, std::size_t depth)
    {
        if (depth > kMaximumJsonDepth || cursor_ == end_ || !CountNode()) return false;
        switch (*cursor_)
        {
        case 'n': return ParseLiteral("null", JsonType::Null, value);
        case 't': value.boolean = true; return ParseLiteral("true", JsonType::Boolean, value);
        case 'f': value.boolean = false; return ParseLiteral("false", JsonType::Boolean, value);
        case '"': value.type = JsonType::String; return ParseString(value.text);
        case '[': return ParseArray(value, depth + 1u);
        case '{': return ParseObject(value, depth + 1u);
        default: return ParseNumber(value);
        }
    }

    bool ParseLiteral(std::string_view literal, JsonType type, JsonValue& value)
    {
        if (static_cast<std::size_t>(end_ - cursor_) < literal.size() ||
            !std::equal(literal.begin(), literal.end(), cursor_))
        {
            return false;
        }
        cursor_ += literal.size();
        value.type = type;
        return true;
    }

    static void AppendUtf8(std::string& output, std::uint32_t codePoint)
    {
        if (codePoint <= 0x7fu) output.push_back(static_cast<char>(codePoint));
        else if (codePoint <= 0x7ffu)
        {
            output.push_back(static_cast<char>(0xc0u | (codePoint >> 6u)));
            output.push_back(static_cast<char>(0x80u | (codePoint & 0x3fu)));
        }
        else if (codePoint <= 0xffffu)
        {
            output.push_back(static_cast<char>(0xe0u | (codePoint >> 12u)));
            output.push_back(static_cast<char>(0x80u | ((codePoint >> 6u) & 0x3fu)));
            output.push_back(static_cast<char>(0x80u | (codePoint & 0x3fu)));
        }
        else
        {
            output.push_back(static_cast<char>(0xf0u | (codePoint >> 18u)));
            output.push_back(static_cast<char>(0x80u | ((codePoint >> 12u) & 0x3fu)));
            output.push_back(static_cast<char>(0x80u | ((codePoint >> 6u) & 0x3fu)));
            output.push_back(static_cast<char>(0x80u | (codePoint & 0x3fu)));
        }
    }

    bool ParseHexQuad(std::uint32_t& value)
    {
        if (static_cast<std::size_t>(end_ - cursor_) < 4u) return false;
        value = 0;
        for (int index = 0; index < 4; ++index)
        {
            const char digit = *cursor_++;
            value <<= 4u;
            if (digit >= '0' && digit <= '9') value |= static_cast<std::uint32_t>(digit - '0');
            else if (digit >= 'a' && digit <= 'f') value |= static_cast<std::uint32_t>(digit - 'a' + 10);
            else if (digit >= 'A' && digit <= 'F') value |= static_cast<std::uint32_t>(digit - 'A' + 10);
            else return false;
        }
        return true;
    }

    bool ParseString(std::string& output)
    {
        if (cursor_ == end_ || *cursor_++ != '"') return false;
        output.clear();
        while (cursor_ != end_)
        {
            const unsigned char value = static_cast<unsigned char>(*cursor_++);
            if (value == '"') return true;
            if (value < 0x20u) return false;
            if (value != '\\')
            {
                output.push_back(static_cast<char>(value));
                continue;
            }
            if (cursor_ == end_) return false;
            switch (*cursor_++)
            {
            case '"': output.push_back('"'); break;
            case '\\': output.push_back('\\'); break;
            case '/': output.push_back('/'); break;
            case 'b': output.push_back('\b'); break;
            case 'f': output.push_back('\f'); break;
            case 'n': output.push_back('\n'); break;
            case 'r': output.push_back('\r'); break;
            case 't': output.push_back('\t'); break;
            case 'u':
            {
                std::uint32_t codePoint = 0;
                if (!ParseHexQuad(codePoint)) return false;
                if (codePoint >= 0xd800u && codePoint <= 0xdbffu)
                {
                    if (static_cast<std::size_t>(end_ - cursor_) < 6u || cursor_[0] != '\\' || cursor_[1] != 'u') return false;
                    cursor_ += 2;
                    std::uint32_t low = 0;
                    if (!ParseHexQuad(low) || low < 0xdc00u || low > 0xdfffu) return false;
                    codePoint = 0x10000u + ((codePoint - 0xd800u) << 10u) + (low - 0xdc00u);
                }
                else if (codePoint >= 0xdc00u && codePoint <= 0xdfffu) return false;
                AppendUtf8(output, codePoint);
                break;
            }
            default: return false;
            }
        }
        return false;
    }

    bool ParseNumber(JsonValue& value)
    {
        const char* start = cursor_;
        if (cursor_ != end_ && *cursor_ == '-') ++cursor_;
        if (cursor_ == end_) return false;
        if (*cursor_ == '0') ++cursor_;
        else
        {
            if (*cursor_ < '1' || *cursor_ > '9') return false;
            while (cursor_ != end_ && *cursor_ >= '0' && *cursor_ <= '9') ++cursor_;
        }
        if (cursor_ != end_ && *cursor_ == '.')
        {
            ++cursor_;
            if (cursor_ == end_ || *cursor_ < '0' || *cursor_ > '9') return false;
            while (cursor_ != end_ && *cursor_ >= '0' && *cursor_ <= '9') ++cursor_;
        }
        if (cursor_ != end_ && (*cursor_ == 'e' || *cursor_ == 'E'))
        {
            ++cursor_;
            if (cursor_ != end_ && (*cursor_ == '+' || *cursor_ == '-')) ++cursor_;
            if (cursor_ == end_ || *cursor_ < '0' || *cursor_ > '9') return false;
            while (cursor_ != end_ && *cursor_ >= '0' && *cursor_ <= '9') ++cursor_;
        }
        value.type = JsonType::Number;
        value.text.assign(start, cursor_);
        return true;
    }

    bool ParseArray(JsonValue& value, std::size_t depth)
    {
        ++cursor_;
        value.type = JsonType::Array;
        SkipWhitespace();
        if (cursor_ != end_ && *cursor_ == ']') { ++cursor_; return true; }
        while (cursor_ != end_)
        {
            JsonValue item;
            if (!ParseValue(item, depth)) return false;
            value.array.push_back(std::move(item));
            SkipWhitespace();
            if (cursor_ != end_ && *cursor_ == ']') { ++cursor_; return true; }
            if (cursor_ == end_ || *cursor_++ != ',') return false;
            SkipWhitespace();
        }
        return false;
    }

    bool ParseObject(JsonValue& value, std::size_t depth)
    {
        ++cursor_;
        value.type = JsonType::Object;
        SkipWhitespace();
        if (cursor_ != end_ && *cursor_ == '}') { ++cursor_; return true; }
        while (cursor_ != end_)
        {
            std::string key;
            if (!ParseString(key)) return false;
            SkipWhitespace();
            if (cursor_ == end_ || *cursor_++ != ':') return false;
            SkipWhitespace();
            JsonValue child;
            if (!ParseValue(child, depth)) return false;
            if (!value.object.emplace(std::move(key), std::move(child)).second) return false;
            SkipWhitespace();
            if (cursor_ != end_ && *cursor_ == '}') { ++cursor_; return true; }
            if (cursor_ == end_ || *cursor_++ != ',') return false;
            SkipWhitespace();
        }
        return false;
    }

    const char* cursor_ = nullptr;
    const char* end_ = nullptr;
    std::size_t nodes_ = 0;
};

const std::string* StringMember(const JsonValue& object, std::string_view key)
{
    const JsonValue* value = object.Find(key);
    return value != nullptr && value->type == JsonType::String ? &value->text : nullptr;
}

std::optional<bool> BooleanMember(const JsonValue& object, std::string_view key)
{
    const JsonValue* value = object.Find(key);
    if (value == nullptr || value->type != JsonType::Boolean) return std::nullopt;
    return value->boolean;
}

std::string OptionalStringMember(const JsonValue& object, std::string_view key)
{
    const JsonValue* value = object.Find(key);
    if (value == nullptr || value->type == JsonType::Null) return {};
    return value->type == JsonType::String ? value->text : std::string{};
}

bool IsTrustedReleasePage(std::string_view page, std::string_view tag)
{
    return page.size() == kReleasePagePrefix.size() + tag.size() &&
           page.starts_with(kReleasePagePrefix) &&
           page.substr(kReleasePagePrefix.size()) == tag;
}

std::string BoundedNotes(std::string notes)
{
    if (notes.size() <= kMaximumReleaseNotesBytes) return notes;
    constexpr std::string_view suffix =
        "\n\n[Release notes truncated. Open the GitHub release page for the full notes.]";
    std::size_t prefixBytes = kMaximumReleaseNotesBytes - suffix.size();
    while (prefixBytes < notes.size() && prefixBytes != 0u &&
           (static_cast<unsigned char>(notes[prefixBytes]) & 0xc0u) == 0x80u)
    {
        --prefixBytes;
    }
    notes.resize(prefixBytes);
    notes.append(suffix);
    return notes;
}

UpdateCheckResult Result(UpdateCheckStatus status,
                         std::string installedVersion,
                         std::string diagnostic)
{
    UpdateCheckResult result;
    result.status = status;
    result.installedVersion = std::move(installedVersion);
    result.diagnostic = std::move(diagnostic);
    return result;
}
}

std::optional<SemanticVersion> ParseSemanticVersion(std::string_view text)
{
    if (text.empty() || text.size() > 128u) return std::nullopt;
    if (text.front() == 'v') text.remove_prefix(1u);
    if (text.empty()) return std::nullopt;

    SemanticVersion version;
    version.normalized.assign(text);

    const std::size_t buildSeparator = text.find('+');
    if (buildSeparator != std::string_view::npos)
    {
        if (text.find('+', buildSeparator + 1u) != std::string_view::npos ||
            !ValidateBuildMetadata(text.substr(buildSeparator + 1u)))
        {
            return std::nullopt;
        }
        text = text.substr(0u, buildSeparator);
    }

    const std::size_t prereleaseSeparator = text.find('-');
    if (prereleaseSeparator != std::string_view::npos)
    {
        if (!ParsePrerelease(text.substr(prereleaseSeparator + 1u), version.prerelease)) return std::nullopt;
        text = text.substr(0u, prereleaseSeparator);
    }

    const std::size_t firstDot = text.find('.');
    const std::size_t secondDot = firstDot == std::string_view::npos ? std::string_view::npos : text.find('.', firstDot + 1u);
    if (firstDot == std::string_view::npos || secondDot == std::string_view::npos ||
        text.find('.', secondDot + 1u) != std::string_view::npos ||
        !ParseCoreNumber(text.substr(0u, firstDot), version.major) ||
        !ParseCoreNumber(text.substr(firstDot + 1u, secondDot - firstDot - 1u), version.minor) ||
        !ParseCoreNumber(text.substr(secondDot + 1u), version.patch))
    {
        return std::nullopt;
    }
    return version;
}

int CompareSemanticVersions(const SemanticVersion& left, const SemanticVersion& right)
{
    if (left.major != right.major) return left.major < right.major ? -1 : 1;
    if (left.minor != right.minor) return left.minor < right.minor ? -1 : 1;
    if (left.patch != right.patch) return left.patch < right.patch ? -1 : 1;
    if (left.prerelease.empty() != right.prerelease.empty()) return left.prerelease.empty() ? 1 : -1;

    const std::size_t common = std::min(left.prerelease.size(), right.prerelease.size());
    for (std::size_t index = 0; index < common; ++index)
    {
        const auto& a = left.prerelease[index];
        const auto& b = right.prerelease[index];
        if (a.numeric && b.numeric)
        {
            if (a.numericValue != b.numericValue) return a.numericValue < b.numericValue ? -1 : 1;
        }
        else if (a.numeric != b.numeric) return a.numeric ? -1 : 1;
        else if (a.text != b.text) return a.text < b.text ? -1 : 1;
    }
    if (left.prerelease.size() == right.prerelease.size()) return 0;
    return left.prerelease.size() < right.prerelease.size() ? -1 : 1;
}

GitHubHttpRequest BuildHordeGitHubReleaseRequest()
{
    GitHubHttpRequest request;
    request.url = kReleaseApiUrl;
    request.accept = "application/vnd.github+json";
    request.apiVersion = "2022-11-28";
    request.userAgent = "Horde-Lantern-RT-Updater/1";
    request.maximumResponseBytes = kMaximumResponseBytes;
    return request;
}

UpdateCheckResult CheckForGitHubReleaseUpdate(std::string_view installedVersion,
                                              ReleaseChannel channel,
                                              const GitHubReleaseFetcher& fetcher)
{
    const auto installed = ParseSemanticVersion(installedVersion);
    if (!installed)
    {
        return Result(UpdateCheckStatus::InvalidInstalledVersion,
                      std::string(installedVersion),
                      "The installed build does not expose a valid immutable semantic version.");
    }

    const GitHubHttpRequest request = BuildHordeGitHubReleaseRequest();
    GitHubHttpResponse response;
    try
    {
        response = fetcher(request);
    }
    catch (...)
    {
        return Result(UpdateCheckStatus::NetworkError, installed->normalized,
                      "The public GitHub release check could not be completed.");
    }

    if (response.statusCode != 200)
    {
        return Result(UpdateCheckStatus::NetworkError, installed->normalized,
                      "GitHub returned HTTP " + std::to_string(response.statusCode) + " while checking for updates.");
    }
    if (response.body.size() > request.maximumResponseBytes)
    {
        return Result(UpdateCheckStatus::InvalidResponse, installed->normalized,
                      "GitHub returned an update response larger than the fixed safety limit.");
    }

    JsonValue root;
    JsonParser parser(response.body);
    if (!parser.Parse(root) || root.type != JsonType::Array || root.array.size() > 10u)
    {
        return Result(UpdateCheckStatus::InvalidResponse, installed->normalized,
                      "GitHub returned malformed or unbounded release metadata.");
    }
    if (root.array.empty())
    {
        return Result(UpdateCheckStatus::NoPublishedRelease, installed->normalized,
                      "No GitHub Release is currently published for this channel.");
    }

    std::optional<SemanticVersion> selectedVersion;
    std::optional<UpdateMetadata> selected;
    for (const JsonValue& release : root.array)
    {
        if (release.type != JsonType::Object) continue;
        const std::string* tag = StringMember(release, "tag_name");
        const std::string* page = StringMember(release, "html_url");
        const auto draft = BooleanMember(release, "draft");
        const auto prerelease = BooleanMember(release, "prerelease");
        if (tag == nullptr || page == nullptr || !draft.has_value() || !prerelease.has_value() || *draft) continue;

        const auto version = ParseSemanticVersion(*tag);
        if (!version || !IsTrustedReleasePage(*page, *tag)) continue;
        if (channel == ReleaseChannel::StableOnly && (*prerelease || !version->prerelease.empty())) continue;
        if (CompareSemanticVersions(*version, *installed) <= 0) continue;
        if (selectedVersion && CompareSemanticVersions(*version, *selectedVersion) <= 0) continue;

        UpdateMetadata metadata;
        metadata.version = version->normalized;
        metadata.tag = *tag;
        metadata.title = OptionalStringMember(release, "name");
        if (metadata.title.empty()) metadata.title = "Horde Lantern RT " + metadata.version;
        metadata.notes = BoundedNotes(OptionalStringMember(release, "body"));
        metadata.publishedAt = OptionalStringMember(release, "published_at");
        metadata.releasePageUrl = *page;
        metadata.prerelease = *prerelease;
        selectedVersion = *version;
        selected = std::move(metadata);
    }

    if (selected)
    {
        UpdateCheckResult result = Result(UpdateCheckStatus::UpdateAvailable, installed->normalized,
                                          "Horde Lantern RT " + selected->version + " is available.");
        result.update = std::move(selected);
        return result;
    }
    return Result(UpdateCheckStatus::UpToDate, installed->normalized,
                  "This is the latest eligible GitHub Release.");
}
}
