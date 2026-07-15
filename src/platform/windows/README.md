# Windows Platform Path

Windows is the secondary/equal hardware target for this phase.

## Build and run

- Build and run the CLI capability probe:
  - `cmake -S . -B build`
  - `cmake --build build --config Debug --target horde_rt_capability_probe`
  - `.\build\Debug\horde_rt_capability_probe.exe`
- Build and run the native diagnostic window:
  - `cmake --build build --config Debug --target horde_rt_diagnostic_window`
  - `.\build\Debug\HordeLanternRT.exe`
- Reports are written to:
  - `reports/vulkan_capability_report.txt`
  - `reports/vulkan_capability_report.json`

## What is implemented

- Shared probe logic from `src/vulkan/*` and `src/ui/DiagnosticOverlay.cpp`.
- The interactive Win32 path presents the shared RT scene through a Vulkan swapchain with `WASD`, mouse-drag look, right mouse/Space swing, `Esc` pause, `R` restart, `F1` controls, `F2` diagnostics, and `Alt+Enter` fullscreen.
- The renderer builds BLAS/TLAS, dispatches `vkCmdTraceRaysKHR`, and preserves clear unsupported-device diagnostics.
- Branded entry/pause/settings/diagnostics UI, one-enemy combat, FilmCow SFX, persisted settings, and 50-100% RT render scaling are implemented.
- Release assets resolve beside `HordeLanternRT.exe`; the portable zip uses the static MSVC runtime and leaves Vulkan as a driver prerequisite.
- Console output remains available from the CLI probe target.

## Intentionally bounded

- One enemy only; no horde AI, block/dodge, or player-death system.
- No fake RT or non-Vulkan success fallback.
- The current alpha has one clear skylight transmission slice and reflection groundwork, not the complete coloured-light/mirror route.

Windows remains the equal validation target, but phone-safe renderer and asset decisions remain authoritative.
