# Topographic terrain integration handoff

Reference checkpoint status: measurements are verified source/host evidence;
the scene, subtitle, three-character and tree integration changes described
below are still uncommitted and awaiting their final affected build/package and
physical checks. This documentation checkpoint does not admit that candidate.

Preparation for Eric's separate Blender terrain package; no replacement terrain
authoring or planning-grid runtime import was performed here. The existing
exterior remains a development blockout and does not satisfy final woodland art
or terrain-envelope acceptance. This record supplements the scene/subtitle
repairs on PR27 rather than starting another implementation branch.

## Verified reference and correspondence

The supplied revision-2 archive is 7,602,193 bytes, SHA-256
`5b3d1eadb7606e0348f19c9959aa34f901b3c1986350d69f4ebc6a21bc7cba8b`.
All fifteen entries in its FINAL-MANIFEST match their recorded bytes and hashes.
The forest and Bellwether terrain PNG pixels were inspected. The coordinate JSON
matches the immutable planning copy semantically; its Git blob is
`4ce1a6852c1572659b3498b948e538587053d829` at `d37f16381311c6e620f0c4a9d01c1cce83260dc8`.

References: [forest sheet](https://github.com/Samfa12-tech/The-Horde-RT-demo/blob/d37f16381311c6e620f0c4a9d01c1cce83260dc8/docs/design/world/connected/01-forest-track-1-7.webp),
[town terrain relationship](https://github.com/Samfa12-tech/The-Horde-RT-demo/blob/d37f16381311c6e620f0c4a9d01c1cce83260dc8/docs/design/world/connected/02a-bellwether-terrain-1-8.webp),
[coordinate source](https://github.com/Samfa12-tech/The-Horde-RT-demo/blob/d37f16381311c6e620f0c4a9d01c1cce83260dc8/docs/design/world/connected/shared-coordinate-layout.json),
[generator and limitations](https://github.com/Samfa12-tech/The-Horde-RT-demo/blob/d37f16381311c6e620f0c4a9d01c1cce83260dc8/docs/design/world/connected/README.md).

Use metres and this existing correspondence, without rotating/rescaling the
accepted tomb: engine X = map East + 78.3; engine Y = map Up - 31.55;
engine Z = 126.2 - map North. This is a proper axis rotation plus translation,
not a floor-origin change. The map's F01 marker is aligned to the existing safe
exterior landing, not to the shaft centre 2.4 m south of it.

| Landmark | Map East/North/Up | Engine X/Y/Z |
|---|---|---|
| F01 safe landing | -112 / 139 / 33.6 | -33.7 / 2.05 / -12.8 |
| F02 reunion | -107 / 132 / 33.0 | -28.7 / 1.45 / -5.8 |
| Intermediate bend | -94 / 126 / 32.1 | -15.7 / 0.55 / 0.2 |
| Shallow dip | -96 / 111 / 31.0 | -17.7 / -0.55 / 15.2 |
| F03 clue pocket | -82 / 106 / 31.3 | -3.7 / -0.25 / 20.2 |
| F04 lookout | -65 / 90 / 30.4 | 13.3 / -1.15 / 36.2 |

Measured proposed centreline length is 76.264196 m; maximum straight-segment
grade is 7.269005%. The references propose a 2.4 m clear trail, 6.4 m reunion
pocket and 7 m lookout pocket. These are authoring targets, not runtime collision
or performance acceptance. The current 24 m-wide procedural support envelope is
not a request for a 24 m-wide finished trail.

The supplied NPZ has 1,002 north rows by 612 east columns: 613,224 finite float64
heights, 0.5 m spacing, east -145..160.5 and north -260..240.5. The inspected forest
rectangle East[-130,-45], North[80,155] has 25,821 samples and heights
26.937588..34.206028 m. Heights at all six route points match the JSON. The full
grid includes later regions; do not export all 613,224 cells into the 1.7 scene.
Smooth the documented planning-grid junction spikes in a versioned derivative.

## Required measured interfaces for the Blender handoff

- Preserve lower support Y=-0.95, eye Y=+0.70 before local bob, and upper safe
  support Y=2.05. Eye remains 1.65 m above support. Do not move the tomb or rope
  to fit terrain. The physical shaft centre is X=-33.7, Z=-15.2; fixed rope
  anchor is (-33.7,3.678,-15.5).
- The lower aperture is X[-34.9,-32.5], Z[-16.6,-13.8]. Current coping outside
  extent is X[-35.12,-32.28], Z[-16.82,-13.58], top Y=2.23. The current safe apron
  is X[-35.12,-32.28], Z[-14.95,-11.92], support Y=2.05. Treat these as integration
  keepouts/interfaces, with the existing moved lid, anchor frame and clearance.
- Reserve all nine occupied room/passage rectangles from `ShowcaseRoute.h`,
  floor -0.95 through roof 1.35. `OccupiedTombVolume.h` supplies exact triangle
  clipping for exterior admission, including thin diagonal crossings. This is
  not a substitute for tomb collision or a licence to blanket-fill skylights.
  Keep the grate, entry skylight, waterfall apertures and rescue hole distinct.
- Provide a continuous envelope around/behind the rim, graded path and pockets,
  grounded boundaries and closed edge/underside treatment that cannot cross the
  tomb. A bottom extruded indiscriminately to Y=-5 caused interior curtains in
  the earlier blockout. Inspect sections and player-height 360-degree views.
- Deliver a uniquely versioned editable `.blend`, export recipe, GLB, exact
  hashes, bounds, triangle/vertex/material counts, and overhead/section/eye-level
  inspection views. Preserve every existing source/backup. Keep masters outside
  runtime packaging; no private assets or automatic archive admission.
- Bake axes/transforms with positive determinant and correct winding. Provide
  authored UV0 and tangents for normal-mapped materials. Reuse admitted material
  families and their real-world scale; no new transparency, lights or texture
  features. Android requires the established ASTC texture pipeline, without an
  uncompressed fallback. Record source and redistribution rights per asset.
- Mark walkable versus nonwalkable surfaces and shaft cutout explicitly. Runtime
  support must consume the admitted exported surface or a verified equivalent;
  the existing analytic route cannot silently remain under a different mesh.
  Test both directions, 0.24 m capsule clearance, run/turns, safe edges, readiness
  generation/reset and equipment/light attachment before expanding placement.
- Keep Bellwether a nonplayable real shell under the same transform. Preserve
  necessary off-camera shadows/reflections/transmission; camera invisibility is
  not residency permission. No shader budget changes or invented outdoor costs.

## Asset and measurement constraints

The owner-supplied original tree-pair package has been recovered, with all 48
manifest payloads preserved byte-for-byte. Native import rejected its missing
bark tangents; a separately hashed Blender tangent derivative keeps topology,
UVs and decoded texture pixels. The selected LOD1 pine/alder pair supplies ten
8.69/8.48 m bank placements under the approved 18-layer ceiling. The old atlas
layers remain byte-identical. This is a bounded scene admission, not an outdoor
GPU capacity or artistic acceptance result. Forest dressing, dead-snag and
stump packages are also recovered for inspection; the owner has now confirmed
paid-plan creation and public Horde redistribution of the exact Meshy stump.
Its rights blocker is resolved, but runtime admission remains pending.

Kit's exact Warden source has passed native import and 35 skinned poses across
seven source clips. The owner explicitly confirmed public Horde redistribution
rights for this model as well as the separate thirteen voice cuts. Visible Kit
remains unadmitted. The owner clarified that Kit appears above ground after the
Keeper has disappeared, so a fourth simultaneous instance is not required by
the design. The next bounded proposal must inspect reuse of the dedicated
character slot, its geometry/texture handoff and safe resource lifetime; it must
not assume the need for a larger character or atlas capacity. No private project archive
or unrelated source is transferred.

Terrain authoring has not started according to the owner. Independent repairs
and this handoff proceed now; runtime terrain integration waits for an actual
reviewed package. Required subsequent evidence: native import negatives,
surface/support correspondence, matched real RT views on both backends, full
rope/route/backtrack sweep, actual resource/timing measurements, and Android
touch/visual/lifecycle checks. CPU geometry counts and Blender views cannot
establish those passes. Final outdoor admission and mist #28 remain open.

## Next residency/Kit admission boundary

The owner's intended rope transition prepares and activates the forest, then
unloads tomb resources that no longer contribute to the active side. Descent
prepares and restores the tomb in reverse. Both waterfall corpse states persist
in shared simulation, rather than being lost with their GPU allocations. Kit
appears only above ground after the Keeper's disappearance; the dedicated
third-character slot may be reused instead of adding a fourth concurrent actor.

This checkpoint still uses shared resident GPU geometry and CPU-stage readiness.
It neither implements GPU zone unloading nor proves that reusing a TLAS index
alone safely transfers BLAS, vertex buffers, material textures or descriptors.
The next bounded proposal must measure the larger Kit mesh, specify generation-
safe preparation/commit/rollback and fence-safe resource retirement, and preserve
shaft/rim/rope plus any still-relevant secondary-ray/light contributors. A failed
preparation retains the last safe side, ordinary traversal has no loading screen,
and stale completions cannot resurrect a retired zone. Durable saves remain out
of scope. No fourth-instance or 19-layer increase is assumed or admitted here.

Owner direction: the connected world uses deliberate seamless preparation gates
(similar in intent to the owner's Jak and Daxter reference), rather than permanent
whole-world residency. Gate-controlled travel must overlap preparation, commit
only collision/render-ready destinations, and then retire eligible old resources.
This is an accepted design direction, not evidence of an implemented streaming
system or permission to drop off-camera ray/light contributors indiscriminately.
