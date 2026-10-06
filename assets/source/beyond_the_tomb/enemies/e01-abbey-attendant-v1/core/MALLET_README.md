# Abbey attendant maintenance mallet

Original low-poly hand tool for The Horde E01 Abbey-attendant pilot. This is a small, utilitarian iron-headed maintenance mallet with a worn wooden handle and restrained freshwater mineral/rust staining. No blade, spike, emblem, decorative weapon treatment, ocean growth, transparent cards, or fused character geometry.

## Deliverables

- `abbey_attendant_maintenance_mallet.glb`: self-contained glTF 2.0 binary, with all three maps embedded.
- `abbey_attendant_maintenance_mallet.blend`: editable source mesh, retained triangulation modifier, packed maps, material, lights, and review camera.
- `source/build_mallet.py`: deterministic local Blender authoring. Fixed seed 261006. No external assets, paid provider, or network request.
- `source/validate_mallet.py`: static GLB, editable-mesh, and fresh-import checks; fresh-import render.
- `textures/`: 512×512 base color, ORM, and tangent-space normal PNGs.
- `previews/`: front, side, head close-up, three-quarter, grip close-up, and fresh-GLB-import renders.
- `evidence/validation.json`: actual measured counts and checks.
- `evidence/asset_specification.json`: dimensions, axes, limits, and component descriptions.
- `provenance.json`, `manifest.json`: source/provenance and file-integrity records.

## Geometry and material

The delivered GLB has 516 triangles, 850 vertices after UV/hard-normal attribute splits, one mesh, one primitive, and one opaque, single-sided material. The editable base mesh has 264 vertices and 238 polygons. It consists of three closed component shells: wooden shaft, iron head, and iron neck collar. These shells intentionally overlap at mechanical joints; a hidden head eye/cavity is not modeled.

The overall Blender XYZ bounds are about 0.120 × 0.04274 × 0.340 meters. The handle runs from Z -0.055 to +0.285m. The iron head is 0.120m wide. This scale is authored in meters and is not inferred from an engine bone.

Three 512×512 PNG maps share one atlas. Base color is sRGB. ORM is linear with neutral R=1 AO, G roughness, and B metallic; wood is nonmetallic, iron is metallic with rough mineral/corrosion variation. No baked ambient occlusion is claimed. The normal map is OpenGL/tangent +Y with material strength 0.6. Alpha is opaque. There are no external image dependencies in the GLB, and images are packed in the Blender source.

## Grip convention

`Grip` is an identity root Empty at (0,0,0), with the mesh beneath it. It is a tool reference only, not a measured E01 hand bone/socket transform.

- Blender: handle points +Z toward head; head spans X; grip minor diameter is Y.
- Normal Y-up GLB export: handle points +Y toward head; head spans X; Blender +Y becomes glTF -Z.
- Nominal grip cross-section: 27mm × 23mm oval.
- Provisional 100mm grip region: Blender Z -0.040 to +0.060m, equivalent to glTF Y -0.040 to +0.060m.

The E01 hand requires local curling/repair because the source rig has no finger bones. Final RightGrip placement must be measured against that repaired hand. No attachment transform, collision hull, gameplay setup, or attack timing is supplied here.

## Validation boundary

Source and exported geometry have finite positions, no degenerate triangles, no opposed vertex normals, no degenerate UV triangles, and UVs inside the atlas. All three source component shells are manifold and outward-wound. Fresh Blender import succeeds with embedded maps and preserves bounds/counts. The fresh raw glTF mesh retains standard UV/hard-normal splits; a temporary validation-only positional weld restores the three closed shells. The GLB itself is not welded or changed by validation.

The local `game-dev` CLI and Khronos validator were unavailable. This is a source-plus-evidence package, not a claim of game-dev CLI certification or engine/game readiness. No game project, Git repository, user computer, or public deployment was changed.

The Blender exporter logs missing optional Draco support; this GLB is intentionally uncompressed and uses no Draco extension. It also logs a sampler warning while packing shared ORM/occlusion channels. The final GLB has the expected three embedded images, and the fresh-import render shows all three maps applied. These messages do not prevent the recorded checks from passing.

## Reproduce

Run against a new output directory with Blender 4.3.2 or compatible later version:

```sh
blender -b --python source/build_mallet.py -- --output-dir /absolute/new/output
blender -b --python source/validate_mallet.py -- --asset-dir /absolute/new/output
```

Construction and texture pixels are deterministic. Blender save metadata and render sampling are not promised to produce byte-identical source files across Blender versions.

## Provenance and rights

All geometry, texture pixels, and source code were originally authored locally for this asset. No third-party model, stock texture, image-generation service, Meshy invocation, or new public license was used. This package applies no new public license. Existing project ownership/agreements remain the applicable context; this file is not a separate license grant.
