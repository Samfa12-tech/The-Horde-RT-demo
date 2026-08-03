# GitHub Releases

Windows ZIP and Android APK builds should be published as **GitHub Release assets**, not GitHub Packages.

GitHub Packages is intended for package registries such as container images, npm, Maven and NuGet. Installable game/demo binaries belong on the repository's Releases page.

## Publish an existing validated candidate

Run this from the repository root on the Windows machine that contains the validated files in `releases/candidates/`:

```powershell
.\tools\publish-github-release.ps1 -Version '0.1.3-alpha.1'
```

The publisher expects:

```text
releases/candidates/Horde-Lantern-RT-Alpha-0.1.3-alpha.1-Windows-x64.zip
releases/candidates/Horde-Lantern-RT-Alpha-0.1.3-alpha.1-Android.apk
releases/candidates/SHA256SUMS.txt
```

Before publication it:

- requires authenticated `gh` and the expected repository remote;
- rejects Android filenames containing `debug`, `unsigned`, or `do-not-publish`;
- verifies both release files against `SHA256SUMS.txt`;
- refuses to overwrite an existing tag/release;
- creates a prerelease by default;
- uploads the Windows ZIP, signed Android APK, and checksum manifest;
- includes the itch page in the release notes.

Use a reviewed release-notes file when available:

```powershell
.\tools\publish-github-release.ps1 `
  -Version '0.1.3-alpha.1' `
  -NotesFile '.\docs\SHOWCASE_ALPHA_RELEASE_NOTES_2026-07-31.md'
```

Create a draft instead of immediately publishing:

```powershell
.\tools\publish-github-release.ps1 -Version '0.1.3-alpha.1' -Draft
```

## Authentication

If GitHub CLI is not authenticated:

```powershell
gh auth login
```

The release publisher does not build or sign Android. Continue using `package-signed-alpha.ps1` first so that GitHub and itch receive the same validated artifacts.
