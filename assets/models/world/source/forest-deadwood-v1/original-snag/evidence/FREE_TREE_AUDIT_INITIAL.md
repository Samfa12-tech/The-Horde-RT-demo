# Horde 1.7 tree assortment: free-source audit

Checked 6 October 2026. No Git writes or paid generations were performed by this audit. No candidate below is claimed engine-ready or artist-approved.

## Direction and recommended assortment

Authority: `docs/superpowers/plans/2026-09-11-beyond-the-tomb-1.7.0.md`, planning commit `2dbbc6924fc32f1bcb74be411c1c6a150a164e31`, especially sections 8.1 and 13. The plan calls for about four modestly varied archetypes, real 3D trunks/branches/canopy, grounded historical-gothic art, real 3D LODs, 40–80m trail, and measured combined workloads. It does not prescribe particular tree species. Suggested complementary silhouettes are editorial proposals:

1. Existing ancient gnarled hero tree: retain the already produced candidate; no duplicate generation.
2. Slender upright broadleaf: narrow, rising irregular crown and fine forks; pale grey-brown bark, subdued green foliage. Place in smaller groups along trail bends. Avoid a scaled-down ancient gnarl.
3. Irregular evergreen: taller taper, uneven branch tiers, visible trunk intervals and broken silhouette; dark restrained green. Place on banks/background framing, not equal Christmas-tree rows.
4. Sparse dead snag: leafless forked vertical silhouette, visibly three-dimensional branches; sparse accents at tomb/old burial edge, not a second dominant ancient hero.

Two independently authored complementary masters plus the hero and one usable snag would meet the modest initial assortment. Changes in clustering, rotation, and approved scale can vary placement, but do not make new masters by scaling one tree. Final species, dimensions, and placements remain art/blockout decisions.

## Free candidate 1: Quaternius Ultimate Stylized Nature (May 2022)

Official source: https://quaternius.com/packs/ultimatestylizednature.html
Official download folder linked by that page: https://drive.google.com/drive/folders/1IV3bXHzkNvuNWFHPi4KPx-G4ghuxIuT-
Official glTF subfolder: https://drive.google.com/drive/folders/1-ob9Aade1RIM3A_XKoQAOG6XqF2IDAIC
Official Blender subfolder: https://drive.google.com/drive/folders/158bwG4K2swS9F0gL2EgdmZ80KmFY6xzr
License: CC0-1.0; https://creativecommons.org/publicdomain/zero/1.0/
Author: Quaternius. Optional credit: “Tree source by Quaternius, Ultimate Stylized Nature Pack, CC0-1.0.”

The official pack page explicitly labels CC0; this allows modified and raw-source redistribution, including a public Git source archive. Downloaded original `License.txt` also specifies CC0-1.0 but its heading says Ultimate Platformer Pack, apparently copied pack boilerplate. Retain that original text unchanged alongside the exact nature-pack page citation rather than silently correcting it. The page is the pack-specific corroboration. No paid licence or new subscription is required.

### DeadTree_1: promising, incomplete source candidate

Downloaded original glTF: Drive file `1hwIwnP7pCXc7b30fdbdia-QvfTpoa8NA`.
Downloaded original binary: Drive file `16hwrQQ_gThFwyPBlDuzDC4rH3YY_uhtX`.
Original Blender source listed at `11s1DirVKcp9inQtZkiVaFhmW7CaEixf-` (922,048 bytes), not downloaded successfully.

Direct byte inspection found:
- 4,792 triangles, 2,905 glTF vertices, one mesh/primitive, no animation.
- Position, normal and TEXCOORD_0 streams; valid finite positions and valid indices; no zero-area triangles at threshold 1e-12.
- Bounds: X [-1.6520121,1.440424], Y [-0.00986903,6.1857605], Z [-0.67636585,0.619011]. Height ~6.196m, width ~3.092m, depth ~1.295m. Scale comes from glTF metre convention; runtime player comparison is still required.
- One `NormalTree_Bark` material, omitted alphaMode therefore OPAQUE; doubleSided true; metallicFactor0 and roughnessFactor0.5.
- UV range [-0.970722,-3.713588] to [2.5554955,5.3], deliberate repeat-tiling rather than a unique0–1 atlas. Normal and diffuse image dependencies are external.
- Required images `NormalTree_Bark_Normal.png` and `NormalTree_Bark.jpg` are NOT present. The file is not a self-contained textured deliverable.
- Orthographic wire inspection confirms a real cylindrical trunk and forked branches; no tree billboard. Root flare is minimal; branch symmetry is moderately stylized. Suitable as a possible mid-distance snag after material/style review, not automatically a close hero.

A second Blender source, `DeadTree_4.blend` (19,903,168 bytes), is listed at `1F90WrR_Q2PZVj1PV6n5L5PG2t0iVeImx`. It might contain packed bark images but this is unverified; file size alone proves nothing. Downloads of these two exact official Blender files were interrupted by tool cancellation. Parent confirmed no user denial of free-asset research and permitted one bounded same-file retry; that retry also cancelled. Retrieval is held, with no fabricated complete package.

### BirchTree_1: reject as delivered

Downloaded descriptor Drive file `1K3kgivCdB10eg57AM0CwSZi6TvCwsvNh` (3,420 bytes); binary/textures not downloaded.
Descriptor declares 4,596 triangles:2,688 trunk and1,908 leaves. Foliage material uses alphaMode BLEND and doubleSided true. Current reviewed importer requires OPAQUE, and the plan explicitly forbids treating blended foliage as equivalent to tested cutouts. The 2022 pack preview also has bright hand-painted bark/foliage which would need deliberate art conversion. Do not claim it fails because all alpha foliage is forbidden: the plan permits measured MASK geometry after consistent primary/shadow/secondary-ray support is implemented. This specific asset is BLEND and not a ready current candidate.

## Free candidate 2: Poly Haven fir and small broadleaf sources

Official fir: https://polyhaven.com/a/fir_tree_01
Official small broadleaf: https://polyhaven.com/a/tree_small_02
Official licence: https://polyhaven.com/license
The licence expressly allows source redistribution; assets are CC0-1.0. Do not redistribute the website's example renders/page branding as if those were CC0 asset files.

The fir's official listing advertises roughly8M triangles across its source, geometry nodes/LODs, and twig alpha maps. Small Tree02 advertises roughly5M triangles, geometry nodes/LODs, leaf alpha maps, and a4.6m height; its listed botanical type is Burkea africana/wild syringa. These are source metadata, not inspected download counts or the counts of any specific LOD. Their existence is not evidence that all LODs are unsuitable. They require significant extraction, material and geometry audits, and possible retopology to establish a modest opaque real-time master. No such bytes were downloaded or validated in this pass. They are legitimate rights-cleared future production sources, not ready drop-in substitutes for the current compact grounded assortment.

## Generation briefs for genuine gaps

Use the existing hero reference for art coherence; preserve its colour/roughness language, but deliberately change silhouette. Each: isolated complete tree, trunk/roots at origin, metre-scale target established before export, UV and textured PBR materials, neutral source lighting with no baked moonlight, no surrounding terrain, vines, stones or props. Real three-dimensional foliage clusters with physical thickness/open gaps; no billboard/camera-facing planes or BLEND material. Broadleaf target initial height8–10m; evergreen initial height10–12m are proposals, not locked world scale. Start near8K rendered triangles per master only as a production test target, not certified device budget. Keep original source, all textures, parameters and licence/provenance. Inspect real all-angle branch connectivity, opaque canopy volume and self-overlap. Prepare lower real3D LODs only after silhouette review; 2K simplification previously damaged the hero's branches, so do not automatically repeat it.

Slender broadleaf: one slightly bent upright grey-brown trunk, three unequal high forks, narrow asymmetric crown made of separated muted-green volume clusters, visible negative space and branch transitions; small credible root flare, modest moss only near base. No fat ancient trunk, face-like knots, huge hanging moss or enormous spreading crown.

Evergreen: one tapered irregular upright trunk, asymmetrical whorls and varied hanging branch lengths, gaps revealing trunk, dark desaturated needle-mass volumes, one slightly broken or off-centre upper leader. Avoid smooth cone canopy, identical tier discs, perfectly radial symmetry or Christmas decorations.

If free snag dependencies remain unavailable, prefer a Blender-authored/derived snag with separately preserved source and bark rights over spending on another hero tree. Changing geometry/materials still needs its own validation and provenance record.

## Evidence boundaries and outstanding steps

No engine import, Vulkan RT render, Android/Windows performance, wind, LOD transition, canonical package verification or artistic acceptance has occurred. The `game-dev` executable was not found in the current cloud environment, so no canonical `game-dev package build/verify` result is claimed. Blender is installed, but the original .blend candidate files are absent. This audit is a source-selection handoff, not completed forest delivery.
