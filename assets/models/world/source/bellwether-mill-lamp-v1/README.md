# Bellwether mill bench oil lamp

The clamp bolts to the north-west corner of the existing bench. A bent iron upright carries a turned clay oil cup and a rain hood. The runtime town GLB uses its existing five primitives and four materials. The lamp adds 372 solid triangles and shares the town's ray geometry and contact surfaces.

Near the bench, the lamp uses Bellwether's existing local exterior fire slot. The river fork lamp still occupies that slot at the shore. Both use the shared fixed-step fire simulation and hardware RT shading.

## Regenerate

Run Blender 5.2 in background mode from the repository root. Pass the town GLB from checkpoint `714dc6fd` as the second argument after `--`.

```powershell
blender -b --factory-startup --python-exit-code 1 --python tools/author-bellwether-mill-lamp.py -- . <previous-town.glb> <new-output-directory>
```

The recipe checks the parent town and terrain hashes before authoring. The [receipt](authoring-receipt.json) records the editable master, measured placement and generated outputs. Two independent runs matched the lamp GLB, merged town GLB, town manifest and generated anchors byte for byte.
