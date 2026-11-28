# Bellwether market and working yards

The [topographic survey](../../../../../docs/references/horde-1.7/d37f1638/docs/design/world/connected/02a-bellwether-terrain-1-8.webp) fixes terrain and placement. The [coloured use plan](../../../../../docs/references/horde-1.7/d37f1638/docs/design/world/connected/02b-bellwether-use-1-8.webp) records the public routes, working parcels and margins. Foundations follow the registered terrain in the existing world coordinates. The owner's *Moonlit Village Hub Map.png* guided the relationship between market, well, yards and woodland.

The two covered pitches face a shared apron southwest of the well. Worn flagstones and two stepping stones lead from the public lane to the vegetable trays and earthenware counter. A clay oil lamp on a braced oak post stands between the pitches and lights both counter fronts. Low stone courses edge the market and the working plots by the garden, store, common and tavern. Supply boxes and split fuel sit at the tavern, store, common and smith. A pollarded tree and smaller plants mark the edges, and a short spur connects the common supply yard to the smith lane.

The store path skirts the east end of the market wall and rejoins the mapped approach to the east door. A loading bench, plank dunnage and two banded casks share the yard. The lane leads to the store frontage, whose door is closed in this update.

The [layout](layout.json) and [editable Blender master](working-square.blend) capture the new scenery. Masonry and timber enter the existing static contact mesh. The paths and slabs enter the walkable stone mesh. Verge leaves and the greens in the trays use the same existing render-only material and atlas as the kitchen garden crops. Blender exports the store path stations and the oil cup's flame and light anchors alongside the geometry counts. The simulation selects the lamp through the existing exterior light slot near the market.

To reproduce the layer, recover the two parent GLBs and their two manifests from `abcc3b9b` into one folder. Use Blender 5.2 from the repository root with a fresh output folder.

```powershell
blender -b --factory-startup --python-exit-code 1 --python tools/author-bellwether-working-square.py -- . <parent-folder> <new-output-folder>
```

The [receipt](authoring-receipt.json) pins the parent files, surveys, terrain, recipe and outputs.
