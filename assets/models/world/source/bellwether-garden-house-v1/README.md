# House A at the kitchen plots

The east door faces the village, at the entrance marked on the connected map.
The stone posts, lintel and landing come from House A in the original v4
master. The landing now has two lower treads on the east slope. The latched
door closes the frontage. House A remains an optional future interior.

The south wall faces the kitchen plots. A pan-tiled lean-to covers a draining
bench, wide washing basin and dipping cup. An open gutter falls towards the
hooped rain barrel. Beside the shelter, a partly open oak shutter reveals
a clay oil lamp. The window returns, sill and back of the recess are solid
masonry. They shape the light that reaches the path.
The narrow path from the kitchen plots skirts the grate and divides towards
the washing bench and east doorstep. Its 53 walkable triangles follow the
terrain. The original west lane retains its surface through the junction.

The editable master is `garden-house.blend`.
The [Blender recipe](../../../../../tools/author-bellwether-garden-house.py)
isolates the 48 original entrance triangles by their authored vertices and
checks their full ownership before moving them. It replaces two triangles
on the south wall with the opening and its five interior faces. The complete
household adds 3,124 solid triangles relative to the tavern-rail town.
The integrated town uses the same five primitives and four material rows.

Keep the town GLB and sibling `asset.manifest.json` from commit `13f7d942`
together. Run Blender 5.2 from the repository root with a fresh output directory.

```powershell
blender -b --factory-startup --python-exit-code 1 --python tools/author-bellwether-garden-house.py -- . <parent-town.glb> <new-output-directory>
```

The [receipt](authoring-receipt.json) pins the map, survey, master, terrain
and runtime. The combined Windows campaign walks around the garden grate
to both sides of the house and back before resuming the grate encounter.
The garden lane and window captures also save the completed RT frame record
for that image. From the fork, the player carries the lantern while the
window and the grate use two world lights.
