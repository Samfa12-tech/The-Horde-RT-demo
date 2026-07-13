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

## Tested Phase 1 status - 2026-07-10

The project has moved beyond the original Phase 0 scaffold/probe.

Current proven path:

- Android app builds, installs, and launches on Samsung `SM-S948B`.
- Native Vulkan swapchain path presents an RT-produced storage image to screen.
- The scene uses BLAS/TLAS acceleration structures, an RT pipeline/SBT, `vkCmdTraceRaysKHR`, and a storage-image-to-swapchain copy.
- Reports only set `rtScene.presented = true` after successful swapchain presentation.
- The visible scene is now a first-person gothic corridor/ruin prototype with a handheld medieval torch, reflective objects, puddle/wet-stone response, horde silhouettes, fog, and moonlight through a physical second-room roof breach.
- The current source represents the held torch as a real emissive BLAS/TLAS instance rather than a screen overlay. Its laptop and target-phone visual proofs are complete.
- The live laptop and phone proofs show the real orange flame and a deterministic ray-query reflection in a high-reflectivity wall insert. The raw RGBA-to-BGRA copy is explicitly compensated so the flame cannot turn cyan at presentation.
- Controls are now left-drag movement/strafe and right-drag 360 look.
- Windows now runs the same RT corridor as an interactive desktop scene: `WASD` moves, left mouse/trackpad click-drag looks, and `Esc` exits.
- The RTX 5050 laptop reported `RayTracingPipeline` and successful RT swapchain presentation at 984 x 661 in the interactive Windows build.
- A Meshy biped skeleton with 11 correctly named animations is live through a narrow CPU-skinned RT path. The sword remains separate; the 12,358-triangle sword LOD stays staged until a measured static GLB/PBR upload path exists.

Important technical finding:

- A recursive path-tracing attempt using closest-hit secondary `traceRayEXT` calls and pipeline recursion depth 2 failed on the phone at RT pipeline creation.
- The stable phone path uses `rayQueryEXT` from raygen to query the same TLAS for primary hits, shadow rays, and a first bounce sample while keeping pipeline recursion depth 1.
- This keeps the project aligned with RT-or-nothing because ray queries use Vulkan hardware acceleration-structure traversal.

## Next smallest task - compressed mobile PBR arrays

The first animated skeleton and imported environment PBR batches are live and phone-validated. The next bounded task is:

1. Replace the three raw runtime PBR arrays with a capability-checked mobile GPU-compressed format.
2. Re-run the cold 126-interval phone benchmark and preserve the 50+ FPS median gate.
3. Keep the textured sword LOD out of the runtime until the static GLB/PBR path exists and is measured on phone.
4. Preserve the phone-safe ray-query path tracing route unless a stronger RT route is proven on device.
