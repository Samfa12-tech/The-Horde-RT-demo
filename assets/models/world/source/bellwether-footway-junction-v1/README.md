# Bellwether footway junction

The north side of the river causeway carries a local stone shoulder across the footway junction. Both retaining faces finish below the walk surface where the two paths meet. The slope returns to the existing shallow-water floor at the seal-test mark. This uses the original surveyed terrain, finite river surface, and shore rail. The new faces are the geometry used for both movement and ray tracing.

The editable Blender source contains the revised grade and retaining faces. The recipe also trims the two corner lips in the existing town mesh and pins the inputs and outputs in the [authoring receipt](authoring-receipt.json). The revision adds five solid and twelve walkable triangles to the town.

To reproduce the export, put `bellwether-tavern-v1.glb` and `asset.manifest.json` from `79f1348b` in an input directory. Run the recipe from the repository root with Blender 5.2 and a fresh output directory.

```text
blender --background --factory-startup --python-exit-code 1 --python tools/author-bellwether-footway-junction.py -- . INPUT_DIRECTORY OUTPUT_DIRECTORY
```
