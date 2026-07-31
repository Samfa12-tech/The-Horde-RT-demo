# Showcase Alpha 0.1.3 release validation - 2026-07-31

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

The final signed `0.1.3-alpha.1` APK passed static and packaging validation but was not installed: the publication gate and a subsequent ADB daemon restart both exposed zero devices. Therefore this record does **not** claim an exact signed-APK install, Release automation rejection, or signed-build Home/resume check. Reconnect the `SM-S948B` and run that focused release smoke before treating those checks as current.

## Preserved boundaries

This release adds the rebuilt animated body, bounded player vitality/retry, and post-lich dawn epilogue. It does not add water, simultaneous enemies, broader AI, block/dodge, or the staged textured sword. The one-active-skinned-enemy limit, 18 TLAS instances, eight BLAS, `vkCmdTraceRaysKHR`, `rayQueryEXT`, one frame in flight, strict ASTC routing, BGRA correction, and honest `rtScene.presented` semantics remain intact.

After publication, candidate and itch scripts make the 0.1.1, 0.1.2, and 0.1.3 lines plus Android `versionCode <= 4` immutable.
