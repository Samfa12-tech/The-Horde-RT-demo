# Windows Platform Path

Windows is the secondary/equal hardware target for this phase.

## Window for this slice

- Build and run the CLI capability probe:
  - `cmake -S . -B build`
  - `cmake --build build --target horde_rt_capability_probe`
  - `.\build\horde_rt_capability_probe.exe`
- Outputs are written to `reports/`:
  - `vulkan_capability_report.txt`
  - `vulkan_capability_report.json`

## What is implemented

- No Win32 windowed overlay is wired yet.
- Native Vulkan capability probe logic is implemented and runs as CLI.
- Probe prints logs to stdout and writes text/JSON reports.

## What is intentionally not implemented yet

- Swapchain and present path.
- Windowed diagnostics overlay.
- Rendering / scene.
- Android overlay parity.

## Next task

- Keep this logic as shared core while Android brings the same `VulkanContext` probe into a native activity.
