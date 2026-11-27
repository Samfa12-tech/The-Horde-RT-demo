# House A at the kitchen plots

The east door faces the village, at the entrance marked on the connected map.
The stone posts, lintel and landing come from House A in the original v4
master. The landing now has two lower treads on the east slope. The latched
door closes the frontage. House A remains an optional future interior.

The south wall faces the kitchen plots. A pan-tiled lean-to covers a draining
bench, wide washing basin and dipping cup. An open gutter falls towards the
hooped rain barrel. A pair of boarded shutters faces the garden lane.

The editable master is `garden-house.blend`.
The [Blender recipe](../../../../../tools/author-bellwether-garden-house.py)
isolates the 48 original masonry triangles by their authored vertices and
checks their full ownership before moving them. It adds 3,080 solid triangles.
The integrated town uses the same five primitives and four material rows.

Keep the town GLB and sibling `asset.manifest.json` from commit `13f7d942`
together. Run Blender 5.2 from the repository root with a fresh output directory.

```powershell
blender -b --factory-startup --python-exit-code 1 --python tools/author-bellwether-garden-house.py -- . <parent-town.glb> <new-output-directory>
```

The [receipt](authoring-receipt.json) pins the map, survey, master, terrain
and runtime. The combined Windows campaign walks around the garden grate
to both sides of the house and back before resuming the grate encounter.
