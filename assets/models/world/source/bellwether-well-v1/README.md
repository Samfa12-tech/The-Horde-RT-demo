# Bellwether square well

The covered stone well occupies the mapped T01 site at world XZ `(78.3, 126.2)`.
Its plinth follows the local terrain. Planks cover the opening under a clay roof,
with a bucket hanging from the timber winch. The square route passes south of
the well and continues to the tavern, the west stall and the mill.

The [Blender recipe](../../../../../tools/author-bellwether-well.py) takes the
town GLB from commit `7455b1e1` and a fresh output directory. The
[authoring receipt](authoring-receipt.json) pins the terrain, input town and
runtime export. The well adds 1,244 solid triangles to the existing opaque
town mesh and shares its stone, clay, timber and char materials.
