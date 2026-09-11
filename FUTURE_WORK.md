# Horde Lantern RT - Future Work

This file is the canonical **post-current-release work queue** for Horde Lantern RT. It exists so future Codex sessions do not lose the intended sequencing or the reasoning behind deferred renderer work.

Do not treat this file as permission to interrupt active release work. Work through the phases below **in order unless the project owner explicitly changes the priority**.

## Ordered sequence

### 1. Finish Showcase Alpha 1.6.1

**Status:** active/current work.

Complete the already-scoped 1.6.1 work first. Do not start the music or compatibility projects inside 1.6.1 unless specifically authorised.

The point of this separation is to preserve a clean release/evidence boundary: 1.6.1 should be completed, validated and released on its own terms before the next feature programme begins.

### 2. Add background music

**Status:** next after 1.6.1.

After 1.6.1 is complete, integrate the supplied Horde music pack using the existing Pocket Chordsmith / Chordsmith music-engine direction.

Goals:

- Add the game-ready adaptive background music without disturbing the shared gameplay authority or renderer architecture.
- Support the intended sections/states of the demo, including exploration, combat, torch-loss tension, lich finale and the post-finale/skylight release where appropriate to the supplied music package.
- Add a **music volume** control in Settings, separate from other audio where practical.
- Keep pause/resume, lifecycle and platform audio behaviour correct on Android and Windows.
- Preserve deterministic gameplay and do not make music playback state authoritative for gameplay.
- Apply the normal change-triggered owner audio-validation rule in `AGENTS.md` because this work intentionally changes audible content and playback behaviour.

The exact supplied music pack and its integration notes may supersede implementation details here. This section records the required sequencing, not a frozen audio implementation.

### 3. Expand Android hardware-RT compatibility with a RayQuery backend

**Status:** planned after music.

This is the next major renderer programme after music.

The immediate motivation is a real Samsung Galaxy S25 Ultra test performed on 2026-09-11. The device reached the Horde Vulkan diagnostics but could not run the current scene because its tested driver exposes ray query but not the full Vulkan ray-tracing-pipeline extension.

Tested S25 Ultra diagnostic result:

- GPU: `Adreno (TM) 830`
- Vendor ID: `20803`
- Device ID: `1141178369`
- Driver: `512.800.64` (packed `2150760512`)
- Vulkan API: `1.3.284`
- `VK_KHR_acceleration_structure`: yes
- `VK_KHR_ray_tracing_pipeline`: **no**
- `VK_KHR_ray_query`: yes
- `VK_KHR_buffer_device_address`: yes
- `VK_KHR_deferred_host_operations`: yes
- Selected capability mode: `RayQuery`
- RT scene: not attempted / not presented

The detailed compatibility evidence is recorded in `docs/ANDROID_RT_DEVICE_COMPATIBILITY_RECORD.md`.

## Compatibility goal

Expand device support **without removing Horde's rendering features and without weakening the `RT or nothing` rule**.

The preferred architecture is two first-class real Vulkan hardware-ray-tracing backends:

1. **RayTracingPipeline** - current preferred backend where `VK_KHR_ray_tracing_pipeline` is available.
2. **RayQueryCompute** - alternate hardware-RT backend for devices that expose acceleration structures + `VK_KHR_ray_query` but do not expose the full ray-tracing pipeline.

A device that exposes neither acceptable path remains unsupported and must continue to receive honest diagnostics.

Target selection logic should conceptually become:

```text
if full RT-pipeline requirements are available:
    RayTracingPipeline
else if acceleration structures + ray query + required buffer/device features are available:
    RayQueryCompute
else:
    Unsupported
```

The exact feature/extension gate should be derived from the real resource and shader requirements, not copied blindly from this pseudocode.

## Why this should be feasible in the current engine

The current renderer is already unusually close to this architecture.

- `RayTracingRequirements.cpp` already distinguishes `RayTracingPipeline`, `RayQuery`, and `Unsupported` capability states.
- The current phone-safe ray-generation shader uses `rayQueryEXT` for primary visibility, shadows, first-bounce work and bounded reflection/transmission.
- Shared shader helpers such as `traceScene(...)`, material decoding, direct-light visibility, water transport and dielectric/reflection logic already use ray-query traversal against the Vulkan acceleration structures.
- The current full pipeline still uses `vkCmdTraceRaysKHR` and a ray-generation shader as the frame launcher/presentation route.

This means the compatibility project should **not** begin by rewriting all Horde lighting. The likely route is to separate the frame-launch wrapper from the shared RT shading work.

Conceptually:

```text
Current:
    vkCmdTraceRaysKHR
      -> raygen entry point
      -> shared rayQueryEXT traversal/shading
      -> RT storage image
      -> swapchain presentation

Planned alternate backend:
    vkCmdDispatch
      -> compute entry point
      -> same shared rayQueryEXT traversal/shading
      -> same RT storage image
      -> same swapchain presentation
```

The compute launcher is acceptable only because actual scene traversal remains Vulkan hardware ray tracing through `VK_KHR_ray_query` and BLAS/TLAS. Do **not** replace this with software BVH traversal, generic compute-only path tracing, rasterisation, SSR, baked lighting or other fake RT substitutes.

## Feature-parity requirement

The RayQuery backend should target **feature parity**, not a deliberately cut-down visual mode.

Preserve, where supported by the shared existing renderer:

- the authored gothic route and all existing world geometry;
- BLAS/TLAS scene representation;
- player, held sword, torch and reward lantern RT visibility;
- moving fire light and emissive fire presentation;
- dynamic direct-light visibility/shadows;
- wet stone and puddle response;
- reflections and bounded secondary rays;
- water reflection/transmission/refraction and its physical roof/occlusion rules;
- dielectric glass/reward-lantern glass behaviour;
- lich mist and staff electricity;
- material/PBR shading;
- player-body and enemy visibility/reflection/shadow behaviour;
- RT Lab behaviour where the underlying controls are backend-independent;
- tone mapping, exposure and final presentation;
- shared gameplay, simulation, combat, audio and haptic semantics.

Do not call the RayQuery backend a low-quality or fake fallback. It is an **alternate hardware RT execution backend**. Existing quality/workload controls can still vary cost independently of backend choice.

## Performance rule

Feature parity does **not** mean identical performance.

The S25 Ultra may need a different internal render scale or workload setting than the S26 Ultra. That is acceptable provided:

- the engine does not silently remove RT features to manufacture performance;
- any automatic/default quality selection is explicit and evidence-driven;
- diagnostics report backend, internal render resolution, workload/quality choice and measured timing honestly;
- comparisons are made against the existing sustained-performance methodology in `AGENTS.md` rather than a one-off fresh-process number.

Do not lower quality or resolution merely to make an acceptance gate pass. Measure first, investigate regressions, then choose a sensible device default if needed.

## Required engine work

Codex should inspect the current code before deciding exact implementation, but likely work includes:

1. **Backend capability model**
   - Extend the current `RtMode`/capability reporting so a presentable RayQuery backend is a real runnable mode rather than a diagnostic dead end.
   - Preserve honest reporting of which Vulkan extensions/features are present.
   - Never report `rtScene.presented = true` until a hardware-RT-produced frame actually reaches successful swapchain presentation.

2. **Shared shader refactor**
   - Move per-pixel Horde RT frame logic into shared shader code that can be called from both the existing raygen entry point and a compute entry point.
   - Avoid maintaining two diverging copies of the lighting/material/water/fire/glass implementation.
   - Preserve current ABI/layout contracts where practical.

3. **RayQuery compute entry point**
   - Add a compute shader that launches one invocation per output pixel (or another justified mapping) and performs the same shared `rayQueryEXT` scene traversal/shading.
   - It must not require `GL_EXT_ray_tracing` or ray-pipeline shader stages on RayQuery-only hardware.
   - It may require `GL_EXT_ray_query` and the appropriate acceleration-structure descriptors.

4. **Vulkan backend dispatch**
   - Keep the existing `vkCmdTraceRaysKHR` path intact for pipeline-capable devices.
   - Add a compute pipeline/dispatch route for RayQuery-only devices.
   - Reuse the same scene acceleration structures, descriptor-backed resources, storage output and swapchain-copy/presentation architecture where sound.
   - Do not create a duplicate game renderer with separate materials/assets/gameplay state.

5. **Resource and synchronisation audit**
   - Confirm descriptor layouts, acceleration-structure access, storage image barriers, dynamic BLAS/TLAS refits, held-item updates and one-frame-in-flight assumptions are valid under both launch routes.
   - Preserve correct Android lifecycle reconstruction and Home/resume behaviour.

6. **Diagnostics**
   - Report the exact selected backend, for example `RayTracingPipeline` or `RayQueryCompute`.
   - Report Vulkan extension/feature state, output/internal resolution, CPU frame timing and valid GPU timing where available.
   - Do not present diagnostic-screen FPS as scene performance when no RT scene has run.

7. **Documentation and compatibility record**
   - Update `AGENTS.md`, `PROJECT_DECISIONS.md`, `PROJECT_MEMORY.md`, `docs/PHASE_PLAN.md`, shader documentation and `docs/ANDROID_RT_DEVICE_COMPATIBILITY_RECORD.md` as the architecture becomes real.
   - Preserve historical evidence rather than rewriting old results to imply support existed earlier.

## Primary acceptance device

The **Samsung Galaxy S25 Ultra / Adreno 830 / driver 512.800.64** is the first acceptance device for the new backend because it gives a clean capability boundary:

- hardware acceleration structures: yes;
- ray query: yes;
- full ray-tracing pipeline: no.

Minimum acceptance criteria on that exact device:

1. App selects `RayQueryCompute` (or the final equivalent name) without pretending that `VK_KHR_ray_tracing_pipeline` exists.
2. BLAS/TLAS-backed Vulkan ray-query traversal is proven active.
3. The real Horde scene is dispatched and a hardware-RT-produced frame reaches the Android swapchain.
4. `rtScene.presented` becomes true only after that successful presentation.
5. The normal playable route launches rather than the diagnostics-only dead end.
6. Existing major visual features remain present; any unavoidable backend-specific difference must be documented and justified rather than silently removed.
7. Touch controls and shared simulation/gameplay remain intact.
8. Pause/Home/resume recreates and presents correctly.
9. Sustained performance is measured with internal resolution, quality/workload state, temperature, Android thermal status and Samsung GPU power level where available.
10. The compatibility record is updated with exact-device evidence.

## Cross-check device

The Galaxy S26 Ultra / Adreno 840 remains the primary development reference.

After the RayQuery backend works on the S25 Ultra:

- confirm the existing `RayTracingPipeline` path still works unchanged on the S26 Ultra;
- where practical, run both backends on a device that exposes both and compare output/performance to detect shader divergence;
- do not switch the S26 default away from the current full pipeline unless evidence gives a compelling reason.

Windows RTX must also retain its existing pipeline path and validation behaviour.

## Non-goals

This programme does **not** authorise:

- raster-only rendering;
- baked lighting as a substitute for live RT;
- SSR/screen-space fake reflections;
- software BVH traversal dressed up as RT;
- browser/WebGPU substitution;
- generic compute path tracing that bypasses Vulkan hardware ray-query traversal;
- removal of existing visual effects merely to widen the supported-device list;
- unrelated enemy-count/gameplay expansion;
- replacing the clean engine with a large external engine/sample dump.

## Recommended development shape

Treat this as a measured renderer milestone, not one giant unverified rewrite.

Suggested slices:

1. **Capability + empty compute-dispatch proof** - select the S25 RayQuery path and write/present a simple output safely.
2. **Primary-ray parity** - compute entry point traces the existing TLAS through `rayQueryEXT` and renders the scene with shared material decode.
3. **Lighting/shadow parity** - reuse existing direct visibility/fire/torch behaviour.
4. **Secondary transport parity** - reflections, wet materials, water and glass.
5. **Dynamic scene parity** - player body, held props, animated enemies, dynamic BLAS/TLAS updates.
6. **Full route/lifecycle pass** - gameplay, finale, RT Lab, Home/resume.
7. **Performance/default selection** - sustained S25 measurement and honest device default.
8. **Regression closeout** - S26 pipeline + Windows RTX remain clean.

Each slice should leave a runnable build and should end with files changed, build/run instructions, evidence gathered, limitations and the exact next task.

## Future Codex handoff

When 1.6.1 and the music milestone are complete, the owner can start this programme with a prompt as small as:

> Read `AGENTS.md`, `PROJECT_DECISIONS.md`, `PROJECT_MEMORY.md`, `docs/PHASE_PLAN.md`, `docs/ANDROID_RT_DEVICE_COMPATIBILITY_RECORD.md`, and `FUTURE_WORK.md`. The 1.6.1 and music milestones are complete. Begin the planned **RayQuery hardware-RT compatibility programme** in `FUTURE_WORK.md`. First audit current main and produce an implementation/validation plan grounded in the actual architecture; then execute the smallest safe vertical slice. Preserve the existing RayTracingPipeline backend and all RT-or-nothing rules. The Galaxy S25 Ultra / Adreno 830 / driver 512.800.64 is the first acceptance device. Do not fake RT or remove visual features to gain compatibility.

For a larger autonomous Codex run, add:

> You may continue through subsequent slices when the previous slice is verified, but stop on any architectural contradiction, unproven hardware assumption, unexplained >15% matched performance regression, broken existing S26/Windows pipeline path, or evidence that feature parity would require weakening the project's RT rules. Keep documentation and compatibility evidence current as you work.

---

**Priority reminder:** finish **1.6.1 -> music -> RayQuery compatibility**, in that order, unless the project owner explicitly reprioritises.