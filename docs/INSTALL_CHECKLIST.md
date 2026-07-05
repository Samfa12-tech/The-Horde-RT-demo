# Install Checklist

## Phase 0C target

Phase 0C adds the native diagnostic window shell on Windows and keeps the Android TextView-based probe UI.

## Windows checks

- Confirm Vulkan SDK availability.
- Configure:
  - `cmake -S . -B build`
- Build the CLI probe executable:
  - `cmake --build build --target horde_rt_capability_probe`
- Build the Windows diagnostic window executable:
  - `cmake --build build --target horde_rt_diagnostic_window`
- Run:
  - `./build/horde_rt_capability_probe` (Windows: `build\\horde_rt_capability_probe.exe`)
  - `./build/horde_rt_diagnostic_window` (Windows: `build\\horde_rt_diagnostic_window.exe`)
- Verify reports:
  - `reports/vulkan_capability_report.txt`
  - `reports/vulkan_capability_report.json`

## Android checks

From repository root (verified on-device on `SM-S948B`):

```powershell
cd android
./gradlew.bat assembleDebug
./gradlew.bat installDebug
adb shell monkey -p com.samfa12.hordelanternrt 1
```

Optional install:

```bash
adb install app/build/outputs/apk/debug/app-debug.apk
```

Alternate install:

```powershell
./gradlew.bat installDebug
```

Run and collect on-device diagnostics:

```bash
adb shell run-as com.samfa12.hordelanternrt ls files/reports
adb shell run-as com.samfa12.hordelanternrt cat files/reports/vulkan_capability_report.txt
adb shell run-as com.samfa12.hordelanternrt cat files/reports/vulkan_capability_report.json
```

Sample verified output from `SM-S948B`:

```text
Backend: Vulkan
RT mode: RayTracingPipeline
GPU name: Adreno (TM) 840
Vendor ID: 20803
Device ID: 1141180977
Driver version: 512.842.19
Vulkan API version: 1.4.295
VK_KHR_acceleration_structure: yes
VK_KHR_ray_tracing_pipeline: yes
VK_KHR_ray_query: yes
VK_KHR_buffer_device_address: yes
VK_KHR_deferred_host_operations: yes
features:
  accelerationStructure: true
  rayTracingPipeline: true
  rayQuery: true
  bufferDeviceAddress: true
```

## Verification expectations

- The on-screen output must show the required diagnostic fields from the probe report.
- Unsupported hardware must still show `RT mode: Unsupported` and clear missing extension/feature lines.
- For RayTracingPipeline-capable hardware, the report now also includes a tiny RT scene skeleton status line (`RT scene status`) indicating whether the native RT setup path was built and validated.
- The probe must write both reports under `files/reports` in app private storage and `reports/` on desktop paths.
- Windows and Android should continue writing both text and JSON reports.
- Do not claim Android support complete until APK install/run is verified on target hardware.
- Last verified (2026-07-05): Android build, install, launch, and report retrieval succeeded on `SM-S948B` (`samsung`) with `RayTracingPipeline`.
- Do not add fake RT, raster-only fallback, or render-path placeholders as production substitutes in this phase.

## Next milestone guardrails

- Keep the path text-only.
- Keep unsupported-device honesty intact.
- Do not add gameplay, fake RT, raster-only fallback, or scene rendering beyond diagnostics in this phase.
- Proceed to Phase 1B once on-device RT mode validation is confirmed.
