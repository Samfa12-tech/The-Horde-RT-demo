#include "update/GitHubReleaseUpdater.h"

#include <cstdlib>
#include <iostream>
#include <string>

namespace
{
using horde::update::CheckForGitHubReleaseUpdate;
using horde::update::CompareSemanticVersions;
using horde::update::GitHubHttpRequest;
using horde::update::GitHubHttpResponse;
using horde::update::ParseSemanticVersion;
using horde::update::ReleaseChannel;
using horde::update::UpdateCheckStatus;

void Check(bool condition, const char* message)
{
    if (!condition)
    {
        std::cerr << "FAIL: " << message << '\n';
        std::exit(1);
    }
}

GitHubHttpResponse Ok(std::string body)
{
    return {200, std::move(body)};
}

void CheckSemanticVersionOrdering()
{
    const auto stable = ParseSemanticVersion("v1.5.3");
    const auto beta2 = ParseSemanticVersion("1.6.0-beta.2");
    const auto beta11 = ParseSemanticVersion("1.6.0-beta.11");
    const auto release = ParseSemanticVersion("1.6.0");

    Check(stable.has_value(), "v-prefixed stable SemVer must parse");
    Check(beta2.has_value() && beta11.has_value(), "numeric prerelease identifiers must parse");
    Check(release.has_value(), "stable SemVer must parse");
    Check(CompareSemanticVersions(*stable, *beta2) < 0, "a later minor prerelease must outrank an older stable release");
    Check(CompareSemanticVersions(*beta2, *beta11) < 0, "numeric prerelease identifiers must compare numerically");
    Check(CompareSemanticVersions(*beta11, *release) < 0, "a stable release must outrank its prerelease");
    Check(!ParseSemanticVersion("1.05.3").has_value(), "leading-zero version components must be rejected");
    Check(!ParseSemanticVersion("release-1.5.3").has_value(), "free-form tag prefixes must be rejected");
    Check(!ParseSemanticVersion("1.5").has_value(), "partial versions must be rejected");
}

void CheckStableSelectionAndRequestContract()
{
    bool fetched = false;
    const auto result = CheckForGitHubReleaseUpdate(
        "1.5.2", ReleaseChannel::StableOnly,
        [&](const GitHubHttpRequest& request) {
            fetched = true;
            Check(request.url == "https://api.github.com/repos/Samfa12-tech/The-Horde-RT-demo/releases?per_page=10",
                  "the updater must query only the bounded public Horde releases endpoint");
            Check(request.accept == "application/vnd.github+json", "the GitHub JSON media type must be explicit");
            Check(request.apiVersion == "2022-11-28", "the GitHub API version must be pinned");
            Check(request.userAgent == "Horde-Lantern-RT-Updater/1", "GitHub requests must identify the application");
            Check(request.maximumResponseBytes == 256u * 1024u, "release responses must have a fixed size limit");
            return Ok(R"json([
                {
                    "tag_name": "v1.6.0-beta.1",
                    "html_url": "https://github.com/Samfa12-tech/The-Horde-RT-demo/releases/tag/v1.6.0-beta.1",
                    "name": "Preview",
                    "body": "Not for stable users",
                    "draft": false,
                    "prerelease": true,
                    "published_at": "2026-08-30T01:00:00Z"
                },
                {
                    "tag_name": "v1.5.4",
                    "html_url": "https://github.com/Samfa12-tech/The-Horde-RT-demo/releases/tag/v1.5.4",
                    "name": "Horde Lantern RT 1.5.4",
                    "body": "Newest stable fixes",
                    "draft": false,
                    "prerelease": false,
                    "published_at": "2026-08-29T01:00:00Z"
                },
                {
                    "tag_name": "v1.5.3",
                    "html_url": "https://github.com/Samfa12-tech/The-Horde-RT-demo/releases/tag/v1.5.3",
                    "name": "Older stable",
                    "body": null,
                    "draft": false,
                    "prerelease": false,
                    "published_at": "2026-08-28T01:00:00Z"
                }
            ])json");
        });

    Check(fetched, "the release fetch callback must be invoked once");
    Check(result.status == UpdateCheckStatus::UpdateAvailable, "a newer stable release must be reported");
    Check(result.update.has_value(), "an available update must include metadata");
    Check(result.update->version == "1.5.4", "the highest eligible stable SemVer must be selected");
    Check(result.update->title == "Horde Lantern RT 1.5.4", "release title must be exposed");
    Check(result.update->notes == "Newest stable fixes", "bounded release notes must be exposed");
    Check(result.update->releasePageUrl == "https://github.com/Samfa12-tech/The-Horde-RT-demo/releases/tag/v1.5.4",
          "Update now must point only to the verified GitHub release page");
}

void CheckPrereleaseChannelAndCurrentVersion()
{
    const std::string response = R"json([
        {
            "tag_name": "v1.6.0-beta.2",
            "html_url": "https://github.com/Samfa12-tech/The-Horde-RT-demo/releases/tag/v1.6.0-beta.2",
            "name": "Caf\u00e9 \ud83d\udd25 update",
            "body": "Preview glass \u2728",
            "draft": false,
            "prerelease": true,
            "published_at": "2026-08-30T01:00:00Z"
        },
        {
            "tag_name": "v1.5.3",
            "html_url": "https://github.com/Samfa12-tech/The-Horde-RT-demo/releases/tag/v1.5.3",
            "name": "Stable update",
            "body": "Stable build",
            "draft": false,
            "prerelease": false,
            "published_at": "2026-08-29T01:00:00Z"
        }
    ])json";

    const auto preview = CheckForGitHubReleaseUpdate(
        "1.5.2", ReleaseChannel::IncludePrerelease,
        [&](const GitHubHttpRequest&) { return Ok(response); });
    Check(preview.status == UpdateCheckStatus::UpdateAvailable, "the alpha channel must include prereleases");
    Check(preview.update->version == "1.6.0-beta.2", "the alpha channel must select the highest SemVer");
    Check(preview.update->title == "Caf\xc3\xa9 \xf0\x9f\x94\xa5 update",
          "Unicode release titles must survive shared JSON parsing as UTF-8");
    Check(preview.update->notes == "Preview glass \xe2\x9c\xa8",
          "Unicode release notes must survive shared JSON parsing as UTF-8");

    const auto current = CheckForGitHubReleaseUpdate(
        "1.6.0-beta.2", ReleaseChannel::IncludePrerelease,
        [&](const GitHubHttpRequest&) { return Ok(response); });
    Check(current.status == UpdateCheckStatus::UpToDate, "the same published version must not notify again");
    Check(!current.update.has_value(), "up-to-date results must not expose an update action");
}

void CheckUntrustedAndDraftReleasesAreIgnored()
{
    const auto result = CheckForGitHubReleaseUpdate(
        "1.5.2", ReleaseChannel::IncludePrerelease,
        [&](const GitHubHttpRequest&) {
            return Ok(R"json([
                {
                    "tag_name": "v9.0.0",
                    "html_url": "https://evil.example/download.exe",
                    "name": "Untrusted",
                    "body": "Run me",
                    "draft": false,
                    "prerelease": false,
                    "published_at": "2026-08-30T01:00:00Z"
                },
                {
                    "tag_name": "v8.0.0",
                    "html_url": "https://github.com/Samfa12-tech/The-Horde-RT-demo/releases/tag/v8.0.0",
                    "name": "Draft",
                    "body": "Hidden",
                    "draft": true,
                    "prerelease": false,
                    "published_at": null
                },
                {
                    "tag_name": "v1.5.3",
                    "html_url": "https://github.com/Samfa12-tech/The-Horde-RT-demo/releases/tag/v1.5.3",
                    "name": "Safe",
                    "body": "Open this release page",
                    "draft": false,
                    "prerelease": false,
                    "published_at": "2026-08-29T01:00:00Z"
                }
            ])json");
        });

    Check(result.status == UpdateCheckStatus::UpdateAvailable, "a safe lower release must survive rejected entries");
    Check(result.update->version == "1.5.3", "foreign URLs and drafts must never become update actions");
}

void CheckBoundedFailureModes()
{
    const auto noReleases = CheckForGitHubReleaseUpdate(
        "1.5.2", ReleaseChannel::IncludePrerelease,
        [](const GitHubHttpRequest&) { return Ok("[]"); });
    Check(noReleases.status == UpdateCheckStatus::NoPublishedRelease, "an empty release list must mean no published release");

    const auto missingRepository = CheckForGitHubReleaseUpdate(
        "1.5.2", ReleaseChannel::IncludePrerelease,
        [](const GitHubHttpRequest&) { return GitHubHttpResponse{404, R"({"message":"Not Found"})"}; });
    Check(missingRepository.status == UpdateCheckStatus::NetworkError,
          "a missing or unavailable repository endpoint must not look like a healthy empty release list");

    const auto rateLimited = CheckForGitHubReleaseUpdate(
        "1.5.2", ReleaseChannel::IncludePrerelease,
        [](const GitHubHttpRequest&) { return GitHubHttpResponse{403, R"({"message":"rate limit"})"}; });
    Check(rateLimited.status == UpdateCheckStatus::NetworkError, "HTTP failures must not look like update availability");

    const auto malformed = CheckForGitHubReleaseUpdate(
        "1.5.2", ReleaseChannel::IncludePrerelease,
        [](const GitHubHttpRequest&) { return Ok("[{\"tag_name\":"); });
    Check(malformed.status == UpdateCheckStatus::InvalidResponse, "malformed JSON must fail closed");

    const auto oversized = CheckForGitHubReleaseUpdate(
        "1.5.2", ReleaseChannel::IncludePrerelease,
        [](const GitHubHttpRequest& request) {
            return Ok(std::string(request.maximumResponseBytes + 1u, 'x'));
        });
    Check(oversized.status == UpdateCheckStatus::InvalidResponse, "oversized responses must fail before parsing");

    const auto invalidInstalled = CheckForGitHubReleaseUpdate(
        "Showcase 1.5.2", ReleaseChannel::IncludePrerelease,
        [](const GitHubHttpRequest&) { return Ok("[]"); });
    Check(invalidInstalled.status == UpdateCheckStatus::InvalidInstalledVersion,
          "an invalid installed identity must not trigger a network request or update");
}
}

int main()
{
    CheckSemanticVersionOrdering();
    CheckStableSelectionAndRequestContract();
    CheckPrereleaseChannelAndCurrentVersion();
    CheckUntrustedAndDraftReleasesAreIgnored();
    CheckBoundedFailureModes();
    std::cout << "GitHub release updater tests passed.\n";
    return 0;
}
