# Models

Staged source art includes a Meshy sword under `weapons/meshy/` and an animated Meshy skeleton under `enemies/meshy/`; neither is runtime-integrated.

Future model rules:

- Prefer commercial-safe glTF/GLB models.
- Meshy models are allowed later.
- Meshy models must be textured before export.
- Check scale, normals, UVs, materials, texture links, animation clips, and runtime cost.
- Record every model in `ASSET_LICENSES.md`.
- Validate triangle count against the Android RT BLAS budget before importing a model into the renderer.
