# Android Vulkan RT App

The `android/` module is the supported phone path for Horde Lantern RT. It owns the Java activity and lifecycle/JNI bridge while compiling the shared renderer and scene sources from `src/`.

## Current implementation

- Java entrypoint: `app/src/main/java/com/samfa12/hordelanternrt/MainActivity.java`
- JNI/Vulkan bridge: `app/src/main/cpp/android_probe_bridge.cpp`
- Shared native source manifest: `../cmake/HordeRtSources.cmake`
- Native Vulkan RT presentation through the Android swapchain
- One frame in flight while the held-prop TLAS uses a host-written instance buffer
- Touch movement/look, corridor collision, animated skeleton BLAS refit, PBR texture arrays, and phone-safe ray-query shading inside `vkCmdTraceRaysKHR`
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

Reports are stored under `files/reports/` in app-private storage and can be retrieved with `adb shell run-as com.samfa12.hordelanternrt`.

The current primary test device is Samsung `SM-S948B`. Re-run the 126-interval phone performance gate after renderer, animation, or material-path changes.
