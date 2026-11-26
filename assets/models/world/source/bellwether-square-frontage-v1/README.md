# Bellwether square and tavern frontage

The water bench stands on the square side of the well. Its fetching pail and
earthenware jar sit clear of the west lane. The tavern's tankard sign hangs
beside the south ramp. Masonry-mounted braces support the overlapping clay
tiles and a rack holds split fuel beside the window. A bracket holds the
oil cup in front of the sign. Its wick uses the village's shared exterior fire slot.

The editable source is `square-frontage.blend`. The [Blender recipe](../../../../../tools/author-bellwether-square-frontage.py)
adds 2,068 solid triangles to the original town mesh. Their render and
contact surfaces share the same geometry. The town still uses five
primitives and four material rows.

The south approach runs close to the combined collision limit. The earlier
2,244-triangle export exceeded the 163,840-triangle intake by 124. Splitting
the billets and making the hoops part of the pail profile brought the final
export, including the oil lamp, to 2,068 additional triangles. That leaves
52 triangles at this configuration. Further streets need a more economical
collision representation before adding comparable detail.

To regenerate, extract `bellwether-tavern-v1.glb` and its sibling
`asset.manifest.json` from commit `17687eed`. Supply that GLB and an unused
output directory to Blender 5.2 from the repository root.

```powershell
blender -b --factory-startup --python-exit-code 1 --python tools/author-bellwether-square-frontage.py -- . <parent-town.glb> <new-output-directory>
```

The [receipt](authoring-receipt.json) records the terrain, input town, master
and exported geometry hashes. The Debug `bellwether-square-frontage` view
reaches the square using the same route as the player. The
`bellwether-south-approach` view checks the entrance from the ramp.
