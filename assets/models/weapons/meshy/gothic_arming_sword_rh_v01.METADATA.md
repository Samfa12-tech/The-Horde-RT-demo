# Gothic arming sword, right-hand v01 - Meshy provenance

## Asset identity

- File: `gothic_arming_sword_rh_v01.glb`
- Generated: 2026-07-10
- Text-to-3D task: `019f48d9-df47-7c7a-8341-20e8a11adb8b`
- PBR refine task: `019f48dd-bbe8-7d0c-9450-c1c13c0c7f06`
- Model: Meshy-6, standard/realistic, triangle topology, requested 12,000 triangle target, bottom-origin prop.
- Refine: high texture richness and lighting removal enabled.

## Exact generation prompt

> Hero-game prop, single medieval gothic arming sword, intended for a right-hand first-person weapon in a dark torch-lit ruin: straight double-edged tapering blade, shallow central fuller, long straight cruciform guard with subtly downturned quillons, dark leather-wrapped one-hand grip, faceted steel pommel, restrained weathered silhouette. Upright, blade tip pointing up, pommel/hilt at bottom origin, isolated asset only, no hand, arm, sheath, scabbard, character, floor, stand, glow, particles, text, logos, or background. Clean closed geometry, crisp silhouette and believable proportions.

## Exact PBR texture prompt

> Authentic high-detail PBR medieval weapon: forged dark carbon steel blade with cool silver sharpened edges, faint hammer marks, shallow fuller, controlled oxidation and tiny edge nicks; aged blued-steel crossguard and faceted pommel with a small bronze inlay only at the guard centre; worn charcoal-brown leather grip; roughness variation, metallic/roughness/normal/AO maps, no baked directional lighting, no blood, no words, no runes, no logos, no magical glow.

## Verified delivered data

- GLB v2; 9,431,980 bytes; one mesh, one material, four embedded texture images.
- The material declares metallic-roughness, normal, and emissive textures.
- Downloaded 2,048 x 2,048 sidecar maps: base color, metallic, roughness, normal, and emission.
- Actual mesh cost: 27,130 vertices and 49,439 indexed triangles. This fails the intended 10–15k Android held-prop budget; get explicit approval before spending credits on remesh/LOD.

## License gate

The Meshy account plan was not available through the generation workflow. Confirm the plan before public release and update `ASSET_LICENSES.md`: paid/private output may be commercially used under Meshy terms; free output requires CC BY 4.0 attribution. This asset must not be published until that status is resolved.

## Integration gate

The current renderer has no GLB/PBR import, texture upload, material binding, dynamic BLAS update, or right-hand attachment path. This file is staged source art only.
