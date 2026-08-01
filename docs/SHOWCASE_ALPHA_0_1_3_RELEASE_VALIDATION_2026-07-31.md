# Showcase Alpha 0.1.3 release validation - 2026-07-31

> Post-publication device update, 2026-08-01: the exact published signed Android APK is now clean-install and Home/resume validated on `SM-S948B`; the publication-time unavailable-device caveat below is closed by the preserved evidence in `reports/release-smoke/android-signed-20260801-165934/`.

## Published identity

- Display: **Horde Lantern RT - Showcase Alpha 0.1.3**
- Package version: `0.1.3-alpha.1`
- Android package/version: `com.samfa12.hordelanternrt`, `versionCode 4`
- Candidate-source commit: `5bc64ef` (`Release player body, vitality, and dawn finale`)
- Windows itch channel: `samfa12/the-horde:windows-x64`, upload `#18339908`, active build `#1845895`
- Android itch channel: `samfa12/the-horde:android`, upload `#18341739`, active build `#1845896`
- Signing certificate SHA-256: `8245277a11bca5576f116724507f799d6f4c178ce5fbb7e3981415c9e6b3c245`

## Exact published candidates

| Artifact | SHA-256 |
|---|---|
| `Horde-Lantern-RT-Alpha-0.1.3-alpha.1-Windows-x64.zip` | `4ccc86bfab56beb9bce238f025afbf3afbf00589c8396ce937607ae1b95c274f` |
| `Horde-Lantern-RT-Alpha-0.1.3-alpha.1-Android.apk` | `a4eb996104c03734a7fa8a16be1f8f701d5b19c861c066af002f96f9a199eee9` |
| `Horde-Lantern-RT-Alpha-0.1.3-alpha.1-Android-preview-debug-signed.apk` | `97332834e5244e1f4f285b2cb981e6dab8be849c9138b68f759bd0a263802c84` |

The guarded preflight verified the hash manifest, established non-debug release certificate, 16 KiB APK alignment, static C++ runtime, 16 KiB ELF load alignment, native libraries, version resources, credits/licences, and Windows archive root. Only the Windows release content and stable-key-signed Android release APK were uploaded. Butler reported both active at user version `0.1.3-alpha.1`.

## Build and test evidence

The integrated Full gate at `reports/foundation-runs/run-20260731-225307/` passed shader freshness, negative safety cases, fresh Windows Debug and Release builds, all seven CTests in both configurations, deterministic Windows captures, Android clean Debug/Release builds, Release lint, and validation package/licence checks. It stopped at the device stage because ADB reported exactly zero authorised devices. This is an unavailable-device result, not a renderer or test failure.

Signed packaging then rebuilt the Release candidates from commit `5bc64ef`, passed all seven Release CTests, built Android Debug and signed Release, passed Release lint, and passed the package audits above. Signing values came from the established local-only handoff, were never emitted, and were cleared from the process environment after use.

## Exact Windows package smoke

The final ZIP was extracted to a fresh ignored directory and launched using only packaged files. `HordeLanternRT.exe` reported file/product version `0.1.3-alpha.1`, selected `RayTracingPipeline` on the NVIDIA GeForce RTX 5050 Laptop GPU, dispatched `1232x803`, and set `rtScene.presented=true` only after an RT-produced frame reached successful swapchain presentation.

Evidence: `reports/release-smoke/windows-final-20260731-230246/`.

## Android device evidence and boundary

Before the release identity bump, the exact Debug feature build was installed on Samsung `SM-S948B` / Android 16 and passed the full five-checkpoint 75% gate, all 13 replay waypoints, all 12 captures, Home/resume, touch-driven body inspection, the real dawn ending, Continue, and Begin Again with honest presentation. Its APK SHA-256 was `a9fe35932548f38e067c39ef604739aaa7eacd12005d119a88c0b77e2d7fa81e`; evidence is in `reports/android-showcase-runs/run-20260731-221231/` and `reports/android-body-finale-runs/run-20260731-221525/`.

At publication time, the final signed `0.1.3-alpha.1` APK had passed static and packaging validation but could not be installed because the publication gate and a subsequent ADB daemon restart both exposed zero devices. That was the correct boundary for the initial publication record.

On 2026-08-01, the phone reconnected as exact model `SM-S948B` / device code `m3q` / Android 16. The old stable `0.1.2-alpha.1` (`versionCode 3`) and Debug `0.1.2-alpha.1-debug` packages were explicitly removed. The published signed APK was then clean-installed as `com.samfa12.hordelanternrt`, `versionCode 4`, `versionName 0.1.3-alpha.1`; no Debug package remained. The installed on-device `base.apk` SHA-256 was `a4eb996104c03734a7fa8a16be1f8f701d5b19c861c066af002f96f9a199eee9`, exactly matching the published candidate.

The exact Release build visibly identified `SHOWCASE · ALPHA 0.1.3`, selected strict ASTC 6x6 diffuse/ARM plus ASTC 4x4 normals and strict ASTC 6x6 lich textures, loaded 17/17 SoundPool cues, and explicitly logged `Rejected debug capture intent in a non-debuggable build.` It produced two `RT frame reached Android swapchain presentation.` markers across Home/resume, with no fatal Java, native, or renderer marker. The headed menu screenshot was visually inspected; battery temperature was 33.7 C. This closes the exact signed-install, Release automation-rejection, and signed-build Home/resume checks. It remains an automated functional smoke rather than a new performance certification or a claim about human touch feel, haptic perception, or perceived audio quality.

Evidence: `reports/release-smoke/android-signed-20260801-165934/` (`summary.json`, scoped `logcat.txt`, package records, UI hierarchies, and startup/resume PNGs).

## Preserved boundaries

This release adds the rebuilt animated body, bounded player vitality/retry, and post-lich dawn epilogue. It does not add water, simultaneous enemies, broader AI, block/dodge, or the staged textured sword. The one-active-skinned-enemy limit, 18 TLAS instances, eight BLAS, `vkCmdTraceRaysKHR`, `rayQueryEXT`, one frame in flight, strict ASTC routing, BGRA correction, and honest `rtScene.presented` semantics remain intact.

After publication, candidate and itch scripts make the 0.1.1, 0.1.2, and 0.1.3 lines plus Android `versionCode <= 4` immutable.
