# House B and the lower lane

House B is a walk-in store room in the private Bellwether build. Its short
stone fork leaves the lower road at the bend below the church junction.
The east entrance matches the mapped B04 door. A shallow ramp and oak rail
bring the lane up to the threshold. The open leaf leaves room to turn and
return outside. Inside are grain bins, a sorting table and an earthenware rack.

The patch retains the surveyed footprint, floor elevation, north shutter
and existing tiled roof. It replaces the solid wall block and the old north
door surround. The stone floor, lane and ramp enter the shared walkable mesh.
The walls, rail, jambs and furnishings use the same geometry for rendering
and contact. The patch adds 1,534 solid triangles after removals and
30 walkable triangles. It reuses the town stone, timber, earthenware and char
finishes. The room is an optional detour with no new services or save state.

The editable `house-b.blend` contains the authored patch. The previous town
asset, shell master, connected map and terrain are pinned in
[`authoring-receipt.json`](authoring-receipt.json). Use the retained town GLB
whose hash matches `originalTownSha256` and choose a new output directory.

```powershell
blender --background --factory-startup --python-exit-code 1 --python tools/author-bellwether-house-b.py -- <repo-root> <parent-town-glb> <output-directory>
```

The recipe writes the merged runtime GLB and manifest, the native geometry
counts, and separate source and patch outputs. Only the merged GLB and its
manifest belong in the runtime town folder.
