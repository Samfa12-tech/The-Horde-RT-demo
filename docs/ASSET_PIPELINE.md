# Asset Pipeline

The runtime currently uses one animated skeleton and five CC0 Poly Haven material sets. Keep future imports bounded, licensed, and measured so the Android RT path does not accumulate unsafe or unfinished content.

## Asset rules

- All assets must be commercial-safe.
- Asset source and license must be recorded in `ASSET_LICENSES.md`.
- Meshy assets are allowed later.
- Meshy models must be textured before export.
- Do not import untextured Meshy models and call them complete.
- Prefer glTF/GLB where practical.
- Use high-quality PBR textures from the start when actual visual work begins.
- The first step should not import big assets yet.

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

## First visual environment direction

The first real visual work after the RT capability probe should focus on a very small historical gothic test room:

- Wet stone floor.
- One torch or lantern.
- Small puddle or reflective wet patch.
- Fog/smoke only after the basic RT path is stable.
