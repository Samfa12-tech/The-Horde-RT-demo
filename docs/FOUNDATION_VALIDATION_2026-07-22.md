# Foundation validation, capture, and raygen audit - 2026-07-22

> Post-publication update: Showcase Alpha 0.1.2 is now published. Its exact artifact/device evidence is in `SHOWCASE_ALPHA_0_1_2_RELEASE_VALIDATION_2026-07-22.md`; current scripts also make the 0.1.2 line and Android `versionCode 3` immutable.

## Outcome

The documented pre-gameplay foundation is implemented and live-validated on both target platforms. No water, gameplay expansion, version bump, signing, candidate publication, or itch upload was performed.

The qualifying integrated Full run is `reports/foundation-runs/run-20260722-192303/`. It passed every recorded stage in 492.8 seconds and retained the bounded raygen simplification. The report directory is ignored and contains all nine timestamped stage logs, hashes, validation artifacts, capture manifests, 24 PNGs, `summary.json`, `validation.md`, the Windows pixel/timing comparison, and the shader retention decision.

## Interfaces and boundaries

- Daily host gate: `.\tools\run-foundation-validation.ps1`
- Milestone/device gate: `.\tools\run-foundation-validation.ps1 -Mode Full`
- Non-mutating shader check: `.\tools\compile-raygen.ps1 -Check`
- Windows Debug capture: `HordeLanternRT.exe --capture-showcase <directory>`
- Android capture: Debug-only checkpoint/capture intent exercised through `.\tools\run-android-showcase-validation.ps1 -Capture`

Windows Release rejects `--capture-showcase` with exit code 2 and creates no capture directory. Android Release rejects checkpoint/capture automation. Validation-only ZIP/APK artifacts are created only beneath ignored `reports/`, are named `UNPUBLISHABLE` / `UNSIGNED-DO-NOT-PUBLISH`, and never enter `releases/candidates/`. The foundation gate neither reads nor requires keystore paths or signing passwords.

At qualification time, candidate packaging had no version defaults and protected the then-published 0.1.1 identity. After 0.1.2 publication, `package-alpha.ps1` and `package-signed-alpha.ps1` reject both immutable published 0.1.1/0.1.2 lines and require `versionCode > 3`; `push-alpha-to-itch.ps1` rejects another 0.1.1 or 0.1.2 upload before inspecting Butler or contacting itch.

## Build, test, package, and safety evidence

- Fresh Windows Visual Studio x64 Debug and Release builds: pass.
- All seven CTests in Debug: 7/7 pass, 19.39 seconds.
- All seven CTests in Release: 7/7 pass, 8.26 seconds.
- Android `clean assembleDebug assembleRelease lintRelease`: pass.
- Android validation APK: unsigned by explicit validation-only build input; strict ASTC assets, credits, package identity, portrait orientation, native libraries, 16 KiB ZIP alignment, 0x4000 ELF `LOAD` alignment, and absence of `libc++_shared.so` verified.
- Windows validation ZIP: executable-relative asset tree, both enemy models, required raw textures/audio, release notes, licence manifest, and unpublishable marker verified at ZIP root.
- Negative gates: stale embedded SPIR-V, missing licence/asset, invalid nested ZIP layout, immutable 0.1.1 candidate variants, `versionCode <= 2`, and attempted 0.1.1 upload rejection.
- Shader check compares compiled and embedded SPIR-V words, so checkout line-ending differences do not create false staleness failures.

## Deterministic capture evidence

The shared ordered checkpoint set is opening, skeleton, worst bend, lantern drop, skylight, yellow, blue, red, green, mirror, lich, and finale roof.

Windows Debug captured 12 scene-only 960x540 PNGs from the RT storage image after 12 settling/presented frames per checkpoint, with animation time fixed to 0. WIC encoded the host readback. The manifest records camera/state, GPU, build and shader identity, render/swapchain extents, timing, colour-route normalization, honest RT presentation, and each PNG SHA-256. The RTX 5050 Laptop GPU candidate set recorded 144 samples and a 6.05560 ms overall median.

Android Debug captured the same 12 ordered checkpoints at 75% on `SM-S948B` / Adreno 840 / Android 16. Every checkpoint recorded 12 stable honestly presented frames, fixed animation time 0, a 1080x2235 RT extent, camera/state/build/shader/GPU/colour-route metadata, hidden menu/touch/HUD/diagnostic/developer overlays, PNG dimensions and SHA-256. ADB screenshots are 1440x3120 because they include the physical display surface outside the 1440x2980 swapchain. Home/resume recreation returned to honest RT presentation.

Representative opening, blue-bay, and lich captures were visually inspected. Warm fire remained warm, blue lighting remained blue, the violet lich route remained intact, and no HUD/touch/diagnostic overlay was present. Video and orbit-camera work remain deferred.

## Raygen A/B retention decision

The removed branch was the unreachable stained-glass material path plus its CPU material assignment. The live clear-glass route and open architectural threshold remain unchanged.

| SPIR-V measure | Baseline | Candidate | Change |
|---|---:|---:|---:|
| Bytes | 71,908 | 71,180 | -728 |
| Instructions | 4,025 | 3,978 | -47 |
| Branch operations | 568 | 557 | -11 |
| Loops | 6 | 6 | 0 |
| Selection merges | 216 | 212 | -4 |

All 12 Windows A/B captures were bit-exact: zero pixels differed and maximum channel difference was zero. The warmed Windows median changed from 6.05630 ms to 6.05560 ms (-0.012%), satisfying the 2% non-regression rule.

Comparable-temperature `SM-S948B` 75% checkpoint medians all passed the same 2% rule at thermal status 0:

| Checkpoint | Baseline ms | Candidate ms | Change |
|---|---:|---:|---:|
| Opening | 11.940 | 9.623 | -19.405% |
| Worst bend | 8.326 | 7.909 | -5.008% |
| Skylight | 7.186 | 7.173 | -0.181% |
| Green | 9.106 | 8.691 | -4.557% |
| Lich | 12.415 | 10.526 | -15.215% |

The report-only 100% opening result was 17.228 ms and is not treated as the sustained phone acceptance tier. The shader simplification is retained.

NVIDIA Nsight tools were not installed, so no numerical register-pressure claim is made. The available evidence is the compiler-produced SPIR-V structure, bit-exact Windows images, and actual Windows/phone timings.

## Exact identities and hashes

- Raygen source SHA-256: `9d4e8508fa230a6c826c5eb68b4645e7704f9d07205a2afb9ee89256e4645985`
- Compiled SPIR-V SHA-256: `51ea1ba19dd858be340bb99765f178ea85cd9bfb17ff4a6dbc95b61a888f095f`
- Embedded include SHA-256: `39021ed92adc0abdc477b508b2094ff11f2985ca44df382d3fe62a3d2243d399`
- `ASSET_LICENSES.md` SHA-256: `f731b7118d411d70a2447f416479096e1d1605f3ad83d655440a759644b9a18f`
- Windows Release EXE SHA-256: `1ab1b77c0a49fc093b86f82b653179b0ca6c0285e5a234eec53746b5236e2123`
- Unsigned Android validation APK SHA-256: `308b029eadc82520c52491cabc114e5fe608f27c59032614b97f14d2463e065d`
- Unpublishable Windows validation ZIP SHA-256: `148ebc57fda83264104c22728eafbede84fb49e0d19890572a92617fcabd3a7c`
- Windows capture manifest SHA-256: `33d7f99f4cb88fea94001a6fa165b60db030c3aecd1326da19bfb7679424ed24`
- Android capture manifest SHA-256: `9b5478249542def7edc1963fc682ff6bda8c717f631be93b98665102c4f787cf`
- Installed Android Debug APK SHA-256: `f3bfd8f052c6faf03d8a95a4d81ff0326179522e1d77b25219bd6447c879383b`
- Evidence index `SHA256SUMS.txt` SHA-256: `a591cb3514a618f190462cad87b67007a1433bbbd3086f7529c8cfc5a91ecc1d`

The complete per-asset and per-capture hash inventories are in `source-artifact-hashes.json`, `SHA256SUMS.txt`, and both capture manifests beneath the ignored run directory.

## Hands-on limitation

This run supplied automated lifecycle evidence and live scene captures, but automation cannot certify touch feel or perceived spatial audio. The existing owner hands-on touch/audio/lifecycle pass recorded in `IN_APP_BENCHMARK_ANDROID_VALIDATION_2026-07-18.md` remains the current human evidence; it was not relabelled as part of this automated run.
