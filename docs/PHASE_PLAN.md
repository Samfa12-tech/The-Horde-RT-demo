# Phase Plan

## Phase 0 - Vulkan RT Capability Probe

Goal: prove the target devices expose the required Vulkan hardware RT support.

### Completed in this slice

- Real Vulkan instance creation.
- Physical device enumeration.
- Per-device extension + feature-chain probe.
- RT mode evaluation:
  - `RayTracingPipeline`
  - `RayQuery`
  - `Unsupported`
- Diagnostics and report generation.
- Windows CLI executable `horde_rt_capability_probe`.
- Android JNI shell under `android/` with native `TextView` diagnostics and report persistence.
- Win32 diagnostic window app (`horde_rt_diagnostic_window`) with shared probe formatting.
- Native Android minimal RT scene skeleton status check now runs through the same probe core and validates RT pipeline/AS runtime entry points.

### Current known limitations

- No on-screen Vulkan RT rendering path yet.
- No swapchain or surface path yet.
- Android output now includes Vulkan diagnostic surface plus shared text overlay.
- Native swapchain/surface pipeline is in place on Windows and Android for diagnostics.

## Next smallest task

Keep unsupported-device diagnostics unchanged, then start Phase 1B (tiny native RT render path) using shared probe results.

## Planned milestone sequence

1. Phase 1 - Minimal hardware RT scene
2. Phase 2 - Torch corridor visual prototype
3. Phase 3 - Movement and simple combat shell
4. Phase 4 - Asset upgrade
5. Phase 5 - Playable route
6. Phase 6 - Release package
