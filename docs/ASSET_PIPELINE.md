# Asset Pipeline

The runtime currently uses animated skeleton/placeholder-lich assets, production Meshy 7 sword/torch props, five CC0 Poly Haven material sets, seventeen FilmCow SFX clips, and project-created launcher/icon art. Keep future imports bounded, licensed, and measured so the Android RT path does not accumulate unsafe or unfinished content.

## Asset rules

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
