HORDE 1.7 — ORIGINAL WOODLAND TREE PAIR — CANDIDATE v1.0

Two original, sparse real-geometry tree assets for a grounded historical-gothic woodland:
• Upright alder: narrow young forked broadleaf, asymmetrical crown and exposed branches.
• Irregular pine: uneven open bough tiers with visible trunk windows and solid needle-spray forms.

These are complementary candidates, not replacements for the approved ancient gnarl hero.
No image billboards, alpha leaf cards, provider generation, third-party geometry, or bought assets.
Each leaf/needle-spray is an individually closed volumetric mesh part. Moss strands are closed tubes.
All mesh parts are opaque. Branch intersections deliberately overlap; this is a game prop, not a single watertight printable solid.

CONTENTS
source/: editable packed-texture .blend files and deterministic Blender 4.3 authoring script.
runtime/: self-contained GLB candidates with compact original 512px PBR textures.
textures/: original mathematical bark base color, tangent normal and roughness; foliage and moss base colors.
previews/: actual exported-GLB Cycles renders from two sides, with close source bark inspection.
qa/: build statistics, exported-byte checks, geometry/import checks and final manifest.

REBUILD
blender -b --python source/build_horde_trees.py -- --output /NEW/OUTPUT/DIRECTORY
Use a fresh output directory to preserve supplied evidence. Same source and seeds reconstruct geometry.
Original source seeds: alder 7319, pine 4126; original texture seed 74107.
Python inspect_runtime.py additionally requires numpy and Pillow outside Blender.

SCALE / RUNTIME
One Blender unit = one meter. Source is Z-up; standard GLB scene is Y-up.
Asset origins are (0,0,0), at ground level. Exact size/ground bounds are recorded in qa/mesh-summary.json.
Only tree meshes are exported. QA floor, camera and lights remain in editable .blend sources for repeatable review.
Three meshes/materials per asset: bark, foliage, moss. No armature, animation, collision or runtime wind is supplied.
Any collision, instancing, occlusion, runtime LOD-switch integration and wind work belongs to a later engine integration task.

PROVENANCE / RIGHTS
Original geometry and procedural texture source authored for The Horde.
No paid provider was called. No externally licensed source geometry or textures are included.
No new public license assigned by this package; owner controls publication.
Art direction only referenced the existing approved project ancient-tree render. That image is not included and is not runtime geometry.

VALIDATION BOUNDARY
Local static GLB byte inspection and Blender geometry/round-trip import are distinct from game-engine/GPU import.
The game-dev CLI was absent in this environment. Its canonical package build/verify and policy-validation gates have NOT run.
Treat these as art-reviewed source/runtime candidates for project review, not as integrated or benchmarked game-ready assets.

VISUAL REVIEW / KNOWN LIMITATIONS
Local visual review classified these as restrained mid-distance source candidates, not hero-closeup finished art.
Canopies are intentionally sparse. Solid foliage reads chunky at close range and is not a botanical simulation.
Branch and root junctions are overlapping closed parts; close views reveal intersections and texture discontinuity.
Revised bark avoids the rejected regular-stripe trial. Final runtime files contain corrected nonblack PBR images.
Root tips extend about 7mm below the ground plane to avoid apparent floating; origins remain ground level.
No runtime integration, performance benchmark, wind simulation, collision, runtime LOD-switch integration or public release is claimed.

TRUE-3D LOD1
Each tree also includes a conservative lower-detail GLB and editable .blend, named *-lod1.
Alder LOD1: 5,102 triangles. Pine LOD1: 5,000 triangles.
All branch centerlines, tube radii and leaf positions remain identical. Tube radial sides and leaf perimeter resolution are reduced.
Closed 8-triangle leaf lenses replace the 12-triangle full-detail form. No branch or leaf cluster is removed.
This is still real opaque 3D geometry, not an impostor. The crown is not aggressively decimated.
Source rebuilding: add --lod1 to the authoring command. There is no automatic runtime LOD-switch integration.
See qa/lod-comparison.json for numeric front/back silhouette overlap and centerline/attachment checks.
