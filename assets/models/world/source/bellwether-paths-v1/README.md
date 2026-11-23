# Bellwether lanes and paths

Original Horde path geometry follows the existing campaign routes. The
full widths vary around the main lane and narrower service paths. Junctions
share one surface, and the main lane skirts the well and the tavern wall.
The importer uses the existing town stone row for rendering and stone
footsteps.

`layout.json` lists engine-world X/Z centre points and half widths in metres.
The authoring script clips the path footprints to the exact terrain triangles,
retains their grade and raises the finished surface by 18 mm. The existing
tavern, churchyard, mill and smith surfaces remain in the town GLB. The
recipe writes a new output directory and preserves the input town and terrain.

Regenerate with Blender 5.2. Supply the original town GLB pinned in the
[receipt](authoring-receipt.json) beside its original `asset.manifest.json`.

```powershell
blender --background --factory-startup --python-exit-code 1 `
  --python tools/author-bellwether-paths.py -- $repo $originalTownGlb $newOutput
```
