# Bellwether square fingerpost and tavern delivery

The fingerpost stands between the covered well and the south tavern ramp at
world X/Z (80.20, 129.20). A tankard points up the ramp, scales point west
to the dealer, a wheel points along the mill lane and an anvil marks the
smith yard. Each oak arm carries its symbol on both faces. An oil cup on an
iron bracket lights the boards from the approach side.

A coopered ale cask rests on timber skids and chocks on the far side of the
ramp rail, at (85.85, 124.15). The cask, sign and lamp use the town's
existing materials. Their 1,972 triangles provide both visible geometry and
solid contact.

The [layout](layout.json) gives the lane bearings and view positions. The
[editable Blender source](square-waymarker.blend) contains the full set.
The Debug `bellwether-square-waymarker` checkpoint approaches the marker
from the west side of the square. The `bellwether-square-frontage`
checkpoint looks up the tavern ramp.

To rebuild the layer, extract the original runtime town GLB and its sibling
`asset.manifest.json` from commit `3d4f1bd4`, keeping them in one directory.
From the repository root, run Blender 5.2 with that GLB and a fresh output
directory.

```powershell
blender -b --factory-startup --python-exit-code 1 --python tools/author-bellwether-square-waymarker.py -- . <parent-town.glb> <new-output-directory>
```

The [receipt](authoring-receipt.json) pins the parent town, terrain, recipes,
source master and exports. The runtime package uses the combined town GLB
and its updated manifest.
