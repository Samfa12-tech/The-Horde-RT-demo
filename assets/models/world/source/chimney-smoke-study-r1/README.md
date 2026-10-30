# B01 open-flue source study — not a runtime replacement

This is an editable derivative of Horde's owned Bellwether shell v4, prepared for
the approved High/Low chimney-smoke study. The original runtime asset remains
unchanged. No new town gameplay, material, light or texture is introduced.

`prepare_open_flue.py` reads the hash-pinned v4 GLB and writes an isolated candidate
under `reports/chimney-smoke-20261024/open-flue/`. It raises the existing B01 flue
top by 0.7 m, replaces its sealed cap with a rim, inward well and floor, and keeps
all other town attribute/index rows, nodes and materials exact. The result has
982 triangles, 1,990 vertices and two material primitives. The outlet is
`(95.36, 0.3804492092, 109.28)`, 4 cm inside the raised mouth.

`verify_open_flue.py` independently checks the resulting B01 component's oriented
edges and probes the open mouth/rim. Run both scripts from the worktree root with
Python 3. No exporter, denied crown temporary directory, permissions change or
third-party generator is used. The earlier parked owner B01/v5 work is untouched.

`source/b01-open-flue-r1.blend` retains the imported derivative and a close
inspection camera with **authoring-only** area lights. Those lights are not game
assets or baked lighting. `previews/b01-open-flue-close.png` is a Blender Cycles
geometry inspection, not a Horde RT smoke screenshot or material-quality pass.
The mouth and well are visibly open; inherited untextured-looking source preview
and distant runtime framing do not establish the intended smoke story cue.

The isolated Windows pre-smoke control imported and presented this geometry with
the original shading. No smoke candidate bank passed the existing shader gates,
so no High/Low effect image or GPU cost was obtained. This source is retained
independently; the normal loader, triangle guard and renderer were restored.
See [the rejection and evidence](../../../../../docs/evidence/2026-10-24-chimney-smoke-gate/README.md).

Provenance: original project geometry, same underlying MedievalWall02 and
TombEarthenware material owners as v4; see `ASSET_LICENSES.md`. Source files and
previews are excluded from game packages. `asset.manifest.json` pins the original,
derivative and editable-source identities. Regeneration does not admit it.
