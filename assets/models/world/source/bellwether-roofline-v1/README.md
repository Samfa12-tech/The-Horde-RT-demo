# Bellwether roofline v1

Original editable roof and gable refinement for mapped village buildings B01-B08. The main roof slopes use the textureless `HordeVillageRoofTiles` material; triangular end faces are masonry in `HordeTownStone`. Closed oak verge boards, gable collars and king posts, plus closed half-round clay ridge caps, are authored at metre scale.

`bellwether-roofline-v1.blend` is the editable derivative of the nine-building shell source, without dormant terrain meshes. `author-bellwether-roofline.py` regenerates this source bundle and a runtime candidate in a fresh output directory. The delivered runtime uses the original four material groups in their existing order, with the appended roof primitive last in `BellwetherSolid`; `BellwetherWalkable` remains separate.

The pinned parent content is the town GLB at `ec850e56` (SHA-256 `5c0b850b1159e5417f0d58800e8d38dce9c1a12b80f1ddd4c9e32a229b82e6de`) and its sibling manifest (SHA-256 `ac31c5dba4d4e4018fda77662201c2108f6ef42b0f22449770a685a7b826a3f6`). The repository stores the large GLB via LFS. On a fresh checkout, run from repo root:
```powershell
blender --background --python-exit-code 1 --python assets/models/world/source/bellwether-roofline-v1/author-bellwether-roofline.py -- . <fresh-output-directory> <ec850e56-town.glb> <same-directory-asset.manifest.json>
```
The recipe verifies both parent hashes, takes explicit parent inputs, and does not depend on reverting the installed runtime. The output directory must not already exist.

UV contract: exported glTF U runs along the ridge (Z) at 2.4 m per repeat; V starts at 0 on the eave and increases uphill at 2.4 m per repeat. Tangent handedness is derived from each outward roof slope normal.
