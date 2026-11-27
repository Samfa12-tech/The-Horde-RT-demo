# Bellwether south tavern handrail

The square approach rises 3.26 metres from the toe of the ramp to the south
tavern door. Four lengths of planed oak follow the east edge. Five pegged
uprights sit in stone sockets. The upper three spans have a lower guard
rail. The handrail starts uphill of the junction and ends before the door.
The well side stays open.

The [Blender recipe](../../../../../tools/author-bellwether-tavern-rail.py)
uses the south sill, toe and ramp width from the [original tavern
survey](../bellwether-tavern-v1/authoring-receipt.json). The editable
master is `tavern-rail.blend`. Its 1,096 triangles provide both visible
geometry and physical contact. The integrated town uses five primitives
and the existing four material rows.

For regeneration, extract `bellwether-tavern-v1.glb` and its sibling
`asset.manifest.json` from commit `6097cf84` and keep them together.
Run Blender 5.2 from the repository root with an unused output directory.

```powershell
blender -b --factory-startup --python-exit-code 1 --python tools/author-bellwether-tavern-rail.py -- . <parent-town.glb> <new-output-directory>
```

The [receipt](authoring-receipt.json) records the survey, parent town,
master and exported geometry hashes. The `bellwether-square-frontage` and
`bellwether-south-approach` views follow the playable arrival route.
