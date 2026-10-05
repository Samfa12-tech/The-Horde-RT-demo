# 1.7 Asset Plan — Beyond the Tomb

**Planning inventory, 3 October 2026; tomb-dressing addition approved 4 October 2026.** This is a practical production list, not a claim that assets are generated, transferred, licensed for redistribution, imported or phone-validated. Follow the accepted 1.6.2 start gate in the [1.7 master plan](superpowers/plans/2026-09-11-beyond-the-tomb-1.7.0.md), [WORLD_LAYOUT](WORLD_LAYOUT.md), [CAMPAIGN_DESIGN](CAMPAIGN_DESIGN.md), [asset pipeline](ASSET_PIPELINE.md) and [engine contracts](AGENT_ENGINE_CONTRACTS.md). The master plan governs behavior; this file owns the sourcing/authoring checklist.

## Recommendation

**Reuse Horde first; assess Briarhold second; build the simple geometry in Blender; use Poly Haven for verified PBR inputs; reserve Meshy for a genuinely missing bespoke organic asset.** Image generation supplies selected concepts and decorative/UI inputs, not finished geometry, working controls or automatically seamless physically based materials.

The minimum complete chapter is one adapted Kit, one physical rope/rescue kit, a small reusable woodland kit, a curated tomb burial/decay kit, a coherent moonlit environment, one distant Bellwether shell, themed UI and the scoped audio set. It ends at the lookout. It does not require a populated town, new enemies, another lantern, another dungeon, a lake, a bridge or full campaign art.

Here, **minimum** means the smallest finished kit that fulfills the approved chapter, not permission to ship placeholders or silently remove required voice, animation, atmosphere or UI. Counts below are proposed authoring envelopes, not a certified runtime capacity or a fixed purchasing list.

## 1. Inventory status and evidence

Inspected the PR16 planning branch, its current asset tree/readmes, world/campaign documents and 1.7 contracts. The master plan separately records read-only 1.6.2 candidate inspection for existing growth, water and drench behavior; re-audit the actually accepted 1.6.2 build before production. Planning-branch files and older validation reports are not proof of the final baseline.

Also inspected the owner's Briarhold repository tree and relevant Warden, tree and Poly Haven provenance records. Exact owner-repository paths below are **technical candidate locators only**. No private asset bytes, private download links, provider task/account details or spending records are republished here. No binary model inspection, new runtime measurements or license-entitlement audit was performed by this inventory. Public Poly Haven asset/license pages were checked on 3 October 2026; candidates were not downloaded or admitted.

Status keys:
- **Reuse:** already in Horde's inspected inventory; preserve/re-audit accepted baseline
- **Candidate:** source located; rights, visual fit and native import still gated
- **Author:** new/adapted output required after production start/art gates
- **Optional:** proposed investigation, not required shipping scope
- **Later:** outside 1.7

### Existing Horde assets to retain

| Inventory | Verified location / baseline | 1.7 action |
|---|---|---|
| Dungeon masonry, floor, moss, damp ground, metal PBR | `assets/textures/polyhaven/mobile_1k/`: `medieval_wall_02`, `cobblestone_floor_08`, `mossy_stone_wall`, `damp_sand`, `rusty_metal_04` | Reuse on shaft, tomb rim, ruins, boundary stones and appropriate shell surfaces. Do not buy five duplicate material sets. |
| Player/body and held equipment | `assets/models/player/runtime/`, `assets/models/weapons/runtime/`, `assets/models/props/runtime/`; held-item and prop texture manifests | Retain player, sword, chest and reward lantern identity. Reuse the accepted 1.6.2 player hand torch: [the 5 October replacement direction](IMPLEMENTATION_GOAL_1_6_2.md#players-disposable-rag-torch--owner-direction-5-october-2026) specifies a rough wooden stick with a charred rag head, superseding its earlier ornate appearance while preserving behavior. Keep Keeper lantern and lich wall torches distinct. Extend required carry/climb/run poses and sockets; do not make new copies of the equipment to solve animation. |
| Existing enemies | `assets/models/enemies/meshy/`, corresponding metadata/textures | Retain the same two skeletons and lich. 1.7 relocates guards; their earlier 1.6.2 polish is not a new asset batch here. |
| Collapse and four distinct openings | Accepted 1.6.2 scene plus [collapse plan](COLLAPSED_ENTRY_1_6_2.md), world-layout continuity contract | Reuse accepted collapse, small wall grate/overgrowth, removed entry skylight, waterfall hole/vines and separate barred skylight. Author only genuinely missing rescue-rim/shaft extensions for the separate finale opening. |
| Waterfall, catchment and runnel | Existing real geometry and bounded material transport, described in master-plan §6.1 | Reuse. Contact effects do not require replacement water assets or a new stream. Wet stone alone is not a puddle. |
| Existing footsteps/UI/combat audio | `assets/audio/filmcow/player_step_1.wav`, `player_step_2.wav`, skeleton steps and other existing clips; `assets/audio/pixabay/` | Audition existing material before recording replacements. Preserve existing licence/derivation records, event ownership and quieter player-footstep balance. |
| Existing branding and themed UI direction | Current project art and [1.6.2 UI plan](UI_REFRESH_1_6_2.md) | Inspect the accepted UI before producing another theme. Extend its original visual language across new 1.7 states. |

The existing material README records five-layer 512-pixel arrays and strict Android ASTC derivatives; those figures describe that existing batch, not a budget allocation for the new chapter.

## 2. Briarhold reuse shortlist

### A. Kit: strongest first candidate, not yet final selection

- Runtime: `assets/meshy/runtime/briarhold-warden-1k.glb`
- Sources: `assets/meshy/source/briarhold-warden-tpose.glb`, `briarhold-warden-rigged.glb`, `briarhold-warden-walking.glb`, `briarhold-warden-running.glb`
- PBR source directory: `assets/meshy/source/briarhold-warden-tpose_textures/`
- Provenance/preparation: `assets/meshy/provenance/briarhold-warden-meshy.json`, `tools/prepare-warden.py`

The record reports 25,968 triangles, one mesh/material, four 1K runtime textures, 7,034,964 file bytes and a 1.8 m height envelope. It lists idle, walk, run, jump, fall, slide and mantle, with horizontal root motion removed and named hand/equipment sockets. The master plan's earlier inspected candidate records a 24-joint skin and a mask covering nose/mouth. These are source/candidate facts, not Horde runtime cost.

**Use:** evaluate this masked practical silhouette as Kit; keep Kit's separate identity. Approve any material/clothing changes in a small reference review. Existing walk/run/idle can save work; jump/slide clips do not imply new player mechanics or solve rope acting.

**Missing work (updated 4 October 2026):** approach/turn, restrained listening/concerned talk, lantern reaction, starts/stops and wait/look-at behavior; look-down only if the final staging needs it. The [Kit animation plan](KIT_ANIMATION_PLAN_1_7.md) owns the candidate clip shortlist, free-source alternatives, Blender workflow and separate player-motion appendix. Reuse or blend existing actions where convincing. Kit can idle beside the fixed rope anchor; no throw, hand-release, recovery, holding clip or release marker is required. Deployment is a one-time runtime event during lantern pickup. Masked speech can use restrained head/body performance; a full phoneme/lip-sync system is not needed. Player climbing/carry poses and physical rope acceptance remain separate required work. Eric handles animation choice and technical checks; Sam reviews the result in game.

**Gate:** verify original generation entitlement, input rights and required attribution/distribution route, then source hashes, scale/axes, skin/weights, exact clip semantics, textures and native RT importer compatibility. The inspected record names the owner but does not itself prove transferable/public redistribution entitlement. Retain a private audit trail and publish only the appropriate shipping notice. No new Meshy job is needed unless reuse fails this gate or owner art review.

### B. Forest tree: useful source, needs Horde-specific LODs

- Runtime: `assets/meshy/runtime/briarhold-forest-tree-512.glb` and `briarhold-forest-tree-256.glb`
- Source: `assets/meshy/source/briarhold-forest-tree-meshy6.glb` and adjacent `_textures/`
- Record/script: `assets/meshy/provenance/briarhold-forest-tree-meshy.json`, `tools/prepare-forest-tree.py`

The recorded 512 derivative has 8,000 triangles, one mesh/material, 512 maximum texture size, 1,774,316 file bytes and roughly 5.2 × 9 × 5.2 m bounds. A previous 2K-triangle derivative was rejected for detached branch/moss islands. Do not repeat blind decimation or assume the 256 texture tier also lowers geometry.

**Use:** one of approximately four woodland archetypes, plus carefully authored variations/LODs. **Gate:** retain silhouette/connectivity, retune scale, confirm leaf alpha/material behavior in all relevant ray paths, create simple collision and measure instance/BLAS cost. Briarhold's tree density/performance cannot certify Horde's RT forest.

**Do not reuse** `assets/world/briarhold-forest-impostor-512.webp` as the forest: Horde requires actual 3D trees/3D distant silhouettes.

### C. Material inputs: strong reuse candidates

`assets/textures/polyhaven/provenance/{forrest_ground_01,wooden_planks,castle_brick_01}.json` and matching `source/` and `runtime/` directories exist. Their public original asset pages are the source of licence authority. The forest-ground record explicitly identifies CC0 1.0, diffuse/OpenGL-normal/roughness sources and 256/512 WebP derivatives; packed ORM uses constant AO=1 and metallic=0.

Reuse suitable **source maps**, then derive Horde's supported texture arrays/KTX2; do not treat Briarhold WebP files as drop-in native runtime assets. New brick is optional because Horde already has masonry. Source roughness/normal maps are useful even if a higher-quality source is selected later.

### D. Located but not a reason to expand 1.7

Briarhold has `assets/meshy/runtime/briarhold-wall-bay-512.glb`, `briarhold-watchtower-512.glb`, `briarhold-west-gatehouse-512.glb`, `briarhold-courtyard-service-arcade-512.glb`, `briarhold-wave-bell-512.glb` and associated records. These are inventory-only candidates, not inspected/accepted Horde imports. Fortress walls/gatehouses are a poor default for **unwalled Bellwether**. Simple Blender shell massing is preferable. A distant bell/building detail should be modeled only if it changes the lookout silhouette.

Hub-NPC and boss GLBs also exist, but townsfolk, Bellkeeper adaptations and other campaign enemies belong to later scoped batches. Briarhold moon/storm-sky images are LDR art candidates only, not calibrated moon lighting or a ready-made HDR environment. Do not copy its flame sprite or first-person image viewmodels to replace Horde's world-space RT fire/equipment.

## 3. Concrete production register

Destination names are proposed within the master plan's directories; reconcile them with accepted manifest conventions before authoring. `source/` below means `assets/source/beyond_the_tomb/`; runtime model base is `assets/models/`. Each row includes its own acceptance focus in addition to the common gates in §7.

| ID / priority / phase | Asset and bounded authoring envelope | Preferred route and proposed destination | Material / motion requirements | Acceptance and budget status |
|---|---|---|---|---|
| A01 · P0 · WP0–1 | 3 direction studies: forest reveal, Kit adaptation, HUD/menu | Imagegen/reference editing after inspecting actual references; `source/reference/` | Label concepts; coherent phone/desktop crops, no generated UI text | Small owner art gate; at most two candidates/category in master plan, not automatic paid authority |
| A02 · P0 · WP2/5 | Rescue kit: one rim/shaft extension, displaced Keeper gravestone, anchor/tie-off, simple collision set | Blender; `environment/tomb_exit/`, `source/tomb_exit.blend` | Existing stone/metal; tested stone travel, rope anchor and ascent/mantle/descent clearance; fit F01 to the accepted tomb | Preserve all distinct openings; real look-down continuity; static costs unmeasured |
| A03 · P0 · WP5 | One rope mesh/material, optional static starting coil | Blender tube/UV source + engine deformation; `environment/rescue_rope/` | Rope fibre base/normal/roughness, dielectric metal=0; fixed topology; coil must not duplicate deployed rope | Stable UV/tangent/diameter; live load/contact/release; solver/BLAS budgets measured, never Blender-cache physics |
| A04 · P0 · WP6 | One Kit actor + required action set | Briarhold Warden first, Blender adaptation; `npcs/companion/`, `source/kit/` | Base, normal, packed ORM; emission only if genuinely needed; inspect 24-joint candidate; missing gesture clips above | Rights, owner look, native skinning/import and scene-event integration gates; source dimensions/counts known, Horde cost unknown |
| A05 · P0 · WP5/6 | Player ascent/pull-up, separate descent/landing, two-item stow/restore and walk/run poses | Existing player source/rig + Blender; retain current runtime lineage | Two free hands; visible sword and lantern carry attachments; real-height root policy; physical lantern/emitter moves together | No rim/rope/body clipping, duplicate/drop/delete or phantom hand light; grounded restore, safe limited-look/reduced-motion camera; measured skinning updates |
| A06 · P0 · WP4/8 | One 40–80 m initial trail/blockout, terrain/bank segments, lookout and simple boundary collision | Blender, authored placement; `environment/forest/terrain/`, `assets/scenes/beyond_the_tomb/` | Ground/leaf-litter material, existing damp/moss/stone, authored support/surface tags | Distance provisional; walking/running route test; no new bridge or empty travel padding; scene residency measured |
| A07 · P1 · WP8 | Approximately 4 tree archetypes, shared 3D LOD family | Briarhold tree + Blender variants/new simple trunks; `environment/forest/trees/` | Bark and leaf material; inspect alpha versus opaque leaf clusters; bounded optional wind weights/buckets | No billboards; primary/shadow/secondary silhouette agreement; RT instance, overlap and refit measurements before density increase |
| A08 · P1 · WP8 | Small rock/root kit: propose 3 rock forms, 2 root/log forms, 1 stump | Blender first; selected Poly Haven model if better; `environment/forest/dressing/` | Existing moss stone + bark; shared maps; static meshes, simple proxies | Avoid duplicate high-poly scans; silhouette/LOD/collision gate; counts are reusable masters, not placed instances |
| A09 · P1 · WP8 | 2 fern/low-plant clumps and one restrained ground-debris family | Blender; imagegen only reference or carefully cleaned leaf input; same dressing directory | Leaf base/normal/roughness/opacity only if importer supports validated cutout; no painted shadows | Low alpha overlap and no ground carpet hiding traversal; no independent plant rig required |
| A10 · P1 · WP8 | 2–3 ruin/burial markers and one waymarker/clue surface | Blender using existing masonry; `environment/forest/ruins/` | Original authored inscription if approved; readable relief/material response to lantern | Does not reveal campaign twist; distinguish burial ground from active churchyard; exact clue content remains review |
| A11 · P1 · WP3/8 | One night-environment definition; visible moon, star/sky input if needed | Analytic/native first; optional Poly Haven HDRI/reference; `assets/scenes/beyond_the_tomb/night/` | Moon angular direction and actual RT emitter aligned; no material rig | Same night inside/outside/glass; no double-counted moon energy or dawn; source resolution/decoded bytes chosen after tests |
| A12 · P1 · WP3/8 | Bounded mist density descriptors and firefly geometry/emission input | Native world-space effects, minimal Blender mesh if useful | Real geometry occlusion, bounded emissive/light contribution; reuse volume/fire infrastructure | No light-shaft pictures or screen particles; evaluate with Kit, lantern, trees and shell combined |
| A13 · P1 · WP8 | One Bellwether distant exterior shell; use roughly 4–6 reusable building masses to compose needed landmarks | Blender modular walls/roof forms; `environment/bellwether_shell/` | Existing stone plus wood/roof/plaster only where visibly useful; closed exterior volumes | Place future church north, mill east, central tavern/well as silhouette needs; no playable interiors/crowds; fixed future footprint; RT visibility measured |
| A14 · P1 · WP7/9 | One UI kit: heart full/empty states, action/run states, frames, buttons, focus/disabled states, Save/Load/New Game slot presentation | Clean vector/native geometry; imagegen direction/decorative input only; `assets/ui/gothic/` | Proposed three slots share one reusable panel; text dynamic; no three separate save illustrations required | Large-font/inset/DPI and bright/dark readability; accessible health text; active-run state clear; asset memory unmeasured |
| A15 · P1 · WP6/8 | Kit offline voice line bank and subtitle/gesture manifest | Authorized recording/voice provider after script and voice choice; `assets/audio/dialogue/` | Current stable IDs, one consistent voice; source lossless, normalized runtime mono; no player spoken lines | All mandatory lines audible/offline; intelligible grate call; voice rights/cost approval unresolved; final line count follows reviewed script |
| A16 · P1 · WP8 | Surface steps, restrained tomb drips/grit/stone creaks, rope movement, forest ambience and scoped adaptive score inputs | Existing licensed banks/accepted audio system first; record/source only missing cues; `assets/audio/beyond_the_tomb/` | Initially 2–3 variants per admitted step surface; stone/dirt/leaf-litter/wood/water only where present; bounded loops | Existing stone steps audition first; walk/run share contact authority; wet step/effect one event; mono, mute/pause/voice balance review |
| A17 · P1 · WP4/8 | Tomb burial/decay kit: selected real niches first, then a few funerary props, webs, rubble, moisture/crest detail; breakdown below | Horde/Briarhold inspection first, then Blender; `environment/tomb_dressing/`, `source/tomb_dressing.blend` | Shared existing surfaces/compact detail atlas where suitable; static geometry, measured 3D LODs and simple collision; A16 owns sound | [Master §4.1b](superpowers/plans/2026-09-11-beyond-the-tomb-1.7.0.md#41b-burial-purpose-tomb-dressing-and-restrained-decay); no all-wall rebuild, unverified reuse claim or current 1.6.2 runtime work; mobile/RT/clearance gates remain open |
| O01 · Optional · after rope proof | 1–3 initial reachable reactive vine strands, attached sparse leaves | Blender curve/tube source using rope infrastructure; `environment/forest/reactive_vines/` | Bark/vine maps from shared materials; live constrained mesh, no new climb mechanic | Fixed anchors/contact, legal BLAS update and bounded strand/node count; retain static dressing if not admitted |
| O02 · Optional · stage A/B | Water-contact ripple parameters; one reusable droplet mesh/pool if stage B earns admission | Native effects + Blender tiny reusable mesh only if needed; `assets/scenes/beyond_the_tomb/water_contact/` | Real water-hit normal disturbance; physically placed bounded droplets, no splash-sheet overlay | Contact bounds, no dry-floor splash/double torch failure; transparent transport/update costs measured |
| O03 · Optional | Healing flask/food/rest prop | Decide minimal recovery slice at WP0; Blender or existing prop first | Only if admitted recovery needs a visible object; small shared PBR set | No entire health economy, upgrade inventory or floating heart pickups; no art commissioned before mechanic choice |

### Tomb burial and decay kit (A17)

This is the production breakdown for the [approved burial-purpose direction](superpowers/plans/2026-09-11-beyond-the-tomb-1.7.0.md#41b-burial-purpose-tomb-dressing-and-restrained-decay), not six additional work packages or six new paid jobs. Counts are small reusable starting envelopes; placed density and exact dimensions depend on the accepted scene and measured phone budget. No dedicated tomb niche/skull/funerary/web asset has been verified by this addition.

| Component / order | Bounded first kit | Production route and acceptance focus |
|---|---|---|
| T01 · first | One shallow recessed wall-bay family with stone lintel/shelf; intact, empty and broken/occupied variants | Inspect accepted wall construction; author compatible modular bays in Blender. Real cavity depth, wall thickness and RT shadows; no dark decal over a flat wall, adjacent-room leak or whole-tomb wall rebuild. |
| T02 · first | A few reusable skull/bone pieces and composed niche clusters, varied by placement and deliberate empty space | Inspect Horde/Briarhold source availability, rights and fit first; clean/derive suitable geometry in Blender or author a small replacement set. Existing animated enemy models are not automatically reusable clean static bone props. Preserve silhouette/normals/scale and share a modest bone surface. |
| T03 · supporting | One displaced sarcophagus-lid family, one urn-fragment family, offering bowl and candle-stub variations | Blender simple modular forms with shared materials; adapt verified props only where useful. Rest on credible supports, keep route/combat clear and use visible solid extent for any required collision. No new loot or breakable-physics system. |
| T04 · supporting | A few reusable mortar/stone-chip clusters and a rare larger broken block/boulder derived from the same masonry | Reuse suitable source shapes or author in Blender; place below matching wall damage/collapse. Small detail remains nonblocking; large solids have simple matching proxies. Prefer shared meshes/LODs over many unique rubble scans. |
| T05 · restrained experiment | One small web family for unused niches/high corners, plus a torn passage-edge variation | Blender sparse geometry or a validated shared cutout atlas. No opaque backing sheet or player-blocking web collider. Measure thin-detail stability, alpha/any-hit/transparency and pixel/overlap cost in primary, shadow and supported secondary rays; reduce representation/density if the gate fails. |
| T06 · detail layer | Shared tide/mineral-stain and worn Keeper-crest/burial-seal/offerings motifs; selective moss from suitable existing surfaces | Use existing stone/damp/moss/metal inputs first; derive normal/roughness/material masks in Blender. If genuinely missing, select a suitable Poly Haven CC0 stone/wet-stain base only after verifying the exact files, licence and fit. Optional imagegen original crest/decal concepts need cleanup and import checks; no new decal engine or lore/key mechanic is implied. |

Use actual geometry for recesses, lids, skull/bone silhouettes and projecting breaks; use normal/roughness/material detail for fine weathering and worn carving. Share a small number of reusable meshes/materials and atlas motifs where appropriate, with explicit per-LOD, placement, BLAS/TLAS and texture/mip accounting. Respect plausible water/light exposure and progressively richer funerary detail toward the Lich rather than filling every surface.

A16 supplies the sound layer through the existing engine: audition licensed drips, grit/stone-step variation and sparse stone creaks before sourcing missing clips. Preserve actual foot-contact ownership, quieter player steps, bounded event/voice rates, pause/mute and enemy/dialogue intelligibility. No available clip or new surface category is assumed by this list.

Horde/Briarhold reuse requires provenance, entitlement, visual fit and native import checks. Poly Haven additions are conditional and individually verified; none is selected or downloaded here. Meshy is reserved for a genuinely unresolved asset gap after reuse/Blender review, with separate approval for costs; simple architectural niches and funerary forms do not require generation. This documentation authorises no asset generation, transfer or spending.

### Count summary

The proposed visual register is **17 core work packages**, plus **3 explicitly optional investigations/slices**, not 20 new models. A17's component rows are one small tomb-dressing kit, not extra work packages. It intentionally mixes reuse, authoring, UI, data and audio work because all are necessary production dependencies.

A useful initial geometry envelope is one Kit, one rope/rescue kit, approximately four tree masters, a small dressing kit (three rocks, two root/log forms, one stump, two plants), two or three ruin markers, a waymarker, terrain/lookout, four to six simple shell-building masses and A17's small modular tomb-dressing kit. Placements and LOD files are not counted as distinct designs. Scene density remains unset. The shell does not inherit 1.8's provisional building/interior/cast counts as a 1.7 quota.

## 4. Texture and source shortlist

Start with **four new shared surface families** only where existing maps cannot serve: forest floor, bark, foliage and rope. Add wood and a roof/plaster family only if the final shell or near-route objects need them. Reuse existing stone/metal and Kit's own PBR maps. A17 reuses suitable masonry/damp/moss inputs, with a small shared bone/prop surface and motif/weathering atlas only where inspection shows a gap; it does not automatically add a high-resolution texture set per prop. Wetness can modify an existing physically appropriate material; it does not require a separate duplicate “wet” set for every object.

Maps: base colour (sRGB), tangent-space normal (linear, validated orientation), roughness and metallic (linear), AO only where supported/appropriate, packed ORM according to the engine's actual channel contract. Opacity and emissive are optional, justified per asset. Displacement sources can help Blender bake geometry/detail but do not imply runtime displacement support. Normal/AO/material-detail baking is allowed; baking moon/lantern lighting, world shadows or reflections is not.

Official examples checked for selection, not downloaded/admitted:

| Candidate | Proposed use / reason | Decision |
|---|---|---|
| [Forest Ground 01](https://polyhaven.com/a/forrest_ground_01) | Forest floor; source already represented in Briarhold | First reuse candidate; avoid duplicate acquisition |
| [Bark Brown 02](https://polyhaven.com/a/bark_brown_02) | Shared authored trunk/root bark | Compare against existing tree maps; only add if it improves consistency |
| [Wooden Planks](https://polyhaven.com/a/wooden_planks) | Near-route wood or shell detail | Conditional; a wood sound category alone does not justify a wooden bridge |
| [Mud Forest](https://polyhaven.com/a/mud_forest) | Optional damp trail variation | Second choice after existing damp ground/forest material; not a mandatory extra set |
| [Rock Moss Set 01](https://polyhaven.com/a/rock_moss_set_01) | A few authentic rock forms | Select a small subset, clean/LOD/collision in Blender; high-resolution originals are not mobile-ready |
| [Tree Stump 01](https://polyhaven.com/a/tree_stump_01) | Optional distinctive stump | Use only if superior to a simple authored/reused stump |
| [Moonless Golf](https://polyhaven.com/a/moonless_golf) | Night HDRI reference/candidate | Inspect captured horizon/lights and exposure before use; analytic night may fit better; does not replace the physical moon |

Poly Haven's [asset licence](https://polyhaven.com/license) identifies its HDRIs, textures and models as CC0 and permits commercial use/redistribution. This does not grant reuse of its website copy, logos or preview/example renders as game art. Record exact asset URL, selected original files, retrieval date, licence snapshot and hashes on admission. These official pages are a bounded shortlist, not a direction to bulk-download a library.

No representative rope/foliage download is selected yet. Author a tileable rope material and original foliage in Blender or select a verified source later; do not label an untested imagegen output “seamless PBR.” Check colour seams, normal continuity, mip behavior and real lantern response.

## 5. Tool decision rules

- **Blender:** default for terrain, rescue rim/shaft, anchor/rope mesh, roots/vines, modular burial niches/funerary props/rubble, collision, LODs, simple buildings/ruins and geometry cleanup. Also rig repair, missing Kit/player actions, UVs and detail bakes. Preserve editable source and reproducible exports.
- **Briarhold:** saves the most work if Warden and tree pass. Reuse preserves original provenance; it does not import Briarhold lore, runtime engine code, phone certification or a right to redistribute without checking entitlement.
- **Poly Haven:** useful for coherent natural PBR and selected scans. Source quality is not a runtime budget. Crop/derive/LOD legally and record transformations; don't package 8K source archives into the game.
- **Meshy:** reserve for a bespoke hero organic prop/character only when reuse plus Blender is insufficient. No required new generation for simple rope, stairs, shell houses, rocks or foliage. Generation/rigging/texturing job costs and rights need explicit approval; this plan authorizes no credits. Texture and validate before export.
- **Image generation:** limited visual exploration, original UI ornament/reference, optional original Keeper-crest/decal concepts and carefully checked sky input. Clean/vectorize icons; render real text in UI. Reference sheets may be inconsistent and require inspection. Generated art is not runtime evidence or physically calibrated illumination.
- **Native engine:** physical rope/vines, moon illumination, fog, water contact, playback, input, UI behavior, saves and actual RT transport. Asset art cannot stand in for these systems.

## 6. Critical path and procurement order

The master plan's [six-gate outdoor readiness contract](superpowers/plans/2026-09-11-beyond-the-tomb-1.7.0.md#131-six-gate-outdoor-readiness-contract) is the shared production gate. Complete its cheap lookout/town/terrain composition blockout, combined worst-view/transition scene, real near/mid/far geometry/material LOD proof, separate memory/ray/light relevance policy and measured Android/Windows numeric budgets before bulk detailed asset production. A source model, simplified shell or tentative count does not establish runtime capacity. Gate 6 carries these contracts into the later 1.8 slice; do not duplicate the programme here.

1. **Re-audit accepted 1.6.2 and freeze the dependency list.** Reuse accepted dungeon/collapse/lich/skeleton/equipment/UI work. Keep runtime development behind the existing milestone gate.
2. **Settle Kit reuse and the small art-direction review.** Rights/import/skin test before texture polishing or voice production; author a neutral idle/walk/turn test scene and fixed-anchor rope-deployment prototype independent of Kit's animation. A failure here affects the rescue more than optional dressing.
3. **Block out rescue and route with real scale/collision.** Prove rope endpoint, mantle, actual world height, walking/running and shell sightline before detailed art. Establish the terrain's actual surface categories.
4. **Measure a representative combined scene.** One Kit, representative trees, raised lantern/glass, moon, mist, fireflies and Bellwether shell, with rope/wind where applicable; include the worst view and transition/backtrack. Prove actual 3D geometry/material LODs and separate RT relevance/residency, then allocate geometry, materials, instances and memory/upload peaks using numeric budgets derived from sustained Android/Windows evidence.
5. **Finish the core kit and motion.** Prove A17's burial niches first, then its restrained supporting props/decay within measured tomb and combined-chapter budgets. Blender authoring and per-asset validation; selected texture derivation; native RT round-trip. Commission only the missing approved source material.
6. **Finalize Kit script/voice, surface sounds and UI states.** Audition existing clips, approve final recording copy/voice, then bounded licensed production. Save slots require state/readability work, not large artwork.
7. **Add optional vines/water stages only after their individual and combined gates.** They do not delay a complete required chapter unless explicitly promoted to scope.
8. **Accept the exact packaged chapter.** Matched phone/Windows motion/capture/audio tests, source/runtime hashes and documented unresolved gaps.

Main risks: Kit importer/rig and entitlement; physical-rope animation/contact coupling; forest alpha/instance costs; moving geometry acceleration-structure ownership; fog+glass+moon consistency; final art exceeding blockout cost; voice rights/intelligibility; importing a fortress-looking town shell that contradicts geography.

## 7. Common acceptance and budget gates

Each admitted asset needs:
1. Source/licence/provenance record, immutable source/runtime hashes, editable source where relevant and a small manifest naming intended role, dimensions and dependencies. Unknown rights remain a blocker.
2. Scale/axes/pivot, finite bounds, UVs/tangents/normals, material slots, alpha modes, texture colour space and missing-file validation. No hidden source geometry or unwanted embedded texture duplicates.
3. For actors: skeleton/weights, clip names/durations/loop boundaries, root-motion policy, feet/contact stability, sockets and semantic event markers. No whole-scene root motion masquerading as gameplay.
4. Runtime triangle/vertex counts **per LOD**, texture dimensions and mip/decoded/packed bytes, material count, unique BLAS data, placed/TLAS instances, dynamic update workload and loading/residency peaks.
5. Existing Android strict ASTC/KTX2 and supported Windows route, exact manifest/format validation and visual comparison against source. A 256/512 WebP label or a small GLB file is not a GPU-memory or phone-performance result.
6. Primary, shadow and important reflected/transmitted-view review, including alpha foliage, Kit, moving rope/vines and water where relevant. No baked lighting, billboard forest, screen-space fake contact effects or silently reduced quality.
7. Matched baseline/candidate evidence on actual Android and Windows hardware: build/preset/render dimensions, warm frame-time/pacing, resource peaks, transitions and sustained behavior. Establish acceptable thresholds before admission from that evidence. No invented universal triangle, texture, GPU-time or hardware ceiling.
8. Owner art/motion and change-triggered audio/touch acceptance on the exact candidate. For A17, inspect burial-first hierarchy, clear walking/combat routes and prop collision, real niche depth, plausible moisture/damage and measured web cost; A16's tomb sounds must preserve enemy/dialogue readability. Blender renders, CPU tests and one older device pass are not live RT or current phone proof.

Where the asset-vendoring tooling is used, retain the verified canonical package/hash roster, licence and validation result, perform a dry-run against the exact destination, obtain the required admission authority and retain the lock receipt. This planning document neither admits a package nor authorizes an adapter install. Verify copy integrity separately from importer/rendering/artistic acceptance.

## 8. Deferred campaign art

**1.8:** develop the same Bellwether shell into selected playable streets/interiors, hero tavern and bounded cast/services. Reassess appropriate Briarhold NPC/prop candidates then; do not create them now. Detailed interiors, shop stock, active mill machinery, crowds and all hub voice lines are deferred.

**Later:** starter-expedition content beyond its approved milestone, Abbey/Foundry/Court/treasury environments and bosses, advanced tool props, broad upgrade/healing economy, full narrative voice bank and return-state variants. Their thematic direction is in CAMPAIGN_DESIGN, but they are not hidden dependencies of the 1.7 asset pack. Optional RTAO/RTGI is an engine investigation, not another asset-acquisition batch.

**Delivered by this update:** documentation and source-candidate verification only. No asset generation, bulk download, transfer, spending, runtime edit, test execution, merge, release or deployment. Audio/haptic manual revalidation: **NO for this document; YES when affected sound/voice implementations are delivered.**


## Preserved visual references — 5 October 2026

Use the [reference archive](design/README.md) before commissioning duplicate concepts. The [selected UI/HUD boards](design/ui/README.md) and [connected forest/town proposals](design/world/connected/README.md) are documentation inputs only; runtime derivatives still require the existing provenance, importer, scale, collision, readability and measured-cost gates. The loading image's bar is explicitly superseded by a small spinner only. Historical alternatives are clearly separated.


## Locked rescue-transition asset contract — 5 October 2026

Follow the [master §§4.3–5 contract](superpowers/plans/2026-09-11-beyond-the-tomb-1.7.0.md#43-rescue-and-rope). The Keeper gravestone moves aside after defeat and remains visible beside the separate rescue exit on pull-up and backtracking. Fit stone motion, shaft, rope grips, mantle and both grounded landings at real scale to F01 and the accepted tomb; exact dimensions and hip/back carry placement are prototype decisions. Preserve the four other openings and the frozen map/reference art.

Author both sword and lantern stow/restore on the actual player rig so both hands grip freely; the owned lantern and its light stay physically coherent in direct, reflected/transmitted and shadow views. This requires no new lantern, Kit throw/hold clip or paid generation. Descent is a separately authored motion, not automatically a reversed ascent candidate. Kit may wait safely outside; no Kit dungeon traversal is added.

Partition the shared upper room/shaft/rope/rim/gravestone/immediate clearing for continuity in both directions. Early preparation and readiness-before-commitment support no normal loading screen; a 4–7 second ascent target is not a loading guarantee. Validate permitted looks, resource retirement, slow/failing preparation, safe recovery, gear and persistent quest state, plus peak memory/textures/staging/BLAS/TLAS/pipeline costs on phone and Windows. These are production and acceptance requirements, not delivered assets or runtime evidence. Scope remains 1.7 after its existing start gate.
