# Same-topology bough study r2 — rejected

An editable, deterministic recipe for the owner-authored pine/alder originals.
This is retained study evidence, **not a runtime replacement or verified canonical
asset package**. See the [native RT comparison](../../../../../docs/evidence/2026-10-26-forest-crown-r2/README.md).

The study spreads existing closed opaque lens forms outward from their original
base vertex, with coordinated positive affine transforms of positions, normals
and tangents. Wood/moss geometry, images, materials, indices and UV bytes remain
exact. No new renderer path, opacity, model, placement, collision or capacity is
introduced. Pine stays5,000 triangles/5,602 exported vertices; alder stays5,102/
5,632, with three materials each. The original runtime manifest is unchanged.

From the worktree, using the already installed Blender/numpy runtime:

```powershell
& 'C:/Program Files/Blender Foundation/Blender 5.2/blender.exe' -b --threads 2 --python assets/models/world/source/forest-crown-study-r2/reshape.py -- --output reports/<fresh-study>/candidate/assets
```

The output must be fresh and inside the worktree. Production originals are read
only. The recipe edits packed GLB geometry directly and preserves image buffers;
it never invokes Blender's glTF exporter or accesses/changes its previously
denied temporary-image directory. No permission or temporary-path setting changes.
The separate preview imported both variants and saved an editable comparison at
`reports/forest-crown-r2-20261026/editable-crown-comparison.blend`.

The local `game-dev` CLI is unavailable; canonical package verification was not
run. Static checks, Blender import/preview and actual native RT evidence are
different stages. This candidate fails visual and route admission. Full GLBs,
blend, raw screenshots/CSV/OBJ data remain ignored rather than adding rejected
generated assets to downloads. The small recipe and receipts retain provenance.

The next useful art change is actual clustered crown structure and background
woodland mass, preserving trunks/roots/route; simply enlarging lens sprays is
not enough. A successful geometry candidate will also need a fresh reviewed
static-retention proof. Do not replace its exact count or safety criteria merely
to admit different geometry.
