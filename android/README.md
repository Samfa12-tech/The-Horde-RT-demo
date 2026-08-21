# Android Vulkan RT App

The `android/` module is the supported phone path for Horde Lantern RT. It owns the Java activity and lifecycle/JNI bridge while compiling the shared renderer and scene sources from `src/`.

## Current implementation

- Java entrypoint: `app/src/main/java/com/samfa12/hordelanternrt/MainActivity.java`
- JNI/Vulkan bridge: `app/src/main/cpp/android_probe_bridge.cpp`
- Shared native source manifest: `../cmake/HordeRtSources.cmake`
- Shared fixed-step gameplay authority: `../src/gameplay/simulation/GameSimulation.cpp`
- Coherent JNI input publication through the two-slot `InputMailbox`, with independent monotonic attack/parry/reset/retry counters
- Ordered semantic gameplay events with per-event spatial gains drive SoundPool and haptics without collapsing repeated cues
- Native Vulkan RT presentation through the Android swapchain
- Optional Vulkan timestamp queries report a separate GPU RT command-buffer interval without changing CPU benchmark pass/fail
- One frame in flight while the held-prop TLAS uses a host-written instance buffer
- Portrait-first branded entry/pause/settings/controls/diagnostics/credits UI; touch movement/look plus `SWING` and `PARRY`; a bounded two-skeleton opening encounter followed by a singular lich route; layered articulated body/head with a smoothed walk gait, lantern-drop sequence, coloured bays, mirror, sliding-roof dawn reveal, and Continue/Begin Again/Quit ending; strict ASTC assets; and phone-safe ray-query shading inside `vkCmdTraceRaysKHR`
- Persisted SFX volume, look sensitivity, compact HUD, and 50-100% RT render scale; seventeen FilmCow clips play through SoundPool
- The in-app Credits & Licences panel carries Poly Haven, FilmCow, Hotstrike Studio, Meshy, and generated-icon provenance with the APK
- Native libraries use a static C++ runtime plus 16 KiB ELF alignment; the packaging gate verifies 16 KiB APK/ELF alignment and rejects an r26 `libc++_shared.so`
- Unsupported devices retain explicit diagnostics instead of a fake rendering fallback

## Build, install, and launch

```powershell
cd android
.\gradlew.bat assembleDebug installDebug --console=plain
adb shell am start -n com.samfa12.hordelanternrt.debug/com.samfa12.hordelanternrt.MainActivity
```

The debug build uses `com.samfa12.hordelanternrt.debug`, so it can be installed beside the stable-key-signed public alpha without uninstalling or changing the release app. Release builds retain `com.samfa12.hordelanternrt`.

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

That historical exact candidate is clean commit `b3428a7`; it does not validate the later animation/parry work. Exact clean `daa5892` passed functionality and recorded a 20.246 ms warm lich median under the then-current gate. The owner found parry timing good and requested touch-down dispatch. Final exact reconciliation runtime commit `547d89d` uses press-down parry, listener-at-event-time routing, bounded platform feedback, and animated stagger; its complete Full gate passed strict ASTC, honest presentation, fresh 12/12 Debug and Release CTests, Android build/lint, replay, 13 captures, Home/resume, and matching local/installed APK hashes. Sustained 75% lich was 23.069 ms in the descriptive 30-50 FPS band at GPU thermal power level 2. The owner reported that audio sounded good on the earlier exact candidate; the final candidate's audio/haptic/stagger confirmation remains pending. Performance uses sustained evidence and descriptive 60/50/30 FPS bands rather than a hard 20 ms rule. Future manual owner audio/haptic checks are change-triggered rather than automatic per milestone. See `../docs/ANIMATION_COMBAT_PARRY_SLICE_2026-08-13.md`, `../docs/CURRENT_DEVELOPMENT_BASELINE_VALIDATION_2026-08-21.md`, and `../docs/OWNER_RELEASE_SAFETY_CHECKLIST.md`.

The showcase release identity is `0.1.3-alpha.1` with `versionCode 4`. Public candidates must be signed by the established Horde release key, retain strict ASTC routing and 16 KiB compatibility, and pass `tools/package-signed-alpha.ps1`; never replace the existing update identity with a new keystore.

The exact signed 0.1.3 APK is published as itch build `#1845896`; SHA-256 `a4eb996104c03734a7fa8a16be1f8f701d5b19c861c066af002f96f9a199eee9`. On 2026-08-01 the old stable 0.1.2 and Debug packages were removed from `SM-S948B`, this release APK was clean-installed, and the installed `base.apk` hash matched the candidate byte-for-byte. It selected strict environment/lich ASTC, loaded all 17 SoundPool cues, honestly presented before and after Home/resume, visibly identified Alpha 0.1.3, rejected Debug capture automation as non-debuggable, and logged no fatal runtime marker. See `../docs/SHOWCASE_ALPHA_0_1_3_RELEASE_VALIDATION_2026-07-31.md`.

Create/sign future releases with `../tools/create-android-release-key.ps1` and `../tools/package-signed-alpha.ps1`. Keep the JKS and signing properties outside Git.
