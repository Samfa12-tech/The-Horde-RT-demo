# Horde 1.6.2 — executable goal and acceptance plan

**Owner-requested implementation brief: 3 October 2026 (AEST).** Run the implementation with **gpt-6.1-sol, high**. This document consolidates the accepted update bank, post-release source audit and three owner screenshot reports. It does not claim implementation, authorise paid generation, or authorise a release.

## Outcome and baseline

Make the released dungeon a more reliable, controllable and visually coherent demonstration of the engine before 1.7. Deliver the bounded required slices below, resolve each optional investigation with an evidence-based decision, then hand back a reviewed candidate and exact remaining gates. Do not start the 1.7 campaign, dialogue/voice production, forest, new bosses or another renderer.

1.6.1 is published. [Release](https://github.com/Samfa12-tech/The-Horde-RT-demo/releases/tag/v1.6.1) targets frozen package source `a397757249871b6b64fe5b77fc14f24e8cfcbb2b`; [PR15](https://github.com/Samfa12-tech/The-Horde-RT-demo/pull/15) is merged. The [release validation](SHOWCASE_ALPHA_1_6_1_RELEASE_VALIDATION_2026-10-03.md) and current [handoff](ENGINEERING_1_6_1_HANDOFF.md) own artifact and closeout details. The owner has confirmed the goal complete. Start implementation from current released main, preserving subsequent changes, then bring in only the intended planning content from PR16. PR16 contains an older source baseline: do not replace main's renderer or AGENTS with that older tree.

Read current AGENTS.md, relevant AGENT_ENGINE_CONTRACTS/AGENT_VALIDATION sections, this brief, ROADMAP and the specific linked subplans. Read PROJECT_DECISIONS and PROJECT_MEMORY selectively when relevant. Inspect local .agents/skills and available skills before asset work. Preserve unrelated local work; use a dedicated 1.6.2 implementation branch/worktree and draft PR.

The post-release audit was **source/specification and recorded-evidence review, with no new device tests**. Its findings below are actionable starting evidence, not runtime reproduction certificates. Revalidate affected code against the chosen implementation base before patching; retain exact source/artifact identities.

### Preserve accepted baseline and honest limitations

- Preserve Pipeline and genuine BLAS/TLAS-backed RayQueryCompute, shared shading, immutable simulation snapshots and successful RT-produced presentation. No software/raster/screen-space replacement for the world, fake glass or baked scene illumination.
- Mobile's omitted lantern panes are an explicit accepted quality tradeoff; High retains physical glass. Unresolved High contact/near-edge/parity defects remain recorded in FUTURE_WORK, not silently declared fixed. No lowered correctness tolerance or unbounded glass redesign.
- Sustained 30 FPS at fixed 75% on S26 was not achieved. Preserve this separate target/comparison, record measured results honestly, and keep lower-scale/DRS/upscaled/cooled results distinct.
- S24 is working in development but final-release coverage was deferred; S25 remains unverified. No inferred certification from another model.
- Preserve accepted player/world-body ownership, adaptive music and independent volumes. Melody 3 A/D drone removal already shipped; do not repeat that work or change the accepted instrumentation.
- Retain the owner-tracked Hotstrike rights limitation. Owner acceptance of release risk is not a new licence grant. New/reused assets require verified rights and recorded provenance.

## Required sequence

Work in small runnable slices. Independent bounded art/UI work may proceed once baseline and interfaces are fixed; it must not race shared files or consume the phone/GPU without coordination.

### Phase 0 — baseline and durable execution record

Record current main and planned branch, released hashes, current settings/control map, asset inventory and validation commands. Reuse the finite 1.6.1 evidence instead of replaying its whole programme. Capture relevant Windows baseline screens and scene views; queue phone captures only after allocation.

Create or update one compact 1.6.2 status/handoff containing:
- each slice ID, owner, state (not started / implementing / blocked / verified / deferred), next concrete action and dependencies;
- exact changed commit, commands/results, artifact hashes and evidence links;
- decision ledger with hypothesis, bounded test, outcome, rejected alternatives and conditions that would justify reopening;
- device allocation, outstanding owner visual/audio approvals and explicit ship/defer/reject decisions.

Read that record first after context compaction. Never restart completed benchmarks, regenerate accepted assets or repeat rejected optimisations merely because context was compacted. Re-run only when a relevant change invalidates evidence; record why. A blocked phone gate does not stop independent safe host work.

### Phase 1 — audit corrections before new graphics work

| ID | Finding and source evidence | Required outcome and validation |
|---|---|---|
| A1 | High-confidence presentation-semaphore lifetime defect in source: a graphics fence alone does not prove the presentation queue consumed its binary wait semaphore. Windows [allocation](https://github.com/Samfa12-tech/The-Horde-RT-demo/blob/a397757249871b6b64fe5b77fc14f24e8cfcbb2b/src/platform/windows/DiagnosticWindow.cpp#L3627-L3640), [fence/acquire](https://github.com/Samfa12-tech/The-Horde-RT-demo/blob/a397757249871b6b64fe5b77fc14f24e8cfcbb2b/src/platform/windows/DiagnosticWindow.cpp#L4102-L4150), [submit/present](https://github.com/Samfa12-tech/The-Horde-RT-demo/blob/a397757249871b6b64fe5b77fc14f24e8cfcbb2b/src/platform/windows/DiagnosticWindow.cpp#L4355-L4424); Android [frame index](https://github.com/Samfa12-tech/The-Horde-RT-demo/blob/a397757249871b6b64fe5b77fc14f24e8cfcbb2b/android/app/src/main/cpp/android_probe_bridge.cpp#L178-L180), [fence/acquire](https://github.com/Samfa12-tech/The-Horde-RT-demo/blob/a397757249871b6b64fe5b77fc14f24e8cfcbb2b/android/app/src/main/cpp/android_probe_bridge.cpp#L2547-L2603), [submit/present](https://github.com/Samfa12-tech/The-Horde-RT-demo/blob/a397757249871b6b64fe5b77fc14f24e8cfcbb2b/android/app/src/main/cpp/android_probe_bridge.cpp#L3091-L3165). See [Khronos guidance](https://github.com/KhronosGroup/Vulkan-Guide/blob/main/chapters/swapchain_semaphore_reuse.adoc). No crash was reproduced by this audit. | Implement a spec-valid lifetime/reuse scheme on both platforms, including acquire failure, out-of-date/suboptimal, resize, recreation and shutdown. Verify sync validation and repeated live presentation/lifecycle when hardware is available. Do not mistake one frame in flight for proof of present completion. |
| A2 | Acceleration-structure scratch-address alignment portability risk, not observed device misalignment: [scratch construction](https://github.com/Samfa12-tech/The-Horde-RT-demo/blob/a397757249871b6b64fe5b77fc14f24e8cfcbb2b/src/vulkan/raytracing/PresentableTinyRtScene.cpp#L2982-L2996), [build](https://github.com/Samfa12-tech/The-Horde-RT-demo/blob/a397757249871b6b64fe5b77fc14f24e8cfcbb2b/src/vulkan/raytracing/PresentableTinyRtScene.cpp#L3605-L3612), [other build](https://github.com/Samfa12-tech/The-Horde-RT-demo/blob/a397757249871b6b64fe5b77fc14f24e8cfcbb2b/src/vulkan/raytracing/PresentableTinyRtScene.cpp#L3940-L3953), [raw device address](https://github.com/Samfa12-tech/The-Horde-RT-demo/blob/a397757249871b6b64fe5b77fc14f24e8cfcbb2b/src/vulkan/raytracing/RtGpuResources.cpp#L87-L124). | Explicitly satisfy queried minAccelerationStructureScratchOffsetAlignment for all relevant build/update paths, with allocation-range/overflow checks and tests. Verify [SBT alignment/stride/ranges](https://github.com/Samfa12-tech/The-Horde-RT-demo/blob/a397757249871b6b64fe5b77fc14f24e8cfcbb2b/src/vulkan/raytracing/PresentableTinyRtScene.cpp#L4482-L4530) too; SBT is an adjacent verification task, not a confirmed defect. |
| A3 | cgltf notice omission: [licence](https://github.com/Samfa12-tech/The-Horde-RT-demo/blob/a397757249871b6b64fe5b77fc14f24e8cfcbb2b/third_party/cgltf/LICENSE); package paths [Windows](https://github.com/Samfa12-tech/The-Horde-RT-demo/blob/a397757249871b6b64fe5b77fc14f24e8cfcbb2b/tools/package-alpha.ps1#L141-L144), [Android](https://github.com/Samfa12-tech/The-Horde-RT-demo/blob/a397757249871b6b64fe5b77fc14f24e8cfcbb2b/android/app/build.gradle#L335-L340), and credits [Windows](https://github.com/Samfa12-tech/The-Horde-RT-demo/blob/a397757249871b6b64fe5b77fc14f24e8cfcbb2b/src/platform/windows/DiagnosticWindow.cpp#L5890-L5906), [Android](https://github.com/Samfa12-tech/The-Horde-RT-demo/blob/a397757249871b6b64fe5b77fc14f24e8cfcbb2b/android/app/src/main/res/values/strings.xml#L111). | Include the required notice in both distributed package paths and appropriate notices/credits; add package checks. Do not mutate frozen 1.6.1 artifacts. |
| A4 | Windows render-scale change still takes the heavier rebuild route: [Windows](https://github.com/Samfa12-tech/The-Horde-RT-demo/blob/a397757249871b6b64fe5b77fc14f24e8cfcbb2b/src/platform/windows/DiagnosticWindow.cpp#L5314-L5344) versus [shared output resize transaction](https://github.com/Samfa12-tech/The-Horde-RT-demo/blob/a397757249871b6b64fe5b77fc14f24e8cfcbb2b/src/vulkan/raytracing/PresentableTinyRtScene.cpp#L1464-L1497). | Reuse compatible resolution-independent scene resources on Windows, preserving transactional failure/rollback, resource/descriptor lifetimes and correct output/history sizing. Measure resize stalls separately from steady-state FPS. |
| A5 | Report text filter rejects ordinary words containing “bearer”, such as “The torchbearer is silent”: [filter](https://github.com/Samfa12-tech/The-Horde-RT-demo/blob/a397757249871b6b64fe5b77fc14f24e8cfcbb2b/src/reporting/PlaytestReport.cpp#L120-L134), [application](https://github.com/Samfa12-tech/The-Horde-RT-demo/blob/a397757249871b6b64fe5b77fc14f24e8cfcbb2b/src/reporting/PlaytestReport.cpp#L162-L171). Static false positive, not a new delivery failure. | Narrow credential detection without weakening secret protection, add positive/negative boundary tests, preserve bounded diagnostics, separate consent, preview/cancel/retry and privacy. Use local fixtures; do not send new live reports automatically. |

### Phase 2 — graphics configuration, useful choices and representative preview

Implement the [Graphics menu contract](ROADMAP.md#graphics-menu-and-measured-quality-choices) through shared authoritative configuration. Required: real render scale/internal dimensions; supported presets and Custom; truthful Mobile/High/effect dependencies; understandable visual/cost help; effective versus requested values; apply/revert/reset/persistence and unusable-setting recovery on both platforms.

Audit each existing water/fire/glass coupling before exposing independent controls. Implement useful measured reduced-effects options only where real work is removed and physical/gameplay contracts remain valid. Unsupported options remain absent or disabled with reasons; no decorative toggles. Preserve saved settings and provide a named baseline comparison. Every setting has a measured or explicitly unknown cost; no invented percentages.

Build the compact authored preview in ROADMAP using the production rendering path: mirror, flame/lantern, waterfall/pool, glass, representative materials and one actual idle skeleton. Reuse it for material/effects validation. Label live counters “Preview scene performance”; distinguish GPU time, loop time and genuinely presented FPS. Show internal/output dimensions, current effective settings, cap and measured/estimated/unavailable GPU memory. Shared phone RAM is not dedicated VRAM; allocation totals are not exact residency or bandwidth.

Keep game state paused and preview resources isolated. Do not render the entire game concurrently, continuously read back images, or leave an uncapped thermal stress scene running. Test entry/pause routes, repeated apply/revert, background/resume, exit and restoration of gameplay/audio/history.

### Phase 3 — bounded renderer and material foundations

Required material core: audit existing normal paths; material-controlled normal strength and glTF normalTexture.scale through import/ABI/shading; tangent handedness/orientation and missing-tangent policy; authorable believable texture scale with safe defaults. Use the shared material path and authoring contract in ROADMAP, not a parallel bump system. Validate absent maps, material ABI, normal scale/tangents and matched stone/wet stone/wood/metal/ground under moving light. Bound texture memory and secondary-hit cost.

Resolution/temporal work is a staged feasibility decision:
1. Finish efficient fixed-resolution transactions on both platforms first.
2. Assess bounded DRS using reliable timing, min/max scale, hysteresis and stable sizing/history policy; demonstrate oscillation-free behaviour and truthful effective dimensions before admitting it.
3. Evaluate depth, camera/deforming-object motion, jitter, previous transforms, history ownership/invalidation and disocclusion as one shared foundation. Prototype only a bounded vertical slice sufficient to establish cost/correctness and the next step.
4. Assess DLSS Super Resolution on RTX and a justified Android option only against actual Vulkan/SDK/licence/device requirements. Vendor adapters are **not mandatory completion gates**. Record implement/defer/reject with evidence; do not launch a renderer rewrite to satisfy a brand checkbox.

A higher PC shadow tier must genuinely improve the exposed checkerboard/two-sample limit documented in ROADMAP. Compare bounded extra samples and, only if ready, temporal reconstruction, including moving torch/camera/casters and contact edges. Keep phone cost separate and defaults unchanged unless deliberately accepted.

Optional/off-by-default detail normals, a fallback-player/LOD experiment, texture/geometry quality extensions and vendor upscalers need a worthwhile measured benefit. Parallax is deferred unless separately agreed. Do not let optional experiments prevent completion of the required safety/settings/material/polish work.

### Phase 4 — owner-directed visual and audio polish

Implement the three new screenshot requirements below together with the existing [collapsed entry](COLLAPSED_ENTRY_1_6_2.md), [UI refresh](UI_REFRESH_1_6_2.md), [lich reveal](LICH_REVEAL_1_6_2.md) and ROADMAP polish briefs. These are authored targets, not permission to expand the dungeon.

#### New owner screenshot reports, 3 October 2026

All three report release 1.6.1 on SM-S948B / Adreno 840, RayTracingPipeline, Mobile, 63% internal scale (907×1877). These are report settings, not recommended defaults. The screenshots were inspected during intake; exact local scene coordinates remain to be established. Original images stay private and must be inspected through the authorised evidence handoff before image-dependent implementation. Do not infer precise geometry from prose or publish raw images/diagnostics.

| Report ID | Required visual result | Evidence and acceptance |
|---|---|---|
| 43425e1d-de82-40d4-bd33-daf1ebf5318c | Remove/recess the flat rear spawn-wall appearance. Compose rocks, fallen matching bricks/masonry and debris as the impassable collapse, with **stairs visibly receding into darkness behind the blockage**. Surviving architecture must communicate the entry route that failed. | Still confirms the flat-wall appearance; [sanitized intake](https://github.com/Samfa12-tech/The-Horde-RT-demo/pull/16#issuecomment-5965622959). Stairs are a visual remnant, not a new traversable route. Preserve a recessed sealed backing, spawn/held-item envelope, gallery, encounter and forward route. No world/light leaks or climbable escape. |
| 3e461f78-f619-46de-a6ac-5fd4bc115970 | Correct held-torch clipping while walking beneath the pictured low brick opening. | [Sanitized intake](https://github.com/Samfa12-tech/The-Horde-RT-demo/pull/16#issuecomment-5965632882). The screenshot locates the reported opening; motion clipping is owner-reported, not independently reproduced. Reproduce the approach and use shared overhead-clearance/retraction/pose rules. Torch mesh, flame, emitter, hands and real RT shadow must move coherently, with no hidden torch or independent light cheat. Test look angles, movement, attacks/transitions and nearby low roofs. |
| 3696c1a2-5fb3-4476-aaeb-456a130837d8 | The rectangular overhead opening currently exposes sky too soon for the intended underground depth. Extend the shaft/enclosure and its walls, with hanging vines/plants, so the player reads a deeper subterranean space before the visible sky. | Same sanitized intake. Still shows an overhead rectangular opening and pale vertical structures; their exact identity, coordinates and final shaft dimensions are not established. Resolve from actual scene, preserve physical occlusion and finale light progression, avoid inventing a new outside level. Use restrained admitted foliage; validate alpha/material behaviour, ray cost, silhouettes and mobile memory. |

Other required polish:
- Improve shared fire animation/shape, especially the lantern's static-spindle reading; inspect real motion, preserve flame/emission/direct-light/glass/reflection coherence.
- Provide the appropriate visible native environment/sky through actual openings; coordinate with the deeper shaft and future 1.7 moonlit rescue, retaining current finale timing. Check suitable licensed Briarhold assets first.
- Reconcile torch idle pose/flame/emitter sockets before declaring the light static. Add subtle sword idle breathing through the existing pose path, without camera bob, altered hit timing or combat reach.
- Tighten the waterfall audio loop: inspect asset edges/padding and playback scheduling/underruns, then remove audible gaps/clicks across repeated loops and lifecycle/load changes. Do not claim a cause from the report alone.
- Desynchronise skeleton incidental idle phases/sounds while preserving attack events. Locate and licence-check the owner's existing sound inventory before generating replacements.
- Implement the authored approximately six-second Fourth Keeper awakening in the existing chamber, preserving movement/look, safe reveal invulnerability, complete first telegraph, retry/reset and reward idempotence. Use the actual rig's supported motions; no invented articulated staff/cloth, new boss phases or new dialogue.
- Apply the restrained iron/leather/slate, pale-text and amber UI to existing menus/HUD. Retain existing transparency; prototype comfortable touch placement, bounded scale/presets and safe-area/readability before ornament. Preserve multi-touch/parry semantics, accessibility, navigation and reporting consent. Free drag layout editing and generated art are not required.

Use existing/reused/free verified assets and Blender cleanup through the established pipeline. Record source/runtime hashes, licences, scale, topology/material/memory budgets and native RT validation. Paid generation requires separate approval. Honour mandatory layout/visual approval gates; continue independent engineering while awaiting them.

### Phase 5 — integrated candidate and handoff

Run affected tests per AGENT_VALIDATION, then current aggregate CI against the final branch. Include shader compilation/staleness and extracted Shipping checks where changed; Windows Debug/Release and relevant CTests; Android ABI/build/lint/contracts; packaging/licence checks; focused sanitizers as applicable.

Physical validation when allocated:
- RTX target and exact S26, both supported RT backends where admitted; S24/S25 separately evidenced or explicitly unverified.
- Continuous route, combat, torch clearance/bob, collapsed entry, deeper shaft, reveal/reward and waterfall/audio loops; still images alone do not validate these.
- Multi-touch/menu/lifecycle/settings/preview transitions and safe recovery.
- Matched baseline/candidate ordinary and lantern-heavy workloads, same resolution/settings/backend, repeated warm runs with thermals, CPU/GPU timing, pacing, allocations and available memory-pressure evidence. Do not infer bandwidth bottlenecks without counters. Retain negative results.
- Owner visual review of the three requested scenes, UI, fire and lich reveal; manual listening/haptic checks for changed sound/event behaviour. Missing approval is an explicit gate, not a pass.

Completion requires all required slices implemented and validated or a specific owner-approved deferral, optional investigations dispositioned, no unexplained substantial regression, exact artifact/evidence links and a concise remaining-limitations list. Hand back: files/architecture changed; tests actually run; visual/audio evidence; phone/Windows costs; known risks; recommended next step toward 1.7.

**Stop before merge, tagging, production signing, GitHub/itch/site publication or destructive cleanup.** Preserve 1.6.1 frozen artifacts. A development candidate is not a released update. Phone connection alone is not allocation: obtain explicit current permission before installs/testing and never clear user data implicitly.

## Copyable Codex goal

Implement Horde 1.6.2 “Engine readiness and demo polish” using this document as the execution contract and ROADMAP plus its linked UI, collapsed-entry and lich briefs as supporting detail. Work on current released main in a dedicated branch, bringing over intended PR16 planning documents without restoring its older runtime tree. Begin with Phase 0 and the five audit corrections, then deliver truthful graphics controls/preview, bounded material and renderer foundations, and the owner-directed visual/audio/UI polish, including all three stable report IDs above. Maintain a compact checkpoint and decision ledger after every coherent slice so compaction resumes the exact next step instead of repeating completed work. Use gpt-6.1-sol high. Continue safe implementation and affected validation autonomously; coordinate file ownership and scarce device/GPU use. Preserve genuine hardware RT, accepted player/audio/gameplay contracts, privacy, licences and exact evidence. Optional upscalers/detail/LOD work must earn admission and may be deferred; do not expand into 1.7 or a renderer rewrite. Pause only dependent work for missing assets, required owner visual/audio approval, phone allocation or material product decisions, while continuing independent work. Finish with a reviewed candidate, current CI, measured results, explicit limitations and remaining acceptance gates. Do not spend on assets, change credentials/security, clear phone data, merge, sign, tag or publish without separate authority.
