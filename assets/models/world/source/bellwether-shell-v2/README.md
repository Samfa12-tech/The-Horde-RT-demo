# Original Bellwether distant shell

Original Horde geometry authored in Blender5.2 from the reviewed nine footprints
in `docs/references/horde-1.7/d37f1638/docs/design/world/connected/shared-coordinate-layout.json`.
Eight village masses and the separate shrine remain distant scenery; no interior,
quest, actor, light, flame or town support/collision admission is included.

The editable master preserves separate foundations, walls, roofs, cold chimneys,
church tower/spire and the current native terrain as hidden reference geometry.
The runtime export contains only selected shell geometry:342 triangles, two
native material groups, metre scale, projected2m texture repeats, outward winding,
UVs and pitched roof normals/tangents. Native integration maps these groups to
the already-admitted MedievalWall02 and TombEarthenware families. No new map,
texture layer, material row, primitive row or shader feature is needed.

Run `tools/author-bellwether-shell.py` with Blender background, `--python-exit-code 1`
and an absolute repository root. The tool refuses existing source/runtime
destinations and disables Blender backup writes. For another revision, select
new versioned output paths before running; never overwrite this master or v1.

The receipt pins the coordinate JSON, actual exported terrain and runtime GLB.
Each foundation extends from the lowest sampled support minus0.2m to the larger
of the proposed floor or highest footprint support plus0.08m. The maximum floor
adjustments are recorded, rather than hidden by moving the accepted tomb or
floating the town. Detailed1.8 pads/doors/materials remain future work.

v1 is a preserved rejected source iteration: its box/spire face winding was
inward. v2 corrects that winding without rewriting v1. Native signed-volume
tests guard outward closed stone and roof families. No runtime package admits v1.

Original Horde geometry is authorised for public Horde distribution by the
chapter-authoring task. The reused terrain/stone textures retain their existing
individual provenance/licences in ASSET_LICENSES.md; this receipt does not
relicense them or claim all repository assets are MIT/CC0.
