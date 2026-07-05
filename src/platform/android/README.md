# Android Platform Path

Android is the primary target. For this slice, native Android build integration is documented as the next task.

## Current status

- The probe core (`VulkanContext`) is implemented and shared.
- Android CMake/NDK target wiring is not yet implemented in-code.
- No Android-native on-screen report overlay exists yet.

## Android build plan (next slice)

1. Add an Android CMake target that builds `horde_rt_scaffold` and a new native `horde_rt_capability_probe_android` target.
2. Add `AndroidManifest` and activity shell entry point to initialise the probe and trigger a one-shot output.
3. Reuse current `src/vulkan/VulkanContext.cpp` and `src/vulkan/RtCapabilityReport.cpp` with no Windows-only assumptions.
4. Write report files to application storage:
   - `${Context.getFilesDir()}/reports/vulkan_capability_report.txt`
   - `${Context.getFilesDir()}/reports/vulkan_capability_report.json`
5. Display selected RT mode and diagnostics in a temporary native overlay text area.
6. Add one adb command check list to verify on-device output.

## Do not claim Android is complete

Do not mark Android support complete unless:

- the Android native target builds through CMake+NDK,
- the same probe logic runs on device,
- reports are written and retrievable from storage,
- unsupported mode prints exact missing requirement diagnostics.
