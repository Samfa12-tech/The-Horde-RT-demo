# Project Decisions - The Horde RT Demo / Horde Lantern RT

This file records locked decisions for the native Vulkan hardware ray-tracing demo.

## Identity

- Public project: Samfa12 technology demo.
- Repository: `Samfa12-tech/The-Horde-RT-demo`.
- Working creative title: **Horde Lantern RT**.
- Future distribution: downloadable from `samfa12.com`.

## Core decision

> RT or nothing.

The demo must prove real native Vulkan hardware ray tracing. Browser rendering, Godot renderer work, Unreal-first work, Three.js, Babylon.js, WebGPU, raster-only rendering, SSR, baked lighting, compute-only path tracing, and fake RT are not acceptable substitutes for the core proof.

## Target devices

- Primary target: Samsung Galaxy S26 Ultra.
- Secondary / equal target: Windows laptop with RTX 5050.

Phone is a first-class target, not a late port.

## Rendering path

- Native Vulkan.
- `VK_KHR_ray_tracing_pipeline` is the preferred path.
- `VK_KHR_ray_query` is acceptable only when it genuinely uses Vulkan hardware ray traversal and is clearly labelled as RayQuery mode.
- `VK_KHR_acceleration_structure` is essential for Vulkan RT.
- Unsupported devices must show diagnostics instead of silently falling back.

## Phase 0 decision

The first real project step is **Phase 0 - Vulkan RT Capability Probe**.

Phase 0 does not build gameplay. It creates the repo foundation, build-path documentation, source layout, and then the real capability probe.

Phase 0 must eventually query/report:

```text
Backend: Vulkan
RT mode: RayTracingPipeline / RayQuery / Unsupported
GPU name
Vendor ID
Device ID
Driver version
Vulkan API version
VK_KHR_acceleration_structure: yes/no
VK_KHR_ray_tracing_pipeline: yes/no
VK_KHR_ray_query: yes/no
VK_KHR_buffer_device_address: yes/no
VK_KHR_deferred_host_operations: yes/no
Internal render resolution
FPS / frame time
```

Feature structs to check/enable later:

```text
VkPhysicalDeviceAccelerationStructureFeaturesKHR
VkPhysicalDeviceRayTracingPipelineFeaturesKHR
VkPhysicalDeviceRayQueryFeaturesKHR
VkPhysicalDeviceBufferDeviceAddressFeatures
```

## Creative direction

- Historical gothic.
- Dark torch-lit tunnels and corridors.
- Wet stone, puddles, fog, fire torches, lanterns, shadows, and global illumination where feasible.
- Later transition into an open ruined courtyard or colosseum.
- Later simple combat against goblins/gremlins.
- Environment, lighting, textures, and RT proof matter more than complex combat.

## Source/reference policy

Use this hierarchy:

1. Primary base/reference: KhronosGroup/Vulkan-Samples, especially `samples/extensions/ray_tracing_basic`.
2. Main learning/reference: NVIDIA `nvpro-samples/vk_raytracing_tutorial_KHR`.
3. Focused reference snippets: Sascha Willems Vulkan examples.
4. Backup/reference only: Diligent Engine.
5. Deferred/not first base: The Forge and Unreal Engine.

Do not dump a giant third-party engine into this repo. If code is later adapted from a permissive source, preserve license notices and document the source.

## Asset rules

- All assets must be commercial-safe.
- Asset source and license must be recorded in `ASSET_LICENSES.md`.
- Meshy assets are allowed later.
- Meshy models must be textured before export.
- Do not import untextured Meshy models and call them complete.
- Prefer glTF/GLB where practical.
- Use high-quality PBR textures from the start when actual visual work begins.
- Phase 0 should not import big assets.

## Deferred work

The following are intentionally not part of the scaffold step:

- Gameplay systems.
- Horde/enemy behaviour.
- Meshy asset import.
- Pocket Chordsmith audio.
- Torch-lit RT room implementation.
- Shader binding table implementation.
- BLAS/TLAS implementation.
- Platform packaging.

## Next smallest task

Implement the real Vulkan device capability probe that detects and reports Vulkan RT support on Windows and Android, writes a JSON/text capability report, and shows an unsupported diagnostic screen when hardware RT is unavailable.
