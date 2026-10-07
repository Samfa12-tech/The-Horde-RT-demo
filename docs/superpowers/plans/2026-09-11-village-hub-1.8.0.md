# Village Hub — 1.8.0 Provisional Milestone Plan

**Owner:** Sam Small / Samfa12  
**Prepared:** 11 September 2026  
**Working release label:** Showcase Alpha 1.8.0 — Village Hub  
**Status:** Campaign direction updated 30 September 2026; implementation scope must be reworked against the accepted 1.7 baseline. Not a delivered feature.  
**Roadmap:** [Campaign and Engine Roadmap](../../ROADMAP.md)

> Implement only after 1.7.0 has been made, tested on the supported devices and accepted by the owner. Do not expand the unfinished 1.6.1 engineering pass or the agreed 1.7 chapter to start village work. Re-audit the accepted 1.7 baseline and write the concrete implementation/test plan at that point.

**Dialogue authoring draft, 3 October 2026:** See [the whole-campaign bank](../../CAMPAIGN_DIALOGUE_BANK.md#5-village-arrival-first-lead-and-first-small-expedition) for proposed arrival, useful services, access preparation and progress-aware return scenes. It separates canon from new connective tissue and preserves either dungeon order. These are reviewable story examples, not a commitment to deliver the entire bank in 1.8.

## Approved geography update — 3 October 2026

[WORLD_LAYOUT.md](../../WORLD_LAYOUT.md) fixes the settlement's regional position and landmark relationships. 1.7 shows its low-detail distant shell from the lookout; 1.8 extends the **same road** southeast from that lookout into the **same village**, arriving from the northwest. Keep the unwalled hamlet, central tavern/well, northern church and active graveyard, eastern mill/stream, nearby starter-shrine spur and outbound peer routes to the downstream Abbey and eastern Foundry. The northwest abandoned Keeper Tomb burial ground is distinct from the churchyard. The high-ridge Court remains northeast and locked after both middle dungeons; its dry treasury lies beneath/behind the ridge. The land is The Veyrlands and the village is Bellwether, now approved; the church-bell name origin and unpulled-bell folklore are recorded in WORLD_LAYOUT.md without prescribing a runtime event. Exact local distances, building footprints and playable content budgets require blockout/testing; the map does not make every depicted structure an explorable 1.8 interior.

## 1. Purpose and owner direction

Extend the moonlit woodland into a small, believable village with a few explorable houses/interiors, a tavern and NPCs. The settlement should become the recurring hub for later adventures, while the tavern reveals more about the wider **Horde**.

Beyond 1.8: Drowned Abbey and Ashen Foundry in either order, then Glass Court, followed by the treasury finale. Each dungeon combines contextual enemies, light puzzles, a local tool, boss and treasury seal. Read [CAMPAIGN_DESIGN.md](../../CAMPAIGN_DESIGN.md) for approved narrative, alternative access quests and remaining design decisions; later dungeons are not 1.8 implementation scope.

Preserve The Horde as the working title and its lost-army/treasure ambiguity. Kit accompanies a silent player; the eventual narrative is fully voiced. Bellwether is approved; unapproved supporting names remain provisional. Do not use Mara. Spoilers and reveal order belong to CAMPAIGN_DESIGN.md.

This is a compact authored hub, not an open-world RPG conversion. Prioritise atmosphere, meaningful interactions and reliable continuity over settlement size.

## 2. Dependencies and source authority

Read [AGENTS.md](../../../AGENTS.md), [PROJECT_DECISIONS.md](../../../PROJECT_DECISIONS.md), the accepted implementation evidence and [the 1.7 plan](2026-09-11-beyond-the-tomb-1.7.0.md) before final scoping.

The 1.7 plan proposes bounded world-zone ownership, coherent 3D movement, a reusable friendly companion, animation, line-based dialogue/subtitles/audio, checkpoints, outdoor environments and themed platform UI. These would support the village, but their presence in a planning document does not establish that they are implemented, robust or fast enough for a larger cast.

At the start gate, identify what actually shipped and what can be reused. More simultaneous friendly actors, interactive doors, conversation choices and hub-return progression require their own scoped work and evidence; one working companion does not certify seven NPCs.

Retain the native Vulkan hardware-RT path, phone-safe renderer constraints, shared 60 Hz simulation, input/event contracts and separate Android/Windows validation. Keep the accepted player/lantern controls, saves and previous chapters working.

## 3. Proposed content envelope

These are initial authoring targets, **not fixed minimums or measured capacity claims**. Final counts follow the accepted 1.7 evidence and an early village stress scene.

| Area | Provisional target |
|---|---|
| Exterior | One compact village lane/square, framed by approximately 6–10 visible buildings, with believable boundaries and a forest connection. |
| Explorable interiors | Approximately 2–3 total: the tavern plus one or two houses/workshops/story interiors. Closed buildings must read honestly as closed. |
| Tavern | The hero interior, primary social/story destination and recurring point of return. |
| Cast | Approximately 4–7 named/present village characters including the companion where resident; total cast is not a promise that all are simultaneously visible or animated. |
| Important conversations | One or two substantial NPC exchanges, with shorter contextual lines for others. Keep the speaking cast and voice production bounded. |
| Time and weather | Continue the authored night from 1.7. Lich/lantern/tomb-exit progression does not cause dawn. Whether daylight ever enters the later game, and when or why, remains undecided. No day/night or weather simulation. |
| Gameplay | Explore, interact, establish a safe return, gain bounded gear/training benefits, use treasure currency and begin area-access preparation. Magic, Tech and Constitution offer complementary routes. No village combat required. |

Avoid a large empty square surrounded by decorative shells with no purpose. The few accessible interiors should reward entry through character, story or a useful interaction, not merely demonstrate that a door opens.

## 4. Intended player experience

### Arrival

Continue naturally from the 1.7 forest endpoint. The approach introduces signs of habitation and a readable path toward the village without implying an enormous traversable world. The companion can guide the arrival using existing behaviour, but should not push the player through a long forced camera sequence or block a doorway.

The exterior and interiors should agree in scale, door position and window layout. Show a compact settlement with plausible boundaries, not a maze designed to hide missing areas.

### Exploration

Let the player look around, enter the available houses/interiors and meet a small cast. Basic authored activities — seated conversation, tending a fire or a short walk between activity points — are enough for the first hub. Full daily schedules, unrestricted roaming, crowd simulation and needs systems are unnecessary.

Doors, NPCs and furniture must not trap the player or collide with the held lantern unpredictably. Use the existing semantic Interact action and tested touch/controller/keyboard conventions instead of adding a separate bespoke control for every activity.

### Tavern and the wider Horde

The tavern is the narrative anchor. A conversation should acknowledge the tomb, companion and recovered lantern, then reveal a bounded new clue about the wider Horde and a reason to continue investigating. Stage the early keeper/treasury lead without revealing the lantern deception. Exact script and supporting speaker remain authoring work.

Allow the player to pause, skip/revisit appropriate conversation and understand the next objective without hearing every line. Do not dump the entire mystery on arrival. Future returns should support new dialogue conditioned on the relevant dungeon piece and knowledge; 1.8 also needs the bounded useful services and access-preparation slice agreed at its scope gate.

The hub is not just a level-selection menu: people, practical upgrades/training, treasure spending, access mini-quests, checkpoint and evolving information give it purpose. Define a small playable service/quest slice for 1.8; a conversation-only hub no longer meets the direction. An honest future-content endpoint is acceptable until the next dungeon exists.

### Light and atmosphere

Make the transition from cold moonlit exterior to warm tavern meaningful: real fireplace/lantern illumination, characters and furniture casting moving shadows, selective glass/metal/wet-surface response and appropriate interior ambience. Use the accepted fire/glass/material systems; do not promise an unmeasured new optical effect for every prop.

Proposed bounded interaction: one small lantern-based inspection or clue moment using proven light behaviour. Its final design should reinforce the wider light-gameplay direction without pulling advanced dungeon items or a general optical puzzle framework into 1.8. It must be more than a cosmetic glow attached to an unrelated trigger.

## 5. Reusable engine work, not village-only exceptions

### Zones and interior ownership

Extend the accepted bounded zone system for the forest connection, village exterior and accessible interiors. Keep campaign state independent of zone resource lifetimes. Prefer seamless player-perceived arrival using preloading or staged preparation chosen from measurements. An explicit loading transition remains an owner-reviewed fallback when the bounded resource constraints cannot be met; seamless continuity does not mean the whole regional world is resident.

Do not equate off-camera with irrelevant: interiors can contribute through open doors, windows, reflections, transmission and shadows. Keep visible and RT-relevant content resident across transitions, or design a truthful closed/occluded boundary. Never delete an interior while its doorway or reflection still shows it.

Use safe GPU lifetime/readiness handling, failure recovery, backtracking and reload generation IDs. No general open-world streamer is required.

### Actors and conversation

Extend reusable stable-identity actor and animation infrastructure rather than copying the companion controller per villager or borrowing combat enemy slots invisibly. Measure independent animation poses, skinning/refit, collision and local movement costs. Shared assets/poses and bounded activity routes may be appropriate when visually and behaviourally correct.

Build on 1.7's line IDs, subtitles, speaker identity, playback completion and cancellation. Add only the conversation state needed for hub interactions; a small choice surface can be considered where a real exchange benefits, not a universal branching-dialogue editor. Story progress cannot depend on a successful audio callback.

### Progression and persistence

Use stable IDs and versioned logical flags for village arrival, completed introductions, relevant knowledge and available leads. Keep lantern ownership, player state and the accepted checkpoint system coherent. Save after meaningful progress and restore to a safe position, without duplicating rewards or replaying completed one-time scenes.

Provide an extensible place for later dungeon completion/piece IDs without implementing three nonexistent campaigns. Use bounded currency, unique component and training/upgrade records for the selected playable services. Full inventory grids, crafting, XP and simulated economy remain undecided or unnecessary; prevent double-spends, duplicated rewards and build-dependent progression traps. Unavailable content remains clearly unavailable.

### Interaction, audio and controls

Reuse semantic interactions, themed UI and existing music/audio abstractions. Retain independent Music, SFX/Ambience and Dialogue controls where delivered by 1.7. Tavern ambience and speech must remain intelligible, pause correctly and not leak stale callbacks across zone changes.

Keep NPC interaction and doors comfortable on touch, keyboard/mouse and controller; preserve font scaling, focus, Back/Escape and input-consumption rules. Do not add controls merely to show off a future system.

## 6. Asset and performance approach

Use a small cohesive modular architectural kit, shared PBR materials and selected hero assets rather than a unique high-density model and material set for each building. Retain source art and provenance separately from packaged runtime assets. Validate scale, geometry, UVs, materials, animation, collision and mobile texture residency through the accepted asset pipeline.

Use gate 6 of the shared [six-gate outdoor readiness contract](2026-09-11-beyond-the-tomb-1.7.0.md#131-six-gate-outdoor-readiness-contract): make the early stress workload part of one playable arrival-to-tavern slice before expanding cast or interiors. Reuse the established lookout/approach/Bellwether geography, real near/mid/far geometry/material LODs, separate memory/primary/reflection/shadow/light-relevance policy and measured budget method. Include the tavern threshold, a realistic bounded subset of actors, active fire/lantern sources, selected glass and required atmosphere, with the worst exterior/interior view, transitions, backtracking and repeat visits. One exchange, safe checkpoint and reliable return outside must work in this same slice; an empty street or isolated NPC benchmark cannot certify expansion.

Measure warm exact-candidate frame timing, actual internal pixel dimensions/preset, CPU and valid GPU costs, animated-character work, acceleration-structure updates, texture/resource peaks, loading hitches and repeated interior transitions. Test both the S26 Ultra and Windows RTX target. Use the repository's descriptive 60/50/30 FPS bands and matched-regression policy; do not invent a guaranteed frame rate from a document or hide cost by reducing quality/resolution.

Set explicit numeric working budgets and acceptance thresholds from the combined sustained Android/Windows measurements before expanding the provisional cast/interior envelope. Include memory residency, upload/staging peaks and AS build/update costs; accepted 1.7 numbers must be revalidated for the added hub workload. Adjust optional density, ornament and activity counts before widening the scene. Any trade-off affecting required experience or RT correctness needs an explicit scope decision. No subscriptions, paid generation, credit top-ups or new asset purchases are authorised by this plan.

## 7. Explicitly outside 1.8

The three later dungeons and their bosses/items; a general optical puzzle framework; new village combat/friendly-fire/crime systems; full NPC schedules; a general simulated economy and broad crafting (bounded treasure spending, gear/training services and access preparation are now in the hub direction); a general inventory grid; procedural quests; open-world streaming; day/night/weather simulation; multiplayer; a broad renderer/engine replacement; and production of later campaign content. The narrative direction is now approved in CAMPAIGN_DESIGN.md, while detailed scripts and encounters remain future authoring.

Do not prebuild these as speculative infrastructure. Add only the reusable seams required by the playable hub, documenting later dependencies separately in the roadmap.

## 8. Proposed execution order after the 1.7 acceptance gate

1. Audit the accepted 1.7 source, saves, zone/actor/dialogue contracts and exact-device evidence; refine the approved hub geography into a tested street/building blockout, story beat and measured scope.
2. Build the representative exterior/interior/NPC workload and prove the residency, interaction and performance approach before producing the whole asset set.
3. Complete and accept that same playable arrival-to-tavern stress slice with one NPC exchange, safe checkpoint, reliable return outside and repeated visits; record gate 6's combined correctness and numeric-budget decision before proceeding.
4. Extend to the agreed houses/cast and implement the scoped useful service, treasure/training and access-quest slice, progress-aware dialogue and lantern clue. Validate alternate progression routes and save/reward/spending integrity.
5. Apply final art/audio polish, verify repeat visits and previous chapters, and complete cross-platform acceptance before proposing release.

This order is guidance for the later implementation plan, not permission to begin now or to report an incomplete village as 1.8 complete.

## 9. Acceptance evidence to define and collect

The final implementation handoff should cover the following gates at the accepted scope:

- Natural arrival from 1.7; navigable exterior and all promised interiors; coherent doors/windows/collision; no NPC doorway traps or unavailable-content deception.
- Working tavern reveal and next lead, readable subtitles, skip/pause/resume and repeat interactions; no duplicated ownership or progression, including missing-audio paths.
- Practical benefit from the scoped gear/training and access-preparation services; correct treasure transactions and alternative-build progression; no mandatory unavailable dungeon advertised as playable.
- Silent player; Kit and NPCs carry dialogue without asserting unchosen player beliefs, with eventual full voice coverage and current milestone's recorded lines verified.
- Safe saves and recovery at the village and across interiors, process termination, Android lifecycle changes and return to prior available areas.
- Real RT presentation, consistent light/shadow/reflection/transmission through relevant openings, and no disappearing off-camera contributors. Any light interaction must agree with its visible optical cause across supported settings.
- Exact-candidate sustained phone and Windows evidence, resource stability over repeated visits, representative NPC density and matched regression checks on unchanged earlier chapters.
- Asset/licence/package and automated regression gates, with hands-on input/readability/feel checks and change-triggered audio/haptic validation.

No builds, benchmarks, artwork, recordings or runtime tests are delivered by this documentation task. Application versions, release channels and public packages remain unchanged; publication requires separate owner authorisation and the normal release gates.

**Audio/haptic manual revalidation required: NO for this documentation-only update.** When implementation changes dialogue, ambience, spatialisation or feedback, classify the exact work package under the existing change-triggered rule; those affected changes will require the relevant manual check.


## Preserved connected Bellwether references — 5 October 2026

![Bellwether connected town planning reference](../../design/world/connected/02a-bellwether-terrain-1-8.webp)

Read the [connected map set](../../design/world/connected/README.md), building-feature data and geometry caveats together. These are coordinated blockout proposals, not validated distances, exact tree/fence instances, final geometry or additional approved content. Continue the same forest approach and keep Bellwether's fixed regional location. The archive separates the earlier atmospheric town image from these newer coordinated drawings. Natural forest boundaries, dry-stone garden edges and timber pens must agree with readable playable collision; they are proposals pending blockout. Existing hero-tavern, selected-interior, population and performance gates remain authoritative.

## Village life and earned recap addendum — 7 October 2026

The approved humour/food-life direction is developed in [Bellwether village life](../../BELLWETHER_VILLAGE_LIFE.md): a closed outhouse voice and optional rag favour; a man in a hole and contradictory unverified gossip; modest livestock and kitchen plots with credible storage/water/waste relationships. See its bounded asset packages, provisional lines and acceptance cases. Preserve the connected map's eight principal masses, fixed routes and view targets; local placements remain blockout decisions. It adds no farming simulator, mandatory quest, new explorable interior or simultaneous-cast guarantee.

[The Tale Thus Far](../../TALE_THUS_FAR.md) proposes optional earned verse and spoiler-safe recaps. Village stanzas require actual arrival/welcome and relevant knowledge, never just reaching the 1.7 lookout. The latest [English dialogue bank](../../dialogue/en/README.md) and campaign design own Kit's injured, town-based recovery arc; any older escort implication above is superseded. Exact injury, new comedy wording and all recording remain unapproved.
