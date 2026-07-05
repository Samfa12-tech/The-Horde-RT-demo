# Install Checklist

## Phase 0B target

Phase 0B adds a minimal Android native shell for the existing Vulkan capability-probe core.

## Windows checks (from prior slice)

- Confirm Vulkan SDK availability.
- Configure:
  - `cmake -S . -B build`
- Build the probe executable:
  - `cmake --build build --target horde_rt_capability_probe`
- Run:
  - `./build/horde_rt_capability_probe` (Windows: `build\\horde_rt_capability_probe.exe`)
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

- The on-screen output must show the required diagnostic fields from the probe report.
- Unsupported hardware must still show `RT mode: Unsupported` and clear missing extension/feature lines.
- The probe must write both reports under `files/reports` in app private storage.
- Do not mark Android support complete until APK install/run is verified on a target device.