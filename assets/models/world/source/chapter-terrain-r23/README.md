# River bank conformance r23

Original Horde bounded follow-up to r22. Its six-metre water ribbon ended over
terrain about one metre below the water datum. The source now adds a separate
five-metre bank constraint beside the retained three-metre bed constraint. This
samples the narrow existing bank crest instead of extending water over a valley.
No original vertex, macro heightfield grid, core triangle, route, opening,
entry envelope, bound, texture or water-map endpoint changes.

The runtime derivative is 18,441,928 bytes (+33,000 from r22), SHA-256
`c636a67911b7f5c5381c30ec7a49e5d4ff15c70fe688b368ba52efe77b537774`.
37,846 triangles and 19,486 exported vertices fit the unchanged 39,618 / 21,134
limits. The runtime manifest and Forest Ground 0.5 material factor remain exact.
All 17,125 original source vertices and 15,610 protected core/envelope triangles
compare exactly with r21, including its 15,472 near-core triangles.

## Explicit new-crest authoring policy

Ordinary inserted vertices retain exact bilinear sampling of the existing grid.
Of 320 NEW vertices tagged on the longitudinal five-metre bank edges, 278 dry
samples also retain that exact value. Only 42 deficient new crests are raised.
They use `max(old, nearestLineWaterUp - 1 + max(distance - 3, 0) * 0.6)` with the
engine height offset applied. This is the existing continuous unblended carved
bank profile, including the actual distance at miter corners. It removes the
one-metre grid's crest alias; these 42 samples are **not** claimed to equal the
old bilinear field. Already-dry samples are never lowered.

Forty raises are capped at 0.50 m. Only these two reviewed original-outflow
endcap samples, at most 3.225 m beyond the grid's North -510 boundary, have the
separate 1.25 m positive cap:

| Atlas East / North | Exact positive raise |
| --- | --- |
| -50.935615498 / -512.936441114 | 1.097354250906 m |
| -51.178542583 / -513.224354696 | 1.129625891092 m |

The helper rejects any other excessive correction and every deficient crest
that cannot clear the actual widened 42-face water union by at least 0.01 m.
All 320 source crests clear it by at least 0.015085 m; the float32 geometry test
also passes the 0.01 m guard at every actual quantized water edge. All 13 river
station rows clear both five-metre edges (minimum sampled clearance 0.107196 m).
These finite tests do not certify every point along the entire shoreline.

## Reproduction and checks

Use the versioned `tools/author-chapter-terrain.py`, `chapter_regional_terrain.py`
and `chapter_water_conformance.py` with Blender 5.2 background/factory-startup.
Inputs remain topo-r2 `proposed_terrain_heightfield.npz`, chapter-entry-return-r03
`authoring-audit.json`, and the pinned d37f1638 baseline
`shared-coordinate-layout.json`. Run with absolute `--root`, `--source-npz`,
`--entry-audit`, `--regional-layout` and a NEW `--output-directory` whose basename
is `chapter-terrain-r23`. Existing output directories are refused. Exact input,
recipe and generated-file hashes are in `authoring-receipt.json`.

Passed locally: source conformance 12/12; regional foundation 5/5; existing
terrain validator (including 2,781 route probes, nine occupied tomb volumes,
three openings and 263 roof intersections); native intake against unchanged
runtime limits; native terrain support (2,781 queries, 616 sweeps, zero failures,
2,774,696 retained bytes); Khronos validator zero errors and warnings.

The local Blend, duplicate source GLB and surface JSON remain preserved but
Git-ignored. The existing tracked runtime GLB is the sole distribution copy.
Small manifests, receipts and these versioned recipes reproduce the candidate;
old source masters and rejected trials remain untouched.

The failed plain-grid last-row gate and failed blanket analytic replacement
are recorded in the bank receipt: the latter would unnecessarily lower five
dry samples and exceeded the original cap at the two terminal samples. Initial
float32-edge test failures are also retained. That fixture used pre-export
double points just outside finite quantized water faces; the final fixture
probes the actual quantized edge without weakening the clearance assertion.

Actual RT appearance, shoreline art, device acceptance and performance remain
separate pending gates. No game launch, desktop foreground or device action was
performed for this source export/admission slice.
