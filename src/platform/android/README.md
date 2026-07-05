# Android platform integration

Phase 0C keeps the dedicated Android Studio module at `android/` and aligns its diagnostics string source with the shared desktop overlay formatting.

- Native JNI bridge: `android/app/src/main/cpp/android_probe_bridge.cpp`
- Java entrypoint: `android/app/src/main/java/com/samfa12/hordelanternrt/MainActivity.java`
- Diagnostic text is monospaced on a dark background.
- Build entry: `android/build.gradle`, `android/app/build.gradle`, `android/app/src/main/cpp/CMakeLists.txt`
- Reports path on-device: `files/reports/vulkan_capability_report.txt` and `files/reports/vulkan_capability_report.json`
- Retrieval example:

```bash
adb shell run-as com.samfa12.hordelanternrt ls files/reports
adb shell run-as com.samfa12.hordelanternrt cat files/reports/vulkan_capability_report.txt
adb shell run-as com.samfa12.hordelanternrt cat files/reports/vulkan_capability_report.json
```

## Native diagnostics on Android

- Probe text rendering remains TextView-based for now.
- Unsupported hardware still shows a clear warning and diagnostic details via the shared overlay text path.
- Report writing uses the same C++ probe core as the Windows CLI and diagnostic window.
