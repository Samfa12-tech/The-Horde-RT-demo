# The Horde — Approved World Layout

**Sam-approved geometry: 4 October 2026.** Canonical planning reference for routes, landmarks and their relative positions. Read with [CAMPAIGN_DESIGN.md](CAMPAIGN_DESIGN.md), [ROADMAP.md](ROADMAP.md), the [1.7 plan](superpowers/plans/2026-09-11-beyond-the-tomb-1.7.0.md) and the [1.8 plan](superpowers/plans/2026-09-11-village-hub-1.8.0.md).

The owner approved the overhead concept titled **The Horde — Proposed World Layout**. Its printed “Proposed” and “Concept for review” labels describe the image's creation stage; the topology is now approved. The artwork is **not to scale**. Exact distances, traversal times, slopes, building footprints, collision and camera compositions must be established through playable blockout testing. Do not treat illustrative perspective or decorative details as measured level geometry.

This is a connected regional plan delivered through bounded areas and measured zone/residency transitions. It does not promise a continuous seamless open world or require every visible region to be resident and explorable at once.

![The Horde - approved world layout, not to scale](design/world/the-horde-world-layout.png)

*Original concept caption: "The Horde - Proposed World Layout". Sam approved its geometry on 4 October 2026; it remains not to scale. The approved names The Veyrlands, Bellwether and King Veyr are recorded in this document but are not yet printed on the image.*

## Approved names and local history

**The land is The Veyrlands; the town is Bellwether** (owner selected 3 October 2026). The Veyrlands were once the heart of King Veyr's realm, now remembered more through warnings than history. King Veyr is the real historical king in the campaign; the lantern entity's claim to be the dead king remains a deception, not a second king or a renamed entity.

Bellwether takes its name from the church bell that guided travellers home through the forest mist. Local folklore says that some nights it rings without anyone pulling the rope. This is approved setting lore, not a locked supernatural explanation, scripted event or bell gameplay mechanic. The map's generic “Village hub” label still identifies Bellwether.

## 1. Fixed geography

North is up in the approved concept. Preserve these relationships when translating it into engine coordinates:

| Landmark | Agreed location and relationship |
|---|---|
| Keeper Tomb | Northwest, within an ancient abandoned burial ground beyond the woods. This is the existing prologue tomb, separate from the village's active churchyard. |
| Forest route | Leads southeast from the tomb through woodland to a lookout northwest of the village. Preserve the moonlit rescue/reunion and bounded forest chapter. |
| Lookout | The playable 1.7 endpoint. It reveals the future village below and establishes the same approach that 1.8 will extend. |
| Village hub | Compact, unwalled hamlet below/southeast of the lookout, on the western side of the stream/ravine. Arrival comes from the northwest. Its approved name is Bellwether. |
| Tavern and well | Central social focal point of the hamlet, surrounded by a small practical settlement rather than a fortified town. |
| Church and active graveyard | On the northern side of the village, distinct from the abandoned tomb burial ground. |
| Mill and stream | Eastern edge of the village; mill placement follows the watercourse and believable working access. |
| Gardens and smith | Gardens on the western side; smith toward the southeastern side, within the compact hub. Their detailed footprints and service implementation remain blockout work. |
| Starter shrine | A minor nearby spur beyond the northern/northeastern hub edge. It supports the small first-expedition direction, not a fourth major dungeon or required detour between the two middle dungeons. |
| Drowned Abbey | South/downstream in the low, flooded basin. The route descends toward water and the submerged approach. |
| Ashen Foundry | East, on a terrace across the gorge, reached through its broken-bridge/access problem. |
| Glass Court | Northeast, a ruined castle/palace on the high ridge, approached uphill through the Court gate. |
| Treasury | Under/behind the high ridge in a dry vault, associated geographically with the Court. It is not below the flooded Abbey basin. |

The watercourse begins at the northeastern upland headwaters and runs generally south, descending past the village/mill and through the ravine into the Abbey basin. Water direction, elevation changes and crossing geometry must remain coherent; a route line is not evidence that a steep slope or river crossing is already walkable.

## 2. Routes and progression gates

1. Existing tomb prologue → earned lantern → physical rope rescue → forest reunion and trail → lookout.
2. In 1.8, extend the same road from the lookout into the village. Preserve the village's established location and silhouette rather than moving it to accommodate a new level.
3. The village and nearby starter-shrine spur establish the useful hub and first small expedition. Exact shrine quest, dialogue and service implementation remain subject to their existing review gates.
4. From the hub/outbound junction, provide **peer branches** to the southern Abbey and eastern Foundry. Either may be completed first. Map placement does not prescribe a sequence.
5. The Court route remains locked until **both Abbey and Foundry seals** are acquired and the authored access interaction is satisfied. A visible distant Court does not imply early playable access.
6. Court → treasury/final confrontation → playable return follows the approved campaign. These later regions are not 1.7 or 1.8 deliverables.

Keep the existing alternative Magic, Tech and Constitution access directions. Abbey preparation concerns the submerged approach; Foundry access can use bridge repair, a maintenance climb or restored magical crossing. Each middle dungeon must be solvable with its own access solution, starting equipment and its local tool. The other dungeon's tool may open optional shortcuts or secrets only.

The drawn broken bridge is an access landmark, not approval of final span dimensions, engineering, traversal animation or a single mandatory repair build. Crossing alternatives must converge on the same Foundry and remain recoverable for mixed builds.

## 3. Milestone ownership

### 1.7 — Tomb, forest and lookout

- Deliver the accepted rescue/reunion and bounded moonlit trail, ending at the lookout
- Place a **visible low-detail village shell** in its fixed future location as distant scenery, using coherent building massing and landmarks
- Do not add playable streets, interiors, villagers/crowds, services or village quests to 1.7 merely because the shell is visible
- Use an honest chapter boundary at the lookout and the existing player-controlled continuing-story presentation; do not erect an enormous temporary wall across the future road
- Natural terrain, composition and clear chapter messaging must agree about the current playable limit
- Measure the shell together with forest, fog, Kit and lantern; simplified real geometry must preserve relevant visibility, shadows and reflections under the existing RT contracts

The earlier 40–80 m trail figure remains an initial blockout target, not an exact distance approved by this map. Choose traversal length and sightlines through playable testing.

### 1.8 — Extend the road and activate the hub

Continue the established road into the same settlement. Develop the distant shell into the scoped useful hub, with a hero tavern, selected interiors, a bounded cast, services/training and access preparation. Preserve the agreed geography while refining street/building details to measured performance, navigation and story needs.

The existing approximately 6–10 visible buildings, 2–3 interiors and 4–7 cast targets remain provisional authoring envelopes, not fixed minimums or certified capacity. Full later dungeons, population simulation and a general open-world system remain outside 1.8.

### Later campaign

Implement and validate the two order-independent dungeon branches, then Court, treasury and return through separately accepted scopes. No later version numbers or release dates are established by the map.

## 4. Protected prologue continuity

This regional geography does not reopen the four distinct approved dungeon features:

- Retain the small grated wall access panel just right outside the opening room, with the intended overgrowth and later 1.7 Kit call
- Close/remove the entry-room skylight in the queued 1.6.2 visual slice
- Leave the waterfall's own hole and vines untouched
- Fit the separate large waterfall-room skylight with the approved impassable iron grid and real RT bar shadows

Preserve the separate later-created finale/rescue opening. Kit does not know about the lich or knowingly wait for its defeat. The two existing skeletons move into the waterfall room in 1.7, not as a consequence of this map approval.

## 5. Reference custody and verification

Approved source image: **The Horde - Proposed World Layout.png**, PNG, 3,506,134 bytes. SHA-256: `c1ffd94e17fc12793519b6cd03fdbb93f41dc461c201e0bbcbb858d93fcfd95f`.

The original image is retained in the owner's Library. Sam supplied the local original `assets/the horde world layout map.png` and explicitly approved publication in public planning PR16. The exact supplied bytes were visually inspected and admitted at [docs/design/world/the-horde-world-layout.png](design/world/the-horde-world-layout.png) through the repository's PNG Git LFS policy. The verified file is 3,506,134 bytes with SHA-256 `c1ffd94e17fc12793519b6cd03fdbb93f41dc461c201e0bbcbb858d93fcfd95f` (also its LFS object ID). No image edits or regeneration were performed. This remains a documentation reference, separate from runtime assets; the topology above is the canonical contract.

Before later implementation acceptance, verify:
- Tomb and active churchyard remain separate; northwest arrival and fixed village position read clearly from the lookout
- River flow, terrain levels, mill access, Abbey basin, Foundry crossing and dry treasury are geographically coherent
- Both middle-dungeon orders work; Court access cannot bypass the two-seal gate
- 1.7 clearly stops at the lookout without presenting the shell as playable village content; 1.8 extends the same road
- Measured distances, climb/crossing routes, collision, saves/backtracking and RT-relevant residency agree with actual playable geometry
- Other unapproved names, detailed dialogue, encounter rules and numerical content/performance budgets remain provisional

**Documentation only.** No runtime implementation, new asset generation, paid work, merge, release or deployment is delivered or authorised by this update. Audio/haptic manual revalidation required: **NO**.
