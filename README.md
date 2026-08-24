# Horde Lantern RT

Horde Lantern RT is a native Vulkan hardware-ray-tracing technology demo for Android and Windows. Its historical-gothic route moves from a lantern-lit skeleton encounter through coloured-light and mirror studies to a staff-lit lich finale.

- Public alpha: https://samfa12.itch.io/the-horde
- Source repository: https://github.com/Samfa12-tech/The-Horde-RT-demo
- Current package version: `0.1.5-alpha.1`
- Current itch builds: Windows `#1908330`; Android `#1908331`
- Published SHA-256: Windows `631b9f01a4d348e18733c989ebacc9c32ce9005ac9498e49e8757a0a36411166`; Android `1e81238a6e1b0e934c50eb15e80fc8efd39c06f16ca8960c428b22e5f5d5a7f2`
- Primary validated phone: Samsung `SM-S948B` / Adreno 840
- Exact Android release baseline: the stable-key-signed `0.1.5` APK was installed on `SM-S948B`, pulled back byte-for-byte, and matched the published candidate. Strict ASTC and honest RT presentation passed before and after Home/resume; the owner approved waterfall audio and confirmed good haptics plus working pause/resume on that exact installed release.
- Validated Windows GPU: NVIDIA GeForce RTX 5050 Laptop GPU

Showcase Alpha 0.1.5 adds a clear RT roof-water curtain that extinguishes the lantern, a rounded catchment and drain-connected runoff, bounded lich-room ground mist, positional waterfall ambience, Windows Backbone/controller support, controller menu navigation, and collision-safe directional dodge. It preserves the shared deterministic simulation, bounded two-skeleton encounter, timed parry, visible stagger, release-safe benchmark, and Debug-only checkpoint/capture telemetry.

The current development foundation runs Windows and Android gameplay through one deterministic 60 Hz `GameSimulation`. Android input crosses JNI through a coherent snapshot mailbox with independent monotonic swing/parry/reset/retry counters; ordered semantic events drive platform audio and haptics; and one shared adapter preserves the existing `RtSceneFrameInputs` renderer boundary. See `docs/SHARED_SIMULATION_FOUNDATION_2026-08-10.md`.

The current development foundation supports a bounded two-skeleton encounter: stable entities share a skeleton pose/BLAS when their actions match and use at most two pose buckets when they diverge; the lich remains singular. The scene owns ten BLAS and twenty physical TLAS slots, including the independently transformable waterfall-stream instance, with unused slots masked. Historical exact commit `b3428a7` passed the six-checkpoint `SM-S948B` 75% gate, 13 captures, replay, and Home/resume; the owner then reported that two-enemy play felt fine on that installed candidate. That evidence does not automatically prove later source revisions. See `docs/TWO_SKELETON_COMBAT_ANDROID_VALIDATION_2026-08-12.md`.

The animation-owned combat/parry candidate at exact commit `daa5892` passed functional `SM-S948B` checks and recorded a 20.246 ms warm lich median under the then-current gate. The owner found parry timing good. Final exact reconciliation commit `547d89d` publishes Android parry on press-down, animates stagger, retains event-time spatial data, restores positional skeleton hit/fall parity on Windows, and uses bounded Android feedback transport. Its clean Full gate passed strict ASTC, honest presentation, replay, 13 captures, Home/resume, fresh 12/12 Debug and Release CTests, Android build/lint, and exact installed-APK matching. Sustained 75% lich measured 23.069 ms (~43.3 FPS) at GPU thermal power level 2 and is reported honestly in the 30-50 FPS band rather than failed against an arbitrary 20 ms line. The owner gave the final exact candidate a broad audio/haptic pass and explicitly observed its stagger-back/death sequence. No renderer slot, BLAS, pose bucket, or runtime asset was added; Showcase Alpha 0.1.4 later changed release identity and packaging only. See `docs/ANIMATION_COMBAT_PARRY_SLICE_2026-08-13.md`, `docs/CURRENT_DEVELOPMENT_BASELINE_VALIDATION_2026-08-21.md`, and `docs/SHOWCASE_ALPHA_0_1_4_RELEASE_VALIDATION_2026-08-22.md`.

Earlier 23.604 ms lich evidence was a real unmatched hot renderer-foundation run, not an unresolved current-candidate result: the provenance-bound cooled `b3428a7` A/B later measured 19.497/19.268 ms. The 20.246 ms `daa5892` result was a separate warm non-pass under the gate then in force. Both remain useful historical evidence; final current-source evidence is recorded separately.

The APK declares Android 7 / API 24 as its packaging minimum, but hardware support is intentionally much narrower: the device and driver must expose Vulkan acceleration structures, ray-tracing pipeline, ray query, buffer device address, deferred host operations, and the required ASTC formats. Only `SM-S948B` on Android 16 is currently device-certified.

Device compatibility is tracked in [`docs/ANDROID_RT_DEVICE_COMPATIBILITY_RECORD.md`](docs/ANDROID_RT_DEVICE_COMPATIBILITY_RECORD.md). New device results should be recorded there with the exact model code and evidence class: locally tested confirmation, user-reported plus screenshot evidence, user-reported, vendor/SoC inference, or unverified candidate. Hardware marketing claims alone do not establish support; the runtime capability probe and honest RT swapchain presentation are the deciding checks.

## RT or nothing

The demo uses Vulkan acceleration structures, an RT pipeline and shader binding table, `vkCmdTraceRaysKHR`, an RT storage image, and swapchain presentation. The phone-safe shading path uses `rayQueryEXT` inside raygen with pipeline recursion depth 1. Unsupported devices show explicit diagnostics; there is no browser, raster, baked, screen-space, or fake-RT fallback.

`rtScene.presented` becomes true only after an RT-produced frame reaches successful swapchain presentation.

## Showcase alpha contents

- Portrait-first Android presentation and a native Windows desktop build.
- Branded entry, pause, controls, settings, diagnostics, restart, and quit flows.
- A player-facing two-pass `Run benchmark` course ends in a selectable, copyable, exportable text report and automatically archives JSON evidence.
- A `More by Samfa12` menu button opens https://samfa12.com/ in the system browser.
- Persisted 50-100% RT render-resolution scale, defaulting to 100%.
- Android SFX volume and compact-HUD settings; Windows SFX, sensitivity, display-mode, and render-scale settings.
- Collision-safe starting chamber and material gallery, a leashed skeleton encounter, and a three-turn moving-shadow corridor.
- A roof-water drench that gutters and drops the lantern, clear RT reflection/refraction, a rounded catchment and drain-connected runnel, blue skylight chamber, four coloured-light bays, wet stone, fog, and a single-bounce hero mirror.
- Low depth-clipped blue-grey ritual mist in the lich room, bounded to preserve the enemy, sword, and opening-roof sightlines.
- A low-poly held torch with wooden shaft, iron cage, and layered emissive flame volumes.
- A complete camera-relative RT player body with a layered travelling coat, shoulder/collar/belt/strap/tail detail, bevelled articulated limbs, two-bone-IK arms, pelvis sway, torso counter-twist, knee bend, foot lift, heel-to-toe walking, held torch/sword, and wall-aware prop retraction.
- A Hotstrike Studio skeleton derivative followed sequentially by a CC0 Meshy placeholder lich with emissive staff/eyes, charge electricity, spatial audio, three-hit combat, death animation, a physical sliding roof, a moonlight-to-dawn RT reveal, and a contextual ending with continue/restart/quit.
- Three player vitality per encounter, one-second post-hit invulnerability, an RT-visible fatal hold, and death-menu retry at the safe opening or mirror checkpoint.
- Seventeen FilmCow UI, sword, movement, skeleton, and lich reaction/attack WAV cues, plus one positional DRAGON-STUDIO/Pixabay waterfall loop.
- A permanent post-lich RT Lab on Windows and Android. It pauses gameplay while RT presentation continues and tunes waterfall width, finale roof/dawn, fog, four isolated light groups, and Lean/Authored/Max workloads. Unlock progress persists; tuning resets to authored values on route/process restart. Render scale and RT-water quality remain in Settings.

The signed Android APK contains both enemy GLBs, strict ASTC KTX2 environment and lich textures, seventeen FilmCow WAVs, the Pixabay waterfall loop, four ABI libraries, and launcher assets. The Windows ZIP contains `HordeLanternRT.exe`, an executable-relative `assets/` tree including the same audio, release notes, controls, and `ASSET_LICENSES.md`.

The staged Meshy sword LOD and torch study are not used by the runtime or included in either download. Their metadata is retained for later measured GLB/PBR work.

## Controls

### Android

- Left-side drag: walk and strafe
- Right-side drag: 360-degree look
- `SWING`: sword attack
- `PARRY`: timed skeleton-strike parry
- Android Back: pause/resume

### Windows

- `WASD`: walk and strafe
- Left mouse drag: look
- Right mouse or `Space`: sword attack
- `Q`: timed skeleton-strike parry
- `Esc`: pause/resume
- `R`: restart route
- `F1`: controls
- `F2`: RT diagnostics
- `F3`: live non-pausing developer overlay (Debug builds only)
- `Alt+Enter`: fullscreen/windowed
- Backbone/controller: left stick move, right stick look, RT attack, LT parry, B/Circle directional dodge, D-pad menus, A select, Menu/Start pause

At zero vitality, `RETRY ENCOUNTER` restores the current encounter, `RESTART ROUTE` returns to the opening, and Back/`Esc` cannot resume a dead player.

## Current validation

The published Showcase Alpha 0.1.5 packages contain the accepted water/mist/controller/audio slice. The exact signed APK is byte-matched and lifecycle-smoked on `SM-S948B`; the exact Windows ZIP launched from a clean extraction and honestly presented the RT scene. See `docs/SHOWCASE_ALPHA_0_1_5_RELEASE_VALIDATION_2026-08-23.md` and `docs/RT_WATERFALL_LICH_MIST_VALIDATION_2026-08-23.md`.

The player-vitality/retry slice is host-validated by clean Windows Debug and Release builds, all seven CTests in both configurations, twelve fixed RT captures, clean Android Debug and unsigned Release builds across all four configured ABIs, and `lintRelease`. On `SM-S948B`, real skeleton/lich damage, death UI, native opening/mirror retry, 3/3 restoration, the complete showcase automation route, captures, and Home/resume RT presentation are also validated. The earlier owner report of perceived haptics was corrected on 2026-08-01 because haptics had not actually been checked. The audit revision adds direct `Vibrator` effects, an enabled settings toggle/preview, and view-feedback fallback; the exact installed Debug APK subsequently produced completed preview, Swing, damage, and fatal effects through the live encounter, and the owner confirmed the revised haptic was physically felt. See `docs/PLAYER_VITALITY_RETRY_SLICE_2026-07-31.md` and `docs/ANDROID_RT_DEVICE_COMPATIBILITY_RECORD.md`.

For the merged shared-simulation and ordered-event migration, the owner separately reconfirmed basic controls, audible audio, and perceived haptics on `SM-S948B`. That older report lacked exact APK provenance; the later exact two-skeleton candidate now has both automated performance/presentation evidence and a broad owner-reported hands-on pass.

Render scaling was verified at:

| Scale | Android internal RT extent | Result |
|---:|---:|---|
| 100% | `1440x2980` | Full-extent/image check passed; exact two-skeleton candidate opening measured 18.674 ms median of three 120-frame averages, report-only |
| 75% | `1080x2235` | Recommended tier; historical exact two-skeleton run measured 10.589 / 12.139 / 9.246 / 8.888 / 11.060 / 19.735 ms across six checkpoints at thermal status 0; these are workload measurements, not a universal pass/fail boundary |
| 50% | `720x1490` | Initial-alpha opening diagnostic recorded 163.12 FPS / 6.13 ms; retained as historical evidence, not a complete-route baseline |

Windows Release was launched from a clean extraction using only its packaged assets. It reported `RayTracingPipeline`, `RT scene presented: yes`, and live resolution/FPS/frame-time diagnostics. The 100% and 75% render targets were verified at `982x628` and `737x471` respectively.

The in-app benchmark is live-validated on both targets. Windows Release completed 26/26 waypoints with honest RT presentation on every measured frame, copied the full report, and wrote parseable timestamped text/JSON. On `SM-S948B`, Android completed the same 1,838-frame measured lap at 100% and 75%, passed Back/Home recovery and its 1.7-font report layout, and opened the document picker with the expected export filename.

See:

- `docs/DOCUMENTATION_CHECKPOINT_2026-07-17.md`
- `docs/HORDE_SHOWCASE_WINDOWS_VALIDATION_2026-07-16.md`
- `docs/HORDE_SHOWCASE_ANDROID_VALIDATION_2026-07-17.md`
- `docs/ANDROID_SHOWCASE_AUTOMATION_2026-07-17.md`
- `docs/ANDROID_SHOWCASE_AUTOMATION_VALIDATION_2026-07-17.md`
- `docs/IN_APP_BENCHMARK_WINDOWS_VALIDATION_2026-07-17.md`
- `docs/IN_APP_BENCHMARK_ANDROID_VALIDATION_2026-07-18.md`
- `docs/FOUNDATION_VALIDATION_2026-07-22.md`
- `docs/SHOWCASE_ALPHA_0_1_2_RELEASE_VALIDATION_2026-07-22.md`
- `docs/SHOWCASE_ALPHA_RELEASE_NOTES_2026-07-22.md`
- Historical 0.1.0 readiness and validation records remain under `docs/`.

## Build and run

### Windows

```powershell
$cmake = "C:\Program Files (x86)\Microsoft Visual Studio\2022\BuildTools\Common7\IDE\CommonExtensions\Microsoft\CMake\CMake\bin\cmake.exe"
& $cmake -S . -B build
& $cmake --build build --config Debug --target horde_rt_diagnostic_window
.\build\Debug\HordeLanternRT.exe
```

The standalone capability probe remains available as target `horde_rt_capability_probe`.

Additive Visual Studio configure/build/test presets are also available:

```powershell
cmake --preset windows-x64-debug
cmake --build --preset windows-x64-debug
ctest --preset windows-x64-debug
```

### Android development build

```powershell
cd android
.\gradlew.bat assembleDebug installDebug --console=plain
adb shell am start -n com.samfa12.hordelanternrt.debug/com.samfa12.hordelanternrt.MainActivity
adb logcat -d -s HordeRtProbeBridge HordeLanternAudio AndroidRuntime
```

Look for:

- `PBR material encoding: ASTC 6x6 diffuse/ARM + ASTC 4x4 normal (KTX2)`
- `RT frame reached Android swapchain presentation.`
- `SFX loaded` IDs 1 through 17; the waterfall loop is separately owned by Android `MediaPlayer`

Debug builds expose reports with `adb shell run-as`; release builds deliberately do not set `android:debuggable`.

For repeatable Android checkpoint timing and deterministic route-collision replay, connect one authorised device and run:

```powershell
.\tools\run-android-showcase-validation.ps1
```

The debug-only runner collects a timestamped evidence bundle without changing the public release path. See `docs/ANDROID_SHOWCASE_AUTOMATION_2026-07-17.md`.

For a matched Debug-only RT Lab workload comparison on the same installed APK:

```powershell
.\tools\run-android-showcase-validation.ps1 -Mode Benchmark -Scale 75 -Checkpoints @() -GpuTiming Enabled -RtLabWorkloadComparison -SkipBuild -SkipInstall
```

This compares Lean/Authored/Max at the waterfall, skylight, and finale without persisting an unlock. See `docs/RT_LAB_VALIDATION_2026-08-24.md`.

For focused real-enemy vitality, death-menu, and encounter-retry evidence on the installed Debug APK, run:

```powershell
.\tools\run-android-vitality-validation.ps1 -SkipInstall
```

This Debug-only runner preserves screenshots, Android UI hierarchies, scoped logs, hashes, and a structured pass/fail summary without injecting player damage. It does not itself replace hands-on touch or perceived-haptics checks; the separately qualified owner report is recorded in the vitality and device-compatibility documents.

## Foundation validation and deterministic captures

Run the daily host gate from the repository root:

```powershell
.\tools\run-foundation-validation.ps1
```

It performs fresh Windows Debug/Release builds, all configured CTests in both configurations, deterministic Windows captures, clean Android Debug/Release builds, Release lint, non-mutating shader-staleness checks, validation package/layout and asset/licence checks, release-identity safeguards, and evidence hashing.

With the authorised `SM-S948B` connected, run the milestone gate:

```powershell
.\tools\run-foundation-validation.ps1 -Mode Full
```

`Full` adds the six-checkpoint sustained 75% timing/replay report, a separately labelled 100% opening result, Home/resume evidence, and all 13 deterministic Android captures. The current Vulkan-enabled host suites contain 13 CTests per Debug/Release configuration; the portable GitHub Actions lane runs its separate 9-test Vulkan-disabled subset. Timing output uses descriptive 16.667/20.000/33.333 ms reference lines (approximately 60/50/30 FPS); crossing one is reported rather than treated as an automatic product failure. Exact matched regressions above 15% require investigation. Both modes write timestamped logs, manifests, hashes, PNGs, exact installed-APK/source/shader provenance, `summary.json`, and `validation.md` beneath ignored `reports/foundation-runs/`. Their ZIP/APK artifacts are marked unpublishable, stay out of `releases/candidates/`, are not signed, and do not read or require release-key secrets.

Check raygen staleness without modifying the embedded include:

```powershell
.\tools\compile-raygen.ps1 -Check
```

Windows Debug also supports `HordeLanternRT.exe --capture-showcase <directory>`; Windows Release and Android Release reject capture/checkpoint automation. Video and orbit-camera capture remain deferred.

The lightweight `.github/workflows/shared-simulation-host.yml` lane configures with `HORDE_RT_BUILD_VULKAN_TARGETS=OFF` and runs portable gameplay tests. It catches deterministic simulation regressions but does not establish Vulkan capability, RT swapchain presentation, device thermals, touch feel, audio perception, or haptics.

## Package and publish

Create a release key once, outside Git:

```powershell
.\tools\create-android-release-key.ps1
```

For a signed rebuild:

```powershell
.\tools\package-signed-alpha.ps1 -KeyStorePath '<outside-repo path>' -Version '<next-version>' -VersionCode <next-version-code>
.\tools\push-alpha-to-itch.ps1 -Version '<next-version>' -Channels Both
```

The packaging and push scripts securely prompt for signing secrets, reject debug/unsigned Android candidates, verify hashes, and keep Windows and Android on separate itch channels. Add `-ConfirmPush` only after the preflight passes.

There are no packaging version defaults: candidate scripts require explicit `Version` and `VersionCode`, reject the immutable published `0.1.1` through `0.1.5` lines, and require Android `versionCode > 6`. One shared policy helper owns these rules for packaging, signing, and itch upload. A future public build must first bump CMake/Windows/Android/package metadata, provide matching release notes, and update the guarded candidate hashes.

Android native code is linked for 16 KiB page compatibility. The release uses a static C++ runtime, 16 KiB ELF `LOAD` alignment, and AGP 8.7.2 APK alignment; `package-alpha.ps1` rejects candidates that fail either APK or ELF verification or reintroduce `libc++_shared.so` from the r26 NDK.

Never commit a keystore, signing properties, credentials, APK, or generated candidate directory. Losing the release JKS or its passwords prevents compatible Android updates.

## Asset and licence policy

All shipped third-party assets are recorded in `ASSET_LICENSES.md`. The current release includes:

- Five Poly Haven environment sets under CC0.
- Free Stylized Skeleton by Hotstrike Studio, modified through Meshy; the release uses the conservative Meshy Free-plan CC BY 4.0 attribution route.
- The active placeholder lich created and animated with Meshy under CC0; the retained licence screenshot and hash are recorded in `ASSET_LICENSES.md`.
- A bounded FilmCow Recorded SFX subset under FilmCow's custom royalty-free project-use terms.
- “Water Dripping” by DRAGON-STUDIO under the Pixabay Content License, processed into the shipped positional loop.

Do not redistribute source assets as standalone asset packs. Preserve the public Hotstrike/Meshy credit and the full licence manifest with releases.

## Architecture and invariants

- Android entrypoint: `android/app/src/main/java/com/samfa12/hordelanternrt/MainActivity.java`
- Android native bridge: `android/app/src/main/cpp/android_probe_bridge.cpp`
- Windows presentation: `src/platform/windows/DiagnosticWindow.cpp`
- Shared scene: `src/vulkan/raytracing/PresentableTinyRtScene.cpp`
- Shared gameplay: `src/gameplay/simulation/GameSimulation.cpp`
- Android input mailbox: `src/gameplay/simulation/InputMailbox.h`
- Shared simulation-to-renderer adapter: `src/vulkan/raytracing/SimulationFrameAdapter.cpp`
- Raygen source: `shaders/raytracing/minimal.rgen`
- Embedded raygen SPIR-V: `src/vulkan/raytracing/MinimalRayGenShader.inc`

After raygen edits, run `tools/compile-raygen.ps1`; use `tools/compile-raygen.ps1 -Check` in validation when mutation is not allowed. Keep one frame in flight while the held-prop TLAS uses host-written instance data. Preserve presentation-format-driven red/blue swapping on the 100% raw-copy path so warm fire does not render cyan.

## Showcase route status

The established route is Windows- and Android-device-validated: lower body and lantern drop, zig-zag shadows, blue skylight, bay-selected coloured torches, an open framed threshold, one hero mirror, and a sequential staff-lit lich finale. The newer body-and-ending slice adds the layered animated player, post-death sliding skylight, warm dawn reveal, and contextual ending; its host and exact-APK `SM-S948B` evidence are recorded in `docs/PLAYER_BODY_AND_FINALE_SLICE_2026-07-31.md`.

Showcase Alpha 0.1.4 animates, refits, and renders up to two simultaneous skeletons with at most two skeleton pose buckets, nine BLAS, and nineteen physical TLAS slots; the later lich route remains singular. No third enemy is permitted without a separately measured design.

The 75% setting is the sustained phone recommendation. Preserve real RT at the documented quality tier; reduce bounded effect area or ray cost before considering any broader feature expansion.
