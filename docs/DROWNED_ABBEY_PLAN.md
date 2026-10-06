# Drowned Abbey — Flooded Basin and Staged Delivery Plan

**Planning update: 6 October 2026.** Future campaign chapter beyond the village milestone; proposed 1.9 delivery package in the [chapter plan](DUNGEON_CHAPTER_PLANS.md), with provisional numbering and no release date. This records the approved flooded-lake direction and a proposed technical approach. It does not add work to 1.6.2, make the Abbey a 1.7/1.8 deliverable, or authorise implementation, asset generation, spending, merging or release.

Read [CAMPAIGN_DESIGN.md](CAMPAIGN_DESIGN.md#drowned-abbey) for creative authority, [WORLD_LAYOUT.md](WORLD_LAYOUT.md) for geography, and [ROADMAP.md](ROADMAP.md#common-design-brief-for-each-later-dungeon) for shared dungeon/light-gameplay rules. Keep this document as the detailed Abbey delivery plan rather than duplicating its implementation stages elsewhere.

## 1. Approved setting and intended experience

- The Abbey occupies a **river-fed lake in the southern flooded basin**, downstream from Bellwether and the mill. It is not an ocean, sea coast or tidal dungeon.
- The religious buildings predate the flooding. The cause, date and extent of that historical flood remain authoring decisions; do not invent a dam failure, deliberate inundation or supernatural event as canon.
- Reveal the lake and the Abbey's broken towers from the approach before descending to the water. The player should understand the destination and submerged entry problem before committing to the crossing.
- Keep the dungeon **predominantly dry, walkable exploration, puzzles and combat**. Water surrounds it and is visible through appropriate windows/openings, with seepage, waterlines and selected flooded side rooms providing atmosphere.
- Use a short submerged approach to make access preparation meaningful. A **guided crossing with minimal steering and retained look control** is the first prototype candidate, not a locked movement system or an uninterruptible camera sequence.
- Preserve bells beneath water, dead attendants at their duties, the approved placeable reflector and drowned armoured Bellkeeper relationship. Acquire the reflector early, teach safe redirection through shutters, escalate that rule through rooms and require actual reflected-light use to expose the boss's armour. Exact placement, timing, encounters and staging remain to be designed.
- Abbey and Foundry remain playable in either order. Abbey access and completion must not require the Foundry tool, seal or a Foundry-only upgrade; any cross-dungeon benefit stays optional.

This scope does not automatically include free swimming across the lake, boats, sustained underwater combat, water-level puzzles, fluid simulation or a fully explorable lakebed.

## 2. Spatial proof before detailed art

Produce a plan and a vertical section together. Mark a shared lake datum, terrain elevations, the descent, entry/exit mouths, crossing depth, air pockets, dry floors, flooded rooms and vista sightlines. Choose actual dimensions through blockout rather than deriving scale from the regional concept image.

Start with a small, legible chain: **overlook → descent/preparation point → submerged crossing → safe arrival → one dry room**. Broken tower silhouettes establish the same destination at each stage; bounded terrain and ruins explain the playable limits without suggesting an open-water sandbox.

A dry room below the lake surface needs a credible enclosed boundary and route to a trapped-air space; an open window below that surface cannot overlook outside water while the room inexplicably stays dry. Alternatively place dry floors and open windows above the floodline. Check connecting doors, stair heights, roof breaks, seepage and air pockets as one connected volume model. Exact engineering/history is undecided, but visible geometry must not contradict itself. Do not assume a magical air bubble to excuse layout errors without a separate creative decision.

Preserve river inflow, plausible containment/outflow and the descent from the village. Keep the treasury beneath/behind the high Court ridge dry and geographically separate. The initial lake vista may use distant simplified real geometry, but it must preserve relevant RT visibility and not imply that every ruin is resident or reachable.

**Gate A:** accept the route, destination readability and vertical floodline/air-pocket logic in a cheap playable blockout before commissioning detailed environments.

## 3. Three access solutions and recoverable traversal

Maintain three independently sufficient access paths:

| Preparation | Intended distinction | Safeguard |
|---|---|---|
| Magic | A temporary breathing ward earned through the access mini-quest | Define duration, activation and expiration; do not require Tech gear or Constitution investment too. |
| Tech | Breathing equipment prepared through practical services/quest work | Define operation and any resource limits; no mandatory Magic or Foundry-only component. |
| Constitution | Trained breath control for a short finite crossing | More breath is not infinite breath or drowning immunity; route length and safe margins must make this a genuine solution. |

Prototype the simplest movement that supports tension and looking at the drowned architecture. Compare a guided forward route with limited adjustment against a short bounded player-steered alternative only if the guide feels restrictive. Keep intentional entry, understandable progress and a clear arrival; retain look control and avoid compulsory camera roll, violent bob or disorienting forced turns. Decide detailed retreat/abort controls through testing, not after building the dungeon. Viable retreat itself is required: the main route works before the seal; a post-seal shortcut is additional.

Breath/exposure and access state belong to the shared fixed-step simulation, independent of rendering FPS and wave animation. Define when the timer starts, pauses and refills, how warnings are conveyed, and what happens on exhaustion or ward/gear failure. Provide readable non-colour-only warnings; do not rely on muffled audio alone. Set duration and safety margin using actual completion times for the slowest supported valid route/input method.

Provide a safe pre-crossing recovery checkpoint and a confirmed dry arrival before updating the destination checkpoint. On failure, return to a viable state with the necessary access capability; never respawn underwater without breath or consume the only essential item irreversibly. Mixed builds, spending, backtracking and the return to the hub must not create a campaign trap. Optional flooded rooms cannot hold an indispensable item beyond the player's recoverable capability.

Test touch in portrait and landscape, mouse/keyboard and supported controller input. Check reduced-motion treatment, interruption, pause/resume, Android background/screen lock, save/load, death/retry and repeated crossing. Do not advance breath while ordinary gameplay is paused or suspended. If underwater manual saves are unsupported, explain the safe-save policy rather than restoring an unsafe transient state.

**Gate B:** each of the three access paths independently completes both necessary travel directions, including failure/retry and either dungeon order. Lock movement, exposure and recovery rules only after this playable proof.

## 4. Rendering approach: establish a sheltered lake first

### Reuse audit

At planning-branch snapshot `31da869c385eac9f8a00f6b51ec77b9908a07c67`, [the engine contracts](AGENT_ENGINE_CONTRACTS.md#reusable-visuals-and-bounded-transport) and inspected `rt_hit_decode.glsl` / `rt_lighting.glsl` already provide water material handling, animated normals and bounded secondary-water behaviour. The hit decoding also includes waterfall stream profiles, near/far exit geometry and specific instance assumptions. These are useful foundations, **not evidence of an implemented lake surface, underwater camera or Abbey volume system**.

Re-audit the eventual accepted source before implementation. Extract/reuse appropriate material and lighting contracts through shared systems; do not enlarge a thin waterfall into a lake or copy its stream exit calculation blindly. The [August water report](RT_WATERFALL_LICH_MIST_VALIDATION_2026-08-23.md) and [transmission-shadow report](WATER_TRANSMISSION_SHADOW_VALIDATION_2026-08-24.md) describe historical evidence, not performance certification for this chapter.

### Baseline and RT ownership

1. Start with a bounded lake surface and a small number of broad, low-amplitude analytic waves, plus fine-scale animated normals. Tune wind, shoreline response and wave scale for a sheltered basin; do not inherit storm-ocean swell, ubiquitous whitecaps or sea spray.
2. Establish convincing Fresnel reflection, finite refraction/absorption and the broken-tower reflection at the overlook. Add restrained shoreline/intersection foam and local inflow disturbance only where justified by the scene.
3. Select the actual RT intersection representation before increasing displacement. Shader normals alone cannot create a changing silhouette. If geometry moves, bound tessellation, acceleration-structure (AS) refit/rebuild frequency, synchronization, scratch memory and CPU/GPU cost. Any procedural-intersection alternative must prove support on both targets. Primary, shadow and reflected/transmitted views must agree on relevant surface position.
4. Define the supported air/water entry and exit cases, inside-medium distance and absorption, underwater camera transitions, interface normals, total internal reflection handling and finite reflection/refraction/interface budgets. Preserve shared light visibility and termination rules; no unbounded water-on-water recursion, hidden lighting floors or duplicated glossy energy.
5. Treat underwater depth fog, suspended colour and caustic patterns as a bounded art-direction problem. Establish world/medium-space depth and scene occlusion so interiors and surface crossings remain legible. This is not a promise of full volumetric multiple scattering or physically complete moving caustics. Decorative caustics must never act as false evidence for a puzzle-critical light path.
6. Tune wet stone, windows, seepage and muffled sound around the dry-room silhouette and navigable floor. Preserve combat readability and the silent protagonist. Audio remains separately scoped and validated when implemented.

Retain real Vulkan hardware RT and honest RT-produced presentation. Quality fallbacks reduce admitted water detail or secondary transport within an explicit supported RT tier; they do not substitute raster/screen-space water, silently lower unrelated settings or disable gameplay-critical water visibility. Reassess any historical water-Off setting before chapter admission: it must not erase the lake, conceal a hazard or bypass access rules.

**Gate C:** the small vista/crossing/dry-room scene looks coherent above, at and below the surface, with a documented material/transport contract and no geometry/AS correctness errors.

### FFT is conditional

Do not begin with an FFT ocean framework. First establish whether analytic waves plus fine normals meet the approved lake views. Consider FFT only if specific matched views demonstrate a remaining visual need and its simulation, texture/buffer, dispatch and dynamic-geometry costs fit the chapter budget. Compare against the accepted analytic baseline, and keep or reject from measured results. No FFTW dependency is approved by this plan.

Permissive reference starting points, checked 5 October 2026:

- [2Retr0/GodotOceanWaves](https://github.com/2Retr0/GodotOceanWaves): MIT-licensed Godot FFT-water experiment; useful research into spectral waves and surface shading, not a native Vulkan RT drop-in.
- [gasgiant/Ocean-URP](https://github.com/gasgiant/Ocean-URP): MIT-licensed Unity/URP reference. The author's [FFT-Ocean README](https://github.com/gasgiant/FFT-Ocean/blob/main/README.md) describes this successor as unfinished; treat it as research, not production-ready integration.

Before borrowing code, pin a revision and audit the particular source, dependencies, shaders and bundled assets individually. A root MIT licence does not certify every third-party asset or dependency. Retain copyright/permission notices, record provenance and imported components in the appropriate licence records, and assess compatibility before adding any dependency, especially FFTW. No code or assets are imported by this documentation.

## 5. Staged implementation and acceptance

Each stage is a future scoped task, entered only after its dependencies and evidence are accepted:

| Stage | Deliverable | Exit evidence |
|---|---|---|
| 0 — Re-audit and budget | Accepted engine/hub baseline, scoped route, vertical section, provisional content and water budgets | Open decisions assigned; actual reusable water, save, streaming and gameplay capabilities identified; no reliance on an unimplemented 1.7 foundation |
| 1 — Cheap greybox | One overlook/descent, one crossing and one dry room; placeholder towers | Gate A spatial proof and Gate B access/control/recovery proof |
| 2 — Water look prototype | Baseline lake, surface transition, bounded optics and dry-room water views | Gate C correctness/readability; analytic baseline captured before considering FFT |
| 3 — Combined slice | Representative room combat and one taught light interaction beside the water, plus chapter residency transitions | Android and Windows combined timing/memory/streaming evidence; puzzle outcomes stable across supported quality tiers |
| 4 — Chapter expansion | Approved encounter/puzzle chain, Bellkeeper, seal, return and either-order story handling | Complete Abbey with starting equipment, each access solution and its own local tool; no Foundry dependency, soft lock or repeated reward |
| 5 — Acceptance | Exact candidate and retained baseline comparison | Scope, visual/gameplay review, affected tests, sustained-device evidence and remaining gaps reviewed before release consideration |

### Measurement and quality gates

Budget the **combined** workload: lake pixels, visible ruins, terrain/sky, fog, lantern/light queries, player/actors, combat, AS work and zone transitions. A water-only screenshot or fast desktop frame cannot certify Android readiness.

Record exact build, device/GPU/driver, render and output resolution, effective settings and matched routes. Measure CPU/GPU frame-time and pacing, water/AS cost where instrumentation permits, transient and resident memory, upload/streaming spikes and sustained thermal behaviour. Distinguish measured GPU time from CPU-present-loop proxies. Choose numerical budgets before expansion from actual target evidence, not an invented universal FPS promise.

Use bounded surface coverage, near/far real-geometry detail, explicit update rates and residency to meet those budgets. Stream the approach/interior deliberately; do not retain the whole lakebed and every exterior detail merely because towers remain visible. Preserve RT-relevant shadows/reflections when simplifying or unloading. Check repeated entry/exit for leaks, missing geometry and stale optical state.

Measure candidate RT tiers on Android and Windows with matched baseline settings. Keep the same navigable route, floodline and puzzle truth in every supported tier. Investigate regressions instead of silently weakening lighting or render resolution. Reject or defer expensive features when the evidence is insufficient; a reduced tier must still deliver the approved chapter experience.

The reflector is a persistent, nonconsumable utility, distinct from the seal. Bound placement and provide safe recall/reset after invalid placement, water, death or reload without duplicating the item or losing the lantern. Acquire and practice before mandatory item tests; the Bellkeeper uses the already-taught rule.

For light puzzles, apply [the shared causal-light safeguards](ROADMAP.md#ray-traced-light-as-a-gameplay-pillar): bounded world-space queries, coherent blockers and fixed-step authority, never screen brightness or render noise as the solution test. Test safe reset, dropped/stowed tools and restored saves before adding the boss.

## 6. Decisions still open

- Historical flooding cause/date; precise architecture and credible dry-volume explanation
- Detailed section, floor elevations, lake extent, route distances, air-pocket placement and optional flooded-room count
- Guided versus bounded free steering, detailed retreat controls, crossing duration, breath/ward/gear limits and exact failure feedback; safe main-route retreat is required
- Detailed reflector placement/recall and optical-query rules, enemy footprint, Bellkeeper actions/model selection and narrative wording; the reflector/shutter/armour dependency itself is approved
- Exact water intersection/displacement representation, update cadence, transport budget, underwater treatment and any justified FFT use
- Numerical sustained-device budgets, supported water tiers, streaming boundaries, save restrictions and chapter version/date

These decisions must not silently change the approved lake setting, predominantly dry dungeon, three viable access paths, either-order middle dungeons or wider campaign canon.

**Validation boundary for this update:** documentation and source inspection only. No runtime prototype, build, asset import or device-performance result is delivered. Audio/haptic manual revalidation required: **NO** for this documentation change.
