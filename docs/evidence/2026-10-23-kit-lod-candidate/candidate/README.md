# Isolated Kit candidate — UNADMITTED

This is the single owner-approved study, stored separately for review and backup.
**It does not replace or select the production Kit asset. Do not ship it.**
The production Kit, player world body and first-person arms are unchanged.

Candidate SHA-256:
`30fea4a66bd1a31b48ee0d24414c48e22570870ff97d141860c93a99c3d0bb43`.
Original runtime SHA-256:
`1ba628235814b5912ae1439b180347dd8e1cce85b0f4d1fae5bbacb41e7d3713`.
Original source is the existing owner-cleared Meshy Warden in
`assets/models/characters/source/kit-warden-v1/`; the existing Kit statement in
`ASSET_LICENSES.md` applies. No new acquisition, generation, retargeting or licence
claim. The package manifests still select only the original under `assets/`.

One installed gltfpack 1.2 invocation proposed indices for eligible torso/limb
triangles: ratio 0.5, relative error 0.005, locked proxy borders, no quantization
or permissive/aggressive simplification. Final geometry grafts exact source rows
onto the untouched source BIN prefix; optimizer rig/animation/attribute outputs
are not adopted. See the invocation, provenance, generation receipt and inert
`prepare_kit.py.txt` snapshot. No second candidate, repair or tuning pass occurred.

Vertices fall from 26,945 to 21,539 (20.06%); triangles from 25,968 to 19,922
(23.28%). Exact protected regions include head/neck and bind Y at least 1.55 m,
hand contacts, boots at or below 0.45 m, and neighborhoods of inherited topology
issues: 11,428 rows and 11,075 source triangles. Rig, seven clips, materials and
all 457 nongeometry accessor byte regions remain unchanged. The original manifest
is a conservative maximum budget, copied unchanged, not an exact new count.

This audit GLB is **3,736,968 bytes versus 2,151,672 original** because it retains
unused original BIN bytes for identity proof. It is not a smaller shipping file.
No repacking or production admission follows from its lower native geometry cost.

The two `.u32le` maps contain unsigned little-endian 32-bit source vertex IDs:
`source-row-map` has one entry per candidate vertex in native primitive-0 order;
`protected-source-rows` identifies the protected source set. `SHA256SUMS.json`
binds the backed-up artifact and receipts. No owner campaign record is included.

The [completed review](../README.md) records native parity, matched actual RT,
the bounded CPU result and material risks. Inherited source defects are retained
as baseline; new exact-position topology and normal-orientation findings remain
unresolved. This is not a blanket zero-defect rule for the original Meshy asset.
