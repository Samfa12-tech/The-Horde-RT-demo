# Bellwether village life design appendix

**The Horde 1.8 planning draft · 7 October 2026**

Bellwether should feel as though people eat, mend things, embarrass themselves and get on with life between expeditions. Add a small layer of optional human nonsense around the existing useful hub: a closed privy, a man in a locked hole, a few domestic animals and working garden patches. Keep the humour brief enough that the village can still be quiet, comforting and occasionally uncanny.

This appendix proposes staging, interactions and production packages for review. It does not approve runtime work, asset or audio generation, spending, new interiors, release scope or a larger settlement.

## 1 Authority and decisions

**Approved direction supplied for this appendix**

- A closed outhouse with an unseen occupant demanding, “Can someone PLEASE get me some rag?!”
- A man locked in a hole for an unknown reason, with conflicting local rumours: he fell in; he lives there; and a nonexplicit, unsubstantiated insinuation concerning pigs.
- Animals and small crop patches so the village has a visible relationship to its food.

**Existing canon and scope retained**

- Bellwether is the compact, unwalled village in The Veyrlands. Its tavern/well remain central, gardens west, church/active graveyard north, mill east and smith southeast. The northwest arrival and later regional routes stay where established.
- Preserve all eight principal building footprints, heights, doors, paths and desired view corridors in the coordinated reference. B09 remains the separately scoped starter shrine. Those reference dimensions still require playable blockout verification; this appendix does not replace them.
- Continue the authored night. No day/night simulation, farming season system, hunger meter, breeding system, livestock combat or settlement economy simulation.
- Kit is injured on the approach, recovers based in Bellwether and remains a useful partner through major returns. Exact injury and recovery timing remain undecided. Older escort/treasury-presence proposals do not override this arc.
- The protagonist is silent. All responses below are player-controlled interactions, never implied spoken answers or an imposed opinion.
- The authoritative English source remains the existing 357-line JSON bank. Its recording approvals are all false. None of the new lines below has been added to it.

**Draft decisions, not newly approved canon:** precise placement; identities/casting; all additional dialogue; rag delivery mechanics; hole construction, care and eventual resolution; species/counts; food-storage detail; clip lists and runtime budgets. “Approved direction” above does not approve every implementation proposed here.

## 2 Fit the existing village

Treat these as small exterior dressings and activity pockets, not additional principal buildings. First test them beside the existing western gardens and in a screened tavern service-yard edge. Do not assign final coordinates before a three-dimensional walk-through.

| Pocket | Proposed additions | Boundaries |
|---|---|---|
| Western gardens, around the existing T02 garden area | Two or three small crop beds, a modest pen/roost, tool basket, covered compost container; locked-hole feature at a separate dry perimeter pocket if it fits | Preserve House A/B access, dry pedestrian openings and arrival views. The hole is neither a garden cesspit nor an animal enclosure. Keep animals out of edible beds. |
| Tavern service yard, beside B01 | Small closed privy, clean-rag basket, water-carrying/handwashing props and one restrained practical gag | Keep the actual tavern entrance at its registered door, the arrival-to-tavern route, well access, mill route and interior sightlines clear. Sanitation layout must pass blockout review. |

Retain B01 Tavern, B02 Church, B03 House A, B04 House B, B05 Mill, B06 Smith, B07 Store and B08 Outbuilding unchanged. B08 is not silently converted into the privy. The proposed privy is a small external service structure, with no explorable room, loading transition or interior promise. Neither it nor the hole counts as a new mandatory interior.

Do not displace the central well, move a main path, raise a screen into the church/roof composition, or extend the village boundary to make a joke fit. If the available pockets conflict, reduce or defer the optional dressing. Keep the hole and privy visibly separate, with different materials and distinct audio anchors, so players cannot mistake a person’s living space for the waste feature.

## 3 The closed privy

### The encounter

The player hears one indignant voice from behind a solid, latched door during a quiet, eligible approach into the service pocket. The demand is disproportionate to the small problem, and the occupant retains their privacy. The camera stays with the player. There is no reaching hand, supernatural toilet character, borrowed silhouette, copied reward scene or toilet-interior reveal.

A clean scrap basket nearby makes the solution comprehensible without another NPC fetch chain. An optional silent “Take a clean rag” interaction offers one local task token. “Pass the rag” deposits it through an ordinary covered delivery slot or on a sheltered shelf beside the door; the exact prop must be tested. The occupant thanks the player, still unseen. A small cloth rustle is sufficient; do not add bodily sound effects.

The ordinary Interact action does all the work. No timed precision, door-opening puzzle, physics cloth, new hand rig or mandatory first-person handover animation is required. If a suitable existing presentation animation is unavailable, use the existing interaction feedback and an honest shelf-state change.

### A modest favour without an inventory trap

- The rag is free, explicitly set aside for this purpose, and separate from bandages, crafting cloth, named components and progression items. Never consume a healing item or infer that any inventory object labelled “cloth” qualifies.
- Grant no seal, access permission, exclusive upgrade, required reputation or currency reward. The payoff is the exchange and a small remembered kindness. Ordinary shop prices and Kit’s care are unaffected.
- A local single-purpose favour flag can represent carrying it; this is not a reason to add a general inventory grid. Prefer keeping it outside limited inventory capacity.
- Cancel, leave or ignore freely. The occupant does not deteriorate, scream on a timer, shame the player or summon a quest marker across town.
- Delivery commits the favour once through a world interaction, independently of sound completion. An interruption after delivery cannot remove another rag or duplicate a reward. Reload restores the basket/shelf and logical state together.
- An interrupted or missing invitation cannot block completion: the clean-rag source and door interaction remain readable. A lost transient task token can be reacquired locally without charge. Finish the delivery and the rag action disappears.

The door remains honestly occupied while this scene is staged. Whether it later becomes an ordinary silent closed prop is a review decision; no visible occupant model or walking-away sequence is assumed.

## 4 The man in the hole

### What the player can establish

There is an adult man below a small locked grate or hatch. He is alert, opinionated and able to speak for himself. The player can inspect the lock and speak from a safe edge, but cannot infer why he is there. No journal, narrator, achievement, subtitle speaker label or environmental clue silently identifies him as a criminal, deviant, prisoner of a named faction or deserving of punishment.

Use a shallow authored pocket or a sealed visual shaft with a dry ledge and a bounded seated/standing performance. Never make the player fall in, descend to an unavailable interior or hunt a key that does not exist. The grate’s lock should read as observed set dressing, not a highlighted general lockpicking interaction. Player collision around the edge must be visible and plausible.

The setting should show basic care: reachable food/water, a dry place to rest and some shelter. These are proposals to make the staging humane, not proof that involuntary confinement is acceptable. Do not show starvation, distress, beatings or a crowd tormenting him for laughs. The joke is the village’s confident, incompatible explanations and his refusal to become their entertainment.

### Rumours remain rumours

1. One resident claims he fell in.
2. Another insists he lives there.
3. A rare, optional exchange alludes vaguely to a story about him and the pigs. A second speaker immediately rejects it as gossip. No sexual act, anatomy, sound, animation, written “evidence” or animal reaction is depicted.

None of these accounts is confirmed. Do not let the pig insinuation become the quest explanation, the man’s name, a permanent UI epithet or the only thing he ever discusses. Prefer existing adult service speakers for the rumours rather than adding a gossip crowd. The player never has to repeat the allegation.

Give the man a boundary and a practical preference: he can end the conversation, direct where a bowl is placed or decline another question. The player can check on him, leave him alone or make an optional small kindness. Do not make basic food/water dependent on the player completing a daily chore. A persistent empty bowl or unanswered rescue plea would turn a light encounter into unresolved cruelty.

**Review gate before production:** decide whether the locked-hole staging needs an optional humane resolution, and what can be offered without establishing a crime or the original reason. This appendix does not invent a rescue quest, a sentence, a keeper, a secret escape route or voluntary confinement. If the scene cannot preserve that ambiguity without making the village cruel, revise its staging with the owner before recording. Do not settle the question by quietly making the lock fake.

## 5 Further small gags to choose from

Select at most two of these draft candidates; they are alternatives within the same small budget, not an accumulating quest list.

- **The claimed stool:** a hen has settled on a low service-yard stool beside a perfectly adequate roost. A resident gestures at it and uses the next stool instead. No chase, kick, animal harassment or required retrieval. One original dry line, then leave the tableau alone.
- **The one short table leg:** a garden tool catalogue or wooden offcut is already wedged under a wobbling outdoor table. The keeper silently swaps the wedge for a slightly different offcut; the table still has a tiny wobble. Reuse one hand-placement gesture and two rigid props. No physics simulation or endless noisy loop.
- **The formidable scarecrow:** a patched, inexpensive scarecrow stands beside the small beds while a hen rests on its boot. It says something about this village’s grand ambitions without requiring a line, moving cloth or a new actor.

Each gag should reveal ordinary competence mixed with stubbornness. Keep comedy away from Kit’s injury, a serious return conversation or major evidence. Kit may notice a nearby event only if physically present; no remote joke commentary. His resting place and later activities must suit the approved recovery staging, without specifying a healed limb or making him run errands.

## 6 Food water and waste

The patches are supplementary kitchen gardens, not implausibly the entire settlement’s food supply. Use a small palette of hardy greens, root vegetables and culinary herbs. Final crop species follow art/historical review; avoid committing the setting to an exact period from a crop choice. Two or three visibly tended beds, a harvesting basket and a repaired fence tell more than a large decorative farm.

Imply staple food through existing places: grain sacks and bins associated with the mill/store, covered roots and dried goods, a tavern stew pot and a bread board. The gardens provide greens; animals imply eggs and husbandry; stores and trade supply the rest. The mill processes grain rather than creating food. Broader fields and suppliers may be implied beyond the playable pocket without becoming new mapped farms. The keeper’s wish to reopen the road can matter without claiming all local food is cut off or adding a famine plot.

At this authored night, most hens should be settled and the pig resting, with only occasional alertness near the player. Feed sacks, straw, a trough and a small gate establish care. Keep a plausible dry human access gap and storage reach; do not require a full day schedule, eating simulation, breeding, butchery or player feeding mechanic.

**Water:** retain the central well as the visual drinking/cooking-water source, with covered clean vessels. Garden watering can use carried water and rain-collection props. A separate trough serves animals. A small basin and clean-water jug serve the privy area. Do not add a river pipe crossing paths, claim the millrace is potable, or assume a planning-map height proves sanitary drainage.

**Waste:** distinguish covered kitchen scraps/compost, animal manure storage and human waste. A draft dry-privy design with a covered removable receptacle and rear service access avoids requiring a sewer or a new cesspit beside the well. Keep servicing visually plausible but offscreen; no collection minigame. Human waste is not spread on the edible beds. Drainage, separation from the well and crops, flood exposure and service access are blockout checks, not safety claims certified by this note. If the tavern edge cannot accommodate them credibly, relocate within an admitted service pocket or defer the privy rather than move the well or a principal building.

The hole is dry, separately maintained and entirely outside waste handling. Do not connect it to the privy, drainage or pig pen for a punchline.

## 7 Provisional dialogue for review

All IDs below belong only to this appendix’s `BELL-LIFE-DRAFT-*` namespace. All lines are **draft; recording_approved=false**. No runtime IDs, authoritative-bank exports, TTS files or new line counts have been changed. Review and deliberately migrate selected lines later, with source revisions, knowledge/participant metadata and subtitle parity. The user-supplied privy demand is retained exactly as the requested concept; final casting and recording remain unapproved.

| Draft ID | Speaker and circumstance | Provisional spoken text |
|---|---|---|
| BELL-LIFE-DRAFT-001 | Unseen privy occupant; first eligible invitation | Can someone PLEASE get me some rag?! |
| BELL-LIFE-DRAFT-002 | Same occupant; optional direct interaction before delivery | The clean scraps. In the basket. No, I'm not coming out to point. |
| BELL-LIFE-DRAFT-003 | Same occupant; successful delivery | Thank you. A small mercy, but a timely one. |
| BELL-LIFE-DRAFT-004 | Existing resident; player requests local talk | Fell in, I heard. Though that doesn't explain the lock. |
| BELL-LIFE-DRAFT-005 | Different existing resident; alternate local talk | He lives down there, so they say. People will call anything a home if it keeps the rain off. |
| BELL-LIFE-DRAFT-006 | Existing resident; rare linked gossip exchange | There's a story about him and the pigs. I wouldn't repeat it. |
| BELL-LIFE-DRAFT-007 | Existing second resident; reserved immediate reply to 006 | You just did. And you don't know a thing about it. |
| BELL-LIFE-DRAFT-008 | Man in the hole; direct introduction | If you're delivering supper, the shelf's on the left. If you're collecting gossip, keep walking. |
| BELL-LIFE-DRAFT-009 | Same man; optional welfare interaction | Water's here. Blanket's dry. I'd welcome a conversation that wasn't about the hole. |
| BELL-LIFE-DRAFT-010 | Same man; optional conversation ending | That's enough questions for one evening. Leave me a bit of quiet. |
| BELL-LIFE-DRAFT-011 | Existing keeper; sees hen occupying service stool | That was my seat. Apparently we've settled the matter. |
| BELL-LIFE-DRAFT-012 | Existing keeper; sees the crop beds on request | These beds keep the pot green. We still need grain coming down the road. |
| BELL-LIFE-DRAFT-013 | Existing keeper; sees the table wedge fail to settle it | Better. If nobody leans on it. |

Use functional speaker labels until identities are reviewed: “Voice behind the door,” “Man below the grate,” and the established service identities. No new proper names are required. Lines 004–007 should not all play on first arrival; the man’s own interaction is available without hearing any rumour. Line 009 requires the listed props to be visible/present and is not evidence of consent to confinement.

## 8 Shared playback and state contract

Extend the established shared dialogue infrastructure. No separate gag audio player or per-prop callback system.

- Preserve the bank’s global limits: at least **90 seconds between optional barks**, at most **two optional barks in five minutes**, no immediate repeat; at least **180 seconds between ambient exchanges**. A proposed joke is not exempt because its speaker is offscreen. Default spontaneous scenes to once per save, with no repeat in the same visit.
- The privy demand consumes its spontaneous invitation on start/skip/interruption, persisted across reload and zone residency. It does not endlessly requeue if unanswered. A direct interaction can show the current favour state silently; there is no requirement to replay the shout.
- Treat the pig rumour and rebuke as one reserved two-speaker exchange with an order. Start only with both speakers present and eligible; never schedule the allegation as a free-floating ambient bark. If participants leave, cancel stale speech. Test interruption so the game does not routinely deliver only the insinuation; defer the whole exchange if this cannot be staged reliably.
- Essential story, Kit’s recovery/return scenes and useful service explanations outrank these optional exchanges; hazards outrank all speech. Suppress or duck incidental animal sounds during dialogue. Do not interrupt a major return with a privy shout.
- Revalidate speaker/listener presence, distance, current local state and priority at enqueue and playback. Kit needs actual presence and witnessed knowledge. Spatial sound comes from the closed door or hole, with bounded occlusion that stays intelligible.
- Spoken text and subtitles match exactly. Show readable, scalable top-safe-area subtitles and a speaker label; never require stereo hearing to locate or understand the scene. Use the shared Dialogue slider, pause/resume, cancellation and offline playback. Missing audio must not block interaction or favour completion.
- Preserve player movement, camera and the ability to leave. The favour’s delivered state follows an atomic world interaction, never a completed audio line. Restore world state before revalidating queued speech after reload; do not duplicate interaction rewards or resurrect completed tasks.

## 9 Bounded production packages

These are planning ceilings for a first asset review, not accepted engine capacity, generation requests or promises to deliver every candidate. First inspect suitable existing Briarhold/Horde sources and their rights. Reuse does not import another game’s character identity or establish native RT/mobile compatibility.

| Package | Proposed source and output | Rig animation and audio boundary |
|---|---|---|
| VL01 Privy and rag favour | One small closed modular privy shell, rigid latch/shelf or covered slot, one clean rag token, one basket; reuse timber/iron/cloth materials | No occupant model, reaching hand, cloth simulation or new rig. At most one rigid door/latch reaction if useful. Three candidate voice lines; one subdued cloth/wood effect. |
| VL02 Hole and resident | One small shaft/rim/grate assembly with dry ledge; reuse blanket, cup and bowl. One existing adult humanoid variant if suitable | Reuse a validated compatible humanoid rig; no assumption that Kit’s rig automatically fits. Seated/rest idle, look-up/talk gesture and return-to-rest, ideally adapted from shared clips. Three direct lines and up to four resident-rumour lines; no screams, restraint struggle or combat set. |
| VL03 Animal pool | One pig model and one hen model, reused for all admitted instances with small material/scale variations | One rig per species. Pig: rest/breath, short walk, sniff, settle. Hen: rest, short walk, peck/look. Use a small licensed/source-recorded sound pool with varied timing; no new locomotion solver or elaborate animal AI. |
| VL04 Food and sanitation kit | Two or three crop clumps, modular bed edging/pen, reused sacks, bins, trough, covered vessels, scraps/compost container and handwashing props | Static geometry and existing material palette. No growing mesh, wind simulation requirement, water fluid simulation, harvest economy or new dynamic light. |
| VL05 Optional extra gag | Choose at most two from stool hen, table wedge and scarecrow | Prefer the same hen and reused human gesture/props. At most two further voiced gag lines; scarecrow is silent. |

For a first stress-scene proposal, cap the complete pocket at **one pig and three hens visible**, with **no more than one pig and two hens actively animated at once**. Additional visible animals use deliberate settled poses or suitable static geometric LOD, not invisible removal. This is an adjustable test starting point, not an FPS guarantee. The human in the hole and existing nearby service actors still count against the hub’s combined animated/visible-character budget.

Use authored short activity points inside the pen/service pocket, simple idle/walk/settle state changes and no pathfinding across the town. Keep all activity away from the player’s required path and let the player pass without pushing animals. Decorative animals do not take damage, award loot or produce crime state.

The privy adds an unseen voice identity; the hole adds one visible supporting character. These must fit the reviewed **4–7 present/named village-character envelope**, including Kit where resident, or receive an explicit scope decision. Reusing an actor or voice performer does not make two fictional characters count as one. Prefer scheduling the small pocket over adding a crowd; defer candidates if the core service cast already fills the envelope.

At distance, use actual lower-cost geometry/poses and authored quiet states. Do not treat camera invisibility as RT irrelevance: shadows, reflections, transmission and audio can still expose a contributor. Freeze/swap only under a measured relevance policy and stable transition, with no conspicuous mid-step snapping, mismatched reflected motion or disappearing shadow. Share materials and clips where valid, use phase offsets rather than synchronized loops, and measure the cost of independent skinning/BLAS updates. Keep the genuine Vulkan RT presentation path and current render quality.

Retain editable Blender sources, rig/clip semantics, hashes, material maps, source/runtime separation and provenance/licence records for any later accepted asset work. Numeric triangle, bone, texture-memory and update budgets must come from the accepted combined 1.8 stress slice on the supported phone and Windows RTX target, not this paper estimate.

## 10 Acceptance and open review

Before making assets, choose the exact service-yard/garden pockets, verify sanitation and safe hole staging, confirm the supporting-cast count, and select or reject the additional gags. Confirm what the man can request and whether confinement requires an optional resolution. Approve exact dialogue, casting and rights separately from the general comic direction.

Later implementation acceptance must show:

1. All existing registered footprints, door access, dry paths and important views remain intact. No new mandatory interior, farm region or main-route detour is needed.
2. First arrival, Kit care and serious campaign returns remain clear and uninterrupted. Both dungeon orders, incomplete returns and the town-based Kit arc remain coherent.
3. Ignoring every joke/favour preserves all services, access choices and campaign completion. Rag delivery cannot consume progression/medical resources, duplicate reward or become stuck after skip, reload, zone change or missing audio.
4. The man has a voice and boundaries; no evidence confirms a crime or pig allegation. No accidental privy/hole/pen connection, implied waste dwelling, blocked rescue promise or animal cruelty is introduced.
5. Nighttime animals look tended, crop scale remains plausible, and food/water/waste props agree with each other. No unsupported agricultural or drainage simulation is implied.
6. The combined tavern-threshold, exterior, actor, lantern/fire, animal and RT-reflection workload is measured on the exact accepted candidate. Verify memory peaks, warm frame timing, skinning/acceleration-structure work, transitions and repeat visits without silently lowering quality.
7. Verify touch/controller/keyboard interactions, subtitle scaling, Dialogue/SFX balance, occluded speech, linked-exchange interruptions and shared cooldowns. Audio/haptic manual revalidation is **NO for this document**, and **required for the relevant later audio/interaction changes** under the project’s existing rules.

### Sources and integration boundary

Read against these repository sources:

- [Campaign design](CAMPAIGN_DESIGN.md), including its 7 October English-bank/Kit precedence note.
- [Connected maps](design/world/connected/README.md), [building register](design/world/connected/building-feature-register.csv) and shared coordinate layout, revision 2 of 5 October.
- [Village 1.8 plan](superpowers/plans/2026-09-11-village-hub-1.8.0.md), especially the provisional cast/interior envelope and combined stress-slice gate.
- [English dialogue bank](dialogue/en/README.md) and [playback contracts](dialogue/en/README-English-en.txt), candidate v0.2.1, for the authoritative 357-line bank, town-based Kit and playback contracts.
- [The Tale Thus Far](TALE_THUS_FAR.md), for the separate earned campaign ballad; ordinary village speech remains natural.

This standalone appendix adds no authoritative-bank lines and changes no source assets, runtime files, map data or registered buildings. Its content is saved for design review only.
