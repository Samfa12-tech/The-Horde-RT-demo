# Showcase Alpha 0.1.5 release validation - 2026-08-23

## Published identity

- Display: **Horde Lantern RT - Showcase Alpha 0.1.5**
- Package version: `0.1.5-alpha.1`
- Android package/version: `com.samfa12.hordelanternrt`, `versionCode 6`
- Feature commit: `2bda83a`
- Merged and packaged runtime source: `172bb0a61311ca704cb92457f153c9be1a9d442c`
- Windows itch channel/build: `samfa12/the-horde:windows-x64`, `#1908330`
- Android itch channel/build: `samfa12/the-horde:android`, `#1908331`

## Exact published artifacts

| Artifact | SHA-256 |
|---|---|
| `Horde-Lantern-RT-Alpha-0.1.5-alpha.1-Windows-x64.zip` | `631b9f01a4d348e18733c989ebacc9c32ce9005ac9498e49e8757a0a36411166` |
| `Horde-Lantern-RT-Alpha-0.1.5-alpha.1-Android.apk` | `1e81238a6e1b0e934c50eb15e80fc8efd39c06f16ca8960c428b22e5f5d5a7f2` |

Android signing used the established update identity. `apksigner` verified the V2 signature and release-certificate SHA-256 `8245277a11bca5576f116724507f799d6f4c178ce5fbb7e3981415c9e6b3c245`. Signing values were read from the external local handoff into process-scoped variables, were never printed or written into the repository, and were cleared immediately after packaging.

## Source and package validation

- The initial Full run `reports/foundation-runs/run-20260823-200938/` passed shader staleness, negative safety fixtures, fresh 13/13 Debug and Release CTests, 13 Windows captures, Android Debug/unsigned Release/lint across four ABIs, and package/licence checks. It stopped during Android capture three only because the local ADB daemon disappeared from port 5037. That partial run remains classified FAIL and unpublishable.
- After restarting ADB and confirming exactly one authorized raw-model `SM-S948B`, the clean device retry `reports/foundation-runs/run-20260823-200938/android-device-retry/run-20260823-201800/` passed the six-checkpoint 75% route, report-only 100% opening, 13-waypoint replay, all 13 captures, and Home/resume. The installed Debug APK matched SHA-256 `2630cc67bea5439fec5b0ce3c388986fae249ceedd57bf8c981873a93ef832ed`.
- Retry medians were 12.899 / 16.891 / 13.620 / 17.254 / 17.550 / 27.770 ms for opening / two-enemy / worst-bend / skylight / green / lich. The later views reached Samsung GPU thermal power level 5; the heaviest lich view is retained honestly in the descriptive 30-50 FPS band. Report-only 100% opening was 30.854 ms.
- A clean-checkout source-contract failure discovered after merge was a CRLF-sensitive test assertion, not a gameplay defect. Commit `172bb0a` made the assertion semantic and line-ending independent; isolated Debug and Release checks passed.
- Fresh merged-main Host run `reports/foundation-runs/run-20260823-202515/` then passed shader staleness and safety fixtures, 13/13 Debug and 13/13 Release CTests, 13 deterministic Windows captures, clean Android Debug/unsigned Release builds and lint, validation package/licence checks, and evidence hashes.
- Signed packaging from clean `main` reran all 13 Windows Release tests, Android Debug/signed Release, lint, APK assets/credits, static-runtime and 16 KiB package/ELF checks before producing the guarded hashes above.
- Final post-publication Host run `reports/foundation-runs/run-20260823-210447/` passed the same shader, 13/13 Debug, 13/13 Release, 13-capture, Android Debug/unsigned Release, lint, package/licence, and evidence-hash gates after the documentation reconciliation and shared immutable-version policy were applied. Its negative fixtures also proved that the published 0.1.5 line cannot be repackaged or re-uploaded and that the next Android release must use a version code greater than 6.

## Exact Windows package smoke

The published ZIP was extracted to `reports/release-smoke/windows-0.1.5-20260823-203142/` and launched using only packaged files. It reported product version `0.1.5-alpha.1`, selected `RayTracingPipeline` on the NVIDIA GeForce RTX 5050 Laptop GPU, dispatched `1232x803`, set `RT scene presented: yes` after swapchain presentation, and exited normally with code 0.

## Exact signed Android smoke

The exact signed APK upgraded successfully on `SM-S948B`. The installed `base.apk` was pulled to `reports/release-smoke/android-signed-0.1.5-20260823-203208/` and matched the candidate SHA-256 byte-for-byte. Package state reported `versionName=0.1.5-alpha.1` and `versionCode=6`. Scoped logs recorded two strict-ASTC markers and two honest RT swapchain-presentation markers across launch and Home/resume, with zero fatal markers.

This is exact release installation, package, renderer, and lifecycle evidence. It is not a new signed-build performance run or hands-on phone artistic approval. After publication, with this exact signed release still installed, the owner confirmed that the waterfall audio, haptics, and pause/resume behavior all work correctly on the physical device.

## Itch publication and page

- Butler independently rechecked the guarded candidates and activated Windows build `#1908330` and Android build `#1908331` at user version `0.1.5-alpha.1`.
- An anonymous cache-busting public fetch exposed `the-horde-windows-x64.zip` and `Horde-Lantern-RT-Alpha-0.1.5-alpha.1-Android.apk`, with no public 0.1.4 filename/version remaining.
- Gamepad input metadata saved successfully alongside the existing Keyboard, Mouse, and Touchscreen entries.
- The owner clicked the final itch Save after the reviewed HTML was returned to rendered-editor mode. A fresh anonymous cache-busting fetch verified the `Horde Lantern RT - Showcase Alpha 0.1.5` heading, waterfall/mist/Backbone copy, DRAGON-STUDIO/Pixabay credit, preserved YouTube video, Gamepad metadata, both current downloads, and absence of the stale one-skeleton paragraph.

The update preserves the existing public visibility, video, platform flags, install instructions, links, licence/AI disclosure, and two Butler channels. No GitHub Release was requested or created.

## Audio/haptic classification

`Audio/haptic manual revalidation required: YES - PASSED` - this slice adds a positional waterfall loop, platform playback ownership, and directional dodge that changes listener position. The owner accepted the exact final Windows candidate, including overall audio and the full water/mist/controller path, then confirmed the exact installed Android release's waterfall audio and pause/resume behavior. The owner also reconfirmed that haptics remain good; no haptic routing or cue changed in this release.

## Remaining owner-only item

The independent release-JKS/password recovery copy remains an owner-only checklist item. The working external handoff is not proof of a separate backup.
