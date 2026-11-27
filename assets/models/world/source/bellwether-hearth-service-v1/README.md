# Bellwether hearth service

A clay water jug, a beaker and a turned wooden trencher sit on the table beside Kit. Against the north wall, the serving dresser holds upright platters, spare beakers and a lidded storage crock. The keeper can approach from the south aisle.

The editable Blender master is `hearth-service.blend`. The [recipe](../../../../../tools/author-bellwether-hearth-service.py) fits the pieces to the original tavern floor and table. The 2,440 triangles use the town's existing oak and clay finishes. The chair and table retain their three distance-based detail levels and the same tabletop height in each.

To reproduce the export, extract `bellwether-tavern-v1.glb` and its sibling `asset.manifest.json` from commit `dcb1f4fe`. Run Blender 5.2 from the repository root with a fresh output directory.

```powershell
blender -b --factory-startup --python-exit-code 1 --python tools/author-bellwether-hearth-service.py -- . <parent-town.glb> <new-output>
```

The [receipt](authoring-receipt.json) records the source and runtime hashes. The combined private Windows campaign checks the occupied hearth from the conversation mark and saves the presented view as `windows-combined-tavern-table.png`.
