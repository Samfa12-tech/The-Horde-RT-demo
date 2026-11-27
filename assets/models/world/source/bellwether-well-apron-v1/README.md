# Bellwether well paving

Eight worn flagstones link the water bench to the public lane at the foot of the south tavern ramp. Each stone follows the terrain. Its centre dips 10 mm below the rim and its sloping edge meets the ground. The tops and bevels supply 192 walkable triangles and use the town's stone finish.

The editable master is `bellwether-well-apron.blend`. The stone outlines and the positions of the well, bench and ramp are in `layout.json`. The export receipt records the source and runtime hashes.

To regenerate, extract `bellwether-tavern-v1.glb` and its sibling `asset.manifest.json` from commit `3aa4a3ff`. From the repository root, run Blender 5.2 with an unused output directory.

```powershell
blender -b --factory-startup --python-exit-code 1 --python tools/author-bellwether-well-apron.py -- . <parent-town.glb> <new-output> assets/models/world/source/bellwether-well-apron-v1/layout.json
```
