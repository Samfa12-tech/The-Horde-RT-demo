# Horde Lantern RT

Horde Lantern RT is a native Vulkan hardware-ray-tracing technology demo for Android and Windows. Its historical-gothic route moves from a lantern-lit skeleton encounter through coloured-light and mirror studies to a staff-lit lich finale.

- Public alpha: https://samfa12.itch.io/the-horde
- Source repository: https://github.com/Samfa12-tech/The-Horde-RT-demo
- Published Showcase Alpha: `1.6.1` / Android `versionCode 9`; Windows itch `#2055201`; Android itch `#2055202`
- Public update announcement and frozen packages: [GitHub v1.6.1](https://github.com/Samfa12-tech/The-Horde-RT-demo/releases/tag/v1.6.1) (non-draft prerelease)
- Frozen GitHub ZIP SHA-256: `3e1cdca75d78e02dbc3bb48553b1db68b6b4bf3e459a784e2be5473ad198c2cf`; signed APK: `bc5c7ce3c755c16ec39e2c16fa9eae01c31983c7393e645f881c5bcc9e6a346c`
- Primary validated phone: Samsung `SM-S948B` / Adreno 840
- Exact Android candidate smoke: stable-key-signed `1.6.1` installed over observed production `1.6.0` without clearing data on `SM-S948B`; installed pullback matches, saved settings survive, and strict ASTC, honest RT presentation and Home/resume pass. This is functional/presentation evidence, not a new sustained-performance run or S24/S25 certification. The earlier published `1.6.0` smoke remains historical evidence.
- Validated Windows GPU: NVIDIA GeForce RTX 5050 Laptop GPU

Showcase Alpha 1.6.1 publishes the engineering pass on the 1.6.0 Fire/PBR/reward-lantern foundation. It adds dedicated modelled native-RT viewmodel arms and an independently owned world body, coherent player semantics, fixed Shipping/Diagnostic and Mobile/High shader variants, adaptive music, separate audio controls, consent-based in-game reporting and bounded resource/lifecycle improvements. Mobile deliberately omits reward-lantern pane geometry; High retains physical dielectric glass. There is no fake transparency or non-RT substitute.

The active source identity remains `1.6.1` / Android `versionCode 9`. Package source is `a397757`; later receipt/guard/documentation commits do not rebuild the frozen runtime. See the [finding disposition](docs/ENGINEERING_1_6_1_FINDING_STATUS.md), [integration matrix](docs/evidence/2026-10-02-final-integration/README.md), [release notes](docs/SHOWCASE_ALPHA_1_6_1_RELEASE_NOTES_2026-09-01.md), [signed validation](docs/SHOWCASE_ALPHA_1_6_1_RELEASE_VALIDATION_2026-10-03.md) and [verified publication receipt](docs/evidence/2026-10-03-release-publication/README.md). Both itch channels are ready; downloaded Windows payload entries and signed APK match the freeze. The unchanged 1.6.0 updater selects the public GitHub1.6.1 announcement; an actual old-install popup was not retested. Owner accepts current cuff/audio and measured performance as-is: sustained30FPS at75% is not achieved. S24 is working but not fully tested; exact S25 is unverified. Remaining High glass defects are future investigation, not fixed. Full graphics-options menu is planned for1.6.2. Historical1.6.0 evidence below retains its original scope.

The 1.5.2 water path removes the waterfall-only lighting approximation and hidden second glossy bounce on submerged cobble. Refracted and reflected opaque hits use terminal shared active-light/material/shadow logic, transparent candidates are explicitly filtered, path distance is accumulated, interface highlights use the same visible lights, and the directional moon traverses physical roof/player geometry. The ray budget remains finite and water-on-water paths do not recurse. Deterministic Windows, fresh Host, and exact `SM-S948B` Debug evidence pass; the owner accepted the moving Windows result. Repeated phone medians are approximately 27.8 ms at `lantern-drop`, 19.0 ms at `skylight`, and 30.8 ms at `lich`, with the bounded real-light cost retained rather than hidden through a quality or resolution reduction. See `docs/WATER_TRANSMISSION_SHADOW_VALIDATION_2026-08-24.md` and `docs/SHOWCASE_ALPHA_1_5_2_RELEASE_VALIDATION_2026-08-25.md`.

Exact clean Host, Windows feature-capture, and `SM-S948B` 75%/100% evidence for 1.6.0 is recorded in `docs/FIRE_PBR_REWARD_LANTERN_PLAYER_UPGRADE_VALIDATION_2026-08-30.md`, `docs/TASK_9_OWNER_CANDIDATE_VALIDATION_2026-08-30.md`, and `docs/SHOWCASE_ALPHA_1_6_0_RELEASE_VALIDATION_2026-08-30.md`. Instrumented Debug feature medians are 53.481-91.176 ms at 75% and 83.954-154.484 ms at 100%, so this is image/correctness evidence rather than a performance claim; Mobile glass remains the main measured risk. The owner accepted the final natural reward route, two-second unlock cue, chest guidance, audio, and haptics on the exact installed Debug candidate. The signed public APK has now also passed exact-device install/hash, strict-ASTC, honest-presentation, Home/resume, and short route smoke; sustained Release timing and owner-feel evidence remain separate.

The current development foundation runs Windows and Android gameplay through one deterministic 60 Hz `GameSimulation`. Android input crosses JNI through a coherent snapshot mailbox with independent monotonic swing/parry/reset/retry counters; ordered semantic events drive platform audio and haptics; and one shared adapter preserves the existing `RtSceneFrameInputs` renderer boundary. See `docs/SHARED_SIMULATION_FOUNDATION_2026-08-10.md`.

The current development foundation supports a bounded two-skeleton encounter: stable entities share a skeleton pose/BLAS when their actions match and use at most two pose buckets when they diverge; the lich remains singular. Current `PresentableTinyRtScene` owns separate world-body/viewmodel buffers and BLAS within21 TLAS metadata slots; resolve roles through the generated named instance semantics, not historical raw indices. Historical exact commit `b3428a7` passed the six-checkpoint `SM-S948B` 75% gate,13 captures, replay and Home/resume; the owner then reported that two-enemy play felt fine on that installed candidate. That older ten-BLAS/twenty-instance evidence does not prove later source revisions. See `docs/TWO_SKELETON_COMBAT_ANDROID_VALIDATION_2026-08-12.md`.

The animation-owned combat/parry candidate at exact commit `daa5892` passed functional `SM-S948B` checks and recorded a 20.246 ms warm lich median under the then-current gate. The owner found parry timing good. Final exact reconciliation commit `547d89d` publishes Android parry on press-down, animates stagger, retains event-time spatial data, restores positional skeleton hit/fall parity on Windows, and uses bounded Android feedback transport. Its clean Full gate passed strict ASTC, honest presentation, replay, 13 captures, Home/resume, fresh 12/12 Debug and Release CTests, Android build/lint, and exact installed-APK matching. Sustained 75% lich measured 23.069 ms (~43.3 FPS) at GPU thermal power level 2 and is reported honestly in the 30-50 FPS band rather than failed against an arbitrary 20 ms line. The owner gave the final exact candidate a broad audio/haptic pass and explicitly observed its stagger-back/death sequence. No renderer slot, BLAS, pose bucket, or runtime asset was added; Showcase Alpha 0.1.4 later changed release identity and packaging only. See `docs/ANIMATION_COMBAT_PARRY_SLICE_2026-08-13.md`, `docs/CURRENT_DEVELOPMENT_BASELINE_VALIDATION_2026-08-21.md`, and `docs/SHOWCASE_ALPHA_0_1_4_RELEASE_VALIDATION_2026-08-22.md`.

Earlier 23.604 ms lich evidence was a real unmatched hot renderer-foundation run, not an unresolved current-candidate result: the provenance-bound cooled `b3428a7` A/B later measured 19.497/19.268 ms. The 20.246 ms `daa5892` result was a separate warm non-pass under the gate then in force. Both remain useful historical evidence; final current-source evidence is recorded separately.

The APK declares Android7 / API24 as its packaging minimum, but hardware support is intentionally much narrower. Both backends require real Vulkan acceleration structures, hardware ray queries, buffer device address, required supporting features and strict ASTC formats. The Pipeline backend additionally requires RT-pipeline/SBT support; the real RayQueryCompute backend does not. Exact `SM-S948B`/Android16 and `SM-S928B` evidence is recorded separately; S24's accepted development images are not final Shipping certification, and exact S25 remains unverified.

Device compatibility is tracked in [`docs/ANDROID_RT_DEVICE_COMPATIBILITY_RECORD.md`](docs/ANDROID_RT_DEVICE_COMPATIBILITY_RECORD.md). New device results should be recorded there with the exact model code and evidence class: locally tested confirmation, user-reported plus screenshot evidence, user-reported, vendor/SoC inference, or unverified candidate. Hardware marketing claims alone do not establish support; the runtime capability probe and honest RT swapchain presentation are the deciding checks.

## Roadmap and future chapters

The [Campaign and Engine Roadmap](docs/ROADMAP.md) records the forward direction: [1.7.0 — Beyond the Tomb](docs/superpowers/plans/2026-09-11-beyond-the-tomb-1.7.0.md), then [1.8.0 — Village Hub](docs/superpowers/plans/2026-09-11-village-hub-1.8.0.md), followed by three themed dungeons with enemies, puzzles, bosses and Horde/key/map-related pieces. Dungeon-specific items and ray-traced light as gameplay are central design considerations; exact themes, lore and later versions remain open.

These are plans, not shipped features. Use the released1.6.1 engineering baseline for future work, and make, test and accept1.7 before finalising or implementing the provisional village hub. The [Phase Plan](docs/PHASE_PLAN.md) retains the historical implementation sequence.

## RT or nothing

The preferred backend uses Vulkan acceleration structures, an RT pipeline and shader binding table, `vkCmdTraceRaysKHR`, an RT storage image, and swapchain presentation. Its phone-safe shading uses `rayQueryEXT` inside raygen with pipeline recursion depth 1. The released1.6.1 engine also supports a hardware `RayQueryCompute` launcher for RayQuery-only drivers: compute dispatch executes the same shading over real BLAS/TLAS and presents through the same storage-image/swapchain path, without an SBT. Unsupported devices show explicit diagnostics; there is no browser, raster, baked, screen-space, software-traversal or fake-RT fallback.

[Targeted1.6.1 backend evidence](docs/ENGINEERING_1_6_1_RAYQUERY_BACKEND_2026-09-13.md) and the [device compatibility record](docs/ANDROID_RT_DEVICE_COMPATIBILITY_RECORD.md) distinguish Windows, exact S26 and exact S24 results. Current Windows and signed S26 functional gates pass within the [release receipt's limits](docs/SHOWCASE_ALPHA_1_6_1_RELEASE_VALIDATION_2026-10-03.md); final S24 coverage is owner-deferred and exact S25 remains unverified. Numerical sub-pixel parity is parked; remaining demonstrated High glass defects are explicitly owner-deferred [future investigation](FUTURE_WORK.md#future-glass-investigation--owner-deferral-2026-10-03), not fixed or passed.

`rtScene.presented` becomes true only after an RT-produced frame reaches successful swapchain presentation.

## Showcase alpha contents

- Portrait-first Android presentation and a native Windows desktop build.
- Branded entry, pause, controls, settings, diagnostics, restart, and quit flows.
- A player-facing two-pass `Run benchmark` course ends in a selectable, copyable, exportable text report and automatically archives JSON evidence. Interactive runs have a rolling native RT-loop FPS/ms counter, not a display-Hz claim; unattended/frozen performance runs keep the counter off. Windows live display/cancel/restart/completion passes; matched observer overhead remains unmeasured.
- A `More by Samfa12` menu button opens https://samfa12.com/ in the system browser.
- Persisted50–100% RT render-resolution scale; new Android settings default to75%, Windows to100%. Existing saved choices are preserved. The bounded output-only resize route retains compatible scene assets/AS/pipelines instead of rebuilding them for a scale change. Current-bank S26 heavy-scene75→100→75→50→75 checks pass on Shipping/Mobile shaders in a Debug shell; this is not a matched production performance result.
- Separate persisted SFX/music volume sliders on Android and Windows; Android compact-HUD and Windows sensitivity, display-mode, and render-scale settings.
- Collision-safe starting chamber and material gallery, a leashed skeleton encounter, and a three-turn moving-shadow corridor.
- A roof-water drench that gutters and drops the lantern, clear RT reflection/refraction, a rounded catchment and drain-connected runnel, blue skylight chamber, four coloured-light bays, wet stone, fog, and a single-bounce hero mirror.
- Low depth-clipped blue-grey ritual mist in the lich room, bounded to preserve the enemy, sword, and opening-roof sightlines.
- Imported PBR sword and torch assets, with a deterministic world-space fire emitter whose RT-visible core, depth-clipped volume, coloured direct light, reflections, and movement share one state.
- A collision-bearing Gothic reward chest and imported reward lantern with warm internal light, shared interaction/held sockets, raise/lower poses and fixed-step physical swing. High retains full physical dielectric panes; Mobile deliberately omits the pane geometry, with no fake transparency substitute. This is a quality-profile decision, not a phone-model exception.
- Dedicated modelled RT `PlayerViewmodel` sleeves/arms/gauntlets and independently owned `PlayerWorldBody`, sharing gameplay animation/IK/grip authority. Normal production no longer uses block arms; explicit diagnostic comparisons remain separate. Body presence, shadow/reflection masks and independently owned dynamic geometry/BLAS avoid duplicated first-person arms.
- A Hotstrike Studio skeleton derivative followed sequentially by a CC0 Meshy placeholder lich with emissive staff/eyes, charge electricity, spatial audio, three-hit combat, death animation, a physical sliding roof, a moonlight-to-dawn RT reveal, and a contextual ending with continue/restart/quit.
- Three player vitality per encounter, one-second post-hit invulnerability, an RT-visible fatal hold, and death-menu retry at the safe opening or mirror checkpoint.
- Seventeen FilmCow UI, sword, movement, skeleton, and lich reaction/attack WAV cues, plus one positional DRAGON-STUDIO/Pixabay waterfall loop.
- Adaptive A–H “What the Dark Keeps” music: canonical revisable PCS JSON/PCS1, accepted whistle-lead48kHz stereo PCM16 loop bodies/tails rendered with retained Chordsmith v68 app voices/live FX. Shared Pocket Audio Core owns reusable PCM decoding/loop/crossfade playback; Horde owns gameplay-to-cue policy. No editor/synth application is vendored.
- In-game playtest reports use the approved Briarhold-derived Cloudflare delivery architecture. Note, bounded typed diagnostics and game-only RT screenshot have explicit consent/optional opt-ins; preview, verification, cancellation and same-report retry are bounded. Local JSON export remains an offline fallback. No client secrets or unrelated logs/files are submitted.
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

The [engineering completion reconciliation](docs/ENGINEERING_1_6_1_COMPLETION_REPORT.md)
lists each original finding, owner supersession, test/device boundary and the
retained negative performance evidence.

The released1.6.1 package and current-source evidence are indexed in the [release validation](docs/SHOWCASE_ALPHA_1_6_1_RELEASE_VALIDATION_2026-10-03.md). Exact945f990 push37089806774/PR37089810005 pass all six CI lanes; post-publication closeout has its own current-head checks. Actual packaged Shipping SPIR-V contains no diagnostic atomics/readback binding; Windows RTX Pipeline/Compute and signed S26 update/presentation/lifecycle have separate native evidence. Compiler/host checks do not certify physical RT, audio perception, sustained performance or untested devices.

Historical Showcase Alpha1.6.0 source Host run `run-20260831-131431` passed the complete seven-stage gate with 31/31 Debug and 31/31 Release tests plus 13 deterministic Windows captures. Its exact published Windows ZIP launched from an isolated extraction, selected `RayTracingPipeline`, honestly presented the RT scene, and exited cleanly. Its signed Android APK passed certificate, identity, package-layout, native-library, asset/licence, lint, and compatibility guards, then passed exact-device install/pullback, strict ASTC, honest RT presentation, Home/resume, and short route smoke on `SM-S948B`. These are retained1.6.0 results, not later-candidate certification. See `docs/SHOWCASE_ALPHA_1_6_0_RELEASE_VALIDATION_2026-08-30.md`.

The Fire/PBR/reward-lantern runtime passed clean-source Windows Debug and Release 31/31 Vulkan-enabled CTests, 13 standard plus 11 feature Windows captures, isolated packaged Windows launch, Android Debug/unsigned Release/lint, asset/licence/package gates, and exact `SM-S948B` 75%/100% RT/ASTC/capture/Home-resume validation. The installed Debug APK SHA-256 is `0b5a59b6e41d2c4d717eff885aaa310b7f5f1512002f6a89cb77e5989ab7edd3`; the owner accepted phone feel, the natural chest/reward route, the two-second unlock timing, audio, and haptics. The feature path is materially slower than the earlier published route and finite Mobile glass-budget terminals remain, both recorded without reducing quality or scale.

The historical Showcase Alpha0.1.5 packages introduced the accepted water/mist/controller/audio slice. Their exact signed-APK installation/lifecycle evidence is historical, not the latest public1.6.0 package or current1.6.1 candidate. See `docs/SHOWCASE_ALPHA_0_1_5_RELEASE_VALIDATION_2026-08-23.md` and `docs/RT_WATERFALL_LICH_MIST_VALIDATION_2026-08-23.md`.

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
.\tools\restore-report-webview2.ps1
$cmake = "C:\Program Files (x86)\Microsoft Visual Studio\2022\BuildTools\Common7\IDE\CommonExtensions\Microsoft\CMake\CMake\bin\cmake.exe"
& $cmake -S . -B build
& $cmake --build build --config Debug --target horde_rt_diagnostic_window
.\build\Debug\HordeLanternRT.exe
```

The standalone capability probe remains available as target `horde_rt_capability_probe`.

The explicit restore verifies the pinned native WebView2 SDK used only for
foreground report verification. Configure never downloads it. The game uses an
installed compatible Evergreen Runtime; it does not install a Runtime and keeps
offline report export available if verification is unavailable.

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

For a broad integration candidate, run the Host gate from the repository root; use affected checks for smaller changes as described in the [validation guide](docs/AGENT_VALIDATION.md):

```powershell
.\tools\run-foundation-validation.ps1
```

It performs fresh Windows Debug/Release builds, all configured CTests in both configurations, deterministic Windows captures, clean Android Debug/Release builds, Release lint, non-mutating shader-staleness checks, validation package/layout and asset/licence checks, release-identity safeguards, and evidence hashing.

With the authorised `SM-S948B` connected, run the milestone gate:

```powershell
.\tools\run-foundation-validation.ps1 -Mode Full
```

`Full` adds the configured sustained75% timing/replay programme, separately labelled100% opening result, Home/resume and deterministic Android captures. Current CTest rosters depend on configuration; use current runner/workflow output, not historical13/9 counts. CI has separate GCC/Clang/MSVC portable, focused Vulkan CPU-host, selected Clang ASan/UBSan and Android build/Java/lint lanes. Vulkan-host fixtures do not create a physical RT device. Timing output uses descriptive16.667/20.000/33.333ms reference lines (60/50/30FPS); crossing one is reported rather than treated as an automatic product failure. Investigate matched regressions above15%. Both modes retain logs, manifests, hashes, PNGs and exact artifact/source provenance under ignored `reports/`. Foundation artifacts are explicitly unpublishable/unsigned, never production signing or release authority; development-signed benchmark artifacts are separately identified.

Check raygen staleness without modifying the embedded include:

```powershell
.\tools\compile-raygen.ps1 -Check
```

Windows Debug also supports `HordeLanternRT.exe --capture-showcase <directory>`; Windows Release and Android Release reject capture/checkpoint automation. Video and orbit-camera capture remain deferred.

The portable jobs in `.github/workflows/shared-simulation-host.yml` configure with `HORDE_RT_BUILD_VULKAN_TARGETS=OFF`; additive Vulkan-host fixtures cover actual player loader/skinning/semantics and resource failure ownership. Neither category establishes RT swapchain presentation, device thermals, touch feel, audio perception or haptics. Current-source branch-push coverage remains available independently of PR integration state.

## Package and publish

Only an explicitly authorised release may sign/publish. This project already has
a stable Android signing identity: do not create a replacement key for an update.
Independent backup/recovery remains [owner-only](docs/OWNER_RELEASE_SAFETY_CHECKLIST.md).
Signing/publication require explicit owner authority; the October3 release was
authorised separately. That approval is not blanket permission for future releases.

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

There are no packaging version defaults: candidate scripts require explicit `Version` and `VersionCode`, then require both to match the root `VERSION` and Android version-code map. They reject the immutable published `0.1.1` through `0.1.5`, `1.5.2`, `1.6.0` and `1.6.1` lines. Future Android updates require `versionCode > 9`. One shared policy helper owns these rules for packaging, signing, and itch upload. A future public build must first provide matching release notes and update the guarded candidate hashes. Foundation validation can test the published source using explicitly unpublishable artifacts; it cannot repackage or upload the published line.

Android native code is linked for 16 KiB page compatibility. The release uses a static C++ runtime, 16 KiB ELF `LOAD` alignment, and AGP 8.7.2 APK alignment; `package-alpha.ps1` rejects candidates that fail either APK or ELF verification or reintroduce `libc++_shared.so` from the r26 NDK.

Never commit a keystore, signing properties, credentials, APK, or generated candidate directory. Losing the release JKS or its passwords prevents compatible Android updates.

## Asset and licence policy

All shipped third-party assets are recorded in `ASSET_LICENSES.md`. The current release includes:

- Five Poly Haven environment sets under CC0.
- Free Stylized Skeleton by Hotstrike Studio, modified through Meshy; the release uses the conservative Meshy Free-plan CC BY 4.0 attribution route.
- The active placeholder lich created and animated with Meshy under CC0; the retained licence screenshot and hash are recorded in `ASSET_LICENSES.md`.
- A bounded FilmCow Recorded SFX subset under FilmCow's custom royalty-free project-use terms.
- “Water Dripping” by DRAGON-STUDIO under the Pixabay Content License, processed into the shipped positional loop.
- The owner-supplied “What the Dark Keeps” score and its rendered music bank, authorised for Horde use only; no general permissive music licence is inferred.

Do not redistribute source assets as standalone asset packs. Preserve the public Hotstrike/Meshy credit and the full licence manifest with releases.

## Architecture and invariants

- Android entrypoint: `android/app/src/main/java/com/samfa12/hordelanternrt/MainActivity.java`
- Android native bridge: `android/app/src/main/cpp/android_probe_bridge.cpp`
- Windows presentation: `src/platform/windows/DiagnosticWindow.cpp`
- Shared scene: `src/vulkan/raytracing/PresentableTinyRtScene.cpp`
- Shared gameplay: `src/gameplay/simulation/GameSimulation.cpp`
- Android input mailbox: `src/gameplay/simulation/InputMailbox.h`
- Shared simulation-to-renderer adapter: `src/vulkan/raytracing/SimulationFrameAdapter.cpp`
- Player route/mask contract: `src/vulkan/raytracing/PlayerRenderSlot.cpp`; independent world-body/viewmodel GPU owners remain in `PresentableTinyRtScene`
- Buffer lifetime/placement: `src/vulkan/raytracing/RtGpuResources.cpp`
- Generated named instance/CPU–GLSL ABI: `src/vulkan/raytracing/RtSceneAbi.generated.h`
- Raygen source: `shaders/raytracing/minimal.rgen`
- Embedded raygen SPIR-V: `src/vulkan/raytracing/MinimalRayGenShader.inc`

After raygen edits, run `tools/compile-raygen.ps1`; use `tools/compile-raygen.ps1 -Check` in validation when mutation is not allowed. Keep one frame in flight while the held-prop TLAS uses host-written instance data. Preserve presentation-format-driven red/blue swapping on the 100% raw-copy path so warm fire does not render cyan.

Dynamic coherent mappings belong to their buffer lifetimes and retain fence/barrier ownership. Immutable geometry/metadata prefer compatible device-local coherent storage; unsupported preferred types retain honest required-host placement. No OOM retry disguises allocation failure. Generic staging on devices without a compatible local/coherent type remains a documented future-platform gap, not a universal device-local claim.

## Showcase route status

The established route is Windows- and Android-device-validated: lower body and the original torch failure at the historical `lantern-drop` checkpoint, zig-zag shadows, blue skylight, bay-selected coloured torches, an open framed threshold, one hero mirror, and a sequential staff-lit lich finale. The newer body-and-ending slice adds the layered animated player, post-death sliding skylight, warm dawn reveal, and contextual ending; its host and exact-APK `SM-S948B` evidence are recorded in `docs/PLAYER_BODY_AND_FINALE_SLICE_2026-07-31.md`.

Showcase Alpha 1.5.2 retains the two-skeleton system first published in 0.1.4: at most two simultaneous skeletons, two skeleton pose buckets, nine character/environment BLAS plus the dedicated water geometry route, and twenty physical TLAS slots; the later lich route remains singular. No third enemy is permitted without a separately measured design.

The 75% setting is the sustained phone recommendation. Preserve real RT at the documented quality tier; reduce bounded effect area or ray cost before considering any broader feature expansion.
