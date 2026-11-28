# Bellwether south tavern threshold

The east side of the south opening moves 24 cm outward. The masonry lintel,
oak jamb and head trim leave 2.304 m between the jambs. The leaf spans the
wider opening with its original west hinge and swing angle. Its pull and the
shared sweep follow the wider leaf. The tavern footprint and ramp follow
the topographic survey.

The original [tavern authoring recipe](../../../../../tools/author-bellwether-tavern.py)
defines the masonry, frame and leaf geometry. This adjustment works on the
integrated town and the hinge-local leaf from commit `1daa1a74`. Retrieve
both Git LFS objects and place the two GLBs in a local input directory. Run
Blender 5.2 with an unused output directory.

```powershell
blender --background --factory-startup --python-exit-code 1 --python tools/author-bellwether-tavern-threshold.py -- <input-directory> <output-directory>
```

The [authoring receipt](authoring-receipt.json) pins the inputs and both
exports. Render and collision use the same meshes. Both exports meet the
original vertex, triangle and material budgets.
