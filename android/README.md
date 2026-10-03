# Android Vulkan RT App

The `android/` module is the supported phone path for Horde Lantern RT. It owns the Java activity and lifecycle/JNI bridge while compiling the shared renderer and scene sources from `src/`. Current development is 1.6.2 / versionCode 10; published 1.6.1 and its acceptance evidence remain separate.

## Current implementation

- Java entrypoint: `app/src/main/java/com/samfa12/hordelanternrt/MainActivity.java`
- JNI/Vulkan bridge: `app/src/main/cpp/android_probe_bridge.cpp`
- Shared native source manifest: `../cmake/HordeRtSources.cmake`
- Shared fixed-step gameplay authority: `../src/gameplay/simulation/GameSimulation.cpp`
- Coherent JNI input publication through the two-slot `InputMailbox`, with independent monotonic attack/parry/dodge/reset/retry counters
- Ordered semantic gameplay events with per-event spatial gains drive SoundPool and haptics without collapsing repeated cues
- Native Vulkan RT presentation through the Android swapchain
- Development 1.6.1 selects the existing RT-pipeline backend when supported, or shared BLAS/TLAS-backed `RayQueryCompute` on verified RayQuery-only devices. Raw capability, selected backend and successful presentation are separate report fields; see [targeted backend evidence](../docs/ENGINEERING_1_6_1_RAYQUERY_BACKEND_2026-09-13.md) for exact device limitations.
- Optional Vulkan timestamp queries report a separate GPU RT command-buffer interval without changing CPU benchmark pass/fail
- One frame in flight while the held-prop TLAS uses a host-written instance buffer
- Portrait-first branded entry/pause/settings/controls/diagnostics/credits UI; touch movement/look plus `SWING` and `PARRY`; a bounded two-skeleton opening encounter followed by a singular lich route; layered articulated body/head with a smoothed walk gait, roof-water drench and lantern drop, rounded catchment/drain runnel, coloured bays, mirror, low ritual mist, sliding-roof dawn reveal, and Continue/Begin Again/Quit ending; a persistent post-lich RT Lab with route-local tuning; strict ASTC assets; and phone-safe ray-query shading inside `vkCmdTraceRaysKHR`
- Persisted independent SFX/music volume, look sensitivity, interface preferences and shared Graphics Apply/Revert/Keep recovery; 50-100% RT render scale and independent water/fire choices. The compact production RT preview owns its timeline while gameplay is paused. FilmCow cues retain their existing mappings; new Keeper cues and a Core-backed positional waterfall worker have separate owner listening gates.
- The in-app Credits & Licences panel carries Poly Haven, FilmCow, DRAGON-STUDIO/Pixabay, Hotstrike Studio, Meshy, and generated-icon provenance with the APK
- Native libraries use a static C++ runtime plus 16 KiB ELF alignment; the packaging gate verifies 16 KiB APK/ELF alignment and rejects an r26 `libc++_shared.so`
- Unsupported devices retain explicit diagnostics instead of a fake rendering fallback

## Build, install, and launch

```powershell
cd android
.\gradlew.bat assembleDebug installDebug --console=plain
adb shell am start -n com.samfa12.hordelanternrt.debug/com.samfa12.hordelanternrt.MainActivity
```

The debug build uses `com.samfa12.hordelanternrt.debug`, so it can be installed beside the stable-key-signed public alpha without uninstalling or changing the release app. Release builds retain `com.samfa12.hordelanternrt`.

For a focused Debug validation of the alternate hardware backend, force-stop only the Debug package and append `--ez horde_require_rayquery_compute true` to its launch intent. Omit the flag on a fresh launch to restore normal backend preference. The native Release build ignores this private flag; RayQuery-only supported devices select compute automatically. Do not change system settings or clear app data for this check.

Expected RT success log:

```powershell
adb logcat -d -s HordeRtProbeBridge HordeLanternAudio AndroidRuntime
```

Look for `RT frame reached Android swapchain presentation.`
Also require `PBR material encoding: ASTC 6x6 diffuse/ARM + ASTC 4x4 normal (KTX2) + strict ASTC 6x6 lich` on the target phone.

Reports are stored under `files/reports/` in app-private storage and can be retrieved from Debug with `adb shell run-as com.samfa12.hordelanternrt.debug`. The stable release is deliberately non-debuggable.

## Repeatable showcase validation

The debug build exposes thirteen deterministic native checkpoints and a 13-waypoint route replay. Run the standard six-checkpoint sustained 75% timing, collision, strict-ASTC, and honest-presentation report from the repository root. Timings are classified against descriptive 60/50/30 FPS reference lines; crossing 20.000 ms is not an automatic failure, while honest presentation/state failures and matched regressions above 15% still require action:

```powershell
.\tools\run-android-showcase-validation.ps1
```

Add `-Include100 -Capture` for the report-only 100% opening check and post-timing screenshots. Evidence is written to a unique ignored `reports/android-showcase-runs/run-<timestamp>/` directory. Release builds reject the debug automation request path. See `../docs/ANDROID_SHOWCASE_AUTOMATION_2026-07-17.md` for checkpoints, evidence semantics, and the remaining hands-on checks.

Use `-RtLabWorkloadComparison` with an empty standard checkpoint list for a matched Debug-only Lean/Authored/Max comparison at `lantern-drop`, `skylight`, and `finale-roof`. The runner verifies the applied native workload, render scale, water quality, checkpoint, and honest presentation for every row. Debug access/tuning never writes the permanent unlock. See `../docs/RT_LAB_VALIDATION_2026-08-24.md`.

After vitality or encounter-retry changes, run `.\tools\run-android-vitality-validation.ps1 -SkipInstall`. It waits for real post-benchmark skeleton and lich attacks, verifies the Android death actions, invokes the Debug-only receiver through the production Java retry handler, and preserves a timestamped `reports/android-vitality-runs/` evidence bundle. It does not inject damage or claim human touch/haptic validation.

The current primary test device is Samsung `SM-S948B`. Use the renderer's 120-frame telemetry after meaningful renderer, animation, or material-path changes and validate the recommended quality tier separately from 100%.

Keep new device results in [`../docs/ANDROID_RT_DEVICE_COMPATIBILITY_RECORD.md`](../docs/ANDROID_RT_DEVICE_COMPATIBILITY_RECORD.md). Include the exact model code, GPU/Vulkan/driver details, whether the result was locally tested or user-reported, any screenshot/report attachment, RT presentation status, and performance evidence. A device is not considered supported from SoC marketing claims alone.

The combat/ASTC build passed that gate on 2026-07-14: strict ASTC selection, honest RT swapchain presentation, stable movement/look/swing input, and two samples at 12.500 ms median / 16.667 ms p95. See `../docs/COMBAT_ASTC_PHONE_VALIDATION_2026-07-14.md`.

The articulated grip-locked, pitch-following revision builds for all Android ABIs and is verified on `SM-S948B`: strict ASTC selection, honest RT presentation, live idle/swing grip composition, and thermal-status-2 sustained evidence at 52.352 SurfaceFlinger TimeStats average FPS / 19.718 ms internal median (approximately 50.7 FPS). See `../docs/PLAYER_BODY_RT_SLICE_2026-07-14.md`.

The previous `0.1.0-alpha.1` APK established the stable signing identity and passed the portrait/UI/audio/render-scale sanity pass. Its 2026-07-16 refresh also verified side-by-side debug installation, 16 KiB native/APK alignment, and fast live diagnostics on Android 16. See `../docs/ALPHA_ANDROID_PHONE_VALIDATION_2026-07-15.md` and `../docs/ALPHA_ANDROID_REFRESH_VALIDATION_2026-07-16.md`.

The complete showcase route is device-validated on `SM-S948B` in the debug package: strict environment plus lich ASTC, honest RT presentation, full route traversal, Home/resume recreation, and warm controlled 75% measurements passed at thermal status 3. Every required zone's median of three 120-frame average windows remained below 13.7 ms. The phone was restored to the recommended 75% tier after the 100% extent/reporting check. See `../docs/HORDE_SHOWCASE_ANDROID_VALIDATION_2026-07-17.md`.

The later debug automation baseline also passed live: five deterministic 75% checkpoints, report-only 100% opening, native state assertions, and all 13 replay waypoints. These cool thermal-status-0 results are regression evidence and do not replace the warm sustained certification above. See `../docs/ANDROID_SHOWCASE_AUTOMATION_VALIDATION_2026-07-17.md`.

The 2026-08-11 shared-simulation/renderer development candidate subsequently verified strict ASTC, honest RT presentation, valid GPU timestamps, 13/13 replay, 12/12 captures, and Home/resume on `SM-S948B`. Its ordered 75% CPU medians were 10.327 / 7.109 / 8.353 / 11.220 / 23.604 ms. This is real historical unmatched/hot evidence, but not an unresolved current failure: later exact cooled A/B is documented below. The owner separately reported that controls, audio, and haptics worked correctly hands-on on the installed development build. That report is owner-reported local-device evidence without a new exact-artifact check and does not certify later revisions. See `../docs/RENDERER_RESOURCE_SLOTS_ANDROID_VALIDATION_2026-08-11.md`.

The exact 2026-08-12 two-skeleton candidate supersedes that performance failure. Matched cooled lich A/B measured 19.497 ms with GPU timestamps and 19.268 ms without; the 1.188% difference did not identify timing instrumentation as a material cause under the tested conditions. The full six-checkpoint 75% medians were 10.589 / 12.139 / 9.246 / 8.888 / 11.060 / 19.735 ms at thermal status 0, with strict ASTC and honest presentation. Replay, all 13 captures, report-only 100% opening at 18.674 ms, and Home/resume passed. The owner subsequently reported that hands-on play on the still-installed exact candidate feels fine. See `../docs/TWO_SKELETON_COMBAT_ANDROID_VALIDATION_2026-08-12.md`.

That historical exact candidate is clean commit `b3428a7`; it does not validate later animation/parry/water work. Final 0.1.4 reconciliation commit `547d89d` remains the accepted two-skeleton/parry baseline. Showcase Alpha 0.1.5 adds the bounded RT waterfall, catchment/runoff/drain, lich mist, positional water loop, and shared directional dodge. The exact Debug water candidate passed 13 captures, replay, Home/resume, and the descriptive six-checkpoint route; owner hands-on acceptance covered the Windows water/mist presentation and controller path. See `../docs/RT_WATERFALL_LICH_MIST_VALIDATION_2026-08-23.md`.

Showcase Alpha `1.6.1` / Android `versionCode 9` is public: itch Android build `#2055202` and Windows build `#2055201` are ready, and GitHub has a public non-draft prerelease `v1.6.1` targeting package source `a397757249871b6b64fe5b77fc14f24e8cfcbb2b`. The frozen GitHub Windows ZIP SHA-256 is `3e1cdca75d78e02dbc3bb48553b1db68b6b4bf3e459a784e2be5473ad198c2cf` (remote digest verified; the ZIP itself was not downloaded). The itch Windows payload was verified as a 73-entry directory; no hash is claimed for a recompressed itch ZIP. The signed Android APK SHA-256 is `bc5c7ce3c755c16ec39e2c16fa9eae01c31983c7393e645f881c5bcc9e6a346c`. The exact signed S26 install/update, RT/ASTC and Home/resume smoke passed; it is functional/presentation evidence, not a sustained performance or 30 FPS claim. S24 is working but not fully tested; S25 is unverified. Performance is accepted as measured, Mobile intentionally omits panes by quality profile, and remaining High glass defects are owner-deferred. The unchanged updater's live parser selected `v1.6.1` from `1.6.0`; platform dialog/device networking was not exercised. See [`../docs/SHOWCASE_ALPHA_1_6_1_RELEASE_VALIDATION_2026-10-03.md`](../docs/SHOWCASE_ALPHA_1_6_1_RELEASE_VALIDATION_2026-10-03.md) and [`../docs/evidence/2026-10-03-release-publication/`](../docs/evidence/2026-10-03-release-publication/).

The previous signed 1.6.0 APK was itch build `#1931951`; SHA-256 `52a64255ad5dec82cc866fb2ea3545be498ca06c73a789019be851c77e5d6c48`. Its original package and device evidence remain historical. See `../docs/SHOWCASE_ALPHA_1_6_0_RELEASE_VALIDATION_2026-08-30.md`.

For future releases, reuse the established release key and update identity, with a package version newer than `1.6.1` and Android `versionCode` greater than `9`. Keep the JKS and signing properties outside Git; treat the published 1.6.1 artifacts as immutable.
