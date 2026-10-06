# The Horde 1.7: original forest dressing

Seven bounded low-poly source candidates: three rock masters, one fallen trunk, one root/branch piece, and two fern clumps. This package does not claim complete Horde 1.7 coverage, engine integration, or hero close-up quality.

## Asset inventory

Dimensions are Blender X/Y/Z in meters (width/depth/height). Portable GLBs are Y-up and grounded at Y=0. Each asset has a horizontal bounds-centered ground origin.

| Asset | Triangles | Materials | Source maps | Dimensions (m) |
|---|---:|---:|---:|---|
| Low boulder | 378 | 1 | 3 | 1.657 × 1.243 × 0.835 |
| Upright crag | 376 | 1 | 3 | 1.058 × 0.920 × 1.351 |
| Split shelf | 788 | 1 | 3 | 1.989 × 1.099 × 0.523 |
| Fallen trunk | 544 | 2 | 6 | 3.415 × 0.893 × 0.999 |
| Root / branch piece | 548 | 2 | 6 | 2.134 × 1.560 × 0.754 |
| Open fern | 1200 | 1 | 1 | 1.557 × 1.698 × 0.799 |
| Small fern | 768 | 1 | 1 | 1.136 × 1.112 × 0.562 |

Total: 4,602 triangles across the seven masters. Four original material families: slate, bark, weathered endgrain, and foliage. Ten unique source maps, all 512×512. There are nine material primitives across the seven GLBs; this is a file inventory, not a measured engine draw-call count.

## Contents and reproduction
- assets/: Seven portable textured GLBs. Each embeds required textures for standalone use.
- source/Horde-1.7-forest-dressing.blend: Editable mesh scene, exploded for inspection. Texture links are package-relative.
- source/build_forest_dressing.py: Deterministic original geometry/export generator, Blender 4.3.2. Run with blender -b --python source/build_forest_dressing.py.
- source/generate_stone_maps.py and generate_endgrain_maps.py: Original texture generators requiring Python, NumPy, and Pillow. The provided maps can be used directly.
- source/shared_tree_material_provenance.py: Preserved upstream original tree authoring source, for lineage only; it is not the dressing build entrypoint. Shared map bytes are copied unchanged from the original tree-pair package.
- source/render_wireframe_evidence.py: Renders readable unlit actual-triangle wire evidence after the standard closeup/silhouette render script.
- previews/: Actual fresh-import closeup, wireframe, and silhouette views for every asset, plus log cut-end detail and a contact sheet.
- validation/: Machine-readable source, binary GLB, fresh-import, pixel-range, and shared-texture lineage evidence.

## Shared texture handling
Bark uses the exact approved original tree-pair maps: basecolor sRGB, normal and roughness Non-Color, tangent normal strength 0.70, metallic 0, IOR 1.4. Cylindrical UV U wraps one circumference and V repeats per 1.7m of path length. Ferns reuse the original alder green map, roughness 0.84. Stone and endgrain are newly authored procedural PBR sets. Cut faces use their own endgrain map and planar UVs.

The GLBs repeat shared maps for portability. An importer should reuse shared maps/materials using the included lineage names and hashes. Across the kit there are 10 unique embedded images: 7,864,320 decoded RGB bytes (7.5 MiB), or a conservative 10 MiB RGBA8 allocation before mipmaps (about 13.33 MiB with full mip chains). These are format estimates, not measured GPU residency. Actual engine format, compression, mips, and deduplication determine runtime use.

## Verified and unverified
Verified: GLB v2 structure; finite positions, normals and UVs; normalized stored normals; explicit triangle/material/image counts; opaque materials; 512px embedded images; ground-origin bounds; fresh Blender GLB import; no near-degenerate imported triangles; actual imported-mesh renders; source-map lineage hashes.

The game-dev CLI was absent. These are independent Blender/Python checks, not game-dev canonical-package validation, Khronos validation, Horde runtime validation, or GPU/mobile performance certification. Native/mobile/RT behavior remains untested. There is no collision mesh or physics certification in this package. This archive introduces no engine integration or public game release.

## Visual and topology limits
These are restrained low-poly dressing candidates. Close-up limitations include hard rock faceting, visible repeated bark structure, simple tube intersections at branch joins, and sparse angular fern pinnae. Fern pinnae are genuinely closed volumetric lenses, not alpha cards. Logs/roots consist of intersecting closed components rather than a single Boolean-unioned watertight shell. Rocks include a deliberately two-component split shelf. No claim is made that intersections are physically or collision-ready.

The project tree reference was used only as a visual direction cue. None of its pixels, impostor planes, or geometry are shipped as runtime content. The existing approved stump is not duplicated.
