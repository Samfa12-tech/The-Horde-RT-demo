# Verified GitHub 1.6.2 release closeout - 9 October 2026

The owner authorized reuse of the exact existing signed APK, Windows publication ZIP and checksum manifest. The supported `tools/publish-github-release.ps1` completed once at 2026-10-09T03:34:14Z. [GitHub v1.6.2](https://github.com/Samfa12-tech/The-Horde-RT-demo/releases/tag/v1.6.2) is a non-draft prerelease, release ID 407473914. [Signed validation](../../SHOWCASE_ALPHA_1_6_2_RELEASE_VALIDATION_2026-10-09.md) and [canonical provenance](../../../release-provenance/horde-lantern-rt-alpha-1.6.2.json) bind the exact tag to `db62032d9ab54ebaa5bd12d17dc7e987fc8f54c9`. [Remote tag response](tag-target.txt) verifies that target. Moving main and all 1.7 work remain separate.

## Actual public verification

At 2026-10-09T03:35:14.045581+00:00, fresh unauthenticated public API and release-page requests returned HTTP 200. The release is non-draft/prerelease and contains exactly the three planned uploaded assets. GitHub digests/sizes and full unauthenticated public downloads all match the canonical record:

| Artifact | Bytes | SHA-256 |
| --- | ---: | --- |
| `Horde-Lantern-RT-Alpha-1.6.2-Windows-x64.zip` | 145,574,438 | `18177d0d84c40f0831b22ee959be597deeb3bc0e1cef183e78dd19c1b113f21c` |
| `Horde-Lantern-RT-Alpha-1.6.2-Android.apk` | 121,071,933 | `cc4de768cf9ca2a409984a24888399e9c518363c359b3a9947d1ed0350c254e8` |
| `SHA256SUMS.txt` | 220 | `eb23557b3dcce0016bc13da915cb7ed39c49b5daba83ca6688e96aedc3d35cf3` |

[Release API response](github-release.json), [full-download verification](public-verification.json), [original manifest](SHA256SUMS.txt), [local/retained-itch byte and ELF checks](artifact-verification.json), [stable certificate output](certificate-verification.txt). The original [itch receipt](../2026-10-09-release-publication/README.md) remains unchanged, including its then-true no-GitHub statement. The current public [itch page](https://samfa12.itch.io/the-horde) identifies both 1.6.2 downloads, and the [devlog](https://samfa12.itch.io/the-horde/devlog/1698261/the-horde-162-finishing-the-tomb) is readable. The retained promotion receipt's `published:false` describes its pre-upload snapshot.

## Released 1.6.1 updater eligibility

The exact original shared updater source/header were exported from released 1.6.1 source `a397757249871b6b64fe5b77fc14f24e8cfcbb2b` and compiled as a standalone metadata check. Both original Android and Windows call sites pass `ReleaseChannel::IncludePrerelease`. Their request uses GitHub's `/releases?per_page=10` list. Feeding the fresh public API body (31,071 bytes) to that original parser with installed version `1.6.1` yields UpdateAvailable, version `1.6.2`, tag `v1.6.2`, prerelease true and the exact release-page URL. A StableOnly control excludes v1.6.2. [Actual selection output](live-update-selection.txt), [source/channel/payload identities](updater-verification.json), [fetched public API body](public-releases-response.json).

This is actual code/API validation, not a physically observed installed-client dialog or a new platform-networking test. No phone was accessed, mutated or installed. Update opens the GitHub release page for an explicit download; no auto-install is promised. IncludePrerelease makes this release eligible without marking Latest.

## Local validation and integration boundary

Production [prepublication preflight](prepublication-preflight.json) passed before the supported publisher ran. Production [postpublication preflight](postpublication-preflight.json) passes with the origin tag and actual GitHub uploaded assets matched. [Native version/documentation contract](version-documentation-tests.txt), [PowerShell 7 policy suite](pwsh-policy-tests.txt), [Windows PowerShell policy suite](windows-powershell-policy-tests.txt) and [existing preflight fixtures](preflight-fixture-tests.txt) pass. [Changed-document link/diff check](document-checks.json) passes; one copied historical dust-section anchor retains its original incorrect spelling and is reported explicitly. Its dated facts remain unchanged.

The isolated documentation branch reconciles README package guidance, PROJECT_MEMORY, PROJECT_DECISIONS, FUTURE_WORK, roadmap milestone/navigation and current handoffs. The historical release-note checkpoints are preserved in [their linked archive](../../ENGINEERING_1_6_2_RELEASE_NOTE_HISTORY_2026-10-09.md); the current publication notes retain the established DRAFT path required by immutable-source preflight. Issue #19 remains owner-closed as completed, with no independent fix-reproduction claim. No game rebuild, runtime/source/version change, new signing, force push, credential change, phone operation or destructive cleanup occurred. The original dirty checkout and active 1.7 work are preserved. Protected-main integration requires the six actual workflow checks on the exact documentation head and an up-to-date PR; their live CI/PR record is separate from the historical runtime CI.
