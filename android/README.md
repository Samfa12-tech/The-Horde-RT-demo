# Android Platform Path

Phase 0B now runs the Vulkan capability probe from a minimal Android Studio app shell.

## Current status

- Android Studio module exists at `android/`.
- Native JNI bridge is added at `android/app/src/main/cpp/android_probe_bridge.cpp`.
- Java entrypoint displays plain-text diagnostics in a single `TextView` (`MainActivity`).
- Probe reports are written to app private storage:
  - `files/reports/vulkan_capability_report.txt`
  - `files/reports/vulkan_capability_report.json`
- No renderer, surface swapchain, or gameplay is implemented in this phase.

## Android module files

- `android/build.gradle`
- `android/settings.gradle`
- `android/app/build.gradle`
- `android/app/src/main/AndroidManifest.xml`
- `android/app/src/main/cpp/CMakeLists.txt`
- `android/app/src/main/cpp/android_probe_bridge.cpp`
- `android/app/src/main/java/com/samfa12/hordelanternrt/MainActivity.java`
- `android/app/src/main/java/com/samfa12/hordelanternrt/ProbeBridge.java`

## Build & run (Phase 0B)

```powershell
cd android
./gradlew.bat assembleDebug
```

If using a Unix-style shell:

```bash
cd android
./gradlew assembleDebug
```

Then install and run on the Galaxy S26 Ultra as usual from Android Studio or `adb install`.

## Retrieve reports

From a connected device/shell:

```bash
adb shell run-as com.samfa12.hordelanternrt ls files/reports
adb shell run-as com.samfa12.hordelanternrt cat files/reports/vulkan_capability_report.txt
adb shell run-as com.samfa12.hordelanternrt cat files/reports/vulkan_capability_report.json
```

The app also renders the on-screen text report with:

- `Backend`
- `RT mode`
- GPU/device IDs
- driver/API versions
- required extension/feature presence
- unsupported diagnostics (when present)
- storage confirmation

## Known limitations

- On-screen output is plain text only.
- No Vulkan surface/rendering path is added yet.
- This is a capability probe shell only.

## Next smallest task

Create a minimal native Vulkan surface path on Android and Windows so the same diagnostics text is presented through a proper in-app overlay without any fake rendering path.