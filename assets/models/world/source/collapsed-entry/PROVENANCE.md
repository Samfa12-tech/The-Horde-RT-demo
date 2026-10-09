# Collapsed entry source and export

The owner approved revision two for runtime export on 3 October 2026. The exact
private authoring contact sheet and editable Blender source are identified by
their SHA256 receipts in `export-receipt.json`; no private reports, screenshots,
transfer URLs or Library identifiers are distributed here. Native Runtime
visual, resource, performance and device acceptance remain separate gates.

The three angular rock meshes are measured derivatives of the free
[Poly Haven Boulder 01](https://polyhaven.com/a/boulder_01). Original glTF,
binary, three 1K JPG maps, download hashes and the captured public CC0 licence
are retained in `polyhaven-original/`. Structural masonry is authored in Blender
and uses the already admitted
[Medieval Wall 02](https://polyhaven.com/a/medieval_wall_02) maps. Both Poly Haven
families are CC0; attribution and their original authors are recorded in
`ASSET_LICENSES.md`. No paid generation or illumination bake was used.

`tools/prepare-collapse-texture-inputs.py` verifies original receipts and converts
the six decoded JPG maps to lossless 1K RGBA PNG atlas inputs without changing
pixels. `assets/textures/props/source/collapsed-entry/input-receipt.json` records
each original and derivative hash, dimensions and channel meaning. Base colour
uses sRGB; normal and ORM use linear encoding. Normal maps use OpenGL +Y;
ORM preserves R=occlusion, G=roughness, B=metallic. Metallic factors are zero,
roughness factors one, and authored normal strengths are .42/.34. Occlusion is
not multiplied into base colour or baked scene lighting.

`tools/export-collapse-asset.py` accepts only the hash-pinned approved Blender
source and Form approval. It excludes four inspection context meshes, bakes
world transforms and joins the 63 core meshes into two material primitives:
`Boulder01Rock`, then `MedievalWall02`. No camera, light, inspection floor or
inspection Cobblestone material is exported. The full source GLB embeds the
exact original 1K JPEG payloads; the runtime GLB embeds six 4px routing
placeholders and obtains production pixels from the shared platform atlas.

One source UV singularity yielded a zero Mikk tangent. The export tool authors
a finite perpendicular unit tangent from its real normal, preserving position,
UV and handedness. This correction is recorded in `export-receipt.json`; no
loader repair or asset-specific shader behavior was introduced. The approved
Blender source remains unchanged.

The runtime has 8,422 triangles, 12,467 vertices, 25,266 indices and exactly two
materials/primitives. The strict manifest supplies those measured capacities.
Khronos Validator 2.0.0-dev.3.10 reports zero errors and warnings for both source
and runtime GLBs. `validation-receipt.json` records a real CPU
`StaticMeshAsset` loader pass, finite mapped unit frames, nonmetallic opaque
materials and metre/Y-up bounds `[-2,-1.1,2.92]..[2,12.24,17.48]`.

The integration owner must terminate the existing flat roof at z2.94, add its
bounded neutral physical step closure and extend the ordinary world floor
beyond z3.4 under the enclosure. The independent gameplay collision cap,
spawn and gallery remain unchanged. Atlas layers append at 10/11; existing
layers and the compact preview subset remain stable. Only the runtime GLB and
manifest enter model packages; this source directory and source atlas inputs
are excluded. No hardware RT or driver allocation pass is inferred from these
CPU asset checks.
