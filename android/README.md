# Android Vulkan RT App

The `android/` module is the supported phone path for Horde Lantern RT. It owns the Java activity and lifecycle/JNI bridge while compiling the shared renderer and scene sources from `src/`.

## Current implementation

- Java entrypoint: `app/src/main/java/com/samfa12/hordelanternrt/MainActivity.java`
- JNI/Vulkan bridge: `app/src/main/cpp/android_probe_bridge.cpp`
- Shared native source manifest: `../cmake/HordeRtSources.cmake`
- Native Vulkan RT presentation through the Android swapchain
- One frame in flight while the held-prop TLAS uses a host-written instance buffer
- Touch movement/look, a dedicated `SWING` attack control, one-enemy combat, animated skeleton BLAS refit, a yaw-relative first-person torso plus four pitch-relative IK arm instances, ASTC PBR texture arrays, and phone-safe ray-query shading inside `vkCmdTraceRaysKHR`
- Unsupported devices retain explicit diagnostics instead of a fake rendering fallback

## Build, install, and launch

```powershell
cd android
.\gradlew.bat assembleDebug installDebug --console=plain
adb shell am start -n com.samfa12.hordelanternrt/.MainActivity
```

Expected RT success log:

```powershell
adb logcat -d -s HordeRtProbeBridge AndroidRuntime
```

Look for `RT frame reached Android swapchain presentation.`
Also require `PBR material encoding: ASTC 6x6 diffuse/ARM + ASTC 4x4 normal (KTX2)` on the target phone.

Reports are stored under `files/reports/` in app-private storage and can be retrieved with `adb shell run-as com.samfa12.hordelanternrt`.

The current primary test device is Samsung `SM-S948B`. Re-run the 126-interval phone performance gate after renderer, animation, or material-path changes.

The combat/ASTC build passed that gate on 2026-07-14: strict ASTC selection, honest RT swapchain presentation, stable movement/look/swing input, and two samples at 12.500 ms median / 16.667 ms p95. See `../docs/COMBAT_ASTC_PHONE_VALIDATION_2026-07-14.md`.

The articulated grip-locked, pitch-following revision builds for all Android ABIs and is verified on `SM-S948B`: strict ASTC selection, honest RT presentation, live idle/swing grip composition, and thermal-status-2 sustained evidence at 52.352 SurfaceFlinger TimeStats average FPS / 19.718 ms internal median (approximately 50.7 FPS). See `../docs/PLAYER_BODY_RT_SLICE_2026-07-14.md`.
