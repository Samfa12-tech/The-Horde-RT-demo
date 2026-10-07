# Horde Lantern RT — Campaign and Engine Roadmap

**Owner:** Sam Small / Samfa12  
**Planning update:** 6 October 2026  
**Status:** Owner-approved direction; future milestones are provisional, not implemented or release promises.

## Authority and navigation

This is the canonical forward-looking campaign roadmap. Use it alongside [AGENTS.md](../AGENTS.md) and [PROJECT_DECISIONS.md](../PROJECT_DECISIONS.md), which retain the engineering, safety and validated-baseline rules. The [Phase Plan](PHASE_PLAN.md) preserves the historical implementation sequence; dated audits and validation reports remain evidence of their own snapshots. This roadmap does not rewrite that history or certify newer runtime behaviour.

The [README](../README.md) identifies the published package and its evidence. A future version label here does not change the application version, prove a feature exists, authorise publication or supersede an unfinished engineering pass.

## Current execution handoff — 3 October 2026

1.6.1 is released and merged, and the owner has confirmed goal completion. The post-release source/recorded-evidence audit is complete; its corrective actions and the three new owner screenshot reports are consolidated in [the 1.6.2 executable goal and acceptance plan](IMPLEMENTATION_GOAL_1_6_2.md). That document now owns sequencing, required versus optional scope, durable checkpoints and implementation acceptance. This update bank remains supporting detail; older prospective start-gate wording below must not restart completed 1.6.1 work or the audit. No merge, paid assets or release is authorised by this planning change.

## Direction approved by the owner

The dungeon becomes the prologue to a small, authored historical-gothic adventure. The player and companion emerge into woodland, reach an inhabited village, and learn more in its tavern about the wider **Horde**. The village becomes the recurring hub for three subsequent themed dungeons.

The owner-approved [campaign, characters and progression direction](CAMPAIGN_DESIGN.md) defines the lost army/treasure-hoard ambiguity, cursed keepers, deceptive lantern voice, three dungeons and single ending. Preserve The Horde as the working title. Kit accompanies a silent player; the eventual narrative is fully voiced. The land is The Veyrlands, the village is Bellwether and the real historical king is King Veyr (owner selection, 3 October 2026); the lantern entity's royal claim remains false. Other unapproved names, detailed scripts and balance stay provisional.

The defining design pillar is **light as gameplay, powered by the actual ray-tracing engine**, not ray tracing as decoration added after conventional rooms and combat.

## Approved world layout
[WORLD_LAYOUT.md](WORLD_LAYOUT.md) is the canonical geography reference approved on 3 October 2026. Routes and relative landmark positions are locked; exact distances and local blockout require playable testing. 1.7 reaches the lookout and shows a low-detail village shell in its fixed future location, with no playable hub, interiors or crowds. 1.8 extends the same road into that village. Preserve the downstream Abbey/eastern Foundry peer branches and two-seal Court gate. This approval does not expand the current 1.6.2 run or promise a seamless open world.

## Milestone sequence

| Milestone | Direction | Status / start condition |
|---|---|---|
| 1.6.1 engineering baseline | Finish the current engineering programme and its validation. | Released and merged; owner confirmed goal complete on 3 October 2026. Preserve frozen artifacts and accepted evidence. |
| 1.6.2 — Engine readiness and demo polish | Full post-release audit; clear graphics settings and measured reduced-effects options; temporal/upscaling readiness, material foundations, hands-first themed UI and dungeon polish. | Planned after completed, accepted, merged and released 1.6.1 and before 1.7 gameplay expansion; sequence and acceptance below. |
| 1.7.0 — Beyond the Tomb | Existing dungeon becomes prologue; physical rope rescue, companion, moonlit woodland, dialogue, world-zone ownership, checkpoints and chapter-specific UI using the accepted 1.6.2 theme. | Scoped planning handoff exists. Start after 1.6.2 is implemented, validated and accepted; then complete and obtain owner acceptance of the agreed 1.7 scope. |
| 1.8.0 — Village Hub | Continue from the forest to a small village with a few explorable interiors, a hero tavern and a small NPC cast. Reveal more of the wider Horde and establish the hub. | Pencilled in. Begin only after 1.7 is made, tested and accepted; use its actual performance and system evidence to finalise scope. |
| Proposed 1.9 — Drowned Abbey | Lake approach, recoverable submerged access, predominantly dry religious house, required reflector/shutter/Bellkeeper chain, seal and return. | Provisional package after accepted 1.8; detailed lake gates in DROWNED_ABBEY_PLAN.md. No release date or implementation start is authorised. |
| Proposed 1.10 — Ashen Foundry | Gorge approach and alternative access, mint/forge, required shuttered-stand/furnace-shell chain, seal and return. | Provisional package; preserve Foundry-first play without an Abbey prerequisite. Validate both peer orders once both exist. |
| Proposed 1.11 — Glass Court | High-ridge approach, both-seal gate, palace/garden, required aperture/receiver/ward keeper encounter, final seal and safe return. | Provisional package after both peer chapters; keeper defeat does not mandate death. No release date. |
| Beyond the trio | Treasury, release of the bound dead, confrontation with the deceptive lantern entity and one hopeful epilogue. | Creative direction approved in CAMPAIGN_DESIGN.md; compact finale, not a promised fourth full dungeon. Implementation scope remains gated. |

Detailed plans:

- [1.7.0 — Beyond the Tomb](superpowers/plans/2026-09-11-beyond-the-tomb-1.7.0.md).
- [1.8.0 — Village Hub: provisional plan](superpowers/plans/2026-09-11-village-hub-1.8.0.md).
- [Three dungeon chapter plans](DUNGEON_CHAPTER_PLANS.md): approved item/theme/boss dependencies, distinct approaches, proposed 1.9–1.11 packages and shared engine/content acceptance gates.
- [Drowned Abbey — flooded basin and staged delivery](DROWNED_ABBEY_PLAN.md): later campaign, proposed 1.9 package with provisional numbering and no release date. River-fed lake and a predominantly dry dungeon; prove the vista, short submerged crossing and one dry room before expanding. Preserve three alternative access paths and Foundry/Abbey either-order progression. Start water work from a bounded analytic-wave RT baseline; FFT is conditional on demonstrated need and measured budgets. This adds no 1.6.2, 1.7 or 1.8 implementation scope.

The three later dungeons are additional adventures after the existing tomb/prologue, not a silent relabelling of that prologue as one of the three. Approved order: Abbey and Foundry in either order, then Glass Court, then the treasury finale; see CAMPAIGN_DESIGN.md for access quests and remaining detailed design.

## 1.6.2 — Engine readiness and demo polish

Finish, accept, merge and release **1.6.1 first**, then undertake this bounded preparation/polish milestone before **1.7 gameplay expansion**. These are future tasks, not new 1.6.1 acceptance blockers. Re-audit the accepted release to avoid duplicating work completed in the engineering pass. Its current programme includes music and RayQuery compatibility; the older post-1.6.1 ordering in historical planning must not reopen or resequence that work.

### Workstream order and dependencies

This section is the grouped 1.6.2 update bank. Priorities establish dependencies, not a promise that every investigation becomes a shipping feature.

| Order | Workstream | Dependency and completion decision |
|---|---|---|
| P0 | Post-release audit corrections | Source/recorded-evidence audit complete. Implement and validate semaphore lifetime, scratch alignment, licence notice, Windows resize and report-filter corrections per IMPLEMENTATION_GOAL_1_6_2.md. |
| P1 | Graphics menu and measured reduced-effects choices | Use the audited settings/renderer map and measured baseline. Ship validated choices with explicit visual tradeoffs. |
| P1 | Temporal inputs, resolution and upscaling | Audit existing resolution behaviour first; shared temporal inputs/history correctness precede temporal adapters. Capability, licence and performance evidence decides integration or deferral. |
| P1 | Material depth foundation | Audit the released material/import paths; complete core normal/scale work. Optional detail/height experiments retain separate acceptance rules. |
| P2 | Themed HUD, touch controls and complete menu refresh | Follow [UI refresh proposal](UI_REFRESH_1_6_2.md); preserve existing transparency and gameplay semantics. Inventory current settings before extending customisation; integrate the P1 Graphics menu. |
| P2 | Existing-demo visual, enemy and audio polish | Reproduce each issue, use shared foundations and preserve accepted gameplay/transport. Independent slices may proceed in parallel once dependencies are clear. |
| Final | Integrated acceptance | Validate the combined candidate and supported settings combinations; record accepted, deferred and unresolved work before 1.7. |

### Full post-release 1.6.1 audit

Perform a fresh comprehensive audit of the delivered baseline after **goal completion, merge and release**. This is distinct from the pre-merge review and existing release gates. A mergeable PR, green CI or tag alone does not satisfy the start condition.

- Record released source and package identities, platforms/backends and exact-device evidence. Reconcile the engineering plan, handoff, accepted results and deferred/open items against what actually shipped.
- Audit renderer/material/asset contracts, graphics/resolution configuration, resource ownership, performance/thermals, gameplay/input/animation, audio/haptics, platform UI/lifecycle, reporting/privacy/delivery, dependencies/licences, tests/CI, packaging and provenance.
- Distinguish source inspection and recorded evidence from tests actually run. Use relevant existing evidence and targeted reproduction rather than repeating completed experiments indiscriminately.
- Classify verified capabilities, reproduced defects, misleading documentation, unvalidated claims and future improvements. Give actionable findings severity, evidence, affected paths, dependencies and a bounded recommendation.
- Produce a prioritised 1.6.2 keep/fix/defer plan and assess readiness for 1.7. Preserve completed music, compatibility and other accepted work; new defects warrant triage, not an automatic renderer rewrite.

### Graphics menu and measured quality choices

Build a proper game-facing **Graphics** section on Android and Windows, accessible from entry and pause/settings. Players must understand and choose quality/performance tradeoffs without RT Lab knowledge. Preserve genuine hardware RT and RT-produced presentation in every supported mode; unsupported hardware receives clear diagnostics.

**Audit the existing controls**

The current engineering snapshot exposes persisted render scale and Off/Mobile/High water controls on both platforms, while RT Lab has additional fire/smoke/glass tuning. These do not establish independent production graphics tiers. Water quality currently also selects fire-emitter quality. Audit the final released source: either separate coupled controls through shared configuration or accurately disclose everything each setting changes. Do not simply rename a scene-authoring slider as a measured performance option.

**Menu and profile contract**

- Group settings into Resolution, Quality/Effects and advanced measured options. Define explicit supported presets and a Custom state; show the effective settings each preset changes. Provide an easy named restore/comparison choice for the accepted baseline.
- Each control explains its visible change, detail/effect reduced or removed, dependencies, application timing and restart requirement. Show requested versus effective values when different, and why an option is unavailable or constrained.
- Provide predictable apply/revert, cancel/back behaviour, persistence and recovery from unusable display changes. Test missing/old/out-of-range configuration, migration, relaunch, retry, pause and Home/resume. Never silently lower quality or overwrite the player's choice to conceal a regression.
- Make help usable without hover; support readable/scalable text, adequate touch targets and keyboard/controller focus/navigation where supported. Check small screens and large font scales. Diagnostics remain optional.
- Quality settings must preserve puzzle solutions, combat timing and essential visual cues. Decorative reductions cannot remove a gameplay-critical blocker, source or hazard. If a safe reduced mode cannot meet that contract, defer it.

**Candidate controls — audit and implementation required**

| Control | Player-facing explanation and admission requirement |
|---|---|
| Fixed render scale | Show actual internal dimensions and explain sharpness/aliasing tradeoffs. Fewer pixels do not guarantee proportional savings. Retain the fixed baseline for comparison. |
| Water quality | Explain exactly what each admitted mode changes in visibility, reflection and transmission, including any coupled fire setting. Demonstrate which work is skipped; do not invent a generic “water off” saving. |
| Lantern panes/glass | Evaluate a named choice over accepted physical profiles. If panes are omitted, explain that actual geometry and optical contribution are removed consistently for all rays. This is reduced presentation, not equivalent glass or fake transparency. Preserve the full physical path where admitted; independent menu selection is proposed work. |
| Fire and smoke detail | Investigate bounded reduced-detail tiers only where shared systems genuinely avoid work. Keep flame/emissive geometry, volume, direct light and reflected/transmitted views coherent. Distinguish visual detail from light strength; preserve authoritative gameplay. |
| Other secondary visual effects | Admit controls individually after measuring actual costs/dependencies and preserving the agreed physical contract. Do not silently reduce ray samples, reflection/shadow correctness or transport budgets. Explain unavailable combinations. |
| Dynamic resolution/upscaling | Remain scoped investigations below. Expose options only after implementation, capability checks, history/resource correctness and real-device validation. No dead toggles or unsupported vendor claims. |

**Performance information and validation**

Give an expected cost category in plain language, supported by measurements, and explicitly show **not yet measured** where evidence is absent. Any numerical saving/range must identify hardware, backend, build, scene/workload, dimensions and relevant settings. Prefer frame-time and sustained-behaviour evidence; do not promise a universal FPS boost. A disabled effect may save little in another scene or bottleneck.

Measure representative opening, combat, held-lantern and water/secondary-effect workloads with matched candidates and warm thermal conditions. Keep fixed 75% S26 baseline comparisons separate from reduced-quality, adaptive, upscaled or externally cooled runs. Confirm savings remove real work; inspect CPU/GPU timing, memory and pacing where measurable. Missing counters and untested devices remain explicit gaps, not inferred passes.

Completion requires accurate UI-to-renderer mapping, reliable persistence/apply/revert, accessible controls, tested supported combinations, genuine RT presentation, matched images/motion and sustained-cost evidence. Record **ship / defer / reject** for each candidate control. Automated tests and captures do not replace live Android/Windows usability review.

**Expanded settings inventory**

Treat these as audited candidates, not promises of unsupported renderer features:

- **Display/performance:** Windows display resolution and window/fullscreen mode; explicit internal scale/dimensions; supported upscaler and quality mode; DRS target frame rate and minimum/maximum scale; foreground, menu and background frame caps; supported V-sync/presentation choices. Show current effective DRS scale and interactions with caps/upscalers. Resolution, upscaling and DRS must compose through one tested sizing/history policy.
- **Materials/scene detail:** texture quality backed by real mip/residency choices, texture filtering, and geometry/detail distance only with a validated shared LOD path. Preserve appropriate silhouettes, shadows and reflected/transmitted representations. Optional detail-normal control depends on that capability being implemented and accepted.
- **RT/effects:** offer an understandable overall preset plus supported individual water, fire, glass and smoke/fog overrides. Explain all coupled settings. Reflection/secondary-lighting controls require a proven cheaper mode consistent with the RT contract, rather than exposing unsafe transport parameters.
- **Image/comfort:** brightness/gamma calibration, FOV and existing optional post effects/camera shake where applicable. HDR, motion blur, bloom and other controls appear only if actually supported. Keep comfort/accessibility choices distinct from performance presets.
- **Player representation:** retain a lower-detail/fallback-player idea as an experiment, not a required shipping toggle. First measure matched visual and CPU/GPU costs, including skinning and acceleration-structure work. Prefer a supported LOD using shared animation/IK over maintaining two independent player implementations. Merely hiding the body does not prove its work was removed. Preserve hands, equipment, collision, shadows/reflections and gameplay readability; reject or defer without worthwhile savings.

**GPU memory information**

Where supported, distinguish the game's tracked allocations, driver-reported heap usage/current usable budget, and physical capacity. Label measured, estimated and unavailable quantities accurately; do not call allocation totals exact residency or infer memory bandwidth from them. Consider `VK_EXT_memory_budget` only after capability checks and correct per-heap accounting; do not double-count shared heaps or sum unrelated capacities.

For discrete GPUs, show dedicated capacity separately from the current budget. On Android and integrated GPUs, label shared GPU/system memory honestly rather than presenting the phone's total RAM as dedicated VRAM. Explain that other applications and OS pressure can change the usable budget. A settings-dependent estimate must say it is an estimate and need not match current loaded-scene usage. Bound refresh overhead and give a clear unavailable state; budget-pressure warnings are guidance, not guaranteed crash thresholds.

References: [Khronos memory-budget sample](https://docs.vulkan.org/samples/latest/samples/extensions/memory_budget/README.html) and [Vulkan memory allocation guidance](https://github.khronos.org/Vulkan-Site/guide/latest/memory_allocation.html).

### Torch-shadow sampling and a higher PC quality option

**2 October 2026 observation and source inspection.** The owner confirmed the supplied opening-room screenshot was captured on PC, described solid dark shadows and checkerboard half-shade, and reported that the pattern is less noticeable on the phone. Actual screenshot pixels were inspected: alternating light/dark pixels are visible at shadow boundaries. The source findings below are pinned to engineering checkpoint `90b3d3a736949a3dba5f6e95c72da9657243485f`; the screenshot's executable identity and effective settings were not verified, and no live reproduction or matched phone/PC test was run.

**Verified mechanism and limits**

- Torch direct lighting has **two fixed sample offsets** and selects one with `(pixel.x + pixel.y) & 1`. Ordinary Lean/Authored workload uses one visibility query per contributing emitter/pixel. If both sample positions are blocked, the region is dark; if only one is blocked, neighbouring pixels alternate visibility, explaining the apparent checkerboard half-shadow. See [torch sampling and optional averaging](https://github.com/Samfa12-tech/The-Horde-RT-demo/blob/90b3d3a736949a3dba5f6e95c72da9657243485f/shaders/raytracing/include/rt_lighting.glsl#L1037-L1085).
- **Max workload** evaluates both positions and averages their visibility per primary opaque receiver. The controlling switch is `workloadPreset >= 1.5`, not the Mobile/High water or dielectric-quality setting. Max can remove this particular alternating selection on those receivers, but two samples still produce coarse visibility levels rather than a genuinely smooth, many-sample penumbra. See [workload-to-direct-light mapping](https://github.com/Samfa12-tech/The-Horde-RT-demo/blob/90b3d3a736949a3dba5f6e95c72da9657243485f/shaders/raytracing/include/rt_dielectric_common.glsl#L297-L310).
- Opaque visibility is binary for each sample ([visibility implementation](https://github.com/Samfa12-tech/The-Horde-RT-demo/blob/90b3d3a736949a3dba5f6e95c72da9657243485f/shaders/raytracing/include/rt_lighting.glsl#L846-L879)). Thus the two-sample average can be 0, 0.5 or 1 for that opaque shadow term. This does **not** mean the rendered scene has only three brightness levels: materials, light colour, attenuation, diffuse/specular response, other lighting, transmission, fog and tone mapping also contribute.
- The inspected frame-to-presentation path writes each shaded pixel directly and then copies or scales the image; it has no temporal shadow accumulation or shadow-denoising stage to resolve this pattern. See [frame output](https://github.com/Samfa12-tech/The-Horde-RT-demo/blob/90b3d3a736949a3dba5f6e95c72da9657243485f/shaders/raytracing/include/rt_frame.glsl#L54-L75) and [copy/scaled presentation](https://github.com/Samfa12-tech/The-Horde-RT-demo/blob/90b3d3a736949a3dba5f6e95c72da9657243485f/src/vulkan/raytracing/PresentableTinyRtScene.cpp#L5826-L5855). Treat the checkerboard as an exposed sparse-sampling quality limitation consistent with the screenshot, not proof of a broken RT intersection path or a newly introduced regression.
- Android defaults to **75%** internal render scale ([native default](https://github.com/Samfa12-tech/The-Horde-RT-demo/blob/90b3d3a736949a3dba5f6e95c72da9657243485f/android/app/src/main/cpp/android_probe_bridge.cpp#L76), [persisted preference/default](https://github.com/Samfa12-tech/The-Horde-RT-demo/blob/90b3d3a736949a3dba5f6e95c72da9657243485f/android/app/src/main/java/com/samfa12/hordelanternrt/MainActivity.java#L290-L297)); Windows defaults to **100%** ([settings load](https://github.com/Samfa12-tech/The-Horde-RT-demo/blob/90b3d3a736949a3dba5f6e95c72da9657243485f/src/platform/windows/DiagnosticWindow.cpp#L702-L705)). Saved settings can override both. Scaled presentation uses linear filtering; native-size presentation copies pixels. Linear upscaling can soften adjacent alternating pixels, while a phone's higher physical pixel density may also make them less visible. Neither explanation was isolated by measurement, and the actual device settings are unknown.

**Proposed 1.6.2 investigation**

Evaluate an explicit higher PC shadow-quality option within the Graphics settings workstream after the released-baseline audit. Separate shadow sampling quality from scene-authoring/light-strength controls and disclose any coupling. Compare a bounded increase in area-light samples and/or temporal reuse with appropriate reconstruction against the accepted baseline; temporal approaches depend on the shared history/motion/disocclusion foundation below. Existing Max is a useful two-sample comparison, not an already implemented smooth-shadow tier. Preserve genuine hardware-RT visibility, physical casters and gameplay-critical lighting. Retain a measured mobile budget and assess actual Android cost before exposing or promoting any shared change; do not silently raise phone cost or hide it with reduced resolution.

**Acceptance**

Use matched stills and motion in the opening room and graphics preview: moving torch, moving camera, static holds, contact/grazing edges, moving casters and newly revealed surfaces. Demonstrate reduced checkerboarding and useful penumbra quality without flicker, ghosting, detached/contact-light leaks or lost gameplay cues. For temporal candidates, include resets, cuts, lifecycle and resolution changes. Record exact build/backend/device, effective workload and shadow settings, sample/history policy, internal/output dimensions and filtering, with matched quality and warm sustained frame-time/memory evidence. Keep phone and PC results separate; show honest requested/effective settings and measured or unavailable costs in the menu. Decide ship/defer/reject from those results. This note adds no 1.6.1 implementation or release gate.

### Authored graphics preview room

**Owner-approved direction:** provide a compact authored scene within the Graphics menu, with real-time FPS/frame-time feedback as settings change. Include an **idle animated skeleton**, mirror, flame, waterfall and glass pane. Reuse this room as the representative material/effects validation scene where practical; do not build a second renderer or unrelated playable level.

**Scene brief**

- A small historical-gothic dungeon alcove with detailed stone, wood and metal, a torch and reward-style lantern, a clearly visible glass pane, a modest waterfall/pool, restrained fog/smoke and a mirror positioned to expose reflection changes.
- One skeleton using the actual admitted actor, material, rig and skinning/animation path, in a bounded looping idle. Its movement and mirror/shadow visibility exercise deforming geometry and temporal stability. It is not a combat encounter and must not alter player progression or consume gameplay events.
- Where the player-body option is under evaluation, show the actual player representation through the same viewmodel/world-body ownership rules. Do not substitute the skeleton as evidence of player-skinning cost.
- Keep the room small enough for supported phone settings. Reuse licensed assets/shared effects and bound resident resources. A waterfall or other effect that still needs engine work is a scoped dependency, not permission for a new fluid simulation.

**Interaction and comparison**

- Selecting a setting can focus a relevant fixed camera: textures at an angled wall/floor, glass at the pane/lantern, water at the pool, animation at the skeleton and reflection at the mirror. Allow a stable overview and avoid forced camera movement while comparing settings.
- Use a deterministic scene seed, camera, lighting and animation timeline. A visual A/B mode resets to the same point; make clear which configuration is shown. Invalidate/re-warm temporal history correctly when required.
- Provide a repeatable moving-camera/animation test for upscaling, DRS, ghosting, shimmer and disocclusion. Frozen captures alone cannot validate these paths.
- Apply the real selected configuration using the production renderer, shaders/materials, RT geometry and effects. Explain deferred/restart-required settings instead of showing an approximation.
- Keep the underlying gameplay safely paused and isolated. Avoid rendering the full game and preview simultaneously; manage shared resource ownership and unload preview-only assets on exit without corrupting gameplay, audio, saves or temporal history. Prevent preview input from reaching combat.

**Live performance panel**

- Show presented FPS where measurable, frame time in milliseconds and a short rolling frame-time graph. Define the sampling scope; if only rendered/submitted frames are observable, label that limitation rather than claiming display presentation. Smooth numeric readouts for legibility while retaining stutters in the graph. Add separately labelled CPU/GPU times only where reliable instrumentation supports them; GPU work time is not whole-frame or presentation time.
- Display actual internal/output dimensions, effective quality/upscaler/DRS settings, and any active frame cap/V-sync limit. A capped result cannot demonstrate that an optimisation has no benefit; do not quietly uncap the device.
- Label data **Preview scene performance**. This room is illustrative and is not a prediction of every fight, level or lantern-heavy workload. Retain the separate deterministic full-game benchmark with summaries and comparison against the previous run.
- Distinguish cold loading, shader/pipeline creation, resource rebuilds, setting transitions and history warm-up from stable samples. Keep transition stalls observable, but do not mix them silently into steady-state comparisons.
- Show the memory information above where available. Optional temperature/throttling indications require actual platform evidence. Record thermal/power context for comparisons; a cool short preview is not sustained-performance proof.
- Measure observer/UI overhead and maintain bounded buffers/update frequency. Do not add synchronous readbacks or logging that materially distort the measured result.

**Phone, lifecycle and acceptance**

Provide a user-controlled preview pause and a reasonable visible menu frame cap. Pause rendering when backgrounded; resume safely with valid resources/history. An explicitly invoked performance test may use its documented test cap, with thermal guidance, rather than leaving an uncapped stress scene running throughout settings navigation.

Acceptance requires honest setting-to-image correspondence, working skeleton/mirror/water/fire/glass paths, deterministic A/B and motion tests, labelled trustworthy counters, safe apply/revert/persistence, pause/background/resume/exit recovery, and measured memory/observer cost on actual supported Android hardware and Windows RTX. Test menu-opened-from-game and entry-menu paths; verify gameplay resources/state restore correctly. Missing capability stays unavailable or deferred with a reason. This plan adds no requirement to change 1.6.1 before its release.

### Renderer readiness: temporal inputs, resolution and upscaling

- Evaluate one shared temporal-input foundation: depth, motion vectors for camera and moving/deforming objects, jitter and previous-frame transforms, plus explicit history ownership, invalidation and disocclusion handling. Cover reset/retry, camera cuts, lifecycle and internal-resolution changes before relying on accumulated history.
- Evaluate platform adapters over that foundation: DLSS Super Resolution on supported Windows RTX hardware, and SGSR 2 or another justified Android option. Inspect actual SDK/licence, Vulkan and device requirements, then measure image quality, memory, latency and sustained cost. This is a measured feasibility/integration direction, not a promise to ship either vendor integration or claim support before evidence exists.
- Audit dynamic resolution and efficient render-target resizing on the final 1.6.1 baseline; finish only the missing work. Keep resource/descriptor lifetime and temporal-history reset correct, avoid unnecessary full-scene rebuilds or resize stalls, and report actual internal dimensions and adaptive behaviour.
- Retain the fixed **75% Galaxy S26 Ultra sustained 30 FPS baseline/target** as a distinct acceptance comparison. Record exact source/artifact, scene, workload, dimensions, build type and warm thermal conditions. Adaptive-resolution, upscaled and cooled runs are separate labelled evidence; none substitutes for the fixed-scale baseline or proves an unmeasured pass. Preserve real hardware RT and investigate matched regressions.

### Material depth foundation

After 1.6.1 is released and accepted, audit its actual material/import/shader paths before implementing this bounded 1.6.2 slice. Historical claims about two normal-map paths, a fixed `mix(..., 0.34)` strength or existing ASTC/KTX2 support are audit leads, not verified facts about the final baseline. Extend the shared material ABI/import infrastructure; do not introduce a parallel legacy bump system, redesign the renderer or mass-produce 1.7 forest assets.

**Core scope**

- Audit normal-map correctness across existing dungeon/environment and imported PBR paths. Provide material-controlled strength and correctly carry glTF `normalTexture.scale` through import, GPU material data and RT shading; preserve normalisation, tangent/bitangent handedness, orientation and a tested missing-tangent policy. Strength changes must be deliberate authoring choices, not an automatic global visual retune.
- Make real-world texture scale/tiling authorable where appropriate, keeping stone, wood/bark, metal and ground at believable scales without forcing every existing asset to be re-authored. Preserve safe defaults, established static/skinned asset contracts and accepted lighting/glass/fire/gameplay behaviour.
- Document the authoring rule: large forms and silhouettes use real geometry; important medium relief uses geometry and/or high-to-low baked surface detail as appropriate; fine surface detail uses normal maps. Normal maps do not change physical silhouettes or actual ray intersections. Support high-to-low normal baking from Blender/Meshy sources through the existing provenance-controlled pipeline; do not bake scene lighting, shadows or reflections.
- Keep roughness and metallic response coherent. Define any AO-map policy explicitly: glTF occlusion affects indirect lighting, not direct torch illumination; avoid double-counting visibility already evaluated by RT. Reference the [glTF material specification](https://registry.khronos.org/glTF/specs/2.0/glTF-2.0.html).

**Optional, measured extensions**

- Detail normals are optional and off by default. Evaluate a bounded reusable combination of a primary normal and fine repeating detail only on suitable materials; measure sample cost, memory and secondary-hit shading cost. They are not required to declare the core slice complete and may be deferred if the benefit does not justify sustained phone cost.
- Parallax/height mapping is deferred by default and is not a completion requirement. A separately agreed, one-material experiment may later measure its benefit against GPU/sample cost and consistency with actual RT geometry, reflected/transmitted views, shadows and grazing angles. Never use it to replace silhouette or substantial physical depth. Record a keep/defer decision if an experiment is undertaken; no automatic production admission.
- Use supported compression/mip/texture routes only after verifying the accepted baseline. Bound memory and sampling growth; do not silently reduce resolution, lighting or quality to pay for new material features.

**Validation and acceptance**

Use a compact reusable validation scene or existing captures covering stone, wet stone, wood/bark, metal and ground under moving torch/lantern lighting. Capture matched before/after stills and motion on the exact Galaxy S26 Ultra and Windows RTX 5050, with build/artifact, backend, camera, scale and thermal context recorded. Include the fixed 75% baseline and separately labelled intended lower-resolution/temporal modes when available; inspect shimmering, aliasing and temporal stability, not only attractive frozen captures. Retain genuinely hardware-ray-traced presentation.

Test normal import/scale, tangent failure/fallback cases, material ABI and deterministic configuration, and absent normal/detail/height maps. Run affected shader compilation/staleness checks, Windows Debug/Release CTests and Android builds; verify real RT presentation and matched sustained device costs. Missing-device evidence is an open gap, not a pass. Core completion means correct authorable normals and texture scale, useful material evidence and 1.7 asset-authoring guidance, with optional extensions explicitly accepted or deferred. This planning entry does not add new 1.6.1 blockers or authorise implementation, paid assets, signing or release.

### Themed UI, menus and hands-first controls

Bring the existing-demo themed HUD/menu refresh forward from 1.7 into 1.6.2. Follow [UI_REFRESH_1_6_2.md](UI_REFRESH_1_6_2.md) for the source-grounded dark-iron/leather, parchment and amber direction; compact vitality/status hierarchy; thumb-friendly action shapes; readable pressed/focus/disabled states; and complete entry/pause/settings/death/reporting coverage. **Transparency already exists** and must be retained, not presented as a new feature. Audit released opacity/layout settings before adding bounded customisation. Preserve default movement/look and combat semantics, safe touch targets, centre-screen visibility, platform-native accessibility and keyboard/controller navigation. Optional image-generated ornament is not required or authorised by this plan. 1.7 reuses the accepted system for its new traversal, dialogue/subtitle and chapter-specific UI.

### Existing-demo polish: visuals, enemies and audio

**Queued final visual change, owner clarification 3 October 2026:** After the current run is finished, before 1.6.2 release, retain the small grated wall access panel just right outside the opening room and put the intended overgrowth there; remove/close the entry-room skylight; leave the waterfall's own hole and vines untouched; add an impassable iron-bar grid with strong real RT shadows to the separate large skylight in that same room. These are four distinct features. Follow [the final visual slice](IMPLEMENTATION_GOAL_1_6_2.md#final-visual-slice--after-the-current-run-before-162-release), preserving the separate later-created finale/rescue opening. The 5 October owner direction now brings both existing skeletons' waterfall-room relocation into the post-reset 1.6.2 pass; see the shared equipment/encounter contract below.

- **Fire, especially the reward lantern:** investigate the owner's report that the flame reads as a static coloured tapered spindle. Inspect it in motion before attributing a cause; improve shape, animation and readability through the shared world-space fire system while keeping emissive geometry, volume, direct light, glass and reflections coherent. Check held, raised, swinging and stationary views on phone and Windows.
- **Opening/overgrowth report 3696c1a2-5fb3-4476-aaeb-456a130837d8:** the owner's later clarification identifies the intended growth at the small grated wall access panel just right outside the opening room. Preserve that panel and the separate waterfall hole's existing vines; do not carry the earlier unverified overhead-shaft identity into implementation. Resolve remaining depth/enclosure detail from the actual authorised evidence, preserving physical occlusion/finale lighting and validating native RT foliage/mobile cost. See the reconciled report and final visual slice in IMPLEMENTATION_GOAL_1_6_2.md.
- **Visible exterior sky/skybox:** give existing openings a convincing visible environment through the native RT path. Evaluate Briarhold's sky assets before creating replacements, checking provenance, projection, seams, colour space and suitability for Horde's existing lighting/exposure. A direction-sampled miss environment is valid; preserve physical roof/geometry occlusion and consistent relevant reflected/transmitted views. Do not add a fake background quad or raster fallback. Preserve the existing finale's accepted timing and light progression; 1.7 owns its deliberate moonlit rescue change.
- **Collapsed entrance behind spawn:** replace the plain-wall reading with a visibly collapsed former passage: grounded boulders, matching broken masonry, damaged opening, stairs receding into darkness behind the blockage, and recessed sealed backing. Follow the [asset shortlist, Blender preparation and ready-to-use Codex handoff](COLLAPSED_ENTRY_1_6_2.md). Preserve spawn, gallery, forward route and torch clearance; begin with verified free CC0 sources rather than paid generation.
- **Entryway/skeleton-area props:** identify the flat-looking objects from the actual scene/captures, then improve or replace their geometry/materials as appropriate. Do not guess their identity or assume a particular asset is the cause. Retain collision, route readability and measured RT budgets.
- **Shared equipment and waterfall encounter (owner direction, 5 October 2026):** In post-reset 1.6.2, start with torch held/sword sheathed, draw on safe corridor skeleton cues before contact (or an earlier attack-input request), and move both existing guards into the waterfall room without changing count/stats. Extend shared draw/sheath/stow/restore state, attachments and animation-timed sound through existing engine paths and game-layer triggers. Preserve responsive defensive control, no automatic damage, valid interruption/recovery, and physical item/light coherence. Audition the owner's local Filmcow-on-itch bank first, checking finished-game versus raw public redistribution rights; the owner will seek Pixabay alternatives if needed. Follow [the full scope and acceptance contract](IMPLEMENTATION_GOAL_1_6_2.md#shared-equipment-and-waterfall-encounter--owner-direction-5-october-2026). No rope implementation, Kit voice, forest/night expansion, initial-camera change or restart of paused PR18 is implied.
- **Player's disposable rag torch (owner direction, 5 October 2026):** queue the post-reset 1.6.2 replacement of the ornate player torch with a rough wooden stick and charred rag head, without ornate metalwork. Follow [the bounded replacement and validation contract](IMPLEMENTATION_GOAL_1_6_2.md#players-disposable-rag-torch--owner-direction-5-october-2026). Keep the Keeper reward lantern and lich wall torches unchanged; preserve held/drop/drench/discard behavior and physical flame/light/shadow/reflection coherence. Historical appearance approvals remain historical; this is future art direction, not implementation or a measured performance gain.
- **Held torch/light coherence and sword idle motion:** the owner reports the torch bobbing at rest while light/shadows appear not to follow, and requests more natural sword idle breathing. This is not runtime-reproduced here. Source inspection at [9931cf18](https://github.com/Samfa12-tech/The-Horde-RT-demo/tree/9931cf18fc36369ca6efbe2b785d15f877bf6d31) already propagates a small idle sway into the simulation emitter: [HeldItemKinematics](https://github.com/Samfa12-tech/The-Horde-RT-demo/blob/9931cf18fc36369ca6efbe2b785d15f877bf6d31/src/gameplay/items/HeldItemKinematics.cpp) composes torch/flame/light sockets together, while the renderer has a later final-grip item recomposition and a distinct emitter upload. Reconcile actual source paths and final visible torch/flame/light socket agreement in the released baseline rather than assuming absent light animation. Reproduce the exact live build through idle, movement, look, clearance, drop and reward-lantern transitions. Keep flame and light attached to the same stable authored source; preserve genuine moving RT shadows, with no independent cosmetic light oscillation. Evaluate subtle sword breathing through the shared hand/item pose, blending cleanly into attack/parry without changing hit tests, timing, grip/IK or clearance. Do not add camera bob; a bounded strength/speed/off comfort option is only a candidate if useful and must keep prop/emitter motion coherent. Validate matched PC/phone motion with steady/moving camera, shadow contact, pause/resume and no phase pops, flicker or drift; record cost. The source inspection is not evidence that a visible desynchronisation has been reproduced.
- **Held torch versus low roofs:** reproduce the reported clipping and investigate both visible equipment clearance and physical geometry/collision. Use the shared held-item/retraction contract, including overhead clearance; verify torch, flame, light, hands and shadows agree through movement and look angles rather than hiding the problem with an overlay.

- **Waterfall loop seam:** the owner reports a slight pause between the end and restart of the waterfall sample; this is reported, not reproduced in this planning task. Audit source sample edges, encoded padding/loop metadata, decoder/playback queue and scheduling/underrun behaviour before attributing a cause. Use trimming or a short waveform-matched loop crossfade only when appropriate, preserving sustained level, spatial position/attenuation and the intended waterfall character. Verify many consecutive loops without an audible gap, click or pop on Android and Windows during normal play, representative load and lifecycle return. Record source/runtime asset identity and affected playback evidence; manual audio validation is required for a changed candidate. This is a bounded 1.6.2 ambience fix, not reopening the accepted music programme.

- **Skeletons and lich:** address the reported identical-looking/synchronised skeletons, add or complete skeleton sounds, and give the lich a grander keeper-consistent reveal, animation and sound pass. Follow the [explicit Fourth Keeper reveal brief](LICH_REVEAL_1_6_2.md) for room staging, the six-second awakening, rig-safe animation, sound mapping, retry and combat/reward handoff; this replaces the vague reveal request. See also [enemy polish requirements](CAMPAIGN_DESIGN.md#existing-demo-polish-and-milestone-ownership). Preserve authoritative combat timing and lantern-after-lich reward order; measure independent-pose costs and manually validate changed audio.

This milestone prepares reusable foundations and polishes the existing demo. It does not deliver an outdoor level, rope climb, companion, forest route or the complete moonlit sky/atmosphere programme; those remain in the scoped 1.7 plan. Accept each slice with affected tests, real RT captures/motion review and device measurements, recording any deferred adapter or unresolved quality/performance limit honestly.

### Integrated 1.6.2 acceptance

Review the combined candidate, not only successful isolated slices. Recheck affected host/shader/Android tests, actual RT presentation, settings combinations, lifecycle and representative gameplay routes on exact supported devices and the Windows RTX target. Retain matched fixed-baseline evidence alongside labelled reduced-effects or temporal modes. Audio/haptic changes require applicable manual checks; unrelated graphics/documentation work does not automatically reopen accepted audio.

The final record states what shipped, what was accepted or deferred, actual quality/performance limits and remaining validation gaps. Optional vendor adapters, detail normals and height experiments are not mandatory merely because they appear in this bank. No roadmap entry authorises signing, publication or moving to 1.7 before milestone acceptance.

## Engine product and demo/game packaging proposal

**Owner-approved strategic direction, 3 October 2026:** The engine is the core product; The Horde is the game used to build and test it. The existing dungeon is a compact RT showcase and repeatable regression workload. The growing game supplies concrete needs for zones/residency, characters, traversal/physics, audio and outdoor environments. Build reusable capabilities from those needs, not an unbounded generic-engine framework.

**Proposal, not approved technical architecture:** maintain two application/build/content targets over shared engine modules: a bounded dungeon demo and the developing game. This could preserve a small showcase while letting the game grow from 1.7, without copying the renderer into diverging forks. Exact target names, module boundaries, repository layout, package/save identities, versioning and delivery policy require a scoped design decision. No repository creation, project split, engine extraction or new engineering goal is authorised here.

Keep accepted legacy demo releases and their evidence immutable. A later explicitly scoped renderer-update demo could consume tested shared improvements and receive its own version, validation and release approval; it must not overwrite an old artifact or silently inherit campaign progression. Whether/when the demo stops receiving feature updates remains a product decision. Do not delay 1.7 for a speculative large engine reorganisation.

## 1.7 addition — continuous night and real moonlight

**Owner clarification, 3 October 2026:** Defeating the lich, claiming the lantern and leaving the tomb do not trigger daytime in 1.7. Preserve the separate later-created rescue opening, but decouple geometric opening/reward progression from dawn lighting. Rope rescue, reunion, forest and checkpoint restores remain at night. Whether daylight ever enters the later game, and when or why, is deliberately undecided.

The visible moon must align with a genuine directional/finite-angular light source using hardware-RT scene visibility/transmittance; making a sky texture brighter does not implement moonlight or GI. Reuse one coherent night definition through the tomb opening and outside, with stable exposure and relevant secondary/volumetric paths. See [the 1.7 night and bounded lighting-research contract](superpowers/plans/2026-09-11-beyond-the-tomb-1.7.0.md#82-night-environment).

RTAO and RTGI are optional bounded research, not promised 1.7 features. Audit existing direct/ambient/indirect contributions first to avoid double occlusion or energy. Admit only measured improvements meeting explicit quality, frame-time, memory and sustained platform budgets on exact hardware; defer otherwise. Current phone viability is unmeasured, and future hardware is not a delivery assumption.

This is future campaign planning. It does not change the accepted 1.6.2 dungeon-demo finale by implication, reopen the current run, alter the approved four opening features or geography, or authorise runtime work, merging or release.

## 1.7 addition — Kit at the prologue grate

**Latest owner clarification, 3 October 2026:** Kit's first call comes through the **small grated wall access panel just to the right outside the opening room**, on an early safe approach. It is not the entry-room skylight, the waterfall's own hole or the separate large skylight in the waterfall room. The 5 October direction moves the same two skeletons into the waterfall room during post-reset 1.6.2; the earlier wall-panel call does not wait for that encounter. The panel retains the intended overgrowth from report **3696c1a2-5fb3-4476-aaeb-456a130837d8**; the waterfall's own hole keeps its vines untouched, while the separate large skylight in that room gains an impassable iron grid in the queued post-run 1.6.2 visual slice. Skeleton relocation is now 1.6.2 work; Kit voice/trigger and dialogue controls remain gated 1.7 work. Kit does not know about the lich. The later-created finale opening enables rope rescue; the waterfall grid prevents an obvious earlier escape without inventing a decision by Kit to wait for the fight.

Use directional world-space sound anchored beyond the actual small wall grate, separate Dialogue/Music/SFX gains and a subtitle on/off option with readable top-safe-area placement on mobile. Preserve a silent player, movement/look, combat cue priority, intelligibility and safe checkpoint/re-entry behaviour. The owner's suggested line is provisional recording copy. [Scene, stable line ID and acceptance details](superpowers/plans/2026-09-11-beyond-the-tomb-1.7.0.md#41a-kits-early-wall-panel-call-and-the-waterfall-room-guards).

## 1.7 addition — traversal pace and material-aware footsteps

**Owner direction, 3 October 2026:** Evaluate running for the larger world and add surface-aware footstep sound to the 1.7 plan, after its start gate. Proposed mobile Run toggle and desktop hold/toggle accessibility should retain precise walking, existing combat/traversal semantics and safe input cancellation; no stamina system is implied. Tune pace against the 40–80 m forest blockout and approved map routes rather than fixing a speculative speed multiplier.

Extend the existing collision-distance cadence, shared gameplay events and audio path. Classify actual authored supporting surfaces (stone, dirt, grass/leaf litter, wood or shallow water where present), preserve accepted SFX balance, and use bounded clip variation/voices. Share wet-contact ownership with the existing 1.7 water-interaction investigation to prevent duplicate step/splash audio. See [the canonical traversal and surface-audio contract](superpowers/plans/2026-09-11-beyond-the-tomb-1.7.0.md#55-larger-world-walking-running-and-surface-aware-footsteps) for source evidence, controls, collision/animation and platform acceptance. This planning addition does not change the current/frozen 1.6.2 scope or claim runtime work is complete.

## 1.7 addition — Kit animation acquisition and simpler rope staging

**Owner direction, 4 October 2026:** Preserve the delivered [Kit animation plan](KIT_ANIMATION_PLAN_1_7.md): validate the Warden candidate first, reuse suitable idle/walk, test restrained Meshy gestures/turns, inspect free CC0 alternatives and use Blender for retargeting, transitions and necessary cleanup. Eric chooses motions and handles technical checks; Sam reviews appearance and feel in game. Catalog names/IDs are verified research, not proof of rig compatibility or motion quality. Rights, Kit appearance, Horde import and actual scene performance remain open gates; no spending is authorised.

The rope may arrive offscreen during lantern pickup, hang from a fixed world anchor and leave Kit idling nearby. No visible throw, hand-release, recovery or holding clip is required. The runtime deploys the rope once, independently of Kit animation, with credible visible arrival and existing reward/opening/readiness gates intact. Preserve real rope physics, player grip/climb/crest and equipment handling, fresh manual lantern recognition, unseen early grate call, route/wait behavior and campaign canon. The separate player directional-motion appendix is a candidate/gap list, not approval of new Kit mechanics or every proposed clip.

See the [master contract](superpowers/plans/2026-09-11-beyond-the-tomb-1.7.0.md#owner-clarification--kit-animation-and-rope-staging-4-october-2026) and [asset checklist](ASSET_PLAN_1_7.md#2-briarhold-reuse-shortlist). Production stays behind accepted 1.6.2; this is documentation only.

## 1.7 addition — combat timing and parry readability

**Owner-approved addition, 4 October 2026:** Include a focused combat animation/timing/contact foundation in 1.7 after the accepted 1.6.2 baseline. Retain the shared 60 Hz simulation. The [canonical Section 5.6 contract and WP2a checklist](superpowers/plans/2026-09-11-beyond-the-tomb-1.7.0.md#56-combat-timing-readable-contact-and-parry-feedback) records the source review of candidate `abcfd862f4de21a07206d65a6c7055ecda4a6054`, shared attack timeline, timestamped input edges, immediate-riposte-compatible lasting parry feedback, and FPS/hitch/boundary validation.

The 1.12-second gameplay versus 1.20-second renderer contact conventions require clip inspection; an actual 80 ms visible-contact error has not been established. Calibrate the existing range/cone model first and use bounded simulation contact proxies only if needed and measured. This does not require complex physics, a new engine, expanded encounters or changes to the frozen/current 1.6.2 scope. Source findings are not runtime or owner feel acceptance.

## 1.7 addition — hand motion, low passages and phone Graphics

**Owner direction, 4 October 2026:** Add slight alternating left/right hand motion rather than synchronized bobbing, and lower the sword through low passages, with a restrained upper-body duck only if needed. Reuse and verify the reported working torch-clearance behavior; preserve equipment grips/IK, collision, attack/parry timing, reduced-motion comfort and coherent world-body/viewmodel RT ownership. The [player-motion and clearance contract](superpowers/plans/2026-09-11-beyond-the-tomb-1.7.0.md#57-alternating-hand-motion-and-low-clearance-sword-handling) defines motion, low-passage and secondary-view checks.

**Latest Graphics clarification:** Simplify phone Graphics into concise grouped choices with optional per-section expanded explanations and honest performance impact, retaining the accepted transparency, accessibility and thumb comfort. Preserve the existing apply/save/confirmation/revert safeguards. One Apply & Save button is optional only if straightforward without weakening safety; do not rebuild settings handling just to remove a button. Otherwise the simplified layout and help are sufficient. Failed or unconfirmed graphics changes must preserve the last-known-good saved state. The [mobile Graphics contract](superpowers/plans/2026-09-11-beyond-the-tomb-1.7.0.md#103a-simpler-phone-graphics-with-expandable-help) covers orientations, readability, touch/scroll behavior and regression checks for settings persistence and recovery.

These are 1.7 planning requirements after the accepted 1.6.2 baseline. They do not expand the active 1.6.2 runtime/PR18, demonstrate implementation or authorise a merge or release.

## 1.7 addition — low-power mobile pause

**Owner goal, 4 October 2026:** Reduce unnecessary work and phone heat/battery drain while the game is paused. Read-only PR18 inspection confirms the foreground paused loop still renders before its preview-frame-cap delay; it does not prove the cause or size of the reported heat/drain. Android background suspension already has a separate surface-stop path. Re-audit accepted 1.6.2 before implementation.

The [canonical low-power pause contract](superpowers/plans/2026-09-11-beyond-the-tomb-1.7.0.md#103b-low-power-mobile-pause) requires an evidence-led, bounded solution: prefer reusing the frozen scene with responsive UI-only/on-demand updates, or a safe measured low-rate fallback; suppress unchanged simulation/skinning/AS/RT work and avoid busy waiting. Keep live Graphics preview explicit and separate. Preserve cache invalidation, safe settings apply/acknowledgment/revert, input/audio resume, background/screen-lock suspension and Vulkan resource/swapchain ownership. No active game frames while suspended, apart from finite in-flight retirement.

Acceptance compares matched active play and old/new pause using CPU/GPU/work/submission evidence and longer controlled battery/power/thermal observations with honest limits; a lower FPS counter alone proves no energy saving. No fixed savings claim, screenshot-compositor mandate or renderer rewrite is implied. This adds only 1.7 planning/WP7 acceptance after the existing start gate; it does not expand active 1.6.2, change runtime or authorise a build, merge or release.

## 1.7 addition — Save/Load and tomb-exit respawn

**Owner request, 3 October 2026:** Add player-facing Save/Load, with **three local campaign slots proposed** (count remains provisional), and an automatic safe respawn checkpoint after the completed rope rescue/tomb exit. Activate it only when grounded outside with the destination ready; later death must not replay the tomb or re-award the lantern. Keep manual slot saves distinct from automatic recovery checkpoints, preserve coherent quest/Kit/equipment/one-shot state, and keep settings and RT Lab unlock separate. The [canonical 1.7 save contract](superpowers/plans/2026-09-11-beyond-the-tomb-1.7.0.md#12-save-replay-and-recovery) defines safe resume, overwrite/New Game confirmation, versioned atomic storage and corruption/lifecycle checks. Final arbitrary-save policy and cloud sync are not implied. Planning only, after the accepted 1.6.2 baseline.

## Future visual backlog — carried-torch ember detachment

**Owner observation, 3 October 2026:** While walking with the torch, the embers appear to move with it instead of rising independently and leaving a short trail. This is an owner-reported visual concern, not a reproduced defect or a confirmed root cause. The observed build, device, backend and effective settings were not specified; do not attribute it to a particular release or inherit metadata from unrelated reports.

**Scope:** retain as a bounded 1.7/backlog investigation after acceptance of 1.6.2. It adds no required work or acceptance blocker to the frozen current 1.6.2 scope and must not interrupt or steer the active implementation run.

**Investigation and intended result**
- Establish the exact candidate and reproduce standing, walking, turning and stopping with a moving held torch. Inspect whether the current effect is procedural noise repeatedly anchored to the current emitter transform, persistent particles, or another representation; do not assume an emitter-local implementation merely from the report.
- Each admitted ember should retain its emission-time world position and birth time (or an equivalent reconstructible birth state). After birth, its trajectory must evolve independently of the torch's current transform, so old embers do not translate or rotate rigidly with later torch movement.
- Use bounded lifetime, upward drift/rise and fade. Optional inherited emission-time torch velocity may improve motion but must remain bounded and must not reattach the particle to the emitter. This specifies appearance and ownership, not a mandatory particle architecture or full fluid simulation.
- Preserve coherent world-space visibility, occlusion and relevant reflected/transmitted RT views across supported rendering paths; no camera-only overlay or unsupported reflection claim. Reuse shared effect contracts and retain existing gameplay light behaviour.
- Bound active count, spawning, storage and per-frame update/render/acceleration-structure cost as applicable. Measure phone and Windows costs on identified hardware and settings before admission; no unmeasured performance promise or reduction of unrelated quality to hide the cost.

**Acceptance evidence for any later implementation:** matched motion captures show old embers rising and fading independently while the emitter moves, without rigid dragging, trails across teleports or stale particles after reset/zone unload. Verify appropriate pause/resume and lifecycle handling, supported quality settings and affected secondary views. Record exact build/device/backend/settings, changed source, actual checks, measured budgets and remaining gaps. Decide implement/defer from that evidence; no runtime work, merge or release is authorised by this entry.

## 1.7 foundation and later campaign — hearts, health growth and recovery

**Owner direction, 3 October 2026:** Use original stylized heart icons for player health instead of the ordinary `3/3` display, with accessible current/max text. No floating heart pickups. Preserve and extend the existing authoritative vitality/death system. Consider modest capacity growth through selected boss milestones, discoveries and mixable Constitution/Tech/Magic improvements; Constitution favors health/recovery, Tech flask equipment and Magic restoration/protection.

The recovery direction is a limited healing flask, food/rest in safe places and optional healing abilities. Amounts, capacity, refill/use rules and ability costs remain provisional; do not infer a stamina/mana system, automatic regeneration or mandatory farming. The 1.7 slice owns heart presentation and a scoped health/recovery/save foundation; full campaign upgrades and later rewards are not all 1.7 gates. See [the staged health and recovery contract](superpowers/plans/2026-09-11-beyond-the-tomb-1.7.0.md#105-heart-health-display-and-grounded-recovery-direction). Validate persistent upgrades, consumption/refill/retry integrity and a viable mandatory route without optional health rewards. This does not expand frozen/current 1.6.2 work.

## 1.7 — Prologue combat teaching and deliberate opening room

**Owner direction, 6 October 2026:** after the parry foundation feels right, tune the existing tomb skeletons to survive long enough for useful parry teaching without becoming damage sponges. Prototype the lich as three successful player hits separated by a small cooldown: a successful nonfatal hit triggers bounded collision-safe pushback, followed by clearly telegraphed lightning the player can dodge before returning close. Exact health/timing/displacement values remain prototypes; pulse damage was not specified, with knockback-only the initial proposal. Death on the third hit overrides queued attacks and preserves the existing death-animation, chest/reward and wall-torch-off ordering.

The [1.7 master plan](superpowers/plans/2026-09-11-beyond-the-tomb-1.7.0.md#prologue-combat-teaching--owner-direction-6-october-2026) owns encounter and acceptance details. This does not alter current 1.6.2 parry fixes or the accepted relocation of both guards into the waterfall room. The same 1.7 pass removes or recontextualizes the opening-room tech-demo material collection for a deliberate story, guidance or action purpose; shared renderer/material systems and separate validation fixtures remain intact.

## The hub-and-dungeon loop

Proposed campaign structure:

`Tomb prologue → rescue and forest → village/tavern → dungeon expedition → return to hub with a piece and new knowledge → next expedition`

**Owner-approved scale, 6 October 2026:** a compact access quest → approach → themed dungeon loop, inspired structurally by Ocarina of Time at a smaller scale. NPC gear/training can support that preparation; specific new rewards, effects and quest scripts remain proposals.

The tavern should give the player a reason to care about the wider Horde, introduce useful people and leads, and provide a natural place to interpret discoveries after each return. Reveal the mystery progressively rather than delivering all the lore on arrival. Returning NPC dialogue should acknowledge relevant progress without replaying one-time scenes or rewards.

The hub must now provide practical story and player benefits: bounded gear/training services, treasure currency and area-access preparation across Magic, Tech and Constitution. A conversation and checkpoint alone no longer satisfy the approved hub direction. Re-scope 1.8 against the accepted 1.7 baseline; full economy simulation, crafting and XP are not automatically required. An exit or objective must not suggest an unavailable dungeon is already playable.

## Common design brief for each later dungeon

Each of the three should have its own environmental identity, enemies/encounters, a learnable light-based puzzle language, a boss and a meaningful campaign reward. Distinct themes need not mean unrelated bespoke engines or three entirely separate combat systems.

**Owner-approved, 6 October 2026:** each dungeon has a distinct theme and local item actually required for boss victory: Abbey reflector/shutters/Bellkeeper armour; Foundry shuttered lantern stand/held mechanism/furnace shell; Court focusing aperture/isolated receivers/bounded mirror wards. Use the Zelda-like structure with original items, spaces, characters and solutions. The required learning loop is:

1. Introduce a light-related rule in a safe, readable setting.
2. Acquire or activate the approved local item/capability early in the dungeon, then provide safe practice.
3. Teach its use, then combine it with navigation, enemies and increasingly demanding puzzles.
4. Require actual application of the learned local-item rule to defeat the boss; ownership alone is insufficient. Do not introduce an unexplained mandatory mechanic only in the boss room. Court defeat need not mean killing its keeper.
5. Award one persistent treasury seal and return the player safely to the hub with new information.

A dungeon utility item and its end-of-dungeon campaign piece are separate design roles; the seals unlock campaign progress while useful tools retain gameplay roles. Do not assume the existing reward lantern must be discarded or replaced. Persistent light tools should remain useful where appropriate rather than becoming disposable one-room keys.

The themes, local items and required boss relationships are approved in CAMPAIGN_DESIGN.md and the [chapter plan](DUNGEON_CHAPTER_PLANS.md). Detailed encounter rules, recoverable placement/recall, content footprints and measured device budgets remain to be proven per dungeon. Main-route retreat stays viable before the seal; post-seal shortcuts never supply the only safe exit. Preserve independent Magic/Tech/Constitution access, Abbey/Foundry either-order play and the Court's both-seal gate.

## Ray-traced light as a gameplay pillar

A light mechanic should change what the player can discover, open, traverse, protect, expose or defeat. Its outcome should follow world-space source placement, direction, occlusion and the supported optical interaction, so moving the lantern or an intervening object matters.

General exploration candidates, **not additional promised features**, include directing a lantern through a shutter, using real shadows to conceal or expose something, redirecting light with a reflector, or using a bounded transmission/refraction interaction to reach a receiver. Choose a small set through playable prototypes and phone measurements. This is not permission to resurrect previously rejected visual treatments, require full spectral simulation, or promise unbounded reflections or caustics.

Required design safeguards for future light mechanics:

- The visible RT world and the gameplay optical model must agree on puzzle-critical sources, surfaces, transforms and blockers. A hidden proximity trigger or camera-aim check must not pretend that a beam actually reached a target.
- Puzzle, enemy and boss state stays in the shared fixed-step gameplay authority. Do not base success on exposure-dependent screen brightness, temporal noise, screenshots, frame rate or an unbounded synchronous GPU readback. Select and test the query/result contract when the mechanic is specified.
- Feedback must remain readable at supported quality levels and internal resolutions. Render scaling or exposure changes must not alter a puzzle's solution. Use shape, movement, sound, text or state feedback as appropriate rather than colour alone.
- Missed moves, dropped/stowed tools, retry, pause and save restoration must not create an unrecoverable puzzle. Bound ray paths and interaction counts explicitly and report the actual phone cost.

These are engine-system requirements, not an instruction to implement the future optical puzzle framework during 1.7 or 1.8.

## Optimisation experiment bank and pre-release tests

**Owner update, 4 October 2026:** Keep the [mobile performance audit and experiment bank](MOBILE_PERFORMANCE_AUDIT_2026-10-04.md) as a dated source/evidence reference. It separates verified mechanisms and recorded measurements from hypotheses, visual trade-offs and unmeasured priority rankings.

**Correction, 4 October 2026:** The initial audit review overlooked the [existing 1 October selective-opacity NO-GO](https://github.com/Samfa12-tech/The-Horde-RT-demo/blob/b2dc216a500e690b44ebac20ea05f7bb318349a4/docs/ENGINEERING_1_6_1_PRIMARY_OPACITY_2026-10-01.md#L1-L26), already present in the reviewed source. The [current second-pass decision](https://github.com/Samfa12-tech/The-Horde-RT-demo/blob/07429262215919d71eec7f434b41080a759b8c54/docs/ENGINEERING_1_6_2_SECOND_PASS.md#L113-L121) confirms the same mechanism and restores the control. Retain that rejection; do not automatically requeue this candidate without a materially different mechanism or new contrary matched evidence and review.

The remaining owner-authorised pre-release experiments are an actual eligible opaque-bounce skip preserving important RT reflections, and a bounded SPIR-V-verified texture-footprint/LOD experiment. Keep their measurements and visual checks separate, with an individual keep/reject decision and benefits explicitly unknown. They remain experiments, not guaranteed shipping features or promised savings; current sequencing and any pause/device limits still apply.

The separately queued second pass for independent shadows, lower fire, upscaling, optional benchmark Send statistics and mist density/lighting retains its own implementation/measurement plan. The rest of the audit remains future proposals; preserving it does not commit every priority to 1.6.2, change defaults or authorise a renderer rewrite, credentials, deployment, merge or release. Retain the final independent review and explicit owner release gate.

## Continuing performance work after 1.6.2

**Owner direction, 5 October 2026:** Performance investigation and optimisation must continue beyond 1.6.2. The [future performance investigation plan](PERFORMANCE_INVESTIGATION_FUTURE.md) owns the bounded historical-regression, texture-fetch/material-classification, mip/footprint, actual-phone scaling/upscaling and contextual light-cost hypotheses. It includes later measured Low mist and firefly-light budgets, preserves closed negative experiments, and separates quality/build checks from sustained performance acceptance.

Version and date are unassigned. Finish the current 1.6.2 scope and mobile-defaults review first; this adds no release gate or permission for another run now. The dated audit remains historical evidence, while completed current experiments must be reconciled before selecting one later hypothesis. No automatic 33% default, renderer rewrite, merge or release follows.

## Maybe one day — optional Windows path tracing

**Owner direction, 4 October 2026:** Keep an optional path-tracing mode as a later Windows/high-end hardware-RT feasibility experiment. This is an uncommitted backlog idea, with no version or date promise and no 1.6.2 or 1.7 acceptance requirement. Android remains first-class with its existing default path; retain the current hardware ray tracer as the supported baseline and fallback when path tracing is unavailable, too expensive or visually unsuitable. Unsupported RT hardware still receives clear diagnostics, not a raster fallback.

The existing Vulkan BLAS/TLAS, PBR materials and skinning support provide useful foundations to reassess against the eventual accepted baseline. They do not make the current deterministic, effect-specific renderer with limited approximate bounce lighting a general path tracer. A later experiment must define and validate a stochastic light-transport integrator rather than relabel existing effects.

**Staged feasibility gates**
1. Start with a small static test scene and stationary camera, using progressive accumulation to establish sampling, convergence and reference-image correctness with explicit sample/bounce budgets.
2. Only if useful, test moving cameras and animated/skinned scenes. Define sampling and light selection, temporal history/reprojection, disocclusion and stale-history rejection, reset/invalidation rules and denoising. Inspect noise, ghosting, lag and lost detail in motion as well as still-image quality.
3. Consider adoption only if matched comparisons justify the quality/performance trade-off. Record exact build, GPU, driver, scene, output/internal resolution, settings, frame-time/pacing and memory costs, including skinning/acceleration-structure work and sustained behaviour. Keep a clear opt-in and the accepted ray-traced fallback; defer or reject if the evidence is insufficient.

**Explicit transport scope:** Before implementation, state which material models and lobes are supported, how emissive surfaces contribute and are sampled, and how alpha-cutout surfaces, transparent/refraction paths and volumetric fire/smoke/fog participate. Identify any retained approximations, exclusions, bounded path depths and unsupported combinations. Preserve coherent primary/secondary visibility and gameplay-critical lighting cues; path tracing must not change authoritative puzzle outcomes. This is not a promise of full physical simulation, unrestricted multiple scattering, spectral transport or every possible caustic.

Admission must use measured capability and performance rather than vendor branding or a vendor lock-in requirement. The owner's RTX 5050 Laptop GPU with 8 GB is a possible evaluation target, not a certified path-tracing preset: suitable settings, memory headroom and FPS are unknown until tested. This entry authorises planning only, not runtime work, paid dependencies, a build, merge or release.

## Maybe one day — optional PC VR, very last

**Owner direction, 5 October 2026:** Put a possible PC VR version at the very end of the backlog, **only after the game is finished**. This is the lowest-priority, uncommitted “maybe one day” idea, with no version/date or release commitment and no addition to current 1.6.2, 1.7 or campaign-completion scope.

The owner has a **PSVR2 headset** as a potential PC test device. Verify its PC compatibility, adapter, runtime and hardware prerequisites when this is explored; ownership alone does not establish a ready or compatible test setup. This is a PC VR possibility, not a PS5 port commitment.

Any later feasibility review should cover comfortable movement, stereo RT performance, headset/controller input and readable VR UI against the finished game's actual baseline. Existing rendering and gameplay do not make VR conversion automatic or cost-free. This entry records planning only and starts no implementation, asset work, purchase, build, merge or release.

## Engine growth and scope discipline

Retain native Vulkan hardware RT, Android as a first-class target, Windows RTX validation, the shared 60 Hz simulation and honest presentation evidence. Prefer reusable zone ownership, actors, conversations, progression flags, interactions and light mechanics over hardcoded exceptions for individual houses or bosses.

Do not build a general open world, new engine, giant ECS, economy or universal quest framework merely to prepare for later content. The 1.7 foundations are planned dependencies, not certified capabilities until implemented and tested. Any existing enemy/actor/resource ceiling requires an explicit measured expansion before more simultaneous characters are added.

Each milestone must remain independently playable and accepted. Preserve accepted earlier routes, saves and controls; investigate matched regressions instead of hiding them with lower resolution, weaker lighting or cooled-only timing. No hardware claim or numerical population budget in a design document substitutes for exact-candidate evidence.

## Reuse suitable Briarhold assets

Across the project, assess existing Briarhold 3D models and sky assets before commissioning or generating replacements. Reuse suitable assets where it saves work and fits Horde's historical-gothic direction; availability alone does not establish suitability or permission. Verify each asset's provenance and reuse rights, retain required attribution, and record imported sources/derivatives in Horde's asset-licence records. Check scale, axes, geometry, materials/textures, rig/animation compatibility where relevant, native RT import, collision and measured phone memory/render cost. Preserve source/runtime separation and hashes. Adapt through Horde's existing asset pipeline rather than copying Briarhold's browser renderer or assuming its raster performance transfers. New generation remains an option when existing assets cannot meet the brief.

## Decisions intentionally left open

Detailed street/building blockout (regional geography and the names The Veyrlands/Bellwether are approved in WORLD_LAYOUT.md), and supporting character names (avoid Mara); detailed tool rules and encounters (the three dungeon-item/boss relationships are now approved); voice casting/scripts; entity name/form; detailed finale and treasure distribution; XP/respec/prices and upgrade balance; exact content counts and sustained device budgets. The campaign's approved narrative and order are in CAMPAIGN_DESIGN.md, not open for silent reinvention.

## Documentation-only change boundary

This update preserves the owner's 11 September campaign direction and records the 30 September 1.6.2 sequencing, demo-polish and asset-reuse additions the 1 October material-depth foundation, and the 2 October grouped post-release audit, graphics-menu and measured reduced-effects plan. It implements no gameplay, changes no package/release identity, generates no assets, authorises no paid work and publishes no build.

**Audio/haptic manual revalidation required: NO — documentation only; runtime and semantic inputs are unchanged.**


## Visual-reference custody — 5 October 2026

The [design reference archive](design/README.md) keeps the selected UI/HUD direction, historical concepts and connected forest/Bellwether map proposals beside the plans. Use their status captions: the loading art's bar is superseded by a small spinner only; map dimensions and vegetation marks remain blockout proposals. The archive adds no runtime implementation, release, paid asset work or new milestone scope.

## Earned story recap and Bellwether life — 7 October 2026

The owner-approved additions are developed in [The Tale Thus Far](TALE_THUS_FAR.md) and [Bellwether village life](BELLWETHER_VILLAGE_LIFE.md). The former proposes milestone/knowledge-earned verse, an optional recap and short loading excerpts without extra loading or forced reading; the latter proposes bounded village humour, animals, kitchen plots and practical food/water/waste details. Exact writing, positions, counts and production packages remain drafts. Scope at the existing acceptance gates: 1.7 forest/lookout only; 1.8 Bellwether; later chapters in the existing provisional sequence, with Abbey/Foundry either-order play. No current engineering, paid production or release expansion.
