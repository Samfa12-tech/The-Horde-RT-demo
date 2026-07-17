# Project Decisions - The Horde RT Demo / Horde Lantern RT

This file records locked decisions for the native Vulkan hardware ray-tracing demo.

## Identity

- Public project: Samfa12 technology demo.
- Repository: `Samfa12-tech/The-Horde-RT-demo`.
- Working creative title: **Horde Lantern RT**.
- Distribution: itch hosts the canonical Android and Windows artifacts at `https://samfa12.itch.io/the-horde`; Samfa12.com links to that page instead of hosting a second copy. The live `/games/` card was rendered and link-verified on 2026-07-15.

## Core decision

> RT or nothing.

The demo must prove real native Vulkan hardware ray tracing. Browser rendering, Godot renderer work, Unreal-first work, Three.js, Babylon.js, WebGPU, raster-only rendering, SSR, baked lighting, compute-only path tracing, and fake RT are not acceptable substitutes for the core proof.

## Build / test / demo cycle decision - 2026-07-17

The first supporting foundation is complete: Android Debug has deterministic route checkpoints, three-window measurement, native route replay, fixed screenshots, state evidence, and a bounded one-command runner. The live developer overlay is now complete and live-validated on Windows; its shared Android Debug plumbing compiles but remains device-unvalidated until the phone is attached. The remaining cycle work is that Android device gate, integrated cross-platform clean-build/package/stale-shader/licence gates, and fixed video/presentation capture. Detailed scope and guardrails live in `docs/BUILD_TEST_DEMO_CYCLE_PLAN_2026-07-17.md`.

The normal player-facing route remains intact; development checkpoints and overlays must stay tucked away from branded entry/pause/settings surfaces. Game-facing combat polish follows the tooling foundation. Do not raise the one-active-skinned-enemy limit without a separate phone measurement.

Windows Debug uses F3 for the compact live overlay while F2 retains the full paused RT diagnostics surface. Release builds omit the F3 control, help text, menu command, and live overlay UI. Android Release keeps the native developer-overlay request empty; only debuggable builds may show the hidden view.

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

Feature structs checked and enabled by the current capability path:

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
- Meshy-assisted assets are allowed when the underlying source permits use and the applicable Meshy attribution route is recorded.
- Meshy models must be textured before export.
- Do not import untextured Meshy models and call them complete.
- Prefer glTF/GLB where practical.
- Use high-quality PBR textures from the start when actual visual work begins.
- Phase 0 should not import big assets.

## Deferred from the original scaffold

The following were intentionally outside the original scaffold. Gameplay, Meshy integration, the torch room, BLAS/TLAS, SBT, audio, and packaging have since been implemented in bounded alpha form; broader versions remain deferred.

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
- Windows now runs the same RT corridor as an interactive desktop scene: `WASD` moves, left mouse/trackpad click-drag looks, and `Esc` pauses/resumes.
- The RTX 5050 laptop reported `RayTracingPipeline` and successful RT swapchain presentation at 984 x 661 in the interactive Windows build.
- A Hotstrike Studio stylized skeleton, subsequently textured, rigged, and animated with Meshy, has 11 correctly named animations and is live through a narrow CPU-skinned RT path. The sword remains separate; the 12,358-triangle sword LOD stays staged until a measured static GLB/PBR upload path exists.

Important technical finding:

- A recursive path-tracing attempt using closest-hit secondary `traceRayEXT` calls and pipeline recursion depth 2 failed on the phone at RT pipeline creation.
- The stable phone path uses `rayQueryEXT` from raygen to query the same TLAS for primary hits, shadow rays, and a first bounce sample while keeping pipeline recursion depth 1.
- This keeps the project aligned with RT-or-nothing because ray queries use Vulkan hardware acceleration-structure traversal.

## Initial alpha 0.1.0 route decision - 2026-07-15 historical

- Initial Showing Alpha `0.1.0-alpha.1` is published on separate Windows and Android itch channels.
- The 2026-07-16 hardened refresh was published as Windows build `#1798649` and Android build `#1798652`; its exact hashes remain in the dated 0.1.0 validation records.
- Android uses strict ASTC KTX2 arrays and a stable Samfa12 signing identity; Windows uses a portable executable-relative asset tree.
- Final `SM-S948B` validation passed at 100%, 75%, and 50%. The 75% tier is the sustained phone recommendation; report 100% separately rather than treating it as an unconditional 50+ FPS promise.
- The coloured route was subsequently completed for 0.1.1. The stained transmission pane was rejected in hands-on review and removed; the threshold remains open.
- Keep the textured sword LOD out of runtime until static GLB/PBR support is measured on phone.
- Preserve phone-safe ray-query shading and real `vkCmdTraceRaysKHR` presentation unless a stronger RT route is proven on-device.

## Showcase alpha 0.1.1 publication - 2026-07-17

- Published the completed route as **Showcase Alpha 0.1.1**, package version `0.1.1-alpha.1`, with Android `versionCode 2`.
- Keep the existing itch channels and stable Android update identity. Public build IDs are Windows `#1801016` and Android `#1801017`; exact hashes and validation evidence are recorded in `docs/SHOWCASE_ALPHA_RELEASE_VALIDATION_2026-07-17.md`.
- Treat 75% RT resolution as the sustained Android recommendation. The 100% pass proves full extent and image correctness but carries no 50 FPS promise.
- Keep one rendered/animated skinned enemy at a time. A simultaneous horde requires its own measured phone slice.

## Initial alpha refresh hardening - 2026-07-16

- Keep the local r26 NDK, link the C++ runtime statically, and require 16 KiB ELF/APK alignment in the packaging gate. This removes the unaligned r26 `libc++_shared.so` without adding a second toolchain dependency.
- Publish Android's first performance sample after 30 frames so diagnostics do not appear broken; retain 120-frame steady-state updates afterward.
- Treat 125% as the completed live Windows DPI validation for this refresh. Explicit 100%/150% repeats remain a non-blocking later compatibility check.
- Do not rewrite public Git history until Hotstrike answers the explicit permission request or the owner chooses history remediation.
