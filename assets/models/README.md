# Models

Source art includes staged Meshy sword and torch studies under `weapons/meshy/` and `props/meshy/`, plus a runtime-integrated Hotstrike Studio skeleton derivative under `enemies/meshy/`. The skeleton was textured, rigged, and animated with Meshy and uses a narrow animation loader. The sword and torch GLBs are not runtime-integrated or distributed; the live torch is a compact procedural RT model derived from the approved silhouette direction.

Future model rules:

- Prefer commercial-safe glTF/GLB models.
- Meshy-assisted models are allowed when the underlying source permits distribution and the applicable Meshy attribution route is recorded.
- Meshy models must be textured before export.
- Check scale, normals, UVs, materials, texture links, animation clips, and runtime cost.
- Record every model in `ASSET_LICENSES.md`.
- Validate triangle count against the Android RT BLAS budget before importing a model into the renderer.
