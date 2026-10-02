# Horde Lantern RT — Campaign and Engine Roadmap

**Owner:** Sam Small / Samfa12  
**Planning update:** 2 October 2026  
**Status:** Owner-approved direction; future milestones are provisional, not implemented or release promises.

## Authority and navigation

This is the canonical forward-looking campaign roadmap. Use it alongside [AGENTS.md](../AGENTS.md) and [PROJECT_DECISIONS.md](../PROJECT_DECISIONS.md), which retain the engineering, safety and validated-baseline rules. The [Phase Plan](PHASE_PLAN.md) preserves the historical implementation sequence; dated audits and validation reports remain evidence of their own snapshots. This roadmap does not rewrite that history or certify newer runtime behaviour.

The [README](../README.md) identifies the published package and its evidence. A future version label here does not change the application version, prove a feature exists, authorise publication or supersede an unfinished engineering pass.

## Direction approved by the owner

The dungeon becomes the prologue to a small, authored historical-gothic adventure. The player and companion emerge into woodland, reach an inhabited village, and learn more in its tavern about the wider **Horde**. The village becomes the recurring hub for three subsequent themed dungeons.

The owner-approved [campaign, characters and progression direction](CAMPAIGN_DESIGN.md) defines the lost army/treasure-hoard ambiguity, cursed keepers, deceptive lantern voice, three dungeons and single ending. Preserve The Horde as the working title. Kit accompanies a silent player; the eventual narrative is fully voiced. Remaining names, detailed scripts and balance stay provisional.

The defining design pillar is **light as gameplay, powered by the actual ray-tracing engine**, not ray tracing as decoration added after conventional rooms and combat.

## Milestone sequence

| Milestone | Direction | Status / start condition |
|---|---|---|
| 1.6.1 engineering baseline | Finish the current engineering programme and its validation. | Complete, accept, merge and release 1.6.1 before starting 1.6.2 implementation. Preserve the active engineering pass. |
| 1.6.2 — Engine readiness and demo polish | Full post-release audit; clear graphics settings and measured reduced-effects options; temporal/upscaling readiness, material foundations, hands-first themed UI and dungeon polish. | Planned after completed, accepted, merged and released 1.6.1 and before 1.7 gameplay expansion; sequence and acceptance below. |
| 1.7.0 — Beyond the Tomb | Existing dungeon becomes prologue; physical rope rescue, companion, moonlit woodland, dialogue, world-zone ownership, checkpoints and chapter-specific UI using the accepted 1.6.2 theme. | Scoped planning handoff exists. Start after 1.6.2 is implemented, validated and accepted; then complete and obtain owner acceptance of the agreed 1.7 scope. |
| 1.8.0 — Village Hub | Continue from the forest to a small village with a few explorable interiors, a hero tavern and a small NPC cast. Reveal more of the wider Horde and establish the hub. | Pencilled in. Begin only after 1.7 is made, tested and accepted; use its actual performance and system evidence to finalise scope. |
| Beyond 1.8 — three themed dungeons | Three distinct adventure dungeons, each with enemies, puzzles, a boss and one campaign piece; consider a dungeon-specific item that enables puzzle solving and boss defeat. | Big-picture direction only. Build and validate one complete dungeon before expanding to the next. No version numbers or release dates are assigned. |
| Beyond the trio | Treasury, release of the bound dead, confrontation with the deceptive lantern entity and one hopeful epilogue. | Creative direction approved in CAMPAIGN_DESIGN.md; compact finale, not a promised fourth full dungeon. Implementation scope remains gated. |

Detailed plans:

- [1.7.0 — Beyond the Tomb](superpowers/plans/2026-09-11-beyond-the-tomb-1.7.0.md).
- [1.8.0 — Village Hub: provisional plan](superpowers/plans/2026-09-11-village-hub-1.8.0.md).

The three later dungeons are additional adventures after the existing tomb/prologue, not a silent relabelling of that prologue as one of the three. Approved order: Abbey and Foundry in either order, then Glass Court, then the treasury finale; see CAMPAIGN_DESIGN.md for access quests and remaining detailed design.

## 1.6.2 — Engine readiness and demo polish

Finish, accept, merge and release **1.6.1 first**, then undertake this bounded preparation/polish milestone before **1.7 gameplay expansion**. These are future tasks, not new 1.6.1 acceptance blockers. Re-audit the accepted release to avoid duplicating work completed in the engineering pass. Its current programme includes music and RayQuery compatibility; the older post-1.6.1 ordering in historical planning must not reopen or resequence that work.

### Workstream order and dependencies

This section is the grouped 1.6.2 update bank. Priorities establish dependencies, not a promise that every investigation becomes a shipping feature.

| Order | Workstream | Dependency and completion decision |
|---|---|---|
| P0 | Full post-release 1.6.1 audit | Begin after the agreed goal is complete and the accepted candidate is merged and released. Reconcile delivered work and remaining gaps before finalising implementation scope. |
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

- **Fire, especially the reward lantern:** investigate the owner's report that the flame reads as a static coloured tapered spindle. Inspect it in motion before attributing a cause; improve shape, animation and readability through the shared world-space fire system while keeping emissive geometry, volume, direct light, glass and reflections coherent. Check held, raised, swinging and stationary views on phone and Windows.
- **Visible exterior sky/skybox:** give existing openings a convincing visible environment through the native RT path. Evaluate Briarhold's sky assets before creating replacements, checking provenance, projection, seams, colour space and suitability for Horde's existing lighting/exposure. A direction-sampled miss environment is valid; preserve physical roof/geometry occlusion and consistent relevant reflected/transmitted views. Do not add a fake background quad or raster fallback. Preserve the existing finale's accepted timing and light progression; 1.7 owns its deliberate moonlit rescue change.
- **Collapsed entrance behind spawn:** replace the plain-wall reading with a visibly collapsed former passage: grounded boulders, matching broken masonry, damaged opening and sealed backing. Follow the [asset shortlist, Blender preparation and ready-to-use Codex handoff](COLLAPSED_ENTRY_1_6_2.md). Preserve spawn, gallery, forward route and torch clearance; begin with verified free CC0 sources rather than paid generation.
- **Entryway/skeleton-area props:** identify the flat-looking objects from the actual scene/captures, then improve or replace their geometry/materials as appropriate. Do not guess their identity or assume a particular asset is the cause. Retain collision, route readability and measured RT budgets.
- **Held torch/light coherence and sword idle motion:** the owner reports the torch bobbing at rest while light/shadows appear not to follow, and requests more natural sword idle breathing. This is not runtime-reproduced here. Source inspection at [9931cf18](https://github.com/Samfa12-tech/The-Horde-RT-demo/tree/9931cf18fc36369ca6efbe2b785d15f877bf6d31) already propagates a small idle sway into the simulation emitter: [HeldItemKinematics](https://github.com/Samfa12-tech/The-Horde-RT-demo/blob/9931cf18fc36369ca6efbe2b785d15f877bf6d31/src/gameplay/items/HeldItemKinematics.cpp) composes torch/flame/light sockets together, while the renderer has a later final-grip item recomposition and a distinct emitter upload. Reconcile actual source paths and final visible torch/flame/light socket agreement in the released baseline rather than assuming absent light animation. Reproduce the exact live build through idle, movement, look, clearance, drop and reward-lantern transitions. Keep flame and light attached to the same stable authored source; preserve genuine moving RT shadows, with no independent cosmetic light oscillation. Evaluate subtle sword breathing through the shared hand/item pose, blending cleanly into attack/parry without changing hit tests, timing, grip/IK or clearance. Do not add camera bob; a bounded strength/speed/off comfort option is only a candidate if useful and must keep prop/emitter motion coherent. Validate matched PC/phone motion with steady/moving camera, shadow contact, pause/resume and no phase pops, flicker or drift; record cost. The source inspection is not evidence that a visible desynchronisation has been reproduced.
- **Held torch versus low roofs:** reproduce the reported clipping and investigate both visible equipment clearance and physical geometry/collision. Use the shared held-item/retraction contract, including overhead clearance; verify torch, flame, light, hands and shadows agree through movement and look angles rather than hiding the problem with an overlay.

- **Waterfall loop seam:** the owner reports a slight pause between the end and restart of the waterfall sample; this is reported, not reproduced in this planning task. Audit source sample edges, encoded padding/loop metadata, decoder/playback queue and scheduling/underrun behaviour before attributing a cause. Use trimming or a short waveform-matched loop crossfade only when appropriate, preserving sustained level, spatial position/attenuation and the intended waterfall character. Verify many consecutive loops without an audible gap, click or pop on Android and Windows during normal play, representative load and lifecycle return. Record source/runtime asset identity and affected playback evidence; manual audio validation is required for a changed candidate. This is a bounded 1.6.2 ambience fix, not reopening the accepted music programme.

- **Skeletons and lich:** address the reported identical-looking/synchronised skeletons, add or complete skeleton sounds, and give the lich a grander keeper-consistent reveal, animation and sound pass. Follow the [explicit Fourth Keeper reveal brief](LICH_REVEAL_1_6_2.md) for room staging, the six-second awakening, rig-safe animation, sound mapping, retry and combat/reward handoff; this replaces the vague reveal request. See also [enemy polish requirements](CAMPAIGN_DESIGN.md#existing-demo-polish-and-milestone-ownership). Preserve authoritative combat timing and lantern-after-lich reward order; measure independent-pose costs and manually validate changed audio.

This milestone prepares reusable foundations and polishes the existing demo. It does not deliver an outdoor level, rope climb, companion, forest route or the complete moonlit sky/atmosphere programme; those remain in the scoped 1.7 plan. Accept each slice with affected tests, real RT captures/motion review and device measurements, recording any deferred adapter or unresolved quality/performance limit honestly.

### Integrated 1.6.2 acceptance

Review the combined candidate, not only successful isolated slices. Recheck affected host/shader/Android tests, actual RT presentation, settings combinations, lifecycle and representative gameplay routes on exact supported devices and the Windows RTX target. Retain matched fixed-baseline evidence alongside labelled reduced-effects or temporal modes. Audio/haptic changes require applicable manual checks; unrelated graphics/documentation work does not automatically reopen accepted audio.

The final record states what shipped, what was accepted or deferred, actual quality/performance limits and remaining validation gaps. Optional vendor adapters, detail normals and height experiments are not mandatory merely because they appear in this bank. No roadmap entry authorises signing, publication or moving to 1.7 before milestone acceptance.

## The hub-and-dungeon loop

Proposed campaign structure:

`Tomb prologue → rescue and forest → village/tavern → dungeon expedition → return to hub with a piece and new knowledge → next expedition`

The tavern should give the player a reason to care about the wider Horde, introduce useful people and leads, and provide a natural place to interpret discoveries after each return. Reveal the mystery progressively rather than delivering all the lore on arrival. Returning NPC dialogue should acknowledge relevant progress without replaying one-time scenes or rewards.

The hub must now provide practical story and player benefits: bounded gear/training services, treasure currency and area-access preparation across Magic, Tech and Constitution. A conversation and checkpoint alone no longer satisfy the approved hub direction. Re-scope 1.8 against the accepted 1.7 baseline; full economy simulation, crafting and XP are not automatically required. An exit or objective must not suggest an unavailable dungeon is already playable.

## Common design brief for each later dungeon

Each of the three should have its own environmental identity, enemies/encounters, a learnable light-based puzzle language, a boss and a meaningful campaign reward. Distinct themes need not mean unrelated bespoke engines or three entirely separate combat systems.

The owner suggested a Zelda-like item/puzzle/boss relationship. Use that structural inspiration while creating original items, spaces, characters and solutions. The recommended loop is:

1. Introduce a light-related rule in a safe, readable setting.
2. Acquire or activate a useful item or lantern capability inside the dungeon.
3. Teach its use, then combine it with navigation, enemies and increasingly demanding puzzles.
4. Let the player recognise and apply the learned rule during the boss encounter; do not introduce an unexplained mandatory mechanic only in the boss room.
5. Award one persistent treasury seal and return the player safely to the hub with new information.

A dungeon utility item and its end-of-dungeon campaign piece are separate design roles; the seals unlock campaign progress while useful tools retain gameplay roles. Do not assume the existing reward lantern must be discarded or replaced. Persistent light tools should remain useful where appropriate rather than becoming disposable one-room keys.

Before committing a dungeon, approve its theme, light verb, item/capability, encounter and boss relationship, reward role, recoverable puzzle states, content footprint and measured device budget. The creative direction is recorded in CAMPAIGN_DESIGN.md; exact rules and measured content budgets remain to be locked per dungeon.

## Ray-traced light as a gameplay pillar

A light mechanic should change what the player can discover, open, traverse, protect, expose or defeat. Its outcome should follow world-space source placement, direction, occlusion and the supported optical interaction, so moving the lantern or an intervening object matters.

Exploration candidates, **not selected dungeon themes or promised features**, include directing a lantern through a shutter, using real shadows to conceal or expose something, redirecting light with a reflector, or using a bounded transmission/refraction interaction to reach a receiver. Choose a small set through playable prototypes and phone measurements. This is not permission to resurrect previously rejected visual treatments, require full spectral simulation, or promise unbounded reflections or caustics.

Required design safeguards for future light mechanics:

- The visible RT world and the gameplay optical model must agree on puzzle-critical sources, surfaces, transforms and blockers. A hidden proximity trigger or camera-aim check must not pretend that a beam actually reached a target.
- Puzzle, enemy and boss state stays in the shared fixed-step gameplay authority. Do not base success on exposure-dependent screen brightness, temporal noise, screenshots, frame rate or an unbounded synchronous GPU readback. Select and test the query/result contract when the mechanic is specified.
- Feedback must remain readable at supported quality levels and internal resolutions. Render scaling or exposure changes must not alter a puzzle's solution. Use shape, movement, sound, text or state feedback as appropriate rather than colour alone.
- Missed moves, dropped/stowed tools, retry, pause and save restoration must not create an unrecoverable puzzle. Bound ray paths and interaction counts explicitly and report the actual phone cost.

These are engine-system requirements, not an instruction to implement the future optical puzzle framework during 1.7 or 1.8.

## Engine growth and scope discipline

Retain native Vulkan hardware RT, Android as a first-class target, Windows RTX validation, the shared 60 Hz simulation and honest presentation evidence. Prefer reusable zone ownership, actors, conversations, progression flags, interactions and light mechanics over hardcoded exceptions for individual houses or bosses.

Do not build a general open world, new engine, giant ECS, economy or universal quest framework merely to prepare for later content. The 1.7 foundations are planned dependencies, not certified capabilities until implemented and tested. Any existing enemy/actor/resource ceiling requires an explicit measured expansion before more simultaneous characters are added.

Each milestone must remain independently playable and accepted. Preserve accepted earlier routes, saves and controls; investigate matched regressions instead of hiding them with lower resolution, weaker lighting or cooled-only timing. No hardware claim or numerical population budget in a design document substitutes for exact-candidate evidence.

## Reuse suitable Briarhold assets

Across the project, assess existing Briarhold 3D models and sky assets before commissioning or generating replacements. Reuse suitable assets where it saves work and fits Horde's historical-gothic direction; availability alone does not establish suitability or permission. Verify each asset's provenance and reuse rights, retain required attribution, and record imported sources/derivatives in Horde's asset-licence records. Check scale, axes, geometry, materials/textures, rig/animation compatibility where relevant, native RT import, collision and measured phone memory/render cost. Preserve source/runtime separation and hashes. Adapt through Horde's existing asset pipeline rather than copying Briarhold's browser renderer or assuming its raster performance transfers. New generation remains an option when existing assets cannot meet the brief.

## Decisions intentionally left open

Village final name/layout and supporting character names (avoid Mara); precise tools and encounter rules; voice casting/scripts; entity name/form; detailed finale and treasure distribution; XP/respec/prices and upgrade balance; exact content counts and sustained device budgets. The campaign's approved narrative and order are in CAMPAIGN_DESIGN.md, not open for silent reinvention.

## Documentation-only change boundary

This update preserves the owner's 11 September campaign direction and records the 30 September 1.6.2 sequencing, demo-polish and asset-reuse additions the 1 October material-depth foundation, and the 2 October grouped post-release audit, graphics-menu and measured reduced-effects plan. It implements no gameplay, changes no package/release identity, generates no assets, authorises no paid work and publishes no build.

**Audio/haptic manual revalidation required: NO — documentation only; runtime and semantic inputs are unchanged.**
