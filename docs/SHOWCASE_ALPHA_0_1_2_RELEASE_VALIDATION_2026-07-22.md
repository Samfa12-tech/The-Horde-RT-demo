# Showcase Alpha 0.1.2 release validation - 2026-07-22

## Published identity

- Display: **Horde Lantern RT - Showcase Alpha 0.1.2**
- Package version: `0.1.2-alpha.1`
- Android package/version: `com.samfa12.hordelanternrt`, `versionCode 3`
- Candidate-source commit: `9a82e59` (`Release foundation validation and 0.1.2 candidate`)
- Windows itch channel: `samfa12/the-horde:windows-x64`, upload `#18339908`, active build `#1815416`
- Android itch channel: `samfa12/the-horde:android`, upload `#18341739`, active build `#1815417`
- Signing certificate SHA-256: `8245277a11bca5576f116724507f799d6f4c178ce5fbb7e3981415c9e6b3c245`

## Exact published artifacts

| Artifact | SHA-256 |
|---|---|
| `Horde-Lantern-RT-Alpha-0.1.2-alpha.1-Windows-x64.zip` | `c3688bd8483ec13af2ae8e1d2e1708e17c840b33232822b795ef3283c6b7dee6` |
| `Horde-Lantern-RT-Alpha-0.1.2-alpha.1-Android.apk` | `c9a26c79d4881230d2fd18b3bcbe4c1543032bccea760ade581f9a9fdcbf72b6` |
| `Horde-Lantern-RT-Alpha-0.1.2-alpha.1-Android-preview-debug-signed.apk` | `89fe5e231bbb72a00d739a234b024d075c71dabb80a3ae68038402ced2ad88d8` |

The guarded preflight verified the hash manifest, stable release certificate, non-debug APK, 16 KiB APK alignment, static C++ runtime, native-library presence, and Windows archive root before contacting Butler. Post-publication negative checks also prove that 0.1.1/0.1.2 variants and Android `versionCode <= 3` fail before packaging or Butler contact. Only the two release artifacts above were uploaded. Butler then reported both new builds ready at user version `0.1.2-alpha.1`.

## Build and host evidence

The 0.1.2 source passed the integrated host stages in `reports/foundation-runs/run-20260722-200304/`: shader staleness, negative safety gates, fresh Windows Debug/Release builds, all seven CTests in both configurations, 12 Windows captures, Android clean Debug/Release builds plus Release lint, and validation package/licence checks. That run stopped only because no authorised phone was visible at its device stage.

The signed packaging run rebuilt Windows Release, passed all seven Release CTests, built Android Debug and signed Release, passed Release lint, verified package contents and version surfaces, and produced the exact hashes above. The existing foundation A/B record retained the raygen simplification: 728 fewer SPIR-V bytes, 47 fewer instructions, 11 fewer branch operations, four fewer selection merges, bit-exact Windows captures, unchanged Windows timing, and improved comparable-temperature phone measurements.

## Exact Windows package smoke

The published ZIP was extracted to a fresh ignored directory and launched using only packaged files. `HordeLanternRT.exe` reported file/product version `0.1.2-alpha.1`, selected `RayTracingPipeline` on the NVIDIA GeForce RTX 5050 Laptop GPU, dispatched `1232x803`, and set `rtScene.presented=true` only after a real RT-produced frame reached successful swapchain presentation.

Evidence: `reports/release-smoke/windows-20260722-205357/`.

## Exact signed Android release smoke

The exact signed APK installed on Samsung `SM-S948B` / Android 16 as `versionCode 3` / `versionName 0.1.2-alpha.1`. It selected strict ASTC 6x6 diffuse/ARM, ASTC 4x4 normals, and strict ASTC 6x6 lich textures; loaded all 17 SoundPool cues; and logged honest RT swapchain presentation. Home/resume recreated the surface and produced a second presentation marker. A capture/checkpoint intent launched from a stopped app was explicitly rejected because Release is non-debuggable, while normal RT presentation continued. No fatal Java, native, or renderer marker was present.

Evidence: `reports/release-smoke/android-signed-20260722-205356/`.

## Connected Debug checkpoint/replay/capture gate

The final `SM-S948B` Debug gate is `reports/android-showcase-runs/run-20260722-205558/`. It completed all five required 75% three-window checkpoints, the 13-waypoint replay, all 12 scene-only captures, and Home/resume with honest presentation. Thermal status remained 0 at 33.2-33.7 C.

| Checkpoint | 75% median of three window averages |
|---|---:|
| Opening | 13.929 ms |
| Worst bend | 11.210 ms |
| Skylight | 10.288 ms |
| Green | 12.103 ms |
| Lich | 13.845 ms |

The report-only 100% opening result was 23.353 ms. The release remains qualified at the recommended 75% tier; the 100% result is not a sustained-performance promise.

## Limits and follow-up

This publication did not add water, simultaneous enemies, broader AI, block/dodge, or the staged textured sword. The one-active-skinned-enemy limit, `vkCmdTraceRaysKHR`, `rayQueryEXT`, one frame in flight, strict ASTC routing, BGRA colour correction, and honest `rtScene.presented` semantics remain unchanged.

The automated release smoke confirms audio assets loaded and runtime/lifecycle behavior, but it does not replace the existing owner hands-on evidence for touch feel or perceived spatial audio. The 0.1.2 release line and Android `versionCode 3` are now immutable; a future candidate must use a new version and `versionCode > 3`.