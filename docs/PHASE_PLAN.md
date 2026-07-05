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

### Current known limitations

- No rendering path yet.
- No swapchain or surface path yet.
- Android output is plain text only (no in-app render overlay yet).
- No desktop/Android surface pipeline yet.

## Next smallest task

Wire a minimal native Vulkan surface/swapchain path on both Android and Windows to render the same diagnostic overlay text without a fake scene.

## Planned milestone sequence

1. Phase 1 - Minimal hardware RT scene
2. Phase 2 - Torch corridor visual prototype
3. Phase 3 - Movement and simple combat shell
4. Phase 4 - Asset upgrade
5. Phase 5 - Playable route
6. Phase 6 - Release package
