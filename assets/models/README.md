# Models

Source art includes a staged Meshy sword under `weapons/meshy/` and a runtime-integrated animated Meshy skeleton under `enemies/meshy/`. The skeleton uses a narrow animation loader; the sword is not runtime-integrated.

Future model rules:

- Prefer commercial-safe glTF/GLB models.
- Meshy models are allowed later.
- Meshy models must be textured before export.
- Check scale, normals, UVs, materials, texture links, animation clips, and runtime cost.
- Record every model in `ASSET_LICENSES.md`.
- Validate triangle count against the Android RT BLAS budget before importing a model into the renderer.
