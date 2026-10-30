# Shared forest preparation checkpoint

Basis: `6797e9d3`. This admits an immutable **CPU preparation representation**, not production GPU instancing or a more complete forest view.

Two admitted LOD1 masters now own their asset-local payloads once; 64 records refer to those masters through the same rigid yaw/translation and fitted burial offsets. Existing callers still produce the original four-family world bake. No runtime asset, shader, placement, collision, terrain or quality change is selected.

The sealed control used the original forest implementation with a field-wise semantic digest added to its existing tests. The candidate produces identical receipts in both cases:

| Case | FNV-1a64 semantic receipt | Vertices / indices / placements |
| --- | --- | --- |
| Ribbon support | `3599892596236743553` | 371955 / 995058 / 64 |
| Actual chapter terrain | `4000162986566279105` | 371955 / 995058 / 64 |

The digest covers vertex fields, indices, regions, primitives, node transforms/names, material/texture factors, sockets, bounds and placement/report fields; it excludes pointer values and struct padding. It is a regression witness rather than a mathematical collision-proof certificate. The explicit prepared-handle path also compares to the compatibility wrapper.

Focused `horde_rt_forest_dressing_tests` passes: control 30.04 s, final candidate 31.11 s. These are test durations, **not renderer timings or performance gains**. New checks retain a handle past its original reference, preserve a prior handle on failed preparation and reject malformed master references, burial, material mapping, indices and incomplete triangles without mutating the owner or publishing a report. The independent review found the incomplete-triangle case; it was corrected before admission.

The initial C3538 test compilation failure is retained in `candidate-build.log`; the two declarations were corrected. A later owned sandbox rerun stalled after its MSBuild banner and was stopped; it produced no test receipt. The normal escalated focused build/test then passed. Full local logs remain under ignored `reports/forest-shared-master-20261024/`; the compact receipt and relevant build evidence are committed here.

Production still duplicates 323264 tree triangles. Real shared-BLAS/TLAS ownership, actual allocation/frame cost, surrounding forest depth and lifecycle retirement remain separate gates. See the [bounded RT proof and production boundary](../../design/HORDE_1_7_SHARED_FOREST_BOUNDARY_2026_10_24.md). Audio/haptic manual revalidation required: **NO**; no semantic or playback behavior changed.
