# Horde Lantern RT

Horde Lantern RT is a native Vulkan hardware-ray-tracing technology demo for Android and Windows. Its historical-gothic route moves from a lantern-lit skeleton encounter through coloured-light and mirror studies to a staff-lit lich finale.

- Public alpha: https://samfa12.itch.io/the-horde
- Source repository: https://github.com/Samfa12-tech/The-Horde-RT-demo
- Current development candidate: `1.6.1` / Android `versionCode 9` (not published)
- Latest published itch release: `1.6.0` / Android `versionCode 8`; Windows `#1931949`; Android `#1931951`
- Published SHA-256: Windows `7b0dcf24b4a47771a9c3a27cbc52e3899c87781109afcef20f7a9a8472411d77`; Android `52a64255ad5dec82cc866fb2ea3545be498ca06c73a789019be851c77e5d6c48`
- Primary validated phone: Samsung `SM-S948B` / Adreno 840
- Exact Android device baseline: the published stable-key-signed `1.6.0` APK is now installed and pulled back byte-for-byte on `SM-S948B`, with strict ASTC, honest RT presentation, and Home/resume smoke passing. The exact signed release smoke is functional/presentation evidence only; deterministic feature timing and owner feel remain tied to the accepted final Debug runtime.
- Validated Windows GPU: NVIDIA GeForce RTX 5050 Laptop GPU

Showcase Alpha 1.6.0 publishes the Fire/PBR/reward-lantern programme: imported PBR sword, torch, chest, lantern, and player assets; deterministic world-space fire with coherent coloured light and reflections; reusable bounded dielectric glass; shared held-item sockets; the post-lich chest/reward interaction; fixed-step physical lantern swing; the downward/upward sword combo; the skinned-player/IK foundation; and explicit GitHub Release update alerts. Owner review keeps block arms in normal gameplay until the skinned hands/gauntlets are ready in every scenario, and arm/body shadow/reflection polish remains deferred.

The active source identity is the unpublished `1.6.1` / Android `versionCode 9` development candidate. It has no published artifact, device-performance, or owner-feel claim. The latest published itch release remains the exact `1.6.0` / `versionCode 8` package recorded below.

The 1.5.2 water path removes the waterfall-only lighting approximation and hidden second glossy bounce on submerged cobble. Refracted and reflected opaque hits use terminal shared active-light/material/shadow logic, transparent candidates are explicitly filtered, path distance is accumulated, interface highlights use the same visible lights, and the directional moon traverses physical roof/player geometry. The ray budget remains finite and water-on-water paths do not recurse. Deterministic Windows, fresh Host, and exact `SM-S948B` Debug evidence pass; the owner accepted the moving Windows result. Repeated phone medians are approximately 27.8 ms at `lantern-drop`, 19.0 ms at `skylight`, and 30.8 ms at `lich`, with the bounded real-light cost retained rather than hidden through a quality or resolution reduction. See `docs/WATER_TRANSMISSION_SHADOW_VALIDATION_2026-08-24.md` and `docs/SHOWCASE_ALPHA_1_5_2_RELEASE_VALIDATION_2026-08-25.md`.

Exact clean Host, Windows feature-capture, and `SM-S948B` 75%/100% evidence for 1.6.0 is recorded in `docs/FIRE_PBR_REWARD_LANTERN_PLAYER_UPGRADE_VALIDATION_2026-08-30.md`, `docs/TASK_9_OWNER_CANDIDATE_VALIDATION_2026-08-30.md`, and `docs/SHOWCASE_ALPHA_1_6_0_RELEASE_VALIDATION_2026-08-30.md`. Instrumented Debug feature medians are 53.481-91.176 ms at 75% and 83.954-154.484 ms at 100%, so this is image/correctness evidence rather than a performance claim; Mobile glass remains the main measured risk. The owner accepted the final natural reward route, two-second unlock cue, chest guidance, audio, and haptics on the exact installed Debug candidate. The signed public APK has now also passed exact-device install/hash, strict-ASTC, honest-presentation, Home/resume, and short route smoke; sustained Release timing and owner-feel evidence remain separate.

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
- Imported PBR sword and torch assets, with a deterministic world-space fire emitter whose RT-visible core, depth-clipped volume, coloured direct light, reflections, and movement share one state.
- A collision-bearing Gothic reward chest and imported reward lantern with closed transmissive glass, warm internal light, shared interaction/held sockets, raise/lower poses, and fixed-step physical swing.
- A reusable skinned RT player/animation/IK/socket foundation plus owner-selected block arms for current player-facing sword, torch, and lantern presentation.
- A Hotstrike Studio skeleton derivative followed sequentially by a CC0 Meshy placeholder lich with emissive staff/eyes, charge electricity, spatial audio, three-hit combat, death animation, a physical sliding roof, a moonlight-to-dawn RT reveal, and a contextual ending with continue/restart/quit.
- Three player vitality per encounter, one-second post-hit invulnerability, an RT-visible fatal hold, and death-menu retry at the safe opening or mirror checkpoint.
- Seventeen FilmCow UI, sword, movement, skeleton, and lich reaction/attack WAV cues, plus one positional DRAGON-STUDIO/Pixabay waterfall loop.
- A permanent post-lich RT Lab on Windows and Android. It pauses gameplay while RT presentation continues and tunes the waterfall's real cross-lane geometry width, finale roof/dawn, fog, four isolated light groups, and Lean/Authored/Max workloads. Unlock progress persists; tuning resets to authored values on route/process restart. Render scale and RT-water quality remain in Settings.
- Current development prevents the completed-finale poll from replacing an already-open Android RT Lab. Windows RT Lab wheel/page scrolling and slider repaint now survive opening from pause and repeated down/up traversal. The waterfall control scales the visible curtain span rather than its millimetre-thin depth. RED/GREEN host coverage, fresh Debug/Release/Android builds, and owner Windows acceptance pass; exact fixed-APK phone confirmation is pending.

The signed Android APK contains both enemy GLBs, strict ASTC KTX2 environment and lich textures, seventeen FilmCow WAVs, the Pixabay waterfall loop, four ABI libraries, and launcher assets. The Windows ZIP contains `HordeLanternRT.exe`, an executable-relative `assets/` tree including the same audio, release notes, controls, and `ASSET_LICENSES.md`.

The reviewed sword source and processed runtime LOD now prove the generic GLB/PBR route, and the torch, chest, lantern, and player study use the same audited contract. Source/runtime separation, provenance, hashes, budgets, and commercial-safe licence evidence remain recorded; no generated credential or expiring URL is packaged.

## Controls

### Android

- Left-side drag: walk and strafe
- Right-side drag: 360-degree look
- `SWING`: sword attack
- `PARRY`: timed skeleton-strike parry
- Contextual `INTERACT`: open the unlocked reward chest, then claim its lantern after the lid finishes opening
- Contextual `RAISE` / `LOWER`: change the carried reward-lantern pose
- Android Back: pause/resume

### Windows

- `WASD`: walk and strafe
- Left mouse drag: look
- Right mouse or `Space`: sword attack
- `Q`: timed skeleton-strike parry
- `E`: interact with the unlocked reward chest / available lantern
- `F`: raise or lower the claimed reward lantern
- `Esc`: pause/resume
- `R`: restart route
- `F1`: controls
- `F2`: RT diagnostics
- `F3`: live non-pausing developer overlay (Debug builds only)
- `Alt+Enter`: fullscreen/windowed
- Backbone/controller: left stick move, right stick look, RT attack, LT parry, B/Circle directional dodge, gameplay A interact, gameplay Y raise/lower, D-pad menus, A menu select, Menu/Start pause

At zero vitality, `RETRY ENCOUNTER` restores the current encounter, `RESTART ROUTE` returns to the opening, and Back/`Esc` cannot resume a dead player.

## Current validation

Showcase Alpha 1.6.0 is the current published release. Exact source Host run `run-20260831-131431` passed the complete seven-stage gate with 31/31 Debug and 31/31 Release tests plus 13 deterministic Windows captures. The exact published Windows ZIP launched from an isolated extraction, selected `RayTracingPipeline`, honestly presented the RT scene, and exited cleanly. The exact signed Android APK passed certificate, identity, package-layout, native-library, asset/licence, lint, and compatibility guards, then passed exact-device install/pullback, strict ASTC, honest RT presentation, Home/resume, and short route smoke on `SM-S948B`; sustained Release timing and owner-feel evidence remain separate. See `docs/SHOWCASE_ALPHA_1_6_0_RELEASE_VALIDATION_2026-08-30.md`.

The Fire/PBR/reward-lantern runtime passed clean-source Windows Debug and Release 31/31 Vulkan-enabled CTests, 13 standard plus 11 feature Windows captures, isolated packaged Windows launch, Android Debug/unsigned Release/lint, asset/licence/package gates, and exact `SM-S948B` 75%/100% RT/ASTC/capture/Home-resume validation. The installed Debug APK SHA-256 is `0b5a59b6e41d2c4d717eff885aaa310b7f5f1512002f6a89cb77e5989ab7edd3`; the owner accepted phone feel, the natural chest/reward route, the two-second unlock timing, audio, and haptics. The feature path is materially slower than the earlier published route and finite Mobile glass-budget terminals remain, both recorded without reducing quality or scale.

The historical Showcase Alpha 0.1.5 packages introduced the accepted water/mist/controller/audio slice. That exact signed APK remains the latest public package installed, byte-matched, and lifecycle-smoked on `SM-S948B`. See `docs/SHOWCASE_ALPHA_0_1_5_RELEASE_VALIDATION_2026-08-23.md` and `docs/RT_WATERFALL_LICH_MIST_VALIDATION_2026-08-23.md`.

The player-vitality/retry slice is host-validated by clean Windows Debug and Release builds, all seven CTests in both configurations, twelve fixed RT captures, clean Android Debug and unsigned Release builds across all four configured ABIs, and `lintRelease`. On `SM-S948B`, real skeleton/lich damage, death UI, native opening/mirror retry, 3/3 restoration, the complete showcase automation route, captures, and Home/resume RT presentation are also validated. The earlier owner report of perceived haptics was corrected on 2026-08-01 because haptics had not actually been checked. The audit revision adds direct `Vibrator` effects, an enabled settings toggle/preview, and view-feedback fallback; the exact installed Debug APK subsequently produced completed preview, Swing, damage, and fatal effects through the live encounter, and the owner confirmed the revised haptic was physically felt. See `docs/PLAYER_VITALITY_RETRY_SLICE_2026-07-31.md` and `docs/ANDROID_RT_DEVICE_COMPATIBILITY_RECORD.md`.

For the merged shared-simulation and ordered-event migration, the owner separately reconfirmed basic controls, audible audio, and perceived haptics on `SM-S948B`. That older report lacked exact APK provenance; the later exact two-skeleton candidate now has both automated performance/presentation evidence and a broad owner-reported hands-on pass.

Render scaling was verified at:

| Scale | Android internal RT extent | Result |
|---:|---:|---|
| 100% | `1440x2980` | Full-extent/image check passed; exact two-skeleton candidate opening measured 18.674 ms median of three 120-frame averages, report-only |
| 75% | `1080x2235` | Recommended tier; historical exact two-skeleton run measured 10.589 / 12.139 / 9.246 / 8.888 / 11.060 / 19.735 ms across six checkpoints at thermal status 0; these are workload measurements, not a universal pass/fail boundary |
| 50% | `720x1490` | Initial-alpha opening diagnostic recorded 163.12 FPS / 6.13 ms; retained as historical evidence, not a complete-route baseline |

The exact published 1.5.2 Windows ZIP was launched from a clean extraction using only packaged assets. It reported file/product version `1.5.2`, selected `RayTracingPipeline`, dispatched `1232x803`, set `RT scene presented: yes`, and exited cleanly. Earlier scale validation verified the 100% and 75% render targets at `982x628` and `737x471` respectively.

The in-app benchmark is live-validated on both targets. Windows Release completed 26/26 waypoints with honest RT presentation on every measured frame, copied the full report, and wrote parseable timestamped text/JSON. On `SM-S948B`, Android completed the same 1,838-frame measured lap at 100% and 75%, passed Back/Home recovery and its 1.7-font report layout, and opened the document picker with the expected export filename.

See:

- `docs/SHOWCASE_ALPHA_1_5_2_RELEASE_VALIDATION_2026-08-25.md`
- `docs/SHOWCASE_ALPHA_1_5_2_RELEASE_NOTES_2026-08-25.md`
- `docs/WATER_TRANSMISSION_SHADOW_VALIDATION_2026-08-24.md`
- `docs/RT_LAB_VALIDATION_2026-08-24.md`
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
.\tools\push-alpha-to-itch.ps1 -Version '<next-version>' -VersionCode <next-version-code> -Channels Both
```

The packaging and push scripts securely prompt for signing secrets, reject debug/unsigned Android candidates, verify hashes, and keep Windows and Android on separate itch channels. Add `-ConfirmPush` only after the preflight passes.

There are no packaging version defaults: candidate scripts require explicit `Version` and `VersionCode`, then require both to match the root `VERSION` and Android version-code map. They reject the immutable published `0.1.1` through `0.1.5`, `1.5.2`, and `1.6.0` lines. One shared policy helper owns these rules for packaging, signing, and itch upload. A future public build must first provide matching release notes and update the guarded candidate hashes.

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

The established route is Windows- and Android-device-validated: lower body and the original torch failure at the historical `lantern-drop` checkpoint, zig-zag shadows, blue skylight, bay-selected coloured torches, an open framed threshold, one hero mirror, and a sequential staff-lit lich finale. The newer body-and-ending slice adds the layered animated player, post-death sliding skylight, warm dawn reveal, and contextual ending; its host and exact-APK `SM-S948B` evidence are recorded in `docs/PLAYER_BODY_AND_FINALE_SLICE_2026-07-31.md`.

Showcase Alpha 1.5.2 retains the two-skeleton system first published in 0.1.4: at most two simultaneous skeletons, two skeleton pose buckets, nine character/environment BLAS plus the dedicated water geometry route, and twenty physical TLAS slots; the later lich route remains singular. No third enemy is permitted without a separately measured design.

The 75% setting is the sustained phone recommendation. Preserve real RT at the documented quality tier; reduce bounded effect area or ray cost before considering any broader feature expansion.
