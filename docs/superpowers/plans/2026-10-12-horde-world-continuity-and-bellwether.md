# Whole-world continuity and Bellwether detail/capacity decisions

**Owner decisions: Sam, 12 October 2026. Documentation and planning only.** The [shared roadmap](../../ROADMAP.md#17---beyond-the-tomb) owns update scope and status. This record supplies the current world-authoring, streaming and Bellwether contracts alongside the [world atlas](../../design/world/atlas/README.md). Implementation and exact-platform acceptance remain pending.

The full [WORLD_LAYOUT](https://github.com/Samfa12-tech/The-Horde-RT-demo/blob/d37f16381311c6e620f0c4a9d01c1cce83260dc8/docs/WORLD_LAYOUT.md), [1.7 master](https://github.com/Samfa12-tech/The-Horde-RT-demo/blob/d37f16381311c6e620f0c4a9d01c1cce83260dc8/docs/superpowers/plans/2026-09-11-beyond-the-tomb-1.7.0.md), [1.8 hub plan](https://github.com/Samfa12-tech/The-Horde-RT-demo/blob/d37f16381311c6e620f0c4a9d01c1cce83260dc8/docs/superpowers/plans/2026-09-11-village-hub-1.8.0.md), [connected maps](https://github.com/Samfa12-tech/The-Horde-RT-demo/blob/d37f16381311c6e620f0c4a9d01c1cce83260dc8/docs/design/world/connected/README.md) and [village-life appendix](https://github.com/Samfa12-tech/The-Horde-RT-demo/blob/d37f16381311c6e620f0c4a9d01c1cce83260dc8/docs/BELLWETHER_VILLAGE_LIFE.md) are preserved at historical refs. Read their useful detail with this dated decision record: today's explicit decisions supersede conflicting older scope or capacity recommendations. They do not rewrite historical geometry, receipts or proposed budgets.

## 1.7 shipping requirement: complete low-detail world

When 1.7 ships, it must include the **complete low-detail authored world terrain and accurately positioned major locations, objects and landmarks across the planned mapped world**. The whole coarse topology must remain available above ground while local detail loads in and out. Terrain and the world silhouette must not disappear at local loading boundaries. All visible distant landmarks, specifically the forge, must remain visible when unobstructed; verify actual player-eye sightlines rather than assuming a map arrow guarantees visibility. Both the southeastern Bellwether smith and the eastern Ashen Foundry retain their distinct mapped identities and locations.

Sam's desired experience is a continuous overworld like *Jak and Daxter*: connected geography and distant views that survive local detail transitions. This is experience direction, not a claim to know that game's implementation or evidence that Horde already supports the required behavior. Bounded reusable capabilities and measured resource ownership can fulfil it; the requirement does not prescribe an unbounded general-purpose open-world system.

**Playable scope stays tomb → rescue/reunion → woodland → lookout.** Bellwether and future regions can be represented in coarse terrain/massing without becoming playable streets, interiors, quests or detailed future dungeons in 1.7. Later chapter gates and either-order Abbey/Foundry progression remain intact. The approved world is the planned mapped region; provisional land beyond the atlas window is not another approved region.

### Authoring and registration still required

Use the approved relative topology, preserved proposed coordinates and actual terrain as authoring inputs. Accurate runtime placement requires reviewed registration and 3D proof; the map's exact mathematical correspondence is not owner approval of every metric dimension.

| Work item | Required disposition |
|---|---|
| Major terrain, routes, locations and objects | Register stable identities and world transforms against the regional/local sources; verify elevations, grounding, orientation, footprint and permitted sightlines in 3D. Preserve northwest tomb, southeast approach, unwalled Bellwether west of the river, southern lake/Abbey, eastern gorge/Foundry and northeast Court ridge. |
| Bellwether roofs, rotation and silhouette | Existing mapped footprints alone do not establish roof or rotation fidelity. Reconcile authored rooflines, height, orientation, openings and silhouettes before treating the near/mid/far contract as accepted. |
| Tomb and rescue alignment | The original tomb interior is not surveyed into the atlas rescue coordinates. Reconcile the accepted tomb/shaft/rim geometry and transform explicitly; do not invent an approved interior footprint or reshape it blindly to the draft sheet. |
| Treasury/finale | Topology puts the dry treasury below/behind the Court ridge. Exact layout, extent, placement and finale staging remain open authoring/registration decisions; GC10 is not evidence of those coordinates. Coarse coverage must record this gap rather than fabricate approved geometry. |
| River/lake | Authored beds are distinct from rendered water. Complete the open hydrology integration task below, with credible banks, path, elevations and far appearance. |

## Outdoor river and lake: open 1.7 task

**Read-only source check, 12 October, PR27 head `d130386a19df914532f7a9f442de2103d454f9b7`: regional channel/basin terrain exists; outdoor river/lake water surfaces are absent in the checked build.** PR27 metadata was rechecked at this head. This is a dated source disposition, not a live-render or phone acceptance result.

- [Regional terrain r21 source](https://github.com/Samfa12-tech/The-Horde-RT-demo/blob/d130386a19df914532f7a9f442de2103d454f9b7/assets/models/world/source/chapter-terrain-r21/README.md) explicitly separates river/lake beds from the water surfaces still needing integration.
- [Regional terrain authoring](https://github.com/Samfa12-tech/The-Horde-RT-demo/blob/d130386a19df914532f7a9f442de2103d454f9b7/tools/chapter_regional_terrain.py) uses `river_surface` coordinates to shape terrain heights. That key does not submit a rendered water mesh.
- [Renderer water submission](https://github.com/Samfa12-tech/The-Horde-RT-demo/blob/d130386a19df914532f7a9f442de2103d454f9b7/src/vulkan/raytracing/PresentableTinyRtScene.cpp#L3755-L3762) consumes [WaterSurfaceTriangles](https://github.com/Samfa12-tech/The-Horde-RT-demo/blob/d130386a19df914532f7a9f442de2103d454f9b7/src/gameplay/effects/WaterContactSurfaces.h), the existing tomb catchment pool/runoff. The waterfall is also tomb water; neither proves regional outdoor water.
- [Regional terrain evidence](https://github.com/Samfa12-tech/The-Horde-RT-demo/blob/d130386a19df914532f7a9f442de2103d454f9b7/docs/evidence/2026-10-11-regional-terrain/README.md) leaves water/foliage/landmark visibility and full-route inspection open. Fixed source-density rings are not dynamic LOD or GPU streaming.

**Outstanding task needed to satisfy the complete low-detail world requirement:** author and integrate the regional river/lake surfaces with their terrain/banks, correct water elevations and continuous downstream path from northeastern headwaters past Bellwether/mill into the southern Abbey basin. Reconcile inflow/outflow, joins, floodline and lake datum with the preserved [river/mill height proposals](https://github.com/Samfa12-tech/The-Horde-RT-demo/blob/d37f16381311c6e620f0c4a9d01c1cce83260dc8/docs/design/world/connected/README.md); their numeric engineering remains subject to 3D review.

Provide coherent far silhouette/visibility and detail transitions, then near water where relevant to admitted views, with the necessary genuine RT shadows, reflections and light contributors consistent with chosen measured budgets. Inspect the combined world/lantern/actor workload on phone and Windows. Reuse suitable bounded water foundations where proven; no bespoke full fluid simulation, playable lake, swimming, boats, underwater combat or detailed Abbey rooms is required by this task.

## Continuous coarse world and safe local detail transitions

Keep the coarse world, required distant architecture/landmarks and necessary shadow/reflection/transmission/light contributors available throughout permitted above-ground travel. Residency, primary visibility, detail level, gameplay support and secondary/light relevance are separate policies. Primary-camera occlusion alone does not authorize universal resource retirement.

Sam permits an authored road bend/cutting, a short wall or trees to conceal detailed-loading transitions. Preserve Bellwether's unwalled topology: a short local occluder does not imply a perimeter wall. Prefer stable terrain/architecture for dependable occlusion; trees can be porous, change with viewpoint or sway, and must be tested accordingly. Such transitions must never make the coarse terrain or distant landmarks vanish.

Prefetch ahead of likely travel, validate asset/collision/texture/upload/AS/pipeline readiness before visibility or traversal commitment, and support immediate reversal and backtracking. Publish ready resources safely with generation checks and recovery to a safe side; reject stale completion. Retire only after all relevant paths no longer need the resource and submitted GPU work has completed. Account for source/destination overlap and shared resources. No fixed walk/climb duration is an I/O guarantee.

The existing normal rope ascent/descent remains screen-free in both directions. Preserve its shared upper room/shaft/rim/rope/clearing and persistent progress. CPU-ready data, masked instances and shared resident buffers do not establish memory reclamation or working normal-play streaming.

## Bellwether: one positioned building through near, mid and far detail

The 1.7 distant buildings must develop into coherent near/mid/far versions of the **same positioned buildings** as 1.8 develops. Keep stable building identities, transforms and footprints, then reconcile the eventual roof/silhouette and door/opening contracts with selected interiors. Derive far detail from shared authored geometry where feasible, using compatible origins and registration rather than independently repositioned scenery.

The [1.8 hub plan](https://github.com/Samfa12-tech/The-Horde-RT-demo/blob/d37f16381311c6e620f0c4a9d01c1cce83260dc8/docs/superpowers/plans/2026-09-11-village-hub-1.8.0.md) and [building register](https://github.com/Samfa12-tech/The-Horde-RT-demo/blob/d37f16381311c6e620f0c4a9d01c1cce83260dc8/docs/design/world/connected/building-feature-register.csv) remain the detailed planning inputs. Mapped footprints are useful but do not alone prove matching roofs, rotations or doors. The [PR27 shell milestone](https://github.com/Samfa12-tech/The-Horde-RT-demo/blob/d130386a19df914532f7a9f442de2103d454f9b7/docs/evidence/2026-10-12-bellwether-shell/README.md) is bounded implementation evidence, not accepted 1.8 geometry or runtime LOD.

Test approach, street/interior thresholds, windows and secondary views, turns/backtracking, detail switches and repeat visits. Keep coarse buildings and relevant shadow/light contributors while detailed interiors prepare or retire. Closed buildings must read honestly as closed. Concrete playable 1.8 implementation still follows accepted 1.7.

## NPC capacity: prove reusable three-character capacity first

Sam clarifies that NPCs may load/unload as the player walks; the whole cast is unlikely to occupy one room, and **three may work**. This supersedes the earlier recommendation to increase lanes merely because Bellwether has a four-to-seven named cast. Total cast, currently resident actors, visible actors and simultaneous animation/RT work are different quantities.

Plan the first proof around reusable three-character runtime capacity, with stable persistent identities/state outside resource lifetimes, preloading before visibility, safe allocation/publication/retirement and truthful failure/recovery. Test combined streets, doorways and interiors, including humans/animals overlap and exposed shadows/reflections; being out of the camera view is not sufficient unloading authority.

The existing [CharacterRenderSlot](https://github.com/Samfa12-tech/The-Horde-RT-demo/blob/d130386a19df914532f7a9f442de2103d454f9b7/src/vulkan/raytracing/CharacterRenderSlot.h) supplies **two specialised guard lanes plus a Keeper/Kit lane**, not an already-general three-NPC pool. Prove reuse and coexistence before claiming the hub fits. Village-life animal counts are proposed stress inputs and must fit the combined workload rather than gaining an independent free capacity budget.

**Three enemies on screen is the current encounter-design target**, not an immutable technical cap. Improve combat feel and portrait/touch usability before increasing encounter counts. Expand actor capacity or encounters only when representative scene evidence warrants it and Sam makes the scope decision; no automatic lane increase, extra combat or permanent ceiling is approved here.

## Dated capacity rationale and measured gates

At read-only PR27 `d130386a`, the reviewed application inventory is constrained as follows. These are **application ABI/packing bounds, not GPU hardware limits or proven phone capacity**. Re-audit the accepted candidate before implementation.

| Inventory | Dated snapshot / meaning |
|---|---|
| Texture arrays | Base color 18/18, normal 15/18, ORM 15/18, emissive 1/18 layers. Spare layers in one family do not prove complete material admission. |
| Materials | 30 packed + 2 authored-world records = 32/32. |
| Primitive metadata | High 32/32; non-High 31/32. Quality-dependent geometry is not a free universal slot. |
| TLAS | 28 entries, including reserved/inactive entries; not 28 visible independent props. |
| Registered owners | 13; not 13 world-prop instances or a total scene-asset count. |
| NPC render lanes | Three specialised lanes as described above; reusable capacity is still a proof task. |

Supporting sources are the [scene ABI](https://github.com/Samfa12-tech/The-Horde-RT-demo/blob/d130386a19df914532f7a9f442de2103d454f9b7/src/vulkan/raytracing/RtSceneAbi.def), [owner registrations and static packing](https://github.com/Samfa12-tech/The-Horde-RT-demo/blob/d130386a19df914532f7a9f442de2103d454f9b7/src/vulkan/raytracing/PresentableTinyRtSceneStaticGeometry.cpp), [texture selection](https://github.com/Samfa12-tech/The-Horde-RT-demo/blob/d130386a19df914532f7a9f442de2103d454f9b7/src/vulkan/raytracing/RtChapterTextureLayout.h), [world material palette](https://github.com/Samfa12-tech/The-Horde-RT-demo/blob/d130386a19df914532f7a9f442de2103d454f9b7/src/vulkan/raytracing/RtWorldMaterialPalette.h), [renderer resource creation](https://github.com/Samfa12-tech/The-Horde-RT-demo/blob/d130386a19df914532f7a9f442de2103d454f9b7/src/vulkan/raytracing/PresentableTinyRtScene.cpp#L2806-L2925) and [bounded capture inventory](https://github.com/Samfa12-tech/The-Horde-RT-demo/blob/d130386a19df914532f7a9f442de2103d454f9b7/docs/evidence/2026-10-12-prepared-rocks/pipeline/capture-manifest.json). Short diagnostic captures do not establish sustained budgets. Rough assistant-estimated asset/prototype counts are not approved production budgets.

Use the master's [six outdoor gates](https://github.com/Samfa12-tech/The-Horde-RT-demo/blob/d37f16381311c6e620f0c4a9d01c1cce83260dc8/docs/superpowers/plans/2026-09-11-beyond-the-tomb-1.7.0.md#131-six-gate-outdoor-readiness-contract) and 1.8 arrival-to-tavern gate with the now-required complete coarse world. Representative combined street/interior/NPC-plus-far-world tests must include actual geometry/texture/AS memory, upload/staging peaks, transition overlap, skinning/AS costs, frame pacing and sustained phone/Windows evidence on exact builds/settings. Validate rapid reversal, repeated visits and failure/lifecycle recovery. Set unset numeric budgets from measured evidence and owner decisions; do not silently reduce quality or infer capacity from current constants.

## Retained tree ambition and acceptance boundary

[Easy Tree](2026-10-11-horde-forest-instancing-and-wind.md) remains the selected authoring direction for varied, thick, subtly living canopy, likely 1.7 subject to proof/sequencing. PR27's currently deferred Easy Tree/canopy implementation slice is a separate scheduling disposition, not cancellation of the ambition or tool selection. Preserve one-tree → representative-grove → scaling proofs and outdoor gates.

Accept 1.7's new world requirement only after inspecting the complete mapped coarse coverage, registered major locations/objects, unobstructed landmark visibility (including the forge), hydrology and motion through local boundaries on both supported hardware RT paths and exact supported devices. Record unknown geometry, missing resources, budget failures and absent device evidence explicitly. Existing host CI, terrain imports and finite stills do not close this requirement.

This decision record starts no runtime implementation, asset installation/generation, gameplay work, merge or release. Audio/haptic manual revalidation required: **NO for this documentation change**.
