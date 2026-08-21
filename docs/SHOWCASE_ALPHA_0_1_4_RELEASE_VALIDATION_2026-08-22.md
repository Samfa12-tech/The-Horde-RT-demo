# Showcase Alpha 0.1.4 release validation - 2026-08-22

## Published identity

- Display: **Horde Lantern RT - Showcase Alpha 0.1.4**
- Package version: `0.1.4-alpha.1`
- Android package/version: `com.samfa12.hordelanternrt`, `versionCode 5`
- Release source merge: `f0cba098111527241b00ce56a5ad8bd6a42d9414`
- Windows itch channel/build: `samfa12/the-horde:windows-x64`, `#1903586`
- Android itch channel/build: `samfa12/the-horde:android`, `#1903587`

## Exact published artifacts

| Artifact | SHA-256 |
|---|---|
| `Horde-Lantern-RT-Alpha-0.1.4-alpha.1-Windows-x64.zip` | `82ad58546864f55a6c61ead372811bbe3bebcf7066cb9f97c76008136a82c8b5` |
| `Horde-Lantern-RT-Alpha-0.1.4-alpha.1-Android.apk` | `ff549c2ab1c68fe7a36d4e45c966e664a5ec7e3f19fb8f416b7a6e8971db2e55` |

The Android preflight verified the established release-certificate SHA-256, version identity, V2 signature, credits, native libraries, static C++ runtime, 16 KiB APK alignment, and 16 KiB ELF `LOAD` alignment. Signing values were read only from the external local handoff into process-scoped variables, never printed, and cleared immediately after packaging.

## Validation

- Release-prep tree `1ecc095` passed clean Host foundation run `reports/foundation-runs/run-20260821-210413`: shader freshness and negative safeguards, fresh Windows Debug/Release builds, 12/12 CTests in each configuration, 13 Windows captures, Android Debug/unsigned Release/lint across four ABIs, package/licence checks, and evidence hashes. Merge commit `f0cba09` has the same tree.
- Signed packaging from clean `main` reran 12/12 Windows Release tests, Android Debug/signed Release, and `lintRelease` before producing the final hashes.
- The exact Windows ZIP was extracted to `reports/release-smoke/windows-0.1.4-20260822-042016/` and launched using only packaged files. It reported file/product version `0.1.4-alpha.1`, selected `RayTracingPipeline` on the NVIDIA GeForce RTX 5050 Laptop GPU, dispatched `1232x803`, and set `rtScene.presented=true` after swapchain presentation. It exited cleanly.
- Butler independently rechecked hashes/certificate before upload. Both channels became active at user version `0.1.4-alpha.1`, and an anonymous live-page check exposed both new filenames/versions.

## Android evidence boundary

The public signed APK was not installed during publication because `adb devices -l` exposed zero devices. No signed-release phone, lifecycle, or performance claim is made. The unchanged runtime has direct exact-device evidence from Debug APK `f68fe4cccf2755ef579826080bf76364fe2a48744b23aba765df02a17e2d1dfa` at commit `547d89d`, including byte-matched installation, strict ASTC, honest RT presentation, six checkpoints, replay, 13 captures, Home/resume, and owner audio/haptic/stagger acceptance. Release-source changes after `547d89d` are identity, packaging copy, validation tooling, and documentation only.

## Audio/haptic classification

`Audio/haptic manual revalidation required: NO` - this publication changes version/package metadata and public release copy only; the accepted runtime event, spatialisation, cue, timing, and haptic paths are unchanged.

## Remaining items

- The public itch description still contains pre-0.1.4 one-skeleton/control wording. Both downloads are correct; updating the page copy requires an authenticated itch browser session.
- The owner-only independent signing-backup checklist remains unchecked. A local-only handoff exists, but it is not evidence of a separate recovery copy.
- Hotstrike finished-game use remains permitted; raw derivative retention in public Git/LFS history remains tracked separately as issue #13.
