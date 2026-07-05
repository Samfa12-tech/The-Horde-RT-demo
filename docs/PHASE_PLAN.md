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

### Current known limitations

- No rendering path yet.
- No swapchain or surface path yet.
- Android screen output is still minimal (toast), not a long-lived in-app overlay.
- No on-device overlay yet.

## Next smallest task

Run the native Android probe on the Galaxy S26 Ultra, confirm `run-as` report extraction, and then upgrade the on-screen diagnostic path from toast to a deterministic in-app overlay.

## Planned milestone sequence

1. Phase 1 - Minimal hardware RT scene
2. Phase 2 - Torch corridor visual prototype
3. Phase 3 - Movement and simple combat shell
4. Phase 4 - Asset upgrade
5. Phase 5 - Playable route
6. Phase 6 - Release package
