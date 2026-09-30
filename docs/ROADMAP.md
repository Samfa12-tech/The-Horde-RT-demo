# Horde Lantern RT — Campaign and Engine Roadmap

**Owner:** Sam Small / Samfa12  
**Planning update:** 30 September 2026  
**Status:** Owner-approved direction; future milestones are provisional, not implemented or release promises.

## Authority and navigation

This is the canonical forward-looking campaign roadmap. Use it alongside [AGENTS.md](../AGENTS.md) and [PROJECT_DECISIONS.md](../PROJECT_DECISIONS.md), which retain the engineering, safety and validated-baseline rules. The [Phase Plan](PHASE_PLAN.md) preserves the historical implementation sequence; dated audits and validation reports remain evidence of their own snapshots. This roadmap does not rewrite that history or certify newer runtime behaviour.

The [README](../README.md) identifies the published package and its evidence. A future version label here does not change the application version, prove a feature exists, authorise publication or supersede an unfinished engineering pass.

## Direction approved by the owner

The dungeon becomes the prologue to a small, authored historical-gothic adventure. The player and companion emerge into woodland, reach an inhabited village, and learn more in its tavern about the wider **Horde**. The village becomes the recurring hub for three subsequent themed dungeons.

Preserve **the Horde** as the owner's working term. Do not silently change it to the Hoard, assume it means an army or treasure, or settle its mythology. The precise nature of the three rewards — pieces of the Horde, a key, a map, or another related form — remains open. Names, revelations and the ultimate resolution need a later narrative decision, not invented canon in an implementation pass.

The defining design pillar is **light as gameplay, powered by the actual ray-tracing engine**, not ray tracing as decoration added after conventional rooms and combat.

## Milestone sequence

| Milestone | Direction | Status / start condition |
|---|---|---|
| 1.6.1 engineering baseline | Finish the current engineering programme and its validation. | Complete, accept, merge and release 1.6.1 before starting 1.6.2 implementation. Preserve the active engineering pass. |
| 1.6.2 — Engine readiness and demo polish | Measured temporal/upscaling readiness and targeted polish of the existing dungeon demo. | Planned after released 1.6.1 and before 1.7 gameplay expansion; see the brief below. |
| 1.7.0 — Beyond the Tomb | Existing dungeon becomes prologue; physical rope rescue, companion, moonlit woodland, dialogue, world-zone ownership, checkpoints and themed controls/menus. | Scoped planning handoff exists. Start after 1.6.2 is implemented, validated and accepted; then complete and obtain owner acceptance of the agreed 1.7 scope. |
| 1.8.0 — Village Hub | Continue from the forest to a small village with a few explorable interiors, a hero tavern and a small NPC cast. Reveal more of the wider Horde and establish the hub. | Pencilled in. Begin only after 1.7 is made, tested and accepted; use its actual performance and system evidence to finalise scope. |
| Beyond 1.8 — three themed dungeons | Three distinct adventure dungeons, each with enemies, puzzles, a boss and one campaign piece; consider a dungeon-specific item that enables puzzle solving and boss defeat. | Big-picture direction only. Build and validate one complete dungeon before expanding to the next. No version numbers or release dates are assigned. |
| Beyond the trio | Resolve the combined clues/pieces and the wider mystery. | Intentionally unscoped. No fourth dungeon, final boss, ending or additional feature set is promised. |

Detailed plans:

- [1.7.0 — Beyond the Tomb](superpowers/plans/2026-09-11-beyond-the-tomb-1.7.0.md).
- [1.8.0 — Village Hub: provisional plan](superpowers/plans/2026-09-11-village-hub-1.8.0.md).

The three later dungeons are additional adventures after the existing tomb/prologue, not a silent relabelling of that prologue as one of the three. Their order, themes and unlock rules are still design decisions.

## 1.6.2 — Engine readiness and demo polish

Finish, accept, merge and release **1.6.1 first**, then undertake this bounded preparation/polish milestone before **1.7 gameplay expansion**. These are future tasks, not new 1.6.1 acceptance blockers. Re-audit the accepted release to avoid duplicating work completed in the engineering pass. Its current programme includes music and RayQuery compatibility; the older post-1.6.1 ordering in historical planning must not reopen or resequence that work.

### Renderer readiness

- Evaluate one shared temporal-input foundation: depth, motion vectors for camera and moving/deforming objects, jitter and previous-frame transforms, plus explicit history ownership, invalidation and disocclusion handling. Cover reset/retry, camera cuts, lifecycle and internal-resolution changes before relying on accumulated history.
- Evaluate platform adapters over that foundation: DLSS Super Resolution on supported Windows RTX hardware, and SGSR 2 or another justified Android option. Inspect actual SDK/licence, Vulkan and device requirements, then measure image quality, memory, latency and sustained cost. This is a measured feasibility/integration direction, not a promise to ship either vendor integration or claim support before evidence exists.
- Audit dynamic resolution and efficient render-target resizing on the final 1.6.1 baseline; finish only the missing work. Keep resource/descriptor lifetime and temporal-history reset correct, avoid unnecessary full-scene rebuilds or resize stalls, and report actual internal dimensions and adaptive behaviour.
- Retain the fixed **75% Galaxy S26 Ultra sustained 30 FPS baseline/target** as a distinct acceptance comparison. Record exact source/artifact, scene, workload, dimensions, build type and warm thermal conditions. Adaptive-resolution, upscaled and cooled runs are separate labelled evidence; none substitutes for the fixed-scale baseline or proves an unmeasured pass. Preserve real hardware RT and investigate matched regressions.

### Existing-demo visual polish

- **Fire, especially the reward lantern:** investigate the owner's report that the flame reads as a static coloured tapered spindle. Inspect it in motion before attributing a cause; improve shape, animation and readability through the shared world-space fire system while keeping emissive geometry, volume, direct light, glass and reflections coherent. Check held, raised, swinging and stationary views on phone and Windows.
- **Visible exterior sky/skybox:** give existing openings a convincing visible environment through the native RT path. Evaluate Briarhold's sky assets before creating replacements, checking provenance, projection, seams, colour space and suitability for Horde's existing lighting/exposure. A direction-sampled miss environment is valid; preserve physical roof/geometry occlusion and consistent relevant reflected/transmitted views. Do not add a fake background quad or raster fallback. Preserve the existing finale's accepted timing and light progression; 1.7 owns its deliberate moonlit rescue change.
- **Entryway/skeleton-area props:** identify the flat-looking objects from the actual scene/captures, then improve or replace their geometry/materials as appropriate. Do not guess their identity or assume a particular asset is the cause. Retain collision, route readability and measured RT budgets.
- **Held torch versus low roofs:** reproduce the reported clipping and investigate both visible equipment clearance and physical geometry/collision. Use the shared held-item/retraction contract, including overhead clearance; verify torch, flame, light, hands and shadows agree through movement and look angles rather than hiding the problem with an overlay.

This milestone prepares reusable foundations and polishes the existing demo. It does not deliver an outdoor level, rope climb, companion, forest route or the complete moonlit sky/atmosphere programme; those remain in the scoped 1.7 plan. Accept each slice with affected tests, real RT captures/motion review and device measurements, recording any deferred adapter or unresolved quality/performance limit honestly.

## The hub-and-dungeon loop

Proposed campaign structure:

`Tomb prologue → rescue and forest → village/tavern → dungeon expedition → return to hub with a piece and new knowledge → next expedition`

The tavern should give the player a reason to care about the wider Horde, introduce useful people and leads, and provide a natural place to interpret discoveries after each return. Reveal the mystery progressively rather than delivering all the lore on arrival. Returning NPC dialogue should acknowledge relevant progress without replaying one-time scenes or rewards.

The hub must have a purpose, but it does not require shops, crafting, a full inventory or a simulated economy. In 1.8, a bounded conversation, durable village checkpoint and clear next lead are enough to establish that purpose. An exit or objective must not suggest an unavailable dungeon is already playable.

## Common design brief for each later dungeon

Each of the three should have its own environmental identity, enemies/encounters, a learnable light-based puzzle language, a boss and a meaningful campaign reward. Distinct themes need not mean unrelated bespoke engines or three entirely separate combat systems.

The owner suggested a Zelda-like item/puzzle/boss relationship. Use that structural inspiration while creating original items, spaces, characters and solutions. The recommended loop is:

1. Introduce a light-related rule in a safe, readable setting.
2. Acquire or activate a useful item or lantern capability inside the dungeon.
3. Teach its use, then combine it with navigation, enemies and increasingly demanding puzzles.
4. Let the player recognise and apply the learned rule during the boss encounter; do not introduce an unexplained mandatory mechanic only in the boss room.
5. Award one persistent Horde/key/map-related piece and return the player safely to the hub with new information.

A dungeon utility item and its end-of-dungeon campaign piece are separate design roles; whether they are the same object is unresolved. Do not assume the existing reward lantern must be discarded or replaced. Persistent light tools should remain useful where appropriate rather than becoming disposable one-room keys.

Before committing a dungeon, approve its theme, light verb, item/capability, encounter and boss relationship, reward role, recoverable puzzle states, content footprint and measured device budget. These details are deliberately not locked by this roadmap.

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

The village name/layout and final cast; what the Horde ultimately is; the three dungeon themes and order; the exact utility items and campaign pieces; progression beyond the trio; final content counts and sustained device budgets. Resolve each when its milestone is ready, without treating illustrative ideas as owner-approved canon.

## Documentation-only change boundary

This update preserves the owner's 11 September campaign direction and records the 30 September 1.6.2 sequencing, demo-polish and asset-reuse additions. It implements no gameplay, changes no package/release identity, generates no assets, authorises no paid work and publishes no build.

**Audio/haptic manual revalidation required: NO — documentation only; runtime and semantic inputs are unchanged.**
