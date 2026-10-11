# Varied trees, living canopy and reusable forest tooling

**Owner direction recorded 11 October 2026. Likely target: 1.7, subject to proof and sequencing.** Sam asks for hundreds of varied trees, convincing leaf volume and canopy thickness, and subtle sway so the world feels alive. He approves exploring better instancing and a repeatable Blender-to-native asset route: “These are the challenges we face as we build that make our engine more capable”. The visual/gameplay vision can justify purposeful reusable engine improvements, with evidence deciding their scope.

**Authoring-tool decision, 11 October 2026:** Sam explicitly decided, “And we will use the easy tree system”. Easy Tree is the selected Blender tree-authoring system. Build export/preparation tooling and native import support around its output; the tool choice is settled, while export fidelity, foliage representation, runtime wind and performance still need proof.

The direction and authoring-tool choice are owner-approved; the remaining implementation choices below are proposals. This document delivers planning only, with no imported assets, engine changes, validated format compatibility, device measurements or shipped forest capability. Hundreds is a scale goal, not an admitted simultaneous population or a release guarantee. 200 trees can be an initial stress-test point; it is neither a locked cap nor an acceptance threshold.

## Authority, sequencing and boundaries

- [Shared roadmap](../../ROADMAP.md#varied-trees-living-canopy-and-reusable-forest-tooling) owns status/scope; [source inventory](../../ROADMAP_SOURCES.md#17-plans-production-and-pr27-evidence) owns navigation.
- The [full 1.7 master's six outdoor gates](https://github.com/Samfa12-tech/The-Horde-RT-demo/blob/d37f16381311c6e620f0c4a9d01c1cce83260dc8/docs/superpowers/plans/2026-09-11-beyond-the-tomb-1.7.0.md#131-six-gate-outdoor-readiness-contract) remain mandatory. Its WP1/WP2/WP3/WP8 already own authoring, geography/ownership, combined feasibility and final art. This is a connected implementation proposal within that sequence, not a replacement programme.
- Preserve [asset admission](../../ASSET_PIPELINE.md), [renderer/asset contracts](../../AGENT_ENGINE_CONTRACTS.md#renderer-and-assets), [validation/evidence rules](../../AGENT_VALIDATION.md#performance-evidence) and commercial-safe provenance in [ASSET_LICENSES.md](../../../ASSET_LICENSES.md) before any shipping admission.
- [PR27's reviewed design index](https://github.com/Samfa12-tech/The-Horde-RT-demo/blob/d24a9971016050964a7a9a1f2cf1ac383b411324/docs/design/HORDE_1_7_CURRENT_DESIGN_INDEX.md) retains the active chapter's work and failures. Coordinate later implementation through that job's existing ownership; this proposal does not switch, interrupt or widen its current engineering package.
- Keep the [linked world atlas](../../design/world/atlas/README.md) and approved chapter topology. Exact tree positions, metric geography, canopy density and sightlines remain blockout proposals. Normal rope ascent/descent stays screen-free; safe transition and return gates still apply. No new playable region, procedural open world, weather system, release or deferred mist-issue work is implied.

Do the one-tree asset proof alongside cheap geography/hero-view proof, then the representative grove through WP3. Earn bulk placement/final art in WP8 only after outdoor gates 1–5 and explicit production budgets. If a stage fails, improve the bounded engine path or bring a specific experience compromise to Sam before expanding. The likely 1.7 target may move with evidence and chapter sequencing.

## Historical starting point, to re-audit before implementation

The supplied read-only review of [PR27 at d24a9971](https://github.com/Samfa12-tech/The-Horde-RT-demo/commit/d24a9971016050964a7a9a1f2cf1ac383b411324) found ten world-baked trees, approximately 50,510 triangles, an opaque-only static import path, and shared asset foundations without a scalable forest lifecycle or deformation path. This is a dated snapshot, not a permanent limit, current capacity certificate or benchmark. Re-inspect the accepted implementation when this work starts; later chapter changes can supersede it.

The goal is to reuse those foundations while separating reusable masters from placement, real LOD selection, relevance/residency and wind ownership. Increasing hard-coded asset, material, instance or acceleration-structure bounds alone does not establish scale. Any necessary ABI/capacity extension needs bounded ownership, validation, memory accounting and exact-target evidence, rather than permission inferred from the word “hundreds”.

## Connected Blender-to-native workflow

**Selected authoring system:** [Easy Tree extension listing](https://extensions.blender.org/add-ons/easy-tree/) and [upstream Easy-Tree repository](https://github.com/jacobcjohnston/Easy-Tree). The upstream README describes Geometry Nodes, reusable branch/leaf components, custom leaf meshes and simple wind effects. Sam's decision selects the authoring system; those upstream descriptions do not validate Horde export/import behavior. The extension listing could not be fetched during this review; pin and verify the actual tool/package and matching Blender/exporter versions at intake. No installation or execution was performed for this proposal.

“Direct compatibility” means a documented, supported exported-asset contract that the native engine can ingest with repeatable preparation and useful errors. Blender, Easy Tree and live Geometry Nodes remain authoring tools, with no Blender runtime dependency. A generated Blender wind effect is not automatically portable animation or native deformation metadata. Test its output and author an explicit runtime mapping; neither glTF/GLB preference nor a successful generic export proves fidelity.

| Step | Proposed output and proof |
|---|---|
| 1 · Small master library | A modest reusable set of tree shapes/species plus selected hero variants. Retain editable sources, exact tool versions, parameters/seeds, source/texture rights and hashes. The existing master's four archetypes are a starting authoring suggestion, not a required count. |
| 2 · Repeatable preparation | Evaluate/realize procedural instances and modifiers deliberately for a static reference pose; preserve originals. Produce local-space meshes with ground/root origin, documented units/up axis and applied transforms. Check metres against the player/terrain, finite vertices, topology/winding, normals, UV0/tangents, material slots and bounds. Reject unsupported inputs with actionable diagnostics; avoid world-baking one full mesh per placement. |
| 3 · Material/export fidelity | Compare authored versus exported/loaded meshes, vertex attributes, normals/tangents, material IDs, base/normal/ORM/emissive maps, texture colour spaces and any alpha/double-sided behavior. Convert unsupported procedural materials deliberately. Prefer the measured GLB/PBR/strict-ASTC route where proven; test actual attributes/extensions before promising a supported format or custom-data channel. |
| 4 · Near/mid/far 3D LODs | Prepare separately measurable geometry/material levels with consistent origins and silhouette, branch mass and leaf volume. Measure triangle/primitive/material/texture costs and conservative wind bounds per level. Validate transitions in motion and in RT shadows/reflections/transmission; nearby woodland remains real geometry, with real 3D far silhouettes rather than billboard/skybox substitution. |
| 5 · Collision and wind contract | Supply a stable trunk/root collision proxy and explicit wind metadata: anchored root region, branch/foliage weights or masks, local deformation frame, bounded amplitude/frequency, conservative maximum displacement and group/seed policy. Prove which attributes survive export; use a validated versioned manifest/sidecar if the format cannot reliably carry them. Keep runtime units and coordinate meaning unambiguous. |
| 6 · Native reusable assets | Import one master/LOD set through shared asset infrastructure, with manifests, material/texture reuse, validated bounds and provenance. Record geometry, textures and AS memory separately. A preparation script/report must reproduce the same supported output and report failures before scene admission. |
| 7 · Placement and runtime wind | Stable placement IDs reference master/LOD/material families, transforms, bounded variation and wind group. Use controlled seeds for captures. Native runtime owns LOD/relevance, residency and deformation/AS synchronization; compare the same placements in static and moving modes before expanding. |

Variation should chiefly come from the small library plus bounded scale, rotation, clustering, canopy shape and deliberate route/hero composition. Validate scaled collision and root grounding; bake mirrored variants with corrected winding if required by the importer. Hundreds of individually unique full-resolution meshes would defeat the intended sharing and make authoring, residency and AS work harder to bound.

## Canopy representation is a measured design choice

The acceptance aim is a convincingly thick, layered canopy: readable leaves, branch depth and natural gaps close to the player; credible volume and silhouette at mid distance; coherent 3D mass far away. Inspect upward, sideways and through overlapping trees, under moonlight and a raised lantern, in tall phone and wide desktop views. Avoid both thin see-through shells and uniform solid blobs. Near/mid/far transitions must preserve that intent without visible volume collapse.

Compare **opaque leaf geometry** and **alpha-tested leaf cards** on the same visual target and workload. Opaque geometry is an initial candidate compatible with the reviewed static path; cutout cards require proven import/material support and real RT visibility in both supported backends. Neither choice has an automatic performance advantage. Cards can change triangle count, texture sampling, candidate/any-hit rejection, two-sided handling, shadows and secondary-ray traversal; opaque geometry can increase geometry/AS cost. Measure image quality, frame time, memory and AS work together. Do not silently flatten foliage or replace supported alpha behavior to pass a budget.

## Efficient sharing and coherent subtle wind

Share geometry, material/texture records and immutable BLAS across compatible static placements/LODs where appropriate. Placement transforms and TLAS membership should not duplicate a master's full resources. Account separately for each unique master/LOD, active deformed geometry state, AS allocation and instance; texture sharing alone is not forest scaling.

Independent per-tree vertex deformation changes BLAS geometry and complicates sharing. Start by evaluating a bounded set of **shared wind-phase/pose groups** per compatible master/LOD, so many placements can reference the same deformed geometry/BLAS state while transforms and phases avoid obvious lockstep. Compare this with a static reference and other bounded alternatives, such as limited branch/foliage component groups or independently updated hero trees. Count the resulting resources and actual rebuild/refit work; grouping can introduce visible repetition and is itself a hypothesis to test.

Keep roots anchored, trunk collision stable and motion restrained. Leaves and selected branches may sway; gameplay support and collision must not drift with decorative wind. Explicitly bound any moving interaction surface if later required. Native geometry, conservative bounds, shading, ray-traced shadows and relevant reflected/transmitted views must describe the same pose. Shader-only visual offsets that leave the AS static cannot satisfy that coherence.

Prove legal AS update/rebuild flags, unchanged-topology assumptions, synchronization and ownership on both supported hardware RT paths. Separate per-group BLAS work from instance/TLAS changes and scene uploads; do not assume a refit is always legal or cheap. LOD/group switching, pause/resume, resets and two-way transitions must keep generations and submitted-resource lifetimes safe, with no per-frame unbounded allocation or visible phase/bounds discontinuity.

## Relevance, residency and collision are separate policies

| Policy | Required behavior |
|---|---|
| Primary camera visibility / LOD | Use the bounded authored views and permitted movement, with transition stability. Leaving the camera frustum does not by itself permit removing a tree. |
| Shadows, secondary rays and light influence | Preserve off-camera moon/lantern occluders and required reflection/transmission contributors with independently justified reach/LOD. Test camera turns and backtracking; no disappearing shadows or reflections from camera-only culling. |
| CPU/GPU residency and AS ownership | Choose full modest-forest residency or measured regional preparation from actual memory/overlap evidence. Readiness precedes traversal commitment; GPU completion precedes retirement. Include staging/upload peaks and shared resources retained by both zones. |
| Gameplay support and collision | Stable trunk/root proxies follow authored placement and remain available for permitted traversal, return and safe recovery independently of visual LOD/frustum decisions. Keep path/rope/Kit clearance authoritative. |

This is the existing outdoor gate 4 applied to reusable trees. Distance or visibility is not a universal switch for all four policies, and CPU-ready/shared buffers alone do not prove native GPU streaming.

## Staged proof and decision records

| Stage | Evidence needed before the next stage |
|---|---|
| A · One-tree import proof | Reproduce source → export → preparation → native RT render for one master with near/mid/far LODs, materials, scale, trunk collision and tested wind metadata. Inspect canopy volume and leaf fidelity under moon/lantern; compare static versus moving geometry/shadows/secondary views. Report unsupported attributes/material/export behavior. Format, foliage and deformation decisions remain open until this passes. |
| B · Representative dense grove | Combine several master variants, varied placement and dense overlapping canopy with representative Kit, raised lantern/accepted glass, moon, mist/fireflies, distant shell and the route/rope state that uses them. Preserve deferred effect limits explicitly; absent effects are a workload gap. Test near/mid/far transitions, turns, backtracking, trunk/Kit/rope clearance and source/destination overlap in both directions. Prove outdoor gates 2–5 and set explicit production budgets before bulk art. |
| C · Scaling toward hundreds | Step through recorded placement counts toward hundreds, with 200 as a possible initial stress point. Record total, camera-relevant, secondary/shadow-relevant, resident and collidable populations, unique masters/LODs and active wind groups separately. Increase density only while the agreed budgets, canopy quality and traversal hold; identify the limiting resource or workload when they do not. No blind hard-limit increase or inferred phone capacity. |

For each stage compare **static and moving exact builds** at fixed camera/route, lighting, seeds, render dimensions/scale, quality, backend and representative scene workload. Prefer a controlled wind toggle in the same exact binary when it preserves the comparison; if builds differ, bind each to its source, shader and asset identities. Include both moon-dominated views and lantern/accepted-glass views rather than benchmarking an empty daylight grove.

Run on Windows RTX and **Sam's Galaxy S26 Ultra**, identifying the actual model code, OS/driver and selected backend at test time. Verify both supported hardware RT backends where available; record unsupported or untested lanes explicitly. Use matched native Release/Shipping runs for player-facing comparisons and separate Diagnostic runs for attribution. Sustained warm behavior and repeat traversal/reset/resume matter alongside cold-start context; old phone success does not validate a new forest.

Each proof receipt must retain:

- Exact source/build type, binary/APK hash, shader and asset/manifest hashes, device/driver/backend, internal pixel dimensions, all relevant settings, camera/route order, seeds, duration and thermal/charging context.
- Frame-time/pacing distributions, tails/hitches and sustained trends, with collector scope labelled correctly; a median of window averages is not a per-frame median. Attribute CPU and valid GPU timing separately, marking unavailable GPU intervals rather than inventing them.
- Geometry/material/texture/CPU/GPU memory, BLAS/TLAS allocations and build/update costs, per-group deformation/upload work, residency/staging peaks and repeated transition/reset growth. UMA host/device-local classifications can overlap; do not add them as distinct totals.
- Matched stills and motion for leaf/canopy thickness, LOD transition stability, wind phase/repetition, fixed roots/trunk collision, coherent shadows/reflections, traversal/Kit clearance and two-way readiness/failure recovery. Record Sam's visual/feel review separately from measured performance.
- A pass/fail/unavailable disposition for each inherited gate and open choice, the measured count/group configuration, limiting factor, and proposed next step. New Android evidence belongs in the [device record](../../ANDROID_RT_DEVICE_COMPATIBILITY_RECORD.md).

**Budgets:** existing agreed chapter/asset/shader/quality budgets prevail. Outdoor gate 5 requires explicit numeric working budgets and acceptance thresholds from exact-target measurement before production. Define any unset frame/tail, sustained-duration, memory/overlap, AS-update or transition threshold with the owner rather than inventing acceptance in this proposal. The validation guide's descriptive FPS bands and matched-regression investigation rule remain in force; they do not certify a target forest population. Keep failures and missing evidence visible, with no silent quality/resolution reduction.

## Next implementation handoff

The first concrete package should establish one supported Easy Tree export recipe, preparation/validation tooling for that output, one master/LOD/material/collision/wind contract and its native proof. Then propose the smallest shared placement/deformation/lifetime extension warranted by that evidence, coordinate it with PR27's active terrain/actor work, and measure the representative grove before expanding. Decide foliage representation, wind grouping, residency policy and final counts from those results. This preserves the ambition to make the engine more capable while keeping the likely 1.7 delivery evidence-gated.
