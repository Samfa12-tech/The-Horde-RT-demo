# Bellwether market and working yards

The [topographic survey](../../../../../docs/references/horde-1.7/d37f1638/docs/design/world/connected/02a-bellwether-terrain-1-8.webp) fixes terrain and placement. The [coloured use plan](../../../../../docs/references/horde-1.7/d37f1638/docs/design/world/connected/02b-bellwether-use-1-8.webp) records the public routes, working parcels and margins. Foundations follow the registered terrain in the existing world coordinates. The owner's *Moonlit Village Hub Map.png* guided the relationship between market, well, yards and woodland.

The two covered pitches face a shared apron southwest of the well. Low stone courses edge the market and the working plots by the garden, store, common and tavern. Supply boxes and split fuel sit at the tavern, store, common and smith. A pollarded tree and smaller plants mark the edges, and a short spur connects the common supply yard to the smith lane.

The [layout](layout.json) and [editable Blender master](working-square.blend) capture the new scenery. Masonry and timber enter the existing static contact mesh. The spur enters the walkable stone mesh. Verge leaves use the same existing render-only material and atlas as the kitchen garden crops.

To reproduce the layer, recover the two parent GLBs and their two manifests from `abcc3b9b` into one folder. Use Blender 5.2 from the repository root with a fresh output folder.

```powershell
blender -b --factory-startup --python-exit-code 1 --python tools/author-bellwether-working-square.py -- . <parent-folder> <new-output-folder>
```

The [receipt](authoring-receipt.json) pins the parent files, surveys, terrain, recipe and outputs.
