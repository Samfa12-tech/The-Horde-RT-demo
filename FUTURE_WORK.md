# Horde Lantern RT - Current and Future Work

Owner update: 2026-09-13. The [1.6.1 engineering programme](docs/ENGINEERING_1_6_1_PLAN.md) is the current execution authority. It supersedes the 2026-09-11 post-1.6.1 ordering while retaining the detailed compatibility design below. No release/publication is authorised.

## Ordered sequence

1. Reconcile lean branch guidance and current plans/evidence.
2. Resolve S24/S25 hardware-RT compatibility through the reusable alternate backend below.
3. Finish the original 1.6.1 audit, adaptive What the Dark Keeps A-H score and Briarhold-derived player reporting. Music has separate persisted 0-100% volume on Android and Windows, shared simulation/event-driven transitions, and the supplied composition/handoff as authority.
4. Run targeted checks during implementation. Only after the full feature set is complete, run the comprehensive Windows/Android/device/release candidate matrix and fix regressions.
5. Request the owner's release decision; no automatic publishing. Later campaign milestones remain outside this engineering pass.

## S24/S25 compatibility programme - required within 1.6.1

The immediate motivation is now confirmed across two Qualcomm flagship generations: real Samsung Galaxy S25 Ultra and Galaxy S24 Ultra tests performed on 2026-09-11 both reached Horde's Vulkan diagnostics and exposed acceleration structures plus `VK_KHR_ray_query`, but neither tested driver exposed `VK_KHR_ray_tracing_pipeline`. The current renderer therefore selected `RayQuery` capability mode but did not attempt or present the Horde RT scene.

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

Tested S24 Ultra diagnostic result:

- GPU: `Adreno (TM) 750`
- Vendor ID: `20803`
- Device ID: `1124406273`
- Driver: `512.762.41` (packed `2150604841`)
- Vulkan API: `1.3.128`
- `VK_KHR_acceleration_structure`: yes
- `VK_KHR_ray_tracing_pipeline`: **no**
- `VK_KHR_ray_query`: yes
- `VK_KHR_buffer_device_address`: yes
- `VK_KHR_deferred_host_operations`: yes
- Selected capability mode: `RayQuery`
- RT scene: not attempted / not presented

Both tested-device results are now recorded in `docs/ANDROID_RT_DEVICE_COMPATIBILITY_RECORD.md`. The S24 screenshot-derived evidence also has a dedicated note at `docs/validation/horde-galaxy-s24-ultra-rayquery-2026-09-11.md`.

The two-device result is important because this is no longer merely an S25/Adreno 830 edge case. It suggests the planned RayQuery backend could materially widen support across at least Snapdragon 8 Gen 3 / Adreno 750 and Snapdragon 8 Elite / Adreno 830 class devices whose shipped drivers expose hardware ray query but not the full Vulkan ray-tracing pipeline.

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

The S25 Ultra and S24 Ultra may need different internal render scales or workload settings than the S26 Ultra. That is acceptable provided:

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

## Acceptance devices

### Primary acceptance: Galaxy S25 Ultra / Adreno 830 / driver 512.800.64

This remains the first acceptance device because it provides the cleanest initial capability boundary:

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

### Secondary acceptance: Galaxy S24 Ultra / Adreno 750 / driver 512.762.41

After the S25 path is operational, repeat the same functional and evidence gates on the tested S24 Ultra. This second device is important because it proves the alternate backend is not narrowly special-cased for Adreno 830 and tests an older Snapdragon 8 Gen 3 / Adreno 750 hardware/driver generation with the same `RayQuery`-without-`RayTracingPipeline` pattern.

Do not infer performance from the S24 diagnostic screen's `241.35 fps / 4.14 ms`; the actual RT scene was not dispatched. Measure the real RayQueryCompute scene after implementation.

## Cross-check device

The Galaxy S26 Ultra / Adreno 840 remains the primary development reference.

After the RayQuery backend works on the S25 Ultra and S24 Ultra:

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
7. **Performance/default selection** - sustained S25 measurement, then S24 measurement, and honest per-device defaults if evidence supports them.
8. **Regression closeout** - S26 pipeline + Windows RTX remain clean.

Each slice should leave a runnable build and should end with files changed, build/run instructions, evidence gathered, limitations and the exact next task.

## Execution and acceptance boundary

This programme is now authorised inside 1.6.1 and takes priority over the remaining engineering features. Continue safe implementation through targeted tests; reserve the large cross-device matrix for the complete candidate. Missing physical devices remain explicit validation gaps, not reason to abandon independent local work or claim support. Follow the current [engineering programme](docs/ENGINEERING_1_6_1_PLAN.md) for all remaining audit, music, reporting and final release requirements.
