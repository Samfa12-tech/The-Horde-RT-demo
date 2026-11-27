# Bellwether relic dealer's handcart and balance

From the square, the west lane passes a loaded handcart parked against the
outside of the dealer's stock screen. Its two wheels rest on the surveyed
cross slope and its shafts point away from the lane. The player can round the
corner post to visit the counter or carry on to the kitchen plots. Inside the
stall, the equal-arm balance stands beside the weights and the existing wick.

The editable Blender master is `west-trade.blend`. The [authoring
recipe](../../../../../tools/author-bellwether-west-trade.py) builds the
cart, sealed jars and balance in the town's existing timber, char, clay and
stone materials. The same 2,360 triangles are used for rendering and solid
contact. The integrated town has five primitives, four material rows and
32,864 triangles.

To regenerate, extract `bellwether-tavern-v1.glb` and its sibling
`asset.manifest.json` from commit `cc1fa62e` and keep them together. Run
Blender 5.2 from the repository root with a new output directory.

```powershell
blender -b --factory-startup --python-exit-code 1 --python tools/author-bellwether-west-trade.py -- . <parent-town.glb> <new-output-directory>
```

The [receipt](authoring-receipt.json) records the recipe, inputs, editable
master, isolated patch and integrated town. The combined campaign's west
stall entry and trade views show the exterior and the counter at player height.
