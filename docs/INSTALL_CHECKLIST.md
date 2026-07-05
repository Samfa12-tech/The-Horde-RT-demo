# Install Checklist

Phase 0A implements a real Vulkan capability-probe core on Windows. Android is acknowledged but still pending integration.

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

## Android status for this slice

Android is not yet wired into the native app shell in this PR. Use the documented plan in:

- `src/platform/android/README.md`
- `docs/TECHNICAL_REQUIREMENTS.md`
- `docs/PHASE_PLAN.md`

Do not mark Android support as complete until an Android CMake/NDK build compiles and writes the same probe report on-device.

## Explicit unsupported policy

At no point should the project be treated as working when the probe reports:

- `RT mode: Unsupported`
- required extension and feature diagnostics indicating any missing path-tracing requirements.
