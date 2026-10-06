# The Horde — Three Dungeon Chapters

**Planning update: 6 October 2026.** The owner has approved each dungeon's theme, local item and required item-to-boss relationship. The chapter packaging below proposes **1.9 Abbey, 1.10 Foundry and 1.11 Court**, after accepted 1.7 and 1.8 work. These version numbers are planning labels, not release commitments or a required play order. This is documentation only; no runtime implementation, new implementation PR, asset production, spending, merge or release starts here.

Read [CAMPAIGN_DESIGN](CAMPAIGN_DESIGN.md) for creative authority, [WORLD_LAYOUT](WORLD_LAYOUT.md) for regional geography, [ROADMAP](ROADMAP.md) for sequencing and shared engine constraints, and [DROWNED_ABBEY_PLAN](DROWNED_ABBEY_PLAN.md) for the detailed lake/crossing/water proof. Exact rooms, dimensions, encounter counts, timing, dialogue, upgrade balance and device budgets remain proposals until playable testing.

## 1. Shared dungeon contract

**Owner-approved scale, 6 October 2026:** a small access quest followed by the dungeon, with the structural clarity of Ocarina of Time at a smaller scale. Specific NPC reward effects and new gear quests remain proposals.

Every chapter delivers its **approach, access complication, complete dungeon, local item, taught puzzle chain, item-required boss, persistent seal and safe return**, rather than a disconnected boss room. Major features should serve environmental storytelling, player guidance or a meaningful action. Architecture, damage, tools and enemy duties should explain what this place was and what happened here. Decorative richness is welcome; tech-demo displays without a world purpose are not campaign content.

The playable grammar is: **observe a clear rule → acquire the item early → safe practice → escalating combinations → boss application → seal and return**. The boss tests the rule already learned in rooms. Possession alone is insufficient: the player must actually use the local item/light interaction to make the boss defeatable. No unexplained mandatory rule appears only in its arena.

| Dungeon | Approved local item and verb | Required boss dependency |
|---|---|---|
| Drowned Abbey | Placeable reflector: redirect light through shutters | Reflected light opens/exposes the Bellkeeper's armour; ordinary attacks alone cannot bypass the exposure requirement. |
| Ashen Foundry | Shuttered lantern stand: leave light in place and control illumination while moving | Maintain the learned light-operated mechanism and reposition to expose the Master of Coin's furnace shell. |
| Glass Court | Focusing aperture: isolate receivers and direct bounded mirror/ward interactions | Deliberately focus the correct receiver/ward sequence to break the keeper's protection and defeat the encounter. Defeat is required; death is not automatically required. |

These are persistent utility items distinct from the treasury seals. They do not replace the Keeper lantern, require a consumable purchase for every attempt, or become disposable one-room keys. Detail the placement, stowing, recall and reset behaviour in prototypes so an item cannot become permanently stranded beyond a closed gate.

**Progression protections**

- Abbey and Foundry are peer branches and must each work first. Each requires only starting equipment, one independently sufficient Magic/Tech/Constitution access solution, and its own local item. The peer dungeon's item, seal or unique upgrade is never mandatory. Cross-item secrets or shortcuts remain optional.
- Court requires both middle-dungeon seals plus its authored access interaction. Earlier tools may be combined because both chapters precede it; the focusing aperture retains its distinct required role.
- Main-route retreat remains viable before the boss and seal. A post-seal return shortcut is a reward and convenience, never the first safe way out. Spending, item placement, exposure failure, reload or a one-way door must not trap the campaign.
- Limited content releases must clearly communicate unavailable branches. Their shipping order must not be converted into a permanent Abbey-before-Foundry gameplay dependency. Verify both complete-campaign orders once both are present.
- Quiet spaces, optional discoveries and readable safe recovery belong between demanding sections. A secret cannot hide an indispensable item behind an optional upgrade.

## 2. Proposed 1.9 — Drowned Abbey: the drowned religious house

**Identity:** religious buildings submerged in a river-fed southern lake, broken towers visible before descent, bells beneath water and dead attendants still keeping their duties. Flood cause/date remain undecided. Predominantly dry, walkable rooms sit inside coherent sealed volumes or above the floodline. This is not an ocean, a swimming sandbox or a water-level-puzzle commitment.

1. **Approach and anticipation.** Leave Bellwether downstream, reveal the broken towers across the lake, then descend toward a clearly marked preparation/recovery point. Changing waterlines and interrupted religious circulation establish the old use of the buildings without explanatory clutter.
2. **Access complication.** Complete the short submerged crossing using a Magic breathing ward, Tech breathing equipment or finite trained Constitution breath. Each works alone with safe margins and recovery; no Foundry-only component. Guided versus bounded free steering remains a prototype decision. Both outward and return travel must be safe before the seal.
3. **Arrival and first lesson.** Reach a confirmed dry refuge. Demonstrate a shutter blocking a receiver, then acquire the placeable reflector early and redirect a simple light path in a safe room. Make source, reflector, shutter and response legible from the player's position.
4. **Escalation.** Combine placement, occlusion and navigation through the religious spaces; use dry/wet boundaries and bounded water-related views without making decorative caustics authoritative puzzle evidence. Later rooms add pressure from inhabitants or positioning only after the underlying rule is understood. Exact room order and counts remain blockout proposals.
5. **Boss examination.** The drowned armoured Bellkeeper is exposed by the same learned shutter/reflected-light rule. Create a readable opening, reposition and act; missed setups reset safely. The required reflector action cannot be replaced with raw damage or an inventory-presence flag.
6. **Reward and return.** Award one persistent order-independent seal and the approved evidence of plunder, including an object with a living claimant. Preserve first/second-dungeon variants of the lantern's admission. Open the proposed return shortcut while retaining the already-viable main route and useful hub return.

**Engine proof before content expansion:** shared item placement/recall and world-space light authority; vista → crossing → dry-room volume prototype; exposure and safe checkpoints; bounded lake optics, RT-relevant residency and combined phone/Windows measurement. The [Abbey staged gates](DROWNED_ABBEY_PLAN.md#5-staged-implementation-and-acceptance) remain the detailed authority, including an analytic-wave baseline before any conditional FFT research.

## 3. Proposed 1.10 — Ashen Foundry: labour, heat and the royal mint

**Identity:** mint/forge on the eastern terrace, cursed workers and machinery whose continued function expresses the royal bargain. Heat, smoke, working routes, abandoned stations and the movement of material should make the place understandable, not a collection of unrelated hazards.

1. **Approach and anticipation.** Follow the eastern route to the gorge and broken bridge. Let the foundry silhouette and interrupted industrial access explain the destination and obstacle before asking the player to solve it.
2. **Access complication.** Offer Tech bridge repair, a Constitution maintenance climb or a Magic crossing restoration as independently sufficient routes converging on the same entrance. Prototype footprints, fall/retry safety and return access. Do not require Abbey completion or its reflector.
3. **Arrival and first lesson.** Establish the difference between carrying light and leaving controlled light behind. Acquire the shuttered lantern stand early; in a safe setting, open/close its illumination and hold a simple mechanism active while the player moves elsewhere.
4. **Escalation.** Combine held mechanism state with cover, line of sight, shutters and navigation. If watchers use light/shadow awareness, teach that bounded rule before combining it with machinery. Heat/smoke hazards must agree with their visible footprint, offer viable recovery, and not require all three improvement areas at once. Exact machine cycles and watcher roster remain proposals.
5. **Boss examination.** The Master of Coin's articulated furnace shell is exposed through the learned stand-and-mechanism interaction. Leave/control illumination, move through cover and act on the exposed shell. Timing pressure may deepen a known rule, but cannot invent a new compulsory control or make unlimited damage a substitute for item use.
6. **Reward and return.** Award the other order-independent seal and evidence that a royal bargain bound the soldiers beyond death. The lantern's blame of the keepers must work whether this dungeon is first or second. Open a convenient return route and acknowledge the expedition through the useful hub.

**Engine proof before content expansion:** persistent placed-source ownership, shutter/receiver state, safe recall and reload; coherent moving machinery/collision and bounded hazard logic; truthful light-based watcher queries if selected; a combined machinery/actor/fire/smoke RT slice on phone and Windows. Reuse established systems rather than commissioning a separate foundry engine.

## 4. Proposed 1.11 — Glass Court: preserved beauty and bounded truth

**Identity:** a ruined mirrored palace and roofless winter garden on the northeastern high ridge. Beauty and disciplined architecture should make the wish to preserve it credible. Surviving lucid keepers can understand and oppose what the player carries.

1. **Approach and anticipation.** Climb toward the high-ridge Court gate with clear views back toward the connected region. Preserve the dry treasury beneath/behind this ridge, geographically separate from the Abbey basin.
2. **Access complication.** Both Abbey and Foundry seals are required before the authored gate interaction. Keep independent Magic, Tech or Constitution access solutions in the detailed gate/ascent design; their exact actions are still proposals, not three newly locked quests. Neither bypasses the two-seal condition. Ensure safe retreat and recovery.
3. **Arrival and first lesson.** Show why broad illumination is insufficient for a particular receiver pattern. Acquire the focusing aperture early; safely isolate a receiver and read its response. Teach the finite mirror/ward vocabulary without infinite recursion or invisible solution chains.
4. **Escalation.** Combine deliberate receiver selection, bounded redirection, occlusion and navigation through palace and garden. Earlier reflector/stand skills may contribute because both peer chapters are complete. The aperture must remain meaningfully necessary rather than merely checking that it is owned.
5. **Boss examination.** Defeat the keeper encounter by focusing the learned receiver/ward sequence to break protection, then applying established movement/combat. No brand-new boss-only optics rule. Required defeat does not decide the keeper's death: survival, dialogue and final staging remain authoring decisions.
6. **Reward and return.** Award the final seal and treasury location. The keeper provides the approved evidence that the real king helped imprison the entity. Preserve believable subsequent deception, the single authored ending and a viable return path; do not turn this chapter into a new alternate-ending choice.

**Engine proof before content expansion:** bounded aperture/receiver semantics and mirror/ward paths, inspectable feedback, coherent visible RT response across supported tiers, cross-tool save/restore, and the combined palace/garden/keeper workload. Court content must reuse the same fixed-step authority and source/blocker contracts as the earlier dungeons.

## 5. Delivery gates for each chapter

| Gate | Shared-engine work | Chapter/content evidence |
|---|---|---|
| 0 — Accepted baseline | Re-audit actual 1.7/1.8 foundations, tools, saves, input, actor limits and residency; identify gaps and measured budgets. | Approve approach, purpose of major features, item/boss dependency, protected canon and bounded room scope. |
| 1 — Recoverable greybox | Establish fixed-step access/item/receiver authority, reset and checkpoint rules. | Traverse approach, enter, learn the item and retreat using every permitted access route. Prove the local item cannot be stranded. |
| 2 — Representative combined slice | Measure lighting queries, relevant geometry/AS work, actors, effects, memory and zone transitions together. | One safe lesson and one escalation work across touch, mouse/keyboard and supported controller input. Visual cause and puzzle result agree. |
| 3 — Complete authored chain | Extend proven shared behaviour only where necessary; no hidden per-room simulation. | Complete the chapter with taught item use, boss, single persistent seal, story evidence, safe return and hub acknowledgement. |
| 4 — Acceptance | Exact-candidate Android and Windows regression, sustained frame pacing/memory and lifecycle evidence. | Test both peer orders, all access alternatives, every required item/boss dependency, optional routes, backtracking and earlier chapters. Owner review precedes any release proposal. |

Required recovery tests include dropped/placed/stowed item, recall/reset, missed solution, interrupted boss, death/retry, manual save where supported, checkpoint reload, pause, Android background/resume, repeated zone entry and already-earned reward. Check stale sources, duplicate items/seals, unsafe respawns and irreversible spending. Main-route retreat and post-seal shortcut are separate test cases.

Puzzle outcomes must not depend on screen brightness, temporal noise, render FPS, resolution or quality preset. Use bounded world-space queries agreeing with visible source, receiver, blocker and optical surface transforms. Set numerical light-path, mirror, actor, geometry, residency and performance budgets from actual target evidence before expansion. No diagram, asset count, isolated screenshot or desktop benchmark certifies a chapter.

## 6. Earlier milestone correction: the opening room in 1.7

**Owner direction, 6 October 2026:** the starting tomb room's leftover material collection is a tech-demo display and must be removed or recontextualized in **1.7**. Choose a deliberate story, guidance or interaction purpose for any retained pieces. This is a scene-composition/content change, not permission to delete material systems globally, erase shared source assets, or enlarge current 1.6.2 work. Preserve approved openings, spawn/traversal clearance, established prologue rewards and any separately maintained technical validation fixtures.

## 7. Planning status and custody

The local-item/boss relationships are owner-approved. Detailed room graphs, plan dimensions, approach complications beyond the approved alternatives, placement counts and technical implementations are draft designs. Keep maps and asset inventories visibly marked with that distinction. Reference artwork is not a production asset admission or a gameplay acceptance result.

**Validation boundary:** documentation reconciliation only. No new code, runtime art, generated audio/video, build, gameplay test or device measurement is claimed. Audio/haptic manual revalidation: **NO** for these documents; reassess affected implementation work later.

## Dungeon reference archive — 6 October 2026

The [R2 atlas and production register](design/world/dungeons/README.md) preserve the approach plans, three floorplans, item-to-boss learning chains, editable coordinates and planning inventory. The local item/theme/boss dependencies are approved; room geometry, metric distances, detailed encounters, asset quantities and device budgets remain proposals. Use static graph checks only as planning evidence, never as gameplay acceptance.
