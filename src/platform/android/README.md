# Android platform integration

Phase 0B now uses the dedicated Android Studio module at `android/`.

- Native JNI bridge: `android/app/src/main/cpp/android_probe_bridge.cpp`
- Java entrypoint: `android/app/src/main/java/com/samfa12/hordelanternrt/MainActivity.java`
- Build entry: `android/build.gradle`, `android/app/build.gradle`, `android/app/src/main/cpp/CMakeLists.txt`
- Reports path on-device: `files/reports/vulkan_capability_report.txt` and `files/reports/vulkan_capability_report.json`
- Retrieval example:

```bash
adb shell run-as com.samfa12.hordelanternrt ls files/reports
adb shell run-as com.samfa12.hordelanternrt cat files/reports/vulkan_capability_report.txt
adb shell run-as com.samfa12.hordelanternrt cat files/reports/vulkan_capability_report.json
```