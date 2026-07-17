# Android Vulkan RT App

The `android/` module is the supported phone path for Horde Lantern RT. It owns the Java activity and lifecycle/JNI bridge while compiling the shared renderer and scene sources from `src/`.

## Current implementation

- Java entrypoint: `app/src/main/java/com/samfa12/hordelanternrt/MainActivity.java`
- JNI/Vulkan bridge: `app/src/main/cpp/android_probe_bridge.cpp`
- Shared native source manifest: `../cmake/HordeRtSources.cmake`
- Native Vulkan RT presentation through the Android swapchain
- One frame in flight while the held-prop TLAS uses a host-written instance buffer
- Portrait-first branded entry/pause/settings/controls/diagnostics/credits UI; touch movement/look and `SWING`; sequential skeleton/lich selection through one active skinned BLAS; complete low-poly body/head, lantern-drop sequence, coloured bays, mirror and sliding-roof finale; strict ASTC assets; and phone-safe ray-query shading inside `vkCmdTraceRaysKHR`
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

The debug build exposes deterministic native checkpoints and a 13-waypoint route replay. Run the standard 75% checkpoint, timing, collision, strict-ASTC, and honest-presentation pass from the repository root:

```powershell
.\tools\run-android-showcase-validation.ps1
```

Add `-Include100 -Capture` for the report-only 100% opening check and post-timing screenshots. Evidence is written to a unique ignored `reports/android-showcase-runs/run-<timestamp>/` directory. Release builds reject the debug automation request path. See `../docs/ANDROID_SHOWCASE_AUTOMATION_2026-07-17.md` for checkpoints, evidence semantics, and the remaining hands-on checks.

The current primary test device is Samsung `SM-S948B`. Use the renderer's 120-frame telemetry after meaningful renderer, animation, or material-path changes and validate the recommended quality tier separately from 100%.

The combat/ASTC build passed that gate on 2026-07-14: strict ASTC selection, honest RT swapchain presentation, stable movement/look/swing input, and two samples at 12.500 ms median / 16.667 ms p95. See `../docs/COMBAT_ASTC_PHONE_VALIDATION_2026-07-14.md`.

The articulated grip-locked, pitch-following revision builds for all Android ABIs and is verified on `SM-S948B`: strict ASTC selection, honest RT presentation, live idle/swing grip composition, and thermal-status-2 sustained evidence at 52.352 SurfaceFlinger TimeStats average FPS / 19.718 ms internal median (approximately 50.7 FPS). See `../docs/PLAYER_BODY_RT_SLICE_2026-07-14.md`.

The previous `0.1.0-alpha.1` APK established the stable signing identity and passed the portrait/UI/audio/render-scale sanity pass. Its 2026-07-16 refresh also verified side-by-side debug installation, 16 KiB native/APK alignment, and fast live diagnostics on Android 16. See `../docs/ALPHA_ANDROID_PHONE_VALIDATION_2026-07-15.md` and `../docs/ALPHA_ANDROID_REFRESH_VALIDATION_2026-07-16.md`.

The complete showcase route is device-validated on `SM-S948B` in the debug package: strict environment plus lich ASTC, honest RT presentation, full route traversal, Home/resume recreation, and warm controlled 75% measurements passed at thermal status 3. Every required zone's median of three 120-frame average windows remained below 13.7 ms. The phone was restored to the recommended 75% tier after the 100% extent/reporting check. See `../docs/HORDE_SHOWCASE_ANDROID_VALIDATION_2026-07-17.md`.

The later debug automation baseline also passed live: five deterministic 75% checkpoints, report-only 100% opening, native state assertions, and all 13 replay waypoints. These cool thermal-status-0 results are regression evidence and do not replace the warm sustained certification above. See `../docs/ANDROID_SHOWCASE_AUTOMATION_VALIDATION_2026-07-17.md`.

The showcase release identity is `0.1.1-alpha.1` with `versionCode 2`. Public candidates must be signed by the established Horde release key, retain strict ASTC routing and 16 KiB compatibility, and pass `tools/package-signed-alpha.ps1`; never replace the existing update identity with a new keystore.

The exact signed 0.1.1 APK is published as itch build `#1801017`; SHA-256 `ae73afec2c75b317187aeb61d81a592ec8bb4d8b5e89ef9b474fb2a60ae1354a`. It was installed over the stable package and reconfirmed `versionCode 2`, all seventeen SoundPool loads, strict ASTC environment/lich routing, and honest RT swapchain presentation. See `../docs/SHOWCASE_ALPHA_RELEASE_VALIDATION_2026-07-17.md`.

Create/sign future releases with `../tools/create-android-release-key.ps1` and `../tools/package-signed-alpha.ps1`. Keep the JKS and signing properties outside Git.
