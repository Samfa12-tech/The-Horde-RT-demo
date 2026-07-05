# Install Checklist

Phase 0A implements a real Vulkan capability-probe core on Windows, and Android native wiring now shares the same core and writes device reports to app-private storage.

## Windows host tools

- Windows 11.
- Git.
- Visual Studio 2022 with **Desktop development with C++**.
- CMake.
- Ninja (optional, supported by CMake).
- Vulkan SDK.
- RenderDoc.
- NVIDIA Nsight Graphics.
- Vulkan Hardware Capability Viewer or equivalent.

## Android tools

- Android Studio.
- Android SDK.
- Android NDK.
- Android CMake package.
- USB debugging enabled on the Samsung Galaxy S26 Ultra.
- C++17/CMake-friendly Android native activity project template.

## Phase 0A exact setup checks

1. Confirm Vulkan SDK availability.
2. Configure:
   - `cmake -S . -B build`
3. Build the probe executable:
   - `cmake --build build --target horde_rt_capability_probe`
4. Run from repository root:
   - `.\build\horde_rt_capability_probe.exe`
5. Verify output and ensure files exist:
   - `reports/vulkan_capability_report.txt`
   - `reports/vulkan_capability_report.json`
6. Confirm the log shows device enumeration, extension/feature support, and explicit missing requirements for unsupported hardware.

7. Confirm Android native integration:
   - `cmake -S . -B build-android -DCMAKE_SYSTEM_NAME=Android -DANDROID_PLATFORM=33 -DANDROID_ABI=arm64-v8a -DHORDE_RT_BUILD_ANDROID_CAPABILITY_PROBE=ON`
   - `cmake --build build-android --target horde_rt_capability_probe_android`
   - Package the library into a NativeActivity APK that includes `src/platform/android/AndroidManifest.xml`.
   - Run on-device probe and verify with:
     - `adb shell run-as com.samfa12.hordet.probe cat /data/data/com.samfa12.hordet.probe/files/reports/vulkan_capability_report.txt`
     - `adb shell run-as com.samfa12.hordet.probe cat /data/data/com.samfa12.hordet.probe/files/reports/vulkan_capability_report.json`

## Android status for this slice

Android is now wired to a native activity path in this PR. Do not mark complete until an APK is built and the reports are verified on a Galaxy S26 Ultra device.

## Explicit unsupported policy

At no point should the project be treated as working when the probe reports:

- `RT mode: Unsupported`
- required extension and feature diagnostics indicating any missing path-tracing requirements.
