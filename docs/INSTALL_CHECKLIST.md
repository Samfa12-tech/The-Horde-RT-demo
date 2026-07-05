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

From repository root:

```powershell
cd android
./gradlew.bat assembleDebug
```

Optional install:

```bash
adb install app/build/outputs/apk/debug/app-debug.apk
```

Run and collect on-device diagnostics:

```bash
adb shell run-as com.samfa12.hordelanternrt ls files/reports
adb shell run-as com.samfa12.hordelanternrt cat files/reports/vulkan_capability_report.txt
adb shell run-as com.samfa12.hordelanternrt cat files/reports/vulkan_capability_report.json
```

## Verification expectations

- The on-screen output must show the required diagnostic fields from the shared probe report.
- Unsupported hardware must still show `RT mode: Unsupported` and clear missing extension/feature diagnostics.
- Windows and Android must continue writing both text and JSON reports.
- Do not claim Android or Windows support complete until runtime verification on target hardware is complete.
- Do not mark Vulkan rendering support until a real surface/swapchain rendering path and RT scene path are implemented.

## Next milestone guardrails

- Keep the path text-only.
- Keep unsupported-device honesty intact.
- Do not add gameplay, fake RT, raster-only fallback, or scene rendering in this phase.
