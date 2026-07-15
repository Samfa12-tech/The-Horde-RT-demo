# Android Vulkan RT App

The `android/` module is the supported phone path for Horde Lantern RT. It owns the Java activity and lifecycle/JNI bridge while compiling the shared renderer and scene sources from `src/`.

## Current implementation

- Java entrypoint: `app/src/main/java/com/samfa12/hordelanternrt/MainActivity.java`
- JNI/Vulkan bridge: `app/src/main/cpp/android_probe_bridge.cpp`
- Shared native source manifest: `../cmake/HordeRtSources.cmake`
- Native Vulkan RT presentation through the Android swapchain
- One frame in flight while the held-prop TLAS uses a host-written instance buffer
- Portrait-first branded entry/pause/settings/controls/diagnostics/credits UI, touch movement/look, a dedicated `SWING` control, one-enemy combat, animated skeleton BLAS refit, a yaw-relative first-person torso plus four pitch-relative IK arm instances, ASTC PBR texture arrays, and phone-safe ray-query shading inside `vkCmdTraceRaysKHR`
- Persisted SFX volume, look sensitivity, compact HUD, and 50-100% RT render scale; thirteen FilmCow clips play through SoundPool
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
adb logcat -d -s HordeRtProbeBridge AndroidRuntime
```

Look for `RT frame reached Android swapchain presentation.`
Also require `PBR material encoding: ASTC 6x6 diffuse/ARM + ASTC 4x4 normal (KTX2)` on the target phone.

Reports are stored under `files/reports/` in app-private storage and can be retrieved with `adb shell run-as com.samfa12.hordelanternrt`.

The current primary test device is Samsung `SM-S948B`. Use the renderer's 120-frame telemetry after meaningful renderer, animation, or material-path changes and validate the recommended quality tier separately from 100%.

The combat/ASTC build passed that gate on 2026-07-14: strict ASTC selection, honest RT swapchain presentation, stable movement/look/swing input, and two samples at 12.500 ms median / 16.667 ms p95. See `../docs/COMBAT_ASTC_PHONE_VALIDATION_2026-07-14.md`.

The articulated grip-locked, pitch-following revision builds for all Android ABIs and is verified on `SM-S948B`: strict ASTC selection, honest RT presentation, live idle/swing grip composition, and thermal-status-2 sustained evidence at 52.352 SurfaceFlinger TimeStats average FPS / 19.718 ms internal median (approximately 50.7 FPS). See `../docs/PLAYER_BODY_RT_SLICE_2026-07-14.md`.

The published `0.1.0-alpha.1` APK is stable-key signed and passed the final portrait/UI/audio/render-scale sanity pass. At 75%, 21 warm 120-frame windows measured 10.933 ms median / 15.050 ms p95 at thermal status 3; 75% is the sustained recommendation. The 2026-07-16 refresh also verifies side-by-side debug installation, 16 KiB native/APK alignment, and fast live diagnostics on Android 16. See `../docs/ALPHA_ANDROID_PHONE_VALIDATION_2026-07-15.md` and `../docs/ALPHA_ANDROID_REFRESH_VALIDATION_2026-07-16.md`.

Create/sign future releases with `../tools/create-android-release-key.ps1` and `../tools/package-signed-alpha.ps1`. Keep the JKS and signing properties outside Git.
