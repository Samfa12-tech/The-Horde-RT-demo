# Models

Source art includes staged legacy Meshy sword and torch studies under `weapons/meshy/` and `props/meshy/`, audited production assets under the `weapons/source`, `props/source`, and `player/source` routes, and two sequential runtime enemies under `enemies/meshy/`: the Hotstrike Studio skeleton derivative processed with Meshy and the CC0 Meshy placeholder lich. Production packages include only bounded runtime GLBs, manifests, platform texture arrays, and attribution; source/high files remain Git-LFS evidence and are excluded.

Future model rules:

- Prefer commercial-safe glTF/GLB models.
- Meshy-assisted models are allowed when the underlying source permits distribution and the applicable Meshy attribution route is recorded.
- Meshy models must be textured before export.
- Check scale, normals, UVs, materials, texture links, animation clips, and runtime cost.
- Record every model in `ASSET_LICENSES.md`.
- Validate triangle count against the Android RT BLAS budget before importing a model into the renderer.
