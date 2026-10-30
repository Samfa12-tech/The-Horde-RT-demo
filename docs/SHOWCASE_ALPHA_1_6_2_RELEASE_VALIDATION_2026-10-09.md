# Showcase Alpha 1.6.2 signed release validation - 9 October 2026

Package version: `1.6.2`; Android versionCode 10.

Immutable runtime/build source and GitHub tag target: `db62032d9ab54ebaa5bd12d17dc7e987fc8f54c9`, tree `bccbfe9469c69c0657b4f99c06a22c8c145dba28`. PR18 integrated it at `a3f120cf1f3995035f793fa40400e0a74a4c0459`. The retained promotion receipt identifies the original source and unchanged Android application/Windows runtime payloads; the source-to-merge diff contains documentation/distribution text only. The tag deliberately retains that exact validated artifact source rather than later moving main or any 1.7 branch. [Canonical provenance](../release-provenance/horde-lantern-rt-alpha-1.6.2.json) uses only the publisher's schema.

## Exact publication artifacts

| Artifact | Bytes | SHA-256 |
| --- | ---: | --- |
| `Horde-Lantern-RT-Alpha-1.6.2-Windows-x64.zip` | 145,574,438 | `18177d0d84c40f0831b22ee959be597deeb3bc0e1cef183e78dd19c1b113f21c` |
| `Horde-Lantern-RT-Alpha-1.6.2-Android.apk` | 121,071,933 | `cc4de768cf9ca2a409984a24888399e9c518363c359b3a9947d1ed0350c254e8` |
| `SHA256SUMS.txt` | 220 | `eb23557b3dcce0016bc13da915cb7ed39c49b5daba83ca6688e96aedc3d35cf3` |

These are the exact retained signed/publication files, copied unchanged from `C:\Users\sam_s\Documents\Codex\2026-10-03\task-4\source\releases\candidates\publish-1-6-2-20261009` into this isolated release workspace. The original checksum manifest is retained byte-for-byte. No game rebuild, new signing or substitute validation artifact was used.

Fresh local byte checks match the [signed publication receipt](evidence/2026-10-09-release-publication/README.md) and its signed/downloaded hashes. The retained actual itch download at `task-4/itch-live-1-6-2-20261009/android` matches the APK, and all 96 Windows ZIP file entries match the retained downloaded Windows directory, with no additional/missing files. The executable SHA-256 remains `084e4a345b556cdf6c3e8b4adbaef488c22cee02d41226dc43c49c5cf873260d`. Butler distributes a directory; its generated download ZIP need not match the local publication ZIP. This closeout rechecks retained downloaded bytes, not a new itch fetch. [Machine-readable byte/alignment verification](evidence/2026-10-09-github-release/artifact-verification.json).

Fresh Android SDK 36.1.0 `apksigner verify --verbose --print-certs` passes v2/v3 with one signer and stable certificate SHA-256 `8245277a11bca5576f116724507f799d6f4c178ce5fbb7e3981415c9e6b3c245`; [certificate output](evidence/2026-10-09-github-release/certificate-verification.txt). `zipalign -c -P 16 4` passes. Every LOAD segment of all four packaged native libraries is aligned to 16,384 bytes; no `libc++_shared.so` is present. These are verification checks, with no keystore/recovery access. Owner backup/recovery checklist items remain owner-only and unchecked.

## Evidence scope and retained limits

The [final technical review](ENGINEERING_1_6_2_FINAL_REVIEW_2026_10_09.md) retains the original clean Host source, Debug/Release 144/144 each, 13 RT captures, Android build/lint/package gates and 12/12 exact-source CI. Those are recorded historical results, not independently rerun in this documentation closeout. The signed-package ordinary Android/Windows RT presentation smokes remain the original publication observations. No phone operation or new installed-client update dialog observation is part of this closeout.

Fresh/reset mobile defaults remain 50% / Mobile water/fire / Glass Off / Current shadows / cap30 / Mist On / Dust Low; desktop remains 100% / High / Glass On with the other defaults unchanged. Saved/custom preferences remain authoritative; 33%/40% remain experimental. Current forgiving combat, walking torch-arm wiggle, shafts, seamless music handover and long sustained phone testing remain deferred. No sustained 30 FPS, untested controller/device or completed Eric audit certification follows. Issue [#19](https://github.com/Samfa12-tech/The-Horde-RT-demo/issues/19) is owner-closed as completed; this closeout does not independently reproduce a mist fix. Historical unsigned receipts and failed attempts retain their original facts.

## GitHub publication and frozen attachment plan

The supported `tools/publish-github-release.ps1` passed production preflight and published the non-draft [GitHub v1.6.2 prerelease](https://github.com/Samfa12-tech/The-Horde-RT-demo/releases/tag/v1.6.2) once, with exact public downloads verified. Future verification must retain production preflight against this canonical provenance, the exact target above and the three existing files. The release uses the existing [release notes](SHOWCASE_ALPHA_1_6_2_RELEASE_NOTES_DRAFT.md), including [itch](https://samfa12.itch.io/the-horde) and the [1.6.2 devlog](https://samfa12.itch.io/the-horde/devlog/1698261/the-horde-162-finishing-the-tomb). The notes path retains its historical filename because it exists at the immutable source and is checked by preflight. Public release/asset verification and updater selection are in the separate [GitHub closeout receipt](evidence/2026-10-09-github-release/README.md).
