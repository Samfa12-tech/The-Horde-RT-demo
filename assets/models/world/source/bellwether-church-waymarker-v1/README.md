# Church fork waymarker

The original carved CHURCH sign stands beside the road cache. Its post,
fingerboard and lettering turn together 45° toward the church lane. A
player approaching slightly off-centre from the tavern can pass the arrow
tip and return on the same line.

`church-waymarker.blend` contains the three posed pieces from the original
tavern master. The [recipe](../../../../../tools/author-bellwether-church-waymarker.py)
matches their exact positions and triangle counts in the existing town GLB,
then rotates their positions, normals and tangents around the foot of the
post. The same 364 sign triangles supply hardware RT and player collision.
The native movement test reproduced the original snag from the recorded
phone position. It now clears that line and both shoulders of the church
lane in both directions. The road cache remains reachable from the fork.

To regenerate with Blender 5.2, use the town GLB from commit `e1ef8e88`
and a fresh output directory. The [receipt](authoring-receipt.json) pins
the master, parent GLB, recipe and result.

```powershell
blender -b --factory-startup --python-exit-code 1 --python tools/author-bellwether-church-waymarker.py -- . <parent-town.glb> <new-output-directory>
```
