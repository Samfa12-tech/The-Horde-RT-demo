# 1.6.2 — Collapsed entrance and reusable rubble kit

**Owner request: 2 October 2026.** The wall behind the initial player position should read as the passage they entered through, now collapsed. Retreat is impossible; the usable route is forward into the dungeon. This is a future 1.6.2 demo-polish slice after completed, accepted, merged and released 1.6.1.

## Locked scene intent

Create a static aftermath, not a new falling-rock cutscene, destruction simulator or physics puzzle. Keep the silent protagonist. A player who turns around should understand the blocked route from geometry alone, without a caption or new voice line.

- Show surviving doorway/arch jambs and a broken upper span around a short, genuinely recessed passage. The passage is packed with large irregular rock, fallen dressed stone and smaller debris.
- Put two or three dominant angular boulders across the blockage, with fractured masonry that visibly matches the surrounding dungeon. One tilted lintel/arch fragment and damage above connect the rubble to the ceiling collapse.
- Use an asymmetric settled pile: broad grounded base, interlocked middle, broken upper edge. Avoid a neat row of identical rocks, smooth river stones, a freestanding pile with walkable gaps around it, or a textured plane pretending to have depth.
- Keep a sealed backing behind the rubble to prevent world/light leaks, but recess it so the original flat wall does not remain the visible surface. Model real depth and contact shadows through hardware RT. Any cavity is clearly impassable.
- Reserve the entire player spawn/camera/held-item envelope, skeleton encounter space, forward route and entry pickups. Do not reduce overhead torch clearance or worsen the separately recorded roof-clipping issue.
- Small debris tapers onto the floor toward the player but stays out of the traversable route. Use a few grouped static meshes rather than scores of individually simulated stones.
- No continuous dust volume or extra light is required. An optional short settling sound/dust accent may be considered only after the static scene reads correctly and an appropriate approved asset and measured budget exist. Do not obscure the forward route.
- This collapse is a deliberate exception to the room skill's default open arrival. It depicts an unusable previous entrance. Document the blocked opening and its sealed destination; do not widen the live route or alter the whole room to satisfy a generic template.
- Original dungeon lighting remains dynamic hardware RT. Do not use the room skill's generic baked-lighting baseline in Horde. Surface normal baking is allowed; scene lighting/shadow/reflection baking is not.

## Asset strategy

Prefer suitable existing Briarhold assets, then verified free assets and simple authored masonry. Meshy is a fallback if those cannot meet the visual brief. No paid generation is required by this plan.

**Selected free starting source:** [Poly Haven Boulder 01](https://polyhaven.com/a/boulder_01). The official page lists a 1.8 m-wide, 124K-triangle source with Blend/glTF/USD/FBX downloads and 1K–8K textures. Use it as source art for a reduced hero rock, not as an approved mobile runtime mesh.

**Optional shape variety:** [Rock Moss Set 01](https://polyhaven.com/a/rock_moss_set_01), six rocks with a listed 63K-triangle asset total (not a verified per-rock count), the same model formats and 1K–8K textures. Select only useful shapes. Reject or adapt the moss-heavy treatment if it does not match the dry dungeon interior; don't import an entire set merely because it is free.

**Optional masonry material reference:** [Castle Wall Variation](https://polyhaven.com/a/castle_wall_varriation), a 2 m-coverage PBR material with 1K–8K maps. This is a material, not a ready-made rubble mesh. Prefer the existing dungeon stone wherever it already works. Do not wrap mortar patterns around individual loose stones or enable runtime displacement.

The [official Poly Haven licence](https://polyhaven.com/license) releases these assets as CC0 and permits commercial use and redistribution. Retain source URLs, the licence snapshot and exact downloaded-file hashes in project provenance even though attribution is not required. Source-page specifications were checked on 2 October 2026 AEST; no downloaded topology, LOD availability or engine import has been validated. These sources remove the need to spend Meshy credits for a first attempt.

Build a small reusable kit: approximately three distinct rock shapes, two broken masonry shapes and one grouped rubble patch. Reuse rotated/scaled instances judiciously, with correct transforms, to create variety. These counts are art-production starting points, not measured renderer limits. Match the masonry to the existing dungeon material; geological rocks should support that palette rather than introducing unrelated moss, snow or desert sand.

## Blender preparation brief

When asset preparation is separately started:
1. Preserve each downloaded original and licence/source record. Work on a copy in a dedicated source-art/output area; never modify an upstream source in place or copy an unverified provider file directly into the game.
2. Inspect actual topology, texture channels, units, UVs, tangents, material slots and dependency files. Report source triangle counts and bytes before deciding a reduction target.
3. Normalize to metres and sensible grounded origins; apply transforms and correct axes. Remove cameras/lights, loose geometry, accidental internal shells, degenerate faces and duplicate surfaces. Repair normals and seams; retain believable fractures and silhouette.
4. Produce a restrained mobile runtime tier, initially around 1–3k triangles per dominant rock and cheaper dressing, then adjust using real silhouette and RT-cost evidence. Do not ship a raw dense scan or claim a triangle target alone proves performance.
5. Preserve/bake high-to-low surface normals if decimation needs them. Verify tangent-space orientation under moving light. Keep large cracks and silhouette relief in geometry.
6. Start with one shared 1K or 2K material set where visually adequate, use appropriate mipmaps and the accepted native compression/import route. Avoid unique large textures per stone; do not assume Blender procedural materials or a particular KTX2 path import without checking the released engine.
7. Author broken masonry in Blender from simple solid stone forms when reuse is insufficient; it should inherit the admitted dungeon material rather than inventing a new style. Visible broken edges need geometry.
8. Save editable .blend source and export runtime GLB through the established project pipeline. Keep visible meshes and simple collision definitions identifiable; visual contact and collision must agree.
9. Inspect and validate exported bytes, render a neutral contact sheet and representative moving-light views. Record source/runtime hashes, transforms, dimensions, triangles, material/texture counts, compression and changes. A Blender render is an authoring check, not a native RT runtime pass.

Use Game Development Studio capability discovery and its current inspect/validate/normalize/package/admission workflow if available. Read the current command schema instead of assuming the plugin installs Blender or a working CLI on every machine. Present exact input/output paths before approval-gated normalization/package writes and use a dry-run admission before copying into the project. Preserve the game's existing asset layout and provenance contracts.

## Placement, collision and performance

Inspection of PR15 head 03a6870 places spawn at x=0,z=1.85, floor y=-0.95 and eye y=0.70. The chamber spans x±1.85 with roof y=1.35; the visible rear cap at z=3.4 is only 1.55 m behind spawn, with a sealed rear shell at z=3.47. PresentableTinyRtScene.cpp currently adds the visible mossy-stone cap specifically to prevent a turn exposing the sky. Preserve that containment while changing the visible geometry. The gallery occupies left-rear x[-1.55,-0.72],z[0.05,2.35]; keep it clear.

Recess the collapse mainly behind the current cap rather than filling the 1.55 m spawn clearance. Alter only the bounded rear architecture needed to depict passage depth; don't casually enlarge gameplay space. Validate the full player and held-item volumes using current source, not eye position alone.

The current source ABI has 21 instance-metadata entries, nine static assets, 32 primitives, 32 materials and 16 texture layers. Older asset-pipeline prose says 20 TLAS instances and is stale on that count. These are capacities, not proof of free slots. Audit actual use and prefer a consolidated static collapse cluster/asset registration over one new TLAS instance per pebble. Any required expansion needs its own measured change. Source anchors: [scene](https://github.com/Samfa12-tech/The-Horde-RT-demo/blob/03a6870f66dd9cb4bab62d2f521f4fd663368678/src/vulkan/raytracing/PresentableTinyRtScene.cpp) and the released RtSceneAbi.def. Recheck after 1.6.1 release.

The blockage is static, so imported rock geometry should use the shared static GLB/PBR and immutable-BLAS path. Do not add per-rock shader branches, physics bodies or animation/skinning work. Reuse geometry/material resources and respect actual instance/material capacity; measure any capacity expansion explicitly.

Collision must prevent backing through, squeezing around or climbing over the pile while matching the visible barrier closely. Keep the safe spawn and forward route clear across supported movement/look/held-item states. The pile remains a blocker at every quality setting; a lower-detail tier must not turn it into a traversable or visually open exit.

Measure the existing opening/skeleton scene before and after using matched camera, scale, quality, backend and warm thermal context. Record startup/loading, CPU/GPU frame time, allocations and texture/geometry bytes where available. Reused geometry still incurs instance/traversal cost. Do not pay for the asset by silently lowering render scale or optical quality.

## Ready-to-use Codex handoff

Implement only the accepted 1.6.2 collapsed-entry slice after the 1.6.1 start gate is met. Read AGENTS.md, ROADMAP.md, this brief, the accepted asset pipeline/licence records, and the actual entry geometry/collision/spawn ownership. Inspect available local skills and assets first.

Use the selected free/reused sources above, record exact provenance and actual file properties, and prepare a small reusable rubble kit through Blender and the existing asset pipeline. Keep the authored target: a once-traversable passage behind spawn, visibly blocked by settled boulders and fallen matching masonry, with clear broken architecture above and a safe forward route.

First present a simple layout/contact sheet showing the pile, broken opening, spawn clearance, skeleton space and blocked return. Obtain the room workflow's required layout/Form approvals before its gated final composition/export stages. Do not interpret this prompt as spend approval or permission to publish. If paid Meshy generation becomes necessary, stop with a bounded asset brief, current credit/cost quote and exact approval request; do not submit or automatically retry paid jobs.

Reconcile static collision, source/render transforms, shared instance/material limits and native RT visibility. Add appropriate asset/scene contract checks and validate the exported runtime assets, both platform builds, actual Android/Windows RT presentation, walking/backtracking/turning/torch clearance, retry/reset and the opening encounter. Capture the player looking back from spawn and moving close/sideways, not just an attractive Blender angle.

Finish with source/licence manifest, editable source and runtime output paths, hashes, changed files, actual tests and measurements, before/after runtime stills and motion, limits/gaps, and the exact next step. Do not expand into a new cave, outdoor section, animated collapse, third skeleton, boss rework or renderer redesign.

## Acceptance

The old exit reads unmistakably as a freshly collapsed passage with believable depth and structural damage; the player cannot return but can move forward safely. It matches the dungeon's art, remains clear at supported phone resolutions, uses traceable assets, and preserves combat/torch/collision behaviour. Source-package validation, native RT review and measured device cost must all be distinguished. Owner visual approval is required before calling the scene accepted.

This document organises asset sourcing and the implementation prompt. It does not claim downloads, Blender cleanup, runtime import or generation have occurred.
