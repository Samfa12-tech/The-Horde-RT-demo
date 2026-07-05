# Windows Platform Path

Windows is the secondary/equal hardware target for this phase.

## Windowed diagnostics for this slice

- Build and run the CLI capability probe:
  - `cmake -S . -B build`
  - `cmake --build build --target horde_rt_capability_probe`
  - `.\build\horde_rt_capability_probe.exe`
- Build and run the native diagnostic window:
  - `cmake --build build --target horde_rt_diagnostic_window`
  - `.\build\horde_rt_diagnostic_window.exe`
- Reports are written to:
  - `reports/vulkan_capability_report.txt`
  - `reports/vulkan_capability_report.json`

## What is implemented

- Shared probe logic from `src/vulkan/*` and `src/ui/DiagnosticOverlay.cpp`.
- Win32 diagnostic shell displays probe text in a native window (`horde_rt_diagnostic_window`).
- Console output remains available from the CLI probe target.

## What is intentionally not implemented yet

- No Vulkan surface swapchain path yet.
- No RT scene/scene rendering path yet.
- No Android rendering overlay through Vulkan on this target yet.
- Gameplay, torch room, audio, fake RT, or shader pipeline.

## Next task

- Add a minimal Vulkan surface path (or explicit next-step plan) so the same diagnostics can later render through the swapchain path.
