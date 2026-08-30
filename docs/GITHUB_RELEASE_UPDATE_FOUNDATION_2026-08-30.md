# GitHub Release Update Alert — 2026-08-30

## Scope

This slice adds the shared release decision layer and the complete Windows and
Android update alert. It does not publish a release, change the application
version, download an installer/APK, execute an asset, or add a credential.

The design follows the proven Pocket DAW separation between update discovery
and platform UI. Horde does not yet have Pocket DAW's signed Tauri installer
pipeline, so `Update now` opens a verified GitHub Release page only after the
player explicitly presses it.

## Current public-repository finding

A live anonymous API request on 2026-08-30 returned HTTP 200, 7,195 UTF-8 bytes,
and one release: `v0.1.3-alpha.1`. GitHub marks it as a prerelease, so the
updater uses the bounded releases-list endpoint (`per_page=10`) instead of
`/releases/latest`. An installed `1.5.2` build correctly treats
`0.1.3-alpha.1` as older and does not alert.

## Shared contract

`src/update/GitHubReleaseUpdater.*` provides:

- strict SemVer 2.0 parsing/comparison, with an optional leading `v` tag;
- explicit Stable-only and Include-prerelease channels;
- the exact public GitHub API request contract, including pinned media/API
  versions, a user agent, ten-release limit, and 256 KiB response ceiling;
- bounded JSON parsing with depth/node/response limits;
- rejection of drafts, malformed versions, and release-page URLs outside the
  exact Horde repository;
- highest-eligible-version selection rather than trusting release creation
  order;
- bounded title, notes, publication time, tag, version, and verified release
  page metadata;
- fail-closed outcomes for malformed responses, rate limits/network errors,
  and invalid installed identities.

No gameplay, renderer, or simulation state belongs in this path. The shared
layer never downloads or executes a release asset.

## Windows integration

`WindowsGitHubReleaseUpdate.cpp` fulfils the fetch callback with background
WinHTTP. It requires HTTPS, uses normal certificate/host validation, disables
redirects, applies 3.5/5 second connect/request timeouts, and reads at most the
shared response limit plus one rejection byte. The check begins only after the
interactive RT menu is ready and is disabled for deterministic capture mode.

`Help -> Check for updates...` runs the same operation manually. Startup
failures are silent; manual failures explain that the connection should be
checked. An available update uses a TaskDialog with `Update now` and `Later`.
The first action calls `ShellExecute` only with the verified URL returned by
the shared policy.

Worker completion is represented by a process-owned opaque token, not an
untrusted window-message pointer. A window generation invalidates results on
`WM_DESTROY`, bounds completed state, and prevents a late worker from reviving
or corrupting a later check.

## Android integration

`MainActivity` runs HTTPS on a single background executor after the branded
entry UI becomes responsive and offers the same operation in the main menu.
Java obtains the exact endpoint, media/API headers, user agent, and response
limit from `BuildHordeGitHubReleaseRequest()` through a narrow JNI contract,
so the platform transport cannot silently drift from the tested shared policy.
Redirects are disabled and the same 3.5/5 second timeouts apply.

The response and decision envelope cross JNI as raw UTF-8 byte arrays. This
preserves international release titles/notes and avoids modified-UTF-8 loss.
Completed decisions are retained while the Activity is paused and presented
only after `onResume`; destroyed Activities never show them. The app-owned
AlertDialog offers `Update now` / `Later`, and `Intent.ACTION_VIEW` receives
only the already verified release page. There is no package-install permission,
asset download, or automatic installation path.

## Release operator contract

- Create a new GitHub Release with a unique immutable SemVer tag such as
  `v1.5.3`; do not replace assets behind an existing tag.
- Mark alpha candidates as GitHub prereleases. Showcase Alpha builds use the
  Include-prerelease channel.
- Attach exact reviewed artifacts and checksums only after the existing Horde
  signing/package/device gates pass.
- Opening the release page remains the only update action until a separately
  approved signed-download verifier exists for each platform.

## Validation

`horde_rt_github_release_updater_tests` covers strict SemVer parsing and
ordering, stable/prerelease selection, highest-version selection, trusted URL
enforcement, draft rejection, no-release handling, HTTP failure, malformed
JSON, response-size limits, invalid installed identity, and Unicode metadata.
Android unit coverage verifies base-version selection, the menu resource, and
the ordinary `INTERNET` permission. Fresh Windows Debug/Release, Android
Debug/unsigned Release/lint, capture, package, and licence gates are recorded
by the dated owner-candidate validation record.

Audio/haptic manual revalidation required: NO — the updater does not alter
audio assets, gameplay events, haptic routing, or platform feedback timing.
