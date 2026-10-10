# Horde skeletal source kit v1

A bounded source kit for A17 / T02 tomb dressing and an eventual E06 skeletal-core adaptation. **This is a static Blender/GLB source candidate, not a game-ready enemy, canonical game-dev package, or admitted Horde asset.** No game files, repository state, accounts, paid providers, scenes, enemies or releases were changed.

## Contents

- `source/horde_skeletal_source_kit.blend`: editable, self-contained Blender 4.3.2 file. Three static prop masters are visible. Hidden collections hold source cages, one static whole-skeleton mesh and two linked-mesh arrangement examples. Jaw/teeth and cranium/teeth remain addressable as vertex groups. No external textures need packing.
- `source/fgc_skeleton.original.blend`: exact unchanged Gord Goodwin CC0 source. Open with automatic script execution disabled. The legacy 237-bone rig is retained only here and has no actions; do not regard it as a runtime rig.
- `meshes/`: seven portable GLBs, each one mesh, one primitive, one shared material definition, no skin/animation, no external dependencies. The three cage files are alternative coarse geometry for later LOD assessment.
- `arrangements/editable_examples.json`: two sparse, three-piece local arrangements. These are composition examples only, with no accepted niche dimensions, scene transforms, collision, density or instance-rendering contract.
- `scripts/`: build, source diagnostics, independent GLB structural verification and fresh-import render scripts.
- `evidence/`: exact geometry/bounds/normal/topology checks, build/render logs and fresh GLB-import studio renders. These are Blender Cycles renders, not engine screenshots. Two extra source-topology views mark open boundary edges orange and multi-face edges red; their overlays are diagnostic only and never exported.
- `provenance/`: source licence checks, original embedded notice, source-page snapshot and the distinction between CC0 geometry and newly authored material/scripts/placements.

## Measured geometry

All dimensions below use Blender X/Y/Z metres. GLB uses the standard Blender-to-glTF conversion: +Y up and +Z front. Each GLB's actual metre coordinates and identity transform are checked in `evidence/glb-validation.json`. The visible master objects in the editable .blend have only a presentation offset to separate them; exported mesh origins are clean.

| Asset | Vertices | Triangles | Dimensions, metres | Purpose |
|---|---:|---:|---|---|
| Skull and jaw master | 2,875 | 5,564 | 0.147350 × 0.197268 × 0.208233 | Bounded smoothing of cranium/jaw only; 32 teeth islands left at base topology |
| Femur master | 200 | 396 | 0.105733 × 0.103018 × 0.463929 | Actual FEMUR.L island; one smoothing level |
| Humerus master | 154 | 304 | 0.076145 × 0.044558 × 0.313043 | Actual HUMERUS.L island; one smoothing level |
| Skull/jaw source cage | 1,309 | 2,466 | 0.159710 × 0.205926 × 0.214912 | Unsubdivided alternative |
| Femur source cage | 50 | 96 | 0.105733 × 0.103018 × 0.463929 | Unsubdivided alternative |
| Humerus source cage | 40 | 76 | 0.076145 × 0.044558 × 0.313043 | Unsubdivided alternative |
| Whole static skeletal core | 8,128 | 15,585 | 0.772307 × 0.255069 × 1.720000 | Unsubdivided source candidate; eight anatomical groups preserved as vertex groups |

Source world units were normalized by 0.045849185918292054 to give a 1.72 m adult skeleton. Props retain that anatomical scale. Long bones have their major axis along +Z, proximal end high, lower bound at Z=0 and X/Y-centred origin. Skull front is -Y, up +Z and origin on the jaw support plane. Whole core has a floor-level origin. This is a plausible scale assumption, not an accepted E06 height or niche fit.

One shared, newly authored uniform matte warm-bone surface: linear base colour (0.46, 0.395, 0.29, 1), roughness 0.84, metallic 0. No textures, AO, opacity, emission, normal maps or baked illumination. Constant parameters are deliberately compact and portable; age is restrained in colour/roughness rather than painted stains. Fine weathering can be added after importer and art review.

## What passed, and what has not

Passed local checks: source checksum; source-derived island selection; finite positions and near-unit normals; valid GLB indices/accessors; one mesh/primitive/material per file; no skin/actions/textures; zero degenerate triangles; exact metric bounds; new scene import and studio render of every exported GLB. The femur and humerus masters are closed manifold meshes.

Important inherited topology limits:
- Skull source has 34 open boundary edges, all in the HEAD group; its selectively smoothed master has 68. These are inherited skull openings and remain open. The asset is not asserted to be a watertight thick shell. Cavities are geometry, not black decals.
- The static whole core retains 76 boundary edges and 35 other non-manifold edges, chiefly inherited vertebral topology. Original spine also has two loose wire edges that carry no faces and are omitted by face-based extraction; all 15,585 source triangles are preserved. Do not use the core as a volume/collision mesh or silently rig it as production topology.
- Smooth normals and a restrained subdivision level improve the small prop masters. The whole core is intentionally not globally subdivided; further core topology work and rig/skin adaptation are separate tasks.
- Cage alternatives are supplied without gameplay LOD distances, native runtime support or cost claims. The skull cage is visibly more angular around the brow, jaw and cranial silhouette in the matched close-up; its bounds also differ from the smoothed master. Treat it as a separately reviewed lower-detail option, not a no-pop automatic LOD. No close-up visual equivalence is asserted.

Still open: owner visual review; actual niche/shelf support and clearance; native Horde importer/material review; collision; phone/Windows RT budgets and BLAS/TLAS residency; true in-engine LOD assessment; E06 bespoke mechanical shell, limb axes, deliberate rig adaptation and animations. No enemy replacement or mechanical shell is included.

## Reproduce safely

Blender 4.3.2 was used. No game-dev CLI is installed in this environment, so no canonical certification is claimed. Use a new output directory to keep each run separate:

    blender --background --disable-autoexec --python scripts/build_source_kit.py -- --source source/fgc_skeleton.original.blend --output /absolute/path/to/new-output
    python scripts/validate_glbs.py --package /absolute/path/to/new-output
    blender --background --disable-autoexec --python scripts/render_fresh_imports.py -- --package /absolute/path/to/new-output

The build script rejects a source checksum mismatch. It only writes to its requested output directory; it does not alter the input source. The renderer imports exported GLBs into factory-empty scenes, adds temporary preview lights/floor/camera and writes PNG evidence. Its preview-only floor and lights are absent from all delivered GLBs. Denoising is disabled because this Blender build does not include OpenImageDenoise; the final evidence uses 96 samples.

See `provenance/CC0-NOTICE.txt` before transferring or admitting geometry. Package hashes are in `SHA256SUMS`; the archive checksum is supplied separately to avoid a self-hash cycle.
