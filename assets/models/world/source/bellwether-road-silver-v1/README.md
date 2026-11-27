# Bellwether roadside silver

A hollow silver drinking cup lies on the surviving lid board of the cache at
the church fork. Its mouth is yawed 15 degrees toward the player's approach.
A 7.1 degree pitch seats the lip and foot rims on the board, with the tapered
body between them. A curved U strap projects from the near side, attaches at
both ends and leaves a clear finger opening in the west-facing silhouette.
The generated header places the model in world space. The receipt records the
two contact heights and sightlines to the rim, shoulder and handle.

The one-object Blender master is `BellwetherRoadSilver.blend`. Rebuild the
master, GLB, manifest, generated header and receipt from the repository root
using Blender 5.2.

```powershell
blender --background --factory-startup --python-exit-code 1 --python `
  assets/models/world/source/bellwether-road-silver-v1/build-road-silver.py -- .
```

The 452-triangle mesh uses one opaque `WornSilver` material with a metallic
factor of 0.82 and roughness of 0.62. Its highlights come from the scene lights.
