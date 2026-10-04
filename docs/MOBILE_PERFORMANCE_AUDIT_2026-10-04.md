# Mobile performance audit and optimisation experiment bank — 4 October 2026

**Status:** Preserved audit and experiment bank. The three bounded pre-release experiments below are owner-requested; the remaining proposals are uncommitted future work. No performance saving or production admission is claimed.

**Reviewed source:** [PR18 checkpoint b2dc216a500e690b44ebac20ea05f7bb318349a4](https://github.com/Samfa12-tech/The-Horde-RT-demo/tree/b2dc216a500e690b44ebac20ea05f7bb318349a4)  
**Measured runtime source:** [95eb08070354d4b3b775e6d2368cca8688f9cfe6](https://github.com/Samfa12-tech/The-Horde-RT-demo/commit/95eb08070354d4b3b775e6d2368cca8688f9cfe6)  
**Planning context:** [Roadmap](ROADMAP.md) and [1.6.2 implementation goal](IMPLEMENTATION_GOAL_1_6_2.md)

## Current disposition and scope

The owner requested that this audit be kept for future optimisation experiments, then explicitly requested testing the biggest-ticket items before shipping 1.6.2. Finish the current fixes first, then run these three experiments sequentially, with an individual keep/reject decision:

1. **Selective primary opacity:** allow ordinary opaque geometry to bypass shader candidate confirmation while preserving player near-face/body-remainder rejection, complete secondary visibility and Water Off behaviour.
2. **Eligible opaque-bounce skip:** actually avoid the secondary trace on eligible non-reflective hits while preserving important RT reflections, including mirrors and puddles. Label any loss of indirect contribution as a visual trade-off.
3. **Bounded texture-footprint/LOD experiment:** verify the selected Shipping SPIR-V sampling operations first, then test a limited explicit footprint-aware sampling change. This does not authorise a full ray-cone propagation or renderer rewrite.

These are authorised experiments before the release decision, not guaranteed shipping features or promised speedups. Preserve the accepted baseline and defaults, isolate each change, use comparable Shipping measurements and matched visual checks, retain negative results, and reject changes without a repeatable net benefit. Final independent review and explicit owner release approval remain required.

The separately queued second pass covers independent shadow controls including higher quality, a lower fire tier, upscaling, optional benchmark Send statistics through the existing private service, and mist density/lighting investigation. Its own bounded implementation/measurement plan remains separate. No endpoint, credentials, private evidence or deployment is approved by this document.

The remaining audit proposals stay in the future experiment bank until selected. Do not automatically promote its full priority list, gameplay pacing, DRS, temporal reconstruction, texture reductions, backend changes or frame-overlap work into committed 1.6.2 scope. The newer authorisation for the three experiments above supersedes only their proposed-handoff status in the preserved snapshot below.

## Independent source review — 4 October 2026

This review checked the four principal source claims against the frozen PR18 head and its measured runtime ancestry. It did not build the game or run a new device benchmark.

- **Primary opacity premise verified.** [The primary call](https://github.com/Samfa12-tech/The-Horde-RT-demo/blob/b2dc216a500e690b44ebac20ea05f7bb318349a4/shaders/raytracing/include/rt_frame.glsl#L42-L43) enables near-face filtering; [traceScene](https://github.com/Samfa12-tech/The-Horde-RT-demo/blob/b2dc216a500e690b44ebac20ea05f7bb318349a4/shaders/raytracing/include/rt_hit_decode.glsl#L434-L475) therefore forces NoOpaque. The same mechanism is present in the reviewed main baseline, so this is not a proven PR18 regression. The [rejected metadata-lookup guard](https://github.com/Samfa12-tech/The-Horde-RT-demo/blob/b2dc216a500e690b44ebac20ea05f7bb318349a4/docs/ENGINEERING_1_6_2_PLAYER_CANDIDATE_LOOKUP.md) retained candidate processing and does not settle the selective-opacity proposal. Inspect every caller and copied instance flag; preserve nearest-hit semantics.
- **Bounce premise verified, with a shared-sample caveat.** [shadeOpaquePrimary](https://github.com/Samfa12-tech/The-Horde-RT-demo/blob/b2dc216a500e690b44ebac20ea05f7bb318349a4/shaders/raytracing/include/rt_dielectric_common.glsl#L297-L339) issues one deterministic secondary sample outside Lean, then uses it in both diffuse-like and reflection terms. Removing only a diffuse multiplier while retaining the trace will not produce the proposed traversal saving. A selective ray skip changes visual quality; reflection classification must account for procedural and imported-material semantics rather than metallic alone.
- **Texture-footprint concern verified in source.** [Material/lich samples](https://github.com/Samfa12-tech/The-Horde-RT-demo/blob/b2dc216a500e690b44ebac20ea05f7bb318349a4/shaders/raytracing/include/rt_hit_decode.glsl) use texture() without a supplied footprint, and [the compute entry](https://github.com/Samfa12-tech/The-Horde-RT-demo/blob/b2dc216a500e690b44ebac20ea05f7bb318349a4/shaders/raytracing/minimal.comp) does not enable compute derivatives. Mip-chain availability does not establish suitable mip selection. Verify compiled operations before changes; textureLod with a constant zero would only make existing base-level behaviour explicit. Neither texture bandwidth dominance nor a net benefit from a full ray-cone system has been measured.
- **Timing qualifications verified.** [RenderFrame](https://github.com/Samfa12-tech/The-Horde-RT-demo/blob/b2dc216a500e690b44ebac20ea05f7bb318349a4/android/app/src/main/cpp/android_probe_bridge.cpp#L2765-L2795) starts its CPU cycle before the prior in-flight fence wait. [The GPU bracket](https://github.com/Samfa12-tech/The-Horde-RT-demo/blob/b2dc216a500e690b44ebac20ea05f7bb318349a4/android/app/src/main/cpp/android_probe_bridge.cpp#L3285-L3315) surrounds RecordTraceAndCopy, which includes dynamic AS commands, tracing/shading and output transfer. These are not CPU busy time and pure ray-traversal time; they must not be added together. The three commits after runtime 95eb080 change documentation, collectors and tests, not the renderer runtime.
- **Priority ranking remains unmeasured.** The source mechanisms justify bounded tests, not a claim that these are the largest measured costs. The [recorded scaling courses](https://github.com/Samfa12-tech/The-Horde-RT-demo/blob/b2dc216a500e690b44ebac20ea05f7bb318349a4/docs/ENGINEERING_1_6_2_MOBILE_QUALITY_MEASUREMENTS.md) are single ordered warm/hot runs, not thermally matched causal comparisons or sustained free-play acceptance. No millisecond or percentage saving is established for the proposed changes.

Useful primary references: [Khronos Vulkan ray traversal](https://docs.vulkan.org/spec/latest/chapters/raytraversal.html) and [GLSL 4.60, section 8.9](https://registry.khronos.org/OpenGL/specs/gl/GLSLangSpec.4.60.pdf).

## Preserved source-and-evidence audit

The substantive audit below is preserved as supplied, with heading levels adjusted for this archive. Its imperative wording describes the proposed experiment/handoff plan at that snapshot, not automatic permission or current release scope. The current disposition above records the later owner selection. All repository paths in the preserved audit refer to the [frozen reviewed source tree](https://github.com/Samfa12-tech/The-Horde-RT-demo/tree/b2dc216a500e690b44ebac20ea05f7bb318349a4), not the older runtime tree on this planning branch.

## The Horde — PR18 mobile performance audit and proposed Codex handoff

Audit date: 4 October 2026
Repository: Samfa12-tech/The-Horde-RT-demo
PR: 18 — codex/horde-1.6.2-engine-readiness
Reviewed head: b2dc216a500e690b44ebac20ea05f7bb318349a4
Reviewed main/base: 1df058b77baacaab76dc112f578deb77ddb791e9
Measured runtime source: 95eb08070354d4b3b775e6d2368cca8688f9cfe6

### Scope and confidence

This is a source-and-evidence performance audit, not a fresh device benchmark, executed build, or final release approval. The review examined the PR file inventory, relevant baseline code, Android frame ownership and timing, primary/secondary shader paths, graphics settings, dynamic geometry preparation, texture upload/sampling, asset budgets, existing experiments, and recorded validation. No repository changes, merges, installations, or publication were performed.

The current PR head's changes after the measured runtime are collector/documentation work; do not relabel the measured APK as a newly built head artifact. Re-read the live branch before applying this handoff.

Follow the existing release hold: propose the bounded second-pass plan first. Implementation requires the owner's acceptance of that plan. Keep final independent review and explicit release approval.

### Established performance evidence

Source: docs/ENGINEERING_1_6_2_MOBILE_QUALITY_MEASUREMENTS.md

Exact current ShippingMobile benchmark, same APK, 1,838 valid owning samples per ordinary course, output 1440×2980:

| Scale | Internal extent | CPU cycle median / p95, ms | Recorded GPU interval median / p95, ms |
| --- | --- | --- | --- |
| 75% | 1080×2235 | 64.16 / 83.59 | 53.59 / 73.12 |
| 40% | 576×1192 | 28.63 / 37.90 | 18.49 / 26.65 |
| 33% | 475×983 | 24.07 / 30.60 | 13.73 / 19.45 |

These are single ordered warm/hot USB-connected runs, not thermally matched causal A/B tests, scanout FPS, or sustained free-play results. Native thermal status was not collected. Battery temperature alone does not establish matched GPU conditions. The older 50% measurement used a different APK and must not be interpolated into this cohort.

At 75%, 2,413,800 primary pixels are shaded. At 40%, the count is 686,592, or 28.44% of the 75% workload; at 33%, 466,925, or 19.34%. These are pixel-count ratios, not guaranteed speedups.

Historical July records support that much faster versions existed, but use older builds and window-averaged wall-clock timing. Do not claim a precise regression multiplier by dividing them into today's GPU medians.

#### Timing interpretation — resolve before optimisation attribution

Sources:
- android/app/src/main/cpp/android_probe_bridge.cpp: RenderFrame, SwapchainRenderLoop
- src/vulkan/GpuFrameTimer.cpp: RecordBegin / RecordEnd
- src/vulkan/raytracing/PresentableTinyRtScene.cpp: RecordTraceAndCopy / UpdateDynamicInstances

The reported CPU cycle includes waiting on the previous in-flight fence, acquisition, recording and presentation. It is not CPU busy time. Do not add CPU64.16 and GPU53.59; do not claim CPU is the main bottleneck from the larger number.

The ordinary GPU timer uses TOP_OF_PIPE / BOTTOM_OF_PIPE around RecordTraceAndCopy. This covers more than ray traversal: the recording includes dynamic AS operations, tracing and output transfer, and the interval can include synchronization stalls. Preserve owning submission/slot identity.

Use the existing CPU stage instrumentation. Add a bounded diagnostic GPU timing breakdown for dynamic BLAS, TLAS, trace/shading, and output/upscaling/transfer, without creating waits in the steady-state measurement path. Measure instrumentation overhead; performance decisions use Shipping variants without GPU diagnostic atomics.

### Priority 1 — stop forcing shader candidate handling on ordinary opaque geometry

Sources:
- shaders/raytracing/include/rt_frame.glsl: primary traceScene call
- shaders/raytracing/include/rt_hit_decode.glsl: traceScene
- src/vulkan/raytracing/PresentableTinyRtScene.cpp: dynamic instance construction
- docs/ENGINEERING_1_6_2_PLAYER_CANDIDATE_LOOKUP.md

The primary call passes ignorePlayerNearFace=true. traceScene consequently selects gl_RayFlagsNoOpaqueEXT, so ordinary opaque triangles encountered by the primary ray also require candidate processing. This is inherited from main, not a newly proven PR18 regression.

Prototype selective opacity handling: ordinary opaque geometry should auto-commit in hardware; only geometry requiring primary visibility rejection should be non-opaque for that path. One candidate architecture is instance/geometry-level non-opacity for the relevant world-body geometry with neutral primary ray flags, retaining the current conservative path when Water Off requires rejection inside the mixed procedural-world geometry.

This is NOT the rejected metadata-lookup guard. That experiment retained the candidate-processing architecture and showed no net measured win. Preserve its negative result.

Correctness requirements:
- Keep head/near-face rejection, the explicit visible body remainder, and dedicated viewmodel ownership.
- Keep full world-body shadows, reflections and transmission visibility.
- Inspect every shadow/secondary caller's opacity flags and candidate handling.
- Do not accidentally propagate the world-body non-opacity flag through copied viewmodel/procedural instance definitions.
- Water Off must still reject both the waterfall and water inside the mixed world geometry.
- Preserve nearest-hit semantics; primary rays cannot terminate at an arbitrary first candidate.
- Test both native RT backends, ordinary and inspection routes, mirrors, glass/water, extreme look directions and moving hands.
- Use diagnostic candidate counts only to explain results; acceptance requires Shipping timings and images.

Khronos reference: Vulkan Specification, Ray Traversal, sections Ray Opacity Culling / Determination and Ray Query. Ray flags override instance/geometry opacity; opaque triangles do not return triangle candidates to shader code.

### Priority 2 — make the ordinary diffuse bounce independently optional

Source: shaders/raytracing/include/rt_dielectric_common.glsl: shadeOpaquePrimary

The Authored path issues a secondary trace for every non-emissive opaque hit, even when the surface is not meaningfully reflective. The existing Lean path disables the bounce broadly, including desired reflections.

Add an independent indirect-lighting policy. A proposed low tier keeps true RT primary visibility, finite direct-light shadows, and selected specular/puddle/mirror reflections, while omitting the diffuse secondary bounce. Do not replace it with baked light or screen-space reflections, and do not hide the loss of indirect contribution by increasing ambient lighting.

Separate decisions for diffuse indirect, important specular reflection, and reflected volumetric fire. Mirror/puddle classification and low-metallic dielectric reflectivity must remain explicit rather than assuming metallic alone determines importance.

Removing this bounce can eliminate one secondary trace plus hit decoding for eligible opaque pixels. Its millisecond saving is UNKNOWN until measured; do not publish an invented percentage. Keep authored/full quality available.

### Priority 3 — correct texture footprint / mip selection before destroying assets

Sources:
- shaders/raytracing/include/rt_hit_decode.glsl: material and lich texture calls
- src/vulkan/raytracing/PresentableTinyRtScene.cpp: CreateTexture, CreateMaterialTextures, CreateLichTextures
- shaders/raytracing/minimal.comp and include/rt_frame.glsl

Most material/lich samples use texture(), without a ray footprint or explicit LOD. These non-fragment shader stages do not provide ordinary fragment-derived implicit mip selection. The material sampler starts at LOD zero; uploading a mip chain alone does not select appropriate distant mips. The environment panorama is different: it already uses explicit textureLod.

Inspect the generated SPIR-V sampling operations, then implement a shared explicit footprint-aware material sampling path using ray cones or analytical UV footprints derived from actual projection, hit distance, UV density, instance transform and surface orientation. Propagate an appropriate footprint to secondary rays; account for roughness where justified.

Test minification, oblique surfaces, moving normal maps, distant lich emissive detail and reflections. Compare nearest-mip/trilinear choices and cost; do not blindly raise anisotropy or apply an arbitrary global blur.

Android ASTC already exists for the environment/material paths and strict lich textures. The lich textures are 2048². Consider 1024² mobile lich textures as a separate quality/memory experiment, not as a substitute for correct sampling.

GLSL reference: The OpenGL Shading Language 4.60, section 8.9, Texture Functions — implicit LOD is fragment-stage behavior; non-fragment sampling uses a zero base LOD unless supplied appropriately.

### Priority 4 — bounded resolution and spatial upscaling

Sources:
- src/graphics/GraphicsSettings.h
- src/graphics/BoundedDynamicResolution.h
- docs/ENGINEERING_1_6_2_TEMPORAL_DECISIONS.md
- android/app/src/main/cpp/android_probe_bridge.cpp: graphics Apply handling
- docs/ENGINEERING_1_6_2_WINDOWS_RESIZE_VALIDATION.md

Ordinary scale is 50–100%; 33/40% are specially admitted benchmark-only values. Do not silently lower existing preferences.

Both launchers already have output-only resize machinery; Android calls ResizeOutputAfterDeviceIdle for scale changes. Reuse it. Do not rebuild the full scene for every resolution step. Current resizing still waits for idle and allocates output resources, so measure transition hitches before connecting frequent DRS updates.

The existing DRS controller is a dormant CPU foundation, not runtime DRS. Connect only after valid frame ownership, resize outcomes, target budget, lifecycle state and timing granularity are handled. Retain hysteresis/dwell and explicit effective settings; capped frame-loop durations must not be mistaken for GPU workload.

Evaluate one spatial upscaler against the existing filtered output at matched input/output dimensions. FSR1 is a documented Vulkan-capable spatial option; inspect its anti-aliased-input expectation, colour space and filter order. Its published modes extend down to 50% linear input scale; 40/33% are more aggressive than those standard modes and require their own quality assessment. Do not promise that 33% looks native.

Keep native-resolution UI. Report the upscaler's GPU cost separately and total end-to-end benefit. Consider an absolute internal-pixel budget as well as percentage so display-resolution changes do not silently multiply workload.

Temporal reconstruction is a separate later stage. The current temporal code is CPU ownership/geometry foundation only: no integrated GPU motion/depth/HDR/history producer or reconstruction pass. First-hit motion cannot automatically describe reflection, glass, fire or mist. The documented attachment proposal alone is 32 bytes/pixel, about 77.2 MB at the present 75% extent before additional resources. Do not market it as an existing feature or a free win.

### Priority 5 — useful mobile graphics controls

Proposed controls, not already implemented unless explicitly identified:

| Control | Proposed mobile policy | Important constraint |
| --- | --- | --- |
| Gameplay frame target | 30 FPS default candidate; optional 60 | Existing previewFrameCap is preview-only; pacing does not make an over-budget renderer faster. |
| Resolution | Fixed or bounded adaptive; validate 50/40 first, 33 emergency/experimental | Do not change existing default until owner approves evidence and image quality. |
| Indirect lighting | Low: no diffuse secondary bounce; preserve important RT reflections | Separate from the current all-purpose Lean workload. |
| Reflections | Important-surface RT / full authored | No raster or SSR replacement; preserve mirror/puddle semantics. |
| Fire | New Low shared-emitter core; Mobile; High | Current Mobile is already four volume steps, High ten. A real lower tier must eliminate volume work. |
| Mist | Off / low sample count / authored | Density reduction alone is not a sample-count reduction. |
| RT shadows | One / two / four genuine visibility samples where supported | Authored already uses one per relevant light evaluation; a one-sample selector alone cannot accelerate that baseline. |
| Water | Existing Off / Mobile / High | Measure water-heavy scenes; preserve physical and visibility contracts. |
| Glass | Existing On / Off with accurate optical-profile description | Mobile compiled geometry already omits full reward-lantern panes. |
| Texture detail | Existing compressed textures plus measured lower texture/LOD tier | Fix mip selection first; retain readable hands/held items. |
| Preview | Pause or low update rate when idle; independent budget | Materials preview is not a gameplay FPS estimate. |

ReducedEffectsGraphicsSettings(Android) currently resolves to the same values as BaselineGraphicsSettings(Android), and preset matching checks baseline first. Make any exposed Performance/Reduced preset genuinely different; test the effective tuple, not only its label.

Low fire must first unify the emitter-driven core between showcase, preview and lantern. The showcase has a physical eight-triangle emissive torch core, compact preview does not, and the lantern currently has authored material emission. Simply skipping the volume can remove the preview flame or leave an incorrectly glowing lantern. Preserve shared timing, extinction, sockets, real visibility and reflected/transmitted behavior.

Mist currently takes 2/6/8 samples according to Lean/Authored/Max and is early-bounded to its room volume. Separate density, sample count and lighting. Adding correct extra mist-light visibility may increase cost. Do not describe a lighting correction as a performance optimisation or diagnose Issue19 without inspecting its actual image.

### Priority 6 — CPU and smaller shader cleanup

Sources:
- PresentableTinyRtScene.cpp: UpdateDynamicInstances / recordPlayerBlas
- PlayerRenderSlot.cpp
- CharacterRenderSlot.cpp/.h
- RtGpuResources.cpp
- shaders/raytracing/include/rt_hit_decode.glsl / rt_lighting.glsl

Source-backed candidates:
1. Retain the world-player upload template as the viewmodel already does; update changed position/normal/tangent fields instead of copying the complete immutable source vertex array on every pose.
2. Cache immutable BLAS geometry/range descriptors instead of constructing three vectors per player/viewmodel refit.
3. Dirty-track material settings and active-transmission classification. Avoid full material-vector copies, scans and uploads when nothing material-related changed.
4. Keep world body and viewmodel on one authoritative solved pose. They already share the pose; do not claim that sharing requires a new IK system. Only share actual vertex work where a verified exact mapping exists.
5. Evaluate the built-in world-to-object matrix transpose instead of computing inverse(objectToWorld) per PBR hit. Validate transformed/mirrored/nonuniformly scaled normals.
6. In Max outside the finale, the two directional moon targets are identical. Reuse the same visibility result there, retaining distinct finale targets. This does not accelerate ordinary one-sample Authored shadows.

Character poses already have a 30 Hz refresh policy and can share identical skeleton pose buckets. Dynamic BLAS work already uses UPDATE, with refits requested only for changed pose buckets; it is not a full static rebuild every frame. Preserve these foundations.

Do not move skinning to compute merely because GPU skinning sounds modern: the GPU is already under pressure. First measure active CPU skinning/upload and GPU AS intervals; compare SIMD/task-based CPU preparation against compute including barriers and memory traffic.

### Frame overlap, backends, and sustainable pacing

Android intentionally has one frame in flight and waits before preparing the next scene. CPU-only snapshot/pose preparation may be overlapped with previous GPU execution using separate CPU storage. Any mutable GPU writes still require correct ownership.

Do not change kMaxFramesInFlight from one to two without a resource ownership plan covering dynamic vertices, metadata, AS objects/scratch, output, descriptors and history. Measure memory, latency and sustainable power, not only short throughput.

Run a same-artifact Shipping comparison of RayTracingPipeline and the genuine hardware-RayQueryCompute backend. Both use real RT; do not presume either stage arrangement is faster on Adreno without measurements.

Use Android Frame Pacing/Swappy or an equivalently correct integration for actual gameplay pacing. Incorporate Android thermal status/headroom where supported and tested. Neither a 30 FPS cap nor a larger CPU performance hint fixes an intrinsically over-budget GPU workload.

### Bounded execution plan and acceptance

Stage A: Seal the baseline, locate a genuinely fast historical artifact, retain released 1.6.1 and the current runtime. Record hashes, driver, backend, output and internal dimensions, shader/optical profile, route, power connection, thermal conditions and timing definitions. Add only missing stage attribution; do not restart a sprawling telemetry redesign.

Stage B: Run isolated prototypes for selective opacity, diffuse-bounce policy, texture LOD, then spatial scaling. Keep the same scene/camera/settings for exact optimisations; label quality-changing A/B tests explicitly. Preserve negative results and remove experiments without a repeatable net benefit.

Stage C: Add independent low fire/mist/shadow controls and real presets; integrate a gameplay target and bounded DRS only after scale transitions and ownership pass. Keep the old authored settings available.

Stage D: Validate the chosen combination in several sustained ordinary-play sessions, not only the scripted fixed-step course. Include start, both skeletons, water, mirror, lich/mist, reward lantern, death/retry, settings Apply/Revert and Home/resume. Include camera turns and disocclusions, not only stills. Do not use private evidence from a different artifact as acceptance.

Use repeated, comparable thermal/power runs with alternating run order. Report per-zone median/p95/p99, missed frame deadlines, active CPU stages, GPU stages, actual presentation evidence, scale history and thermal state. Paused menus, benchmark transitions and idle waiting must be labelled rather than mixed into performance distributions.

Suggested target to validate: sustained 30 FPS for the chosen mobile tier, with headroom rather than merely a median below 33.33 ms. A 60 FPS mode is a separate 16.67 ms end-to-end target, not established by a GPU median below 16.67 ms alone.

Final delivery: immutable candidate; concise diff; before/after table; images and motion; exact tests and failures; costs of each quality trade-off; outstanding device coverage; independent review; explicit owner release approval. Do not merge, sign, tag, publish or deploy a reporting service under this audit.

### Audit reference index

Repository references above are relative to the frozen reviewed head unless explicitly described as baseline/historical. Additional ledgers:
- docs/ENGINEERING_1_6_2_SHADER_BUDGET_REVIEW.md
- docs/ENGINEERING_1_6_2_LOW_FIRE_SHADOW_INVESTIGATION.md
- docs/ENGINEERING_1_6_2_PLAYER_CANDIDATE_LOOKUP.md
- docs/ENGINEERING_1_6_2_TEMPORAL_DECISIONS.md
- assets/models/world/runtime/collapsed-entry/asset.manifest.json
- docs/SHOWCASE_ALPHA_RELEASE_NOTES_2026-07-17.md

Official external references checked during the audit:
- Khronos Vulkan Specification: Ray Traversal.
- Khronos GLSL 4.60 specification: section 8.9 Texture Functions.
- AMD GPUOpen: FidelityFX Super Resolution 1.
- Android Developers: Frame Pacing library.
- Android Developers: Android Dynamic Performance Framework.
