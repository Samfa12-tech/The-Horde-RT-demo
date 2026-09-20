# Horde Lantern RT — Campaign and Engine Roadmap

**Owner:** Sam Small / Samfa12  
**Planning update:** 11 September 2026  
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
| 1.6.x foundation | Preserve the dungeon, combat, fire/PBR assets, reward lantern, shared simulation and engineering work. | The inspected README records 1.6.0 as published. Finish and accept the separate 1.6.1 engineering baseline before 1.7 implementation. |
| 1.7.0 — Beyond the Tomb | Existing dungeon becomes prologue; physical rope rescue, companion, moonlit woodland, dialogue, world-zone ownership, checkpoints and themed controls/menus. | Scoped planning handoff exists. Implement and validate its complete agreed scope, then obtain owner acceptance. |
| 1.8.0 — Village Hub | Continue from the forest to a small village with a few explorable interiors, a hero tavern and a small NPC cast. Reveal more of the wider Horde and establish the hub. | Pencilled in. Begin only after 1.7 is made, tested and accepted; use its actual performance and system evidence to finalise scope. |
| Beyond 1.8 — three themed dungeons | Three distinct adventure dungeons, each with enemies, puzzles, a boss and one campaign piece; consider a dungeon-specific item that enables puzzle solving and boss defeat. | Big-picture direction only. Build and validate one complete dungeon before expanding to the next. No version numbers or release dates are assigned. |
| Beyond the trio | Resolve the combined clues/pieces and the wider mystery. | Intentionally unscoped. No fourth dungeon, final boss, ending or additional feature set is promised. |

Detailed plans:

- [1.7.0 — Beyond the Tomb](superpowers/plans/2026-09-11-beyond-the-tomb-1.7.0.md).
- [1.8.0 — Village Hub: provisional plan](superpowers/plans/2026-09-11-village-hub-1.8.0.md).

The three later dungeons are additional adventures after the existing tomb/prologue, not a silent relabelling of that prologue as one of the three. Their order, themes and unlock rules are still design decisions.

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

## Decisions intentionally left open

The village name/layout and final cast; what the Horde ultimately is; the three dungeon themes and order; the exact utility items and campaign pieces; progression beyond the trio; final content counts and sustained device budgets. Resolve each when its milestone is ready, without treating illustrative ideas as owner-approved canon.

## Documentation-only change boundary

This update records the owner's 11 September 2026 direction. It implements no gameplay, changes no package/release identity, generates no assets, authorises no paid work and publishes no build.

**Audio/haptic manual revalidation required: NO — documentation only; runtime and semantic inputs are unchanged.**
