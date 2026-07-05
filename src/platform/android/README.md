# Android platform integration

Phase 0B now uses the dedicated Android Studio module at `android/`.

- Native JNI bridge: `android/app/src/main/cpp/android_probe_bridge.cpp`
- Java entrypoint: `android/app/src/main/java/com/samfa12/hordelanternrt/MainActivity.java`
- Build entry: `android/build.gradle`, `android/app/build.gradle`, `android/app/src/main/cpp/CMakeLists.txt`
- Reports path on-device: `files/reports/vulkan_capability_report.txt` and `files/reports/vulkan_capability_report.json`
- Run/verification sequence:

```bash
cd android
./gradlew.bat assembleDebug
./gradlew.bat installDebug
adb shell monkey -p com.samfa12.hordelanternrt 1
```

On-device checks and retrieval:

```bash
adb devices
adb shell getprop ro.product.model
adb shell getprop ro.product.manufacturer
adb shell run-as com.samfa12.hordelanternrt ls files/reports
adb shell run-as com.samfa12.hordelanternrt cat files/reports/vulkan_capability_report.txt
adb shell run-as com.samfa12.hordelanternrt cat files/reports/vulkan_capability_report.json
```

Verified result:

- Device: `SM-S948B` / `samsung`
- RT mode: `RayTracingPipeline`
- GPU: `Adreno (TM) 840`
- Vulkan API version: `1.4.295`
- All RT capability checks passed:
  - `VK_KHR_acceleration_structure: yes`
  - `VK_KHR_ray_tracing_pipeline: yes`
  - `VK_KHR_ray_query: yes`
  - `VK_KHR_buffer_device_address: yes`
  - `VK_KHR_deferred_host_operations: yes`
