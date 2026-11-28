# Bellwether footway lamp

A hooded oil cup marks the turn from the river causeway up toward the mill wheel. Its oak standard is set into the bank beside the corner. The causeway, raised footway and shallow-water platform keep their surveyed positions. The lamp uses the existing Bellwether exterior light slot and shares the town mesh's ray geometry and collision.

The editable master and [authoring receipt](authoring-receipt.json) pin the stone footing, oak standard, iron hood, clay cup and wick. The stove-black iron, clay, stone and oak reuse the town's existing finishes.

To reproduce it, recover `bellwether-tavern-v1.glb` and `asset.manifest.json` from `233deae8` into a parent directory. Run Blender 5.2 from the repository root with a fresh output directory.

```text
blender --background --factory-startup --python-exit-code 1 --python tools/author-bellwether-footway-lamp.py -- . INPUT_DIRECTORY OUTPUT_DIRECTORY
```
