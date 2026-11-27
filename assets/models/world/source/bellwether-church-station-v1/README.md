# Bellwether church record station

An oak writing ledge sits on stone corbels along the south wall of the church.
It holds the keeper's daybook and writing slate. A hooded oil cup on a wall
bracket lights the keeper and the work surface. The keeper still addresses the
player from the existing church walk. The ledge sits against the wall, clear
of the link to the graveyard.

The original Blender master is `church-record-station.blend`. The [authoring
recipe](../../../../../tools/author-bellwether-church-station.py) adds 744 solid
triangles to the shared town model and reuses its stone, oak, clay, and iron.
The exported GLB keeps the town's six primitives and existing ground mesh.
The flame and light positions come from the same geometry as the oil cup.
The simulation selects the cup through the shared Bellwether exterior light
slot.

Regenerate with Blender 5.2 from the repository root, using the exact input
town and adjacent manifest from `f3be3b1e`, and an unused output directory.
The [receipt](authoring-receipt.json) pins the input, terrain, recipe, and
export hashes.

```powershell
blender -b --factory-startup --python-exit-code 1 --python tools/author-bellwether-church-station.py -- . <parent-town.glb> <new-output-directory>
```
