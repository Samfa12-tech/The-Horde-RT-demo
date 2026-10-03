# Asset Pipeline

The runtime currently uses animated skeleton/placeholder-lich assets, production Meshy 7 sword/torch props, five CC0 Poly Haven material sets, seventeen FilmCow SFX clips, and project-created launcher/icon art. Keep future imports bounded, licensed, and measured so the Android RT path does not accumulate unsafe or unfinished content.

## Asset rules

The1.6.1 player pair is admitted as a five-region `WorldBody` plus two-region
`Viewmodel`. Exact runtime hashes are `f2c3f62b...` and `6f06d77e...`; manifests,
clip semantics and processing receipts must agree. The body remainder shares
the existing Body atlas group, and the viewmodel shares the Body/Gauntlet textures;
neither adds production atlas layers. Regenerate and compare the accepted pair
with `tools/validate-player-regeneration.ps1 -AcceptedPairDirectory` as documented
in `evidence/2026-09-27-segmented-seams/production-preparation/README.md`. This does
not change any asset licence or the owner-controlled Hotstrike issue.

- All assets must be commercial-safe.
- Asset source and license must be recorded in `ASSET_LICENSES.md`.
- Meshy-assisted assets are allowed when the underlying source permits distribution and the applicable Meshy attribution route is recorded.
- Meshy models must be textured before export.
- Do not import untextured Meshy models and call them complete.
- Prefer glTF/GLB where practical.
- Use high-quality PBR textures from the start when actual visual work begins.
- Preserve source/high assets in Git/LFS, but package only measured runtime GLBs/manifests, platform texture arrays, and licence records. Rejected/staged studies remain outside downloads.

## Meshy workflow rule

When using Meshy via MCP or any other flow:

1. Generate or import the model.
2. Generate/apply textures.
3. Verify material set and texture links.
4. Check scale, normals, UVs, and animation clips.
5. Export only after the model is textured.
6. Record the source and license/usage terms in `ASSET_LICENSES.md`.

Do not pass an untextured mesh into the game and call it complete.

## Production held-item route

- Sword runtime: `assets/models/weapons/runtime/gothic-arming-sword-rh-lod0.runtime.glb` (11,499 triangles, exact `Grip`).
- Torch runtime: `assets/models/props/runtime/gothic-hand-torch-lod0.runtime.glb` (4,999 triangles, exact `Grip`, `Flame`, and `Light`). The body carries no emissive/flame geometry; the temporary faceted flame remains an engine-owned effect for Task 4.
- Shared PBR arrays: `assets/textures/held-items/runtime/`, with sword layer 0 and torch layer 1 for base colour, normal, and ORM. Emissive is a single black fallback layer.
- Runtime textures are 1K mipmapped raw RGBA8 KTX2 on Windows and strict ASTC KTX2 on Android (6x6 base/ORM/emissive, 4x4 normal). No uncompressed Android fallback is allowed.
- The generic static PBR slot registers TLAS instance 3 for the sword and instance 1 for the torch while preserving the 20-instance TLAS and one-frame ownership contract. Socket composition stays in shared gameplay/render interfaces, not in asset-specific shader branches.

## Model format preference

Prefer glTF/GLB for models unless a better Vulkan-friendly pipeline is chosen later.

## Texture direction

### Material normals and real-world texture scale (1.6.2)

Author large forms, silhouettes, contact edges and actual occluders as geometry.
Medium relief may use geometry and provenance-recorded high-to-low normal baking;
fine surface detail uses normal maps. Blender/Meshy sources retain their source,
licence and bake settings. Bake surface detail only, never scene lighting, shadows
or reflections. Normal maps do not move physical ray intersections or silhouettes.

Static PBR and the production skinned world body/viewmodel use the same material
records and shading. glTF `normalTexture.scale` is a finite signed XY multiplier,
default 1, followed by normalization in lighting. Linear normal maps use glTF/OpenGL
+Y orientation; tangent W must be exactly +1 or -1. A mapped primitive requires
authored tangents: bake/export them before import. Unmapped primitives may omit
tangents and receive a finite perpendicular basis. Reflected/negative-determinant
nodes must be baked with corrected winding/normals before export; do not silently
repair them in runtime. UV0 is the supported texture coordinate set; bake alternate
sets and texture transforms into UV0 before export.

Optional named `materialOverrides` entries provide `normalScale` and
`textureScale: [u, v]`. Normal scale preserves finite signed values (zero flattens
the mapped normal); each positive UV multiplier must be within [1/1024, 1024].
Missing entries retain imported normal scale and UV multipliers [1, 1]. UV scaling
applies to base color, normal, ORM and emissive together, without new texture layers
or samples. Keep atlas-based characters/props at [1, 1] unless deliberately reauthored.
For repeating surfaces specify metres and desired repeats/metre in the authoring
record; stretching object geometry does not establish believable texture scale.
For example, a four-metre surface with UV span 1 and two repeats/metre needs an
authored UV span 8 or an explicit multiplier 8. Confirm size under moving light.

Dungeon world-projected materials retain 0.42 repeats/metre and a separate 0.34
normal blend by default. That blend is not equivalent to glTF XY scale 0.34; the
shared ABI records signed normal strength, two UV multipliers and blend separately.
Authored dungeon metadata indexes the same material buffer through the upper
surface-code bits; lower material/normal codes continue to own projection and
texture layer. No normal/detail texture is added by this core extension.

glTF occlusion affects unresolved ambient illumination only. Keep base color pure;
direct torch/local/moon lighting and actual RT-tested visibility do not receive an
AO multiplier. Roughness/metallic channels remain independent. This 1.6.2 policy
correction changes existing directly lit appearances and requires matched owner
image acceptance; do not compensate with an exposure/light-strength retune.

Use high-quality PBR textures from the beginning of visual work:

- Albedo/base colour.
- Normal.
- Roughness.
- Metallic where appropriate.
- Ambient occlusion where appropriate.
- Emissive for torches/lanterns/fire sources where appropriate.

## Original visual environment direction - completed baseline

The original post-probe baseline was a small historical gothic test room and is now implemented in expanded alpha form:

- Wet stone floor.
- One torch or lantern.
- Small puddle or reflective wet patch.
- Fog/smoke only after the basic RT path is stable.
