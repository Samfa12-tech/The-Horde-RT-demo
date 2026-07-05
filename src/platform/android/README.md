# Android Platform Path

Android now wires the same probe core into a native app shell.

## Current status

- Android native CMake target is wired:
  - `horde_rt_capability_probe_android`
- Probe source shares the same core with Windows:
  - `src/vulkan/VulkanContext.cpp`
  - `src/vulkan/RtCapabilityReport.cpp`
- Reports are written to app-private storage:
  - `${Context.getFilesDir()}/reports/vulkan_capability_report.txt`
  - `${Context.getFilesDir()}/reports/vulkan_capability_report.json`
- A minimal on-device display path uses a transient Android Toast message with key diagnostics.

## Files added/used in this slice

- `src/platform/android/CapabilityProbeAndroidMain.cpp` (NativeActivity entrypoint)
- `src/platform/android/AndroidManifest.xml` (template activity manifest)
- `CMakeLists.txt` target: `horde_rt_capability_probe_android`

## Build and run (native activity path)

1. Configure an Android build directory with NDK toolchain:

```powershell
cmake -S . -B build-android `
    -DCMAKE_SYSTEM_NAME=Android `
    -DCMAKE_ANDROID_NDK=<path-to-android-ndk> `
    -DCMAKE_ANDROID_ARCH_ABI=arm64-v8a `
    -DCMAKE_ANDROID_PLATFORM=android-33 `
    -DANDROID_PLATFORM=33 `
    -DANDROID_ABI=arm64-v8a `
    -DANDROID_STL=c++_static `
    -DCMAKE_ANDROID_NDK_TOOLCHAIN_VERSION=clang `
    -DHORDE_RT_BUILD_SCAFFOLD_CODE=ON `
    -DHORDE_RT_BUILD_CAPABILITY_PROBE_EXECUTABLE=OFF `
    -DHORDE_RT_BUILD_ANDROID_CAPABILITY_PROBE=ON
```

2. Build native library:

```powershell
cmake --build build-android --target horde_rt_capability_probe_android
```

3. Package the built `.so` (`build-android/libhorde_rt_capability_probe_android.so`) plus `src/platform/android/AndroidManifest.xml` into an APK using your standard Android Studio flow (template or existing app wrapper).

4. Launch from launcher, then pull report artifacts:

```bash
adb shell run-as com.samfa12.hordet.probe ls /data/data/com.samfa12.hordet.probe/files/reports
adb shell run-as com.samfa12.hordet.probe cat /data/data/com.samfa12.hordet.probe/files/reports/vulkan_capability_report.txt
adb shell run-as com.samfa12.hordet.probe cat /data/data/com.samfa12.hordet.probe/files/reports/vulkan_capability_report.json
```

## Do not claim Android support is complete

Mark this path complete only if:
- Native target builds through CMake+NDK.
- The probe writes both reports under app-private storage on device.
- Toast output shows RT mode and core identity (GPU/IDs).
- Unsupported hardware path remains honest (`RT mode: Unsupported` with missing requirements).

## Exact next task if this slice stalls

If the native activity build system is blocked by host tooling, continue with:
Create the Android Studio shell module that consumes `horde_rt_capability_probe_android` and merges `AndroidManifest.xml`, then iterate on display output reliability.
