# Chapter entry return: source candidate

Original Horde architectural derivative of the accepted collapsed entry. The lower collapsed rubble, first twelve risers and blocked player boundary at Z=3.4 are retained. The upper six risers run east through a full-width landing and return south behind a masonry enclosure. The separately identified physical terminal seal is still present. This reduces maximum entry geometry height from 12.24 m to 6.00 m without moving the tomb, shortening the visible lower stair remnant or disguising a missing end wall.

Native source/runtime-form GLBs: 8,254 triangles, 12,131 vertices, two primitives/materials, within the unchanged original collapse manifest limits. Thirteen original frustum views have zero boundary misses and zero remote-seal hits; ten of the first twelve risers have visible sample points. All constituent meshes pass closed-manifold and UV checks. The original failed 18/24 straight-flight audits remain retained. None of these checks is in-game visual acceptance.

`chapter-entry-return.blend` is the portable editable master. `collapsed-entry-lod0.glb` contains the exact admitted original texture payloads. `runtime-candidate/` contains a compact atlas-routing derivative only: it is outside shipped runtime paths and is not scene-admitted. The recipe reports one existing singular Mikk tangent corner repaired using the established bounded perpendicular-frame export rule. No shader, texture, capacity or quality change.

Provenance: original Horde architecture and placement, with existing Poly Haven Boulder01/MedievalWall02 CC0 material and rock reuse. See the original collapsed-entry PROVENANCE.md, preserved `polyhaven-original/download-receipt.json`, `ASSET_LICENSES.md` and this directory's hash/export receipt. No enemy bones, private-project asset, paid generation or new source acquisition.

Reproduce from a fresh directory using installed Blender:

```
blender --background --factory-startup --python-exit-code 1 --python tools/compose-collapse-final.py -- --source assets/source/beyond_the_tomb/terrain/collapse-recovered-inputs/boulder_01_1k.gltf --approved docs/design/1.6.2-collapse --output NEW_REVIEW_DIR --revision 2 --stair-count 18 --upper-return --views none
blender --background --factory-startup --python-exit-code 1 --python tools/export-collapse-asset.py -- --review-root NEW_REVIEW_DIR --chapter-output assets/models/world/source/NEW_CANDIDATE_DIR
```

The legacy export path remains locked to its accepted hashes. New outputs refuse existing destinations. Native scene integration, matched opening RT views, terrain/interface admission and owner acceptance remain pending. Source preview illumination never enters the game.
