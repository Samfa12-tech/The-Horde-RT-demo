# Bellwether resource/culling assessment  -  read-only, 26 October 2026

Baseline source: `f442e333df4644b36e54737a155250a3df706840`. No tracked edits,
builds, app execution, device/ADB, GUI, network or asset admission in this task.
The concurrent approved flask/TLAS106 -> 107 change is outside this assessment.
Baseline ABI counts below deliberately refer to f442e333; that change raises
ordinary physical instances30 -> 31, keeping metadata and other ceilings unchanged.

Sam subsequently approved the strategy: keep a representative low-detail town
resident, then measure detailed areas and transition peaks before deciding what
to stream. Preserve one authoritative layout/scale/silhouette across1.7/1.8.
The ranges below remain initial targets, not guaranteed capacity or a final
asset freeze. This does not authorize every interior/actor/detail concurrently,
scope expansion, an engine rewrite or publication.

## Scope and geographical authority

Use one representative world across 1.7/1.8: same town centres/footprints,
terrain/river interface and major rooflines; 1.7 supplies its distant closed
representation,1.8 adds near surfaces and admitted interiors. Do not create an
unrelated prettier distant village. Approved regional topology is authoritative;
coordinated R2 metric dimensions remain proposed blockout.

R2 register: B01 tavern 22 x 16m (hero interior), B02 church 18 x 24m (exterior/outside
service), B03 house A 16 x 12m and B04 house B 16 x 14m (optional interiors), B05
mill 16 x 16m (working exterior), B06 smith 18 x 14m (service yard), B07 store 13 x 11m
and B08 outbuilding 12 x 9m (closed shells), B09 shrine 8 x 8m (provisional spur).
Also well/square, gardens, active churchyard, arrival/local/outbound paths,
descending river/bed/shallow banks/rocks/bounded cascades; millrace/service bridge
optional. Foundry is a separate regional landmark east across the gorge, never
the B06 Smith. Abbey south/downstream and Court northeast ridge are later regions;
arrows are not verified player-height reveals. Source:
`docs/references/horde-1.7/d37f1638/docs/design/world/connected/README.md:49-77`,
`shared-coordinate-layout.json`, `building-feature-register.csv:2-10`.

Historical 6-10 visible buildings, 2-3 interiors, 4-7 total cast and 1-2 substantial
conversations are provisional envelopes (`docs/superpowers/plans/2026-09-11-village-hub-1.8.0.md:35-43`).
Reuse modular architecture/PBR kits and bound activity. Parent-verified village
life scope includes closed privy/unseen occupant, locked-hole resident, animals
and crops; species/counts/staging remain proposals. One pig+three hens visible,
two active, is a candidate stress cap, not a farm simulation commitment.
Tavern fireplace/lantern, selective glass/wet metal, dialogue/services and logical
save/recovery are later playable workload, absent from the distant shell.

Parent-verified Library R17 is the best editable assembly, not a runtime town GLB:
only B01/B02/B05/B06 refined; others reference-only, roofs/terrain/LOD/collision
unfinished. Neither ZIP size nor the parent receipt measures local resident GPU
bytes. R17 was not downloaded. RouteA burial and unapproved OptionB remain open.

## Actual local baseline

Read GLB JSON/accessors directly and checked runtime SHA256. Logical bytes use the
actual 64-byte StaticRtVertex and 4-byte indices (`RtSceneAbi.def`, loader
`src/scene/assets/StaticMeshAsset.cpp:1398`); they exclude GPU allocation padding,
BLAS, scratch, metadata, decoded textures and CPU copies.

| Runtime component | Geometry | Material/texture ownership | Logical vertex+index bytes |
|---|---|---|---:|
| Normal town v4 |1,954 exported vertex tuples;2,898 indices;966 triangles;2 primitives |2 reused rows: MedievalWall02/TombEarthenware;0 added textures/lights |136,648B (0.130MiB) |
| Current terrain r23 |19,486 vertices;113,538 indices;37,846 triangles;2 primitives |2 ground families;6 source maps (not6 incremental runtime allocations) |1,701,256B (1.622MiB) |
| Existing tree masters |pine 5,602 vertices/5,000 triangles; alder5,632/5,102;3 primitives each |4 shared forest families |840,200B for one pair |
| Normal baked64 trees |32 pine+32 alder;323,264 triangles;4 family primitives |same 4 families; no per-tree asset owner |26,886,400B (25.641MiB), excluding BLAS |

Shell SHA `66058108eee9e014acfa10086b9da81e577d40d9f88a3a6d7c2131388eea06ba`;
terrain SHA `c636a67911b7f5c5381c30ec7a49e5d4ff15c70fe688b368ba52efe77b537774`;
pine SHA `db2d45547ad2f629c48ab8c994db845eb5d097a7db5bed6192b837e29f7a1b3f`;
alder SHA `f545d7e08370b5e451597c3fd903d8d93b6703d11c75039181a04f7f5312e0bb`.

All nine v4 masses are appended into the existing collapse/common static owner;
they do **not** create nine TLAS instances or nine BLAS. Final material-family
consolidation may further merge primitive groups. No interiors/gameplay supplied.
`src/scene/BellwetherShell.cpp:12-59`; `PresentableTinyRtScene.cpp:3229-3241,8170-8173`.

The source intake fixture expects430,850 common-asset triangles (entry+tomb+
terrain+forest+dressing+shell),33 packed primitive rows and32 material rows.
This is a code/test contract, not a freshly executed test or GPU allocation:
`tests/RtSceneResourceInventoryTests.cpp:1238-1291`,
`tests/ForestDressingTests.cpp:144-147`. f442e333 ABI capacities:30 metadata,
30 ordinary physical definitions,106 total physical TLAS slots,15 static assets,
38 primitive rows,34 material rows,18 texture layers. These are scene-wide
capacities/reservations, not available town slots. The fixture's5 primitive and2
material row margin is not a guarantee of remaining independent object capacity.

v4 centres/footprints match R2, but floors differ (tavern+2.440m, mill+1.785m).
Its grounding receipt used another terrain hash; this does not prove a present
defect, but old grounding cannot certify r23 contact. See
`reports/WORLD_STATUS_a3f4277c.md`, `reports/world-layout-compare-a3f4277c.json`.

No fresh f442e333 GPU allocation/timing evidence exists from this task. The older
lookout receipt `reports/town-candidate-pipeline/capture-manifest.json` is a
28-instance Diagnostic High RTX5050 frame:20 BLAS,1 TLAS,401,945,216 device-local
classified bytes,46,949,168 host-visible classified bytes. Its total belongs to
that archived scene/build, not town alone or current HEAD; classifications can
overlap and must not be added.

## What actually culls or unloads

* **Frustum/distance:** no general CPU frustum, screen occlusion/HiZ, or automatic
  distance-based mesh removal/LOD found in the renderer. Hardware BVH traversal
  rejects ray-unintersected bounds, but does not evict meshes/textures. Main rays
  trace to 10,000 m (`shaders/raytracing/include/rt_frame.glsl:51-53`). Fog and finite
  secondary-ray lengths are shading/query limits, not object residency policies.
* **Normal production:** exact opaque passage-face coverage and conservative
  camera/lantern/secondary/moon/water closure certify exactly 84,219
  ForestBark/Pine/Moss/Alder ExteriorDressing triangles, every vertex Z>24m.
  Post-Keeper claimed-lantern camera/source guards and a simulation lease permit
  a reduced index/BLAS replacement only inside the enclosed corridor; complete
  restoration precedes unsafe/new source states. Full static vertices, shared
  maps/materials, CPU source, collision, terrain, distant town and uncertain
  contributors remain. Town is never selected by this policy.
  `PresentableTinyRtSceneStaticGeometry.cpp:153-232,366-473`;
  `src/scene/CorridorStaticDetail.h:8-49`.
* **Shared64 experimental route:** masks 13 whole tree instances under the same
  certificate, restores masks first every frame; both master BLAS/V/I remain
  resident, physical definitions remain present. Mask changes use TLAS BUILD;
  zero residency saving from masking itself. Normal switch remains OFF, R4/F01
  unadmitted. `PresentableTinyRtSceneStaticGeometry.cpp:251-346`;
  `docs/evidence/2026-10-24-shared-forest-retention/README.md`.
* **Independent-page prototype:** actually compacts/splits V/I/private primitives
  and BLAS, removes the page's active reference and frees its completed GPU
  owner; CPU payload and shared textures stay cached/resident. It is private,
  macro default 0 (`PresentableTinyRtScene.h:11-14`,
  `cmake/HordeRtRegionPages.cmake:1-30`), not normal town streaming. Scheduler
  has 12 m prepare/24 m retain/2 s prediction defaults, but unknown all-query
  contribution remains pinned regardless of distance; caller still supplies
  the corridor certificate. `src/world/streaming/RegionStreaming.h:17-55`;
  `PresentableTinyRtSceneRegionPages.cpp:113-217`.
* **LOD:** importer chooses a manifest LOD from filename; existing forest uses
  admitted LOD1. No automatic distance transition implemented. Fire/dust spatial
  selection limits effects, not town triangles. Viewmodel/actor masks and
  event-driven Keeper retirement are separate semantic lifetime policies.

Resident town's world mask 0x01 participates in primary and applicable local/moon
shadow, reflection and transmission queries; an off-camera building can matter.
Shader world masks include 0x01 (`rt_lighting.glsl:929-1005`,
`rt_dielectric_transport.glsl:77-80,377-378`). Closed/occluded proofs must cover
these contributors before deleting detail; camera-facing visibility alone cannot.

Measured savings retain original build/platform scope:

| Historical mechanism | Actual saving | Peak/limit |
|---|---:|---|
| Ordinary Windows index/BLAS replacement |6,215,936B (5.93MiB) |scene371,998,256 -> 365,782,320 -> 371,998,256B; observed peak420,071,600B (+45.85MiB) |
| Private phone independent page |14,249,664B (13.59MiB) |matched Pipeline160,831,232 -> 146,581,568 -> 160,831,232B; peak174,307,392B (+12.85MiB); zero readiness waits |
| Windows normal-shared candidate representation |45,372,864B lower than baked |327,091,744 vs372,464,608B at corresponding post-Keeper state; masters resident; no speed admission, normal OFF |

Sources: `docs/evidence/2026-10-14-ordinary-static-streaming/README.md:3-24,75-89`;
`docs/evidence/2026-10-16-phone-resident-streamed/README.md` (SM-S948B/API37,
Adreno840, exact private APK/settings); `docs/evidence/2026-10-24-normal-shared-forest/README.md`.
Phone matched rendering was already expensive: streaming proves memory/lifetime,
not smooth play, general town benefit or causal FPS gain. No town culling saving
is currently measured.

## Bounded candidate content envelopes  -  estimates, not admitted engine caps

Use these as an initial authoring/measurement shopping basket. Geometry ranges
count unique stored triangles, while placed/traced triangle totals must be logged
separately; a reused tree/roof/furniture master is stored once only when genuine
instancing is implemented. Keep the coarse representation resident during near
detail transitions. Counts are incremental geometry by tier; texture totals are
not cumulative where stated. They are not a promise these fit today's ABI.

| Tier | Candidate workload | Candidate geometry | Material/texture constraint |
|---|---|---|---|
|1 Resident representative distance |all 8 town masses+shrine plus separate Foundry silhouette; other regional silhouettes only where actually visible |10-30k unique triangles total, across a few coarse owners; current 966-triangle shell is baseline, not finished silhouette proof |reuse stone/clay/wood/metal;0-2 new 1k PBR families initially |
|2 Near exterior |same8 masses with doors/windows/roofs, square/well/yard/gravestones/crops/river edges; reused architectural modules/dressing and relevant forest |additional 100-250k unique triangles; log roughly 0.4-1.0M placed triangles as a stress envelope including existing 323k forest, rather than multiplying that number into unique storage |about 8-12 town surface families including reused ones; start with<=4 genuinely new 1k PBR sets, extend only after exact ledger/measurement |
|3 Interior/actors/interaction |tavern first, 2-3 interiors total only after proof; reused furniture; bounded NPC/animal activity,1-2 substantial services/conversations |additional 60-150k static unique triangles as a first basket; actor topology/poses/refit storage separately measured, not guessed |roughly 12-20 total town material families including exterior; target<=8 genuinely new 1k PBR sets across town before considering more |

BLAS/TLAS counts are consequences of packed cell/master/actor ownership, not
building/prop counts. First proof should keep distant town merged as today and
test only a few independent near/interior owners; one BLAS per roof tile or
furniture placement is not a useful budget. The current 30 metadata/15 asset
roster and 106 reserved physical ceiling require an explicit scene-wide ledger
before adding independent roles. No cap change or asset admission is proposed
by this read-only task. Retaining current materials with reuse is the immediate
feasible route; the larger authoring envelope needs subsequent capacity review.

Useful arithmetic: GPU logical static geometry is 64V+12T bytes. Assuming 1-3
imported vertices/triangle gives 76-204B/triangle:10-30k~0.72-5.84MiB;
100-250k~7.25-48.64MiB;60-150k~4.35-29.18MiB, all before BLAS/scratch/copies.
Hard edges/UV seams and repeated source nodes can exceed that assumption; record
actual imported counts. BLAS byte/triangle extrapolation is not portable.

Current texture formats (`tools/build-production-prop-textures.ps1:80-97`) give
one full-mip 1k baseColor 6 x 6 ASTC+normal 4 x 4+ORM 6 x 6 logical payload 2,650,704B
(2.528MiB), versus approximately 16MiB for three RGBA8 maps on Windows. Thus 2/4/8
genuinely additional sets cost approximately 5.06/10.11/20.22MiB Android and
32/64/128MiB Windows *before allocation overhead/emissive*. A 2k set is roughly 4 x ;
do not infer GPU cost from ZIP/GLB file sizes. Reuse/dedup savings count only after
actual byte-identical maps/atlas selections are verified.

The decisive first slice is arrival -> tavern doorway/threshold/backtrack with Kit +
one other independently animated actor, fireplace+held lantern, selective glass,
wet metal, atmosphere and all relevant forest/exterior contributors together.
It must include the threshold overlap peak, not an empty average square. Extend
to proposed animals/cast only after measured actor/skin/refit work. Keep Kit,
player, NPCs and enemies distinct identities;4-7 cast does not require7 active
independent poses, and no enemy slots should be borrowed silently.

Needed evidence: reconcile R17 rooflines and R2/r23 vertical/mill/river interface;
inspect real moving F04 and near views; record imported unique/placed triangles,
materials/maps/mips and collision; then exact Android/Windows live allocations,
BLAS/TLAS/scratch, full -> overlap -> retired -> restored peaks, warm GPU/CPU tails and
repeated doorway/return/lifecycle correctness at fixed settings. Preserve one
layout and accepted quality. No precise future FPS, GPU-ms or headroom promise
can be supported by current shell/file sizes.
