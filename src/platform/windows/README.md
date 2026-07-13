# Windows Platform Path

Windows is the secondary/equal hardware target for this phase.

## Build and run

- Build and run the CLI capability probe:
  - `cmake -S . -B build`
  - `cmake --build build --config Debug --target horde_rt_capability_probe`
  - `.\build\Debug\horde_rt_capability_probe.exe`
- Build and run the native diagnostic window:
  - `cmake --build build --config Debug --target horde_rt_diagnostic_window`
  - `.\build\Debug\horde_rt_diagnostic_window.exe`
- Reports are written to:
  - `reports/vulkan_capability_report.txt`
  - `reports/vulkan_capability_report.json`

## What is implemented

- Shared probe logic from `src/vulkan/*` and `src/ui/DiagnosticOverlay.cpp`.
- The interactive Win32 path presents the shared RT scene through a Vulkan swapchain with `WASD`, mouse-drag look, and `Esc` exit.
- The renderer builds BLAS/TLAS, dispatches `vkCmdTraceRaysKHR`, and preserves clear unsupported-device diagnostics.
- Console output remains available from the CLI probe target.

## What is intentionally not implemented yet

- No enemy AI, combat loop, or audio yet.
- No fake RT or non-Vulkan success fallback.

Windows remains the equal validation target, but phone-safe renderer and asset decisions remain authoritative.
