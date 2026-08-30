#pragma once

#include <cstddef>
#include <cstdint>
#include <functional>
#include <optional>
#include <string>
#include <string_view>
#include <vector>

namespace horde::update
{
struct SemanticVersionIdentifier
{
    bool numeric = false;
    std::uint64_t numericValue = 0;
    std::string text;
};

struct SemanticVersion
{
    std::uint32_t major = 0;
    std::uint32_t minor = 0;
    std::uint32_t patch = 0;
    std::vector<SemanticVersionIdentifier> prerelease;
    std::string normalized;
};

std::optional<SemanticVersion> ParseSemanticVersion(std::string_view text);
int CompareSemanticVersions(const SemanticVersion& left, const SemanticVersion& right);

enum class ReleaseChannel
{
    StableOnly,
    IncludePrerelease,
};

struct GitHubHttpRequest
{
    std::string url;
    std::string accept;
    std::string apiVersion;
    std::string userAgent;
    std::size_t maximumResponseBytes = 0;
};

struct GitHubHttpResponse
{
    int statusCode = 0;
    std::string body;
};

using GitHubReleaseFetcher = std::function<GitHubHttpResponse(const GitHubHttpRequest&)>;

struct UpdateMetadata
{
    std::string version;
    std::string tag;
    std::string title;
    std::string notes;
    std::string publishedAt;
    std::string releasePageUrl;
    bool prerelease = false;
};

enum class UpdateCheckStatus
{
    UpdateAvailable,
    UpToDate,
    NoPublishedRelease,
    InvalidInstalledVersion,
    NetworkError,
    InvalidResponse,
};

struct UpdateCheckResult
{
    UpdateCheckStatus status = UpdateCheckStatus::InvalidResponse;
    std::string installedVersion;
    std::optional<UpdateMetadata> update;
    std::string diagnostic;
};

GitHubHttpRequest BuildHordeGitHubReleaseRequest();

// The fetcher supplies platform networking. This shared layer never downloads
// or executes release assets: a successful result exposes only the verified
// github.com release page for a platform UI to open with explicit user action.
UpdateCheckResult CheckForGitHubReleaseUpdate(std::string_view installedVersion,
                                              ReleaseChannel channel,
                                              const GitHubReleaseFetcher& fetcher);
}
