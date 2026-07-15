# Asset Pipeline

The runtime currently uses one animated skeleton, five CC0 Poly Haven material sets, thirteen FilmCow SFX clips, and project-created launcher/icon art. Keep future imports bounded, licensed, and measured so the Android RT path does not accumulate unsafe or unfinished content.

## Asset rules

- All assets must be commercial-safe.
- Asset source and license must be recorded in `ASSET_LICENSES.md`.
- Meshy-assisted assets are allowed when the underlying source permits distribution and the applicable Meshy attribution route is recorded.
- Meshy models must be textured before export.
- Do not import untextured Meshy models and call them complete.
- Prefer glTF/GLB where practical.
- Use high-quality PBR textures from the start when actual visual work begins.
- Preserve source assets in Git/LFS, but package only measured runtime assets; staged sword/torch studies remain outside downloads.

## Meshy workflow rule

When using Meshy via MCP or any other flow:

1. Generate or import the model.
2. Generate/apply textures.
3. Verify material set and texture links.
4. Check scale, normals, UVs, and animation clips.
5. Export only after the model is textured.
6. Record the source and license/usage terms in `ASSET_LICENSES.md`.

Do not pass an untextured mesh into the game and call it complete.

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
