# Horde dead snag 01 — source candidate v1

A no-spend complementary dead-tree source candidate for Horde1.7, adapted from Quaternius's CC0 DeadTree_1 geometry with newly authored neutral procedural bark PBR maps. Suggested public archive destination: `assets/source/beyond_the_tomb/forest/trees/dead-snag-01-v1/`. This is a reviewable source bundle, not runtime activation or an accepted final game asset.

## Contents

- `models/Horde_Dead_Snag_01_LOD0.glb`:4,792 triangles.
- `models/Horde_Dead_Snag_01_LOD1.glb`:2,395 triangles.
- `models/Horde_Dead_Snag_01_LOD2.glb`:1,198 triangles.
- All GLBs are self-contained glTF2 with one mesh, one OPAQUE double-sided material, embedded1024px base colour/normal/metallic-roughness PNGs, and no required extensions or external dependencies.
- `source/Horde_Dead_Snag_01.blend`: packed original replacement textures and all three source LODs; only LOD0 is visible. Blender4.3.2. Inspection lights/camera are not saved in this source or exported models.
- `source/DeadTree_1.gltf` and `.bin`: unchanged upstream geometry record. Original upstream glTF references two missing upstream bark images and is retained only as provenance. Use normalized GLBs or packed new Blender source for viewing. No upstream texture is used in the completed candidate.
- `textures/`: newly generated base colour, tangent normal, roughness and explicit ORM maps. `source/generate_bark.py` fully reproduces them from deterministic analytic signals/noise. No source photograph or licensed texture was sampled.
- `source/build_snag.py` imports original geometry, welds coincident vertices at0.1mm, moves trunk base to ground, applies original material, creates offline decimation candidates, exports GLBs/packed Blender and renders comparison images. Run after `generate_bark.py` using Blender4.3.2. It overwrites only this bundle's outputs.
- `source/inspect_glbs.py`: static binary validation. `evidence/`: six neutral offline Cycles views, metrics, full build log and retained initial denoiser-support failure. Build was rerun successfully with denoising disabled.

## Source, rights and attribution

Author: Quaternius. Pack: Ultimate Stylized Nature (May2022).
Official pack/CC0 declaration: https://quaternius.com/packs/ultimatestylizednature.html
Official download: https://drive.google.com/drive/folders/1IV3bXHzkNvuNWFHPi4KPx-G4ghuxIuT-
Geometry source IDs: glTF `1hwIwnP7pCXc7b30fdbdia-QvfTpoa8NA`, binary `16hwrQQ_gThFwyPBlDuzDC4rH3YY_uhtX`.
Licence: CC0-1.0, https://creativecommons.org/publicdomain/zero/1.0/ . It permits copying, modification and raw redistribution, including public Git. Preserve source links for provenance even though credit is not required.
The downloaded original `source/QUATERNIUS_LICENSE.txt` declares CC0 but has a copied “Ultimate Platformer Pack” heading. It is deliberately retained unchanged; the official nature-pack page independently and specifically declares CC0.
Suggested credit: “Dead-tree geometry by Quaternius (Ultimate Stylized Nature Pack, CC0-1.0); normalization, LOD candidates and original procedural bark prepared for Horde.”
Newly authored scripts and texture outputs are prepared for the user's Horde source archive. No third-party restriction is introduced by those original additions. This package does not change the wider repository's asset licensing policy or assert ownership of unrelated assets.

## Visual/technical limitations

Height6.19563m, width3.09244m, depth1.29538m at LOD0; glTF+Y up, Blender+Z up. Trunk base is at ground; minimal root flare needs burial/ground dressing. UVs tile beyond0–1 and require repeat sampling. No collision mesh, wind rig, animation, bone weights or runtime LOD controller is included. All three LODs retain the same1024px material inputs; there is no claimed texture-memory LOD saving.

The original mesh is an open-surface tree, not a watertight solid. Static welded-edge checks report280/273/156 boundary edges for LOD0/1/2 and zero nonmanifold edges. No degenerate triangles or invalid/NaN attributes were found. Double-sided material is retained. Do not use as a closed-volume collision proxy or assume perfect close-up branch junctions. Neutral front/side/back and oblique LOD renders preserve the overall silhouette; no missing major branch was observed. Repeating procedural grain, simple root and slender angular branch tips make this more suitable as sparse mid-distance dressing than a close hero.

Runtime importer compatibility is unproven beyond the static OPAQUE/glTF constraints. No Vulkan hardware-RT, Android/Windows performance, combined forest scene, runtime ray-visibility, LOD transition/wind or owner art-acceptance test was run. Triangle targets are production candidates, not certified device budgets. Final material artistic approval remains open.

The game-dev executable was absent from this cloud environment. Consequently this is an independently hashed source ZIP, not a canonical game-dev package/receipt or a vendor-admission result. Do not silently label it engine-ready.
