# Mist interval feasibility correctness run

CPU reference and float32 GLSL-arithmetic mirror only; no GPU was launched.

Result: **FAIL** (6 bounded fixtures; 12 frozen ray/source pairs; 288 scattering equivalence comparisons).

| Fixture | Callbacks | Selected raw | Outcome |
| --- | ---: | ---: | --- |
| aperture-0-overlap-disconnected-thin | 5 | 4 | valid |
| aperture-1-source-filtered-displaced | 5 | 3 | valid |
| raw-64-accepted | 64 | 64 | valid |
| raw-65-rejected | 65 | 65 | rejected-raw-capacity |
| callbacks-512-accepted-64-selected | 512 | 64 | valid |
| callbacks-513-rejected-before-source-filter | 513 | 64 | rejected-callback-capacity |

The equal-length displaced control has distinct endpoints and per-cell vectors. Zero, negative, NaN, and infinite cell lengths reject.
Frozen CPU inputs: 6 predeclared pixels, both apertures (nonempty witnesses {'source_0': [838, 574], 'source_1': [465, 286]}), and all 2/6/8 schedules; max endpoint error 7.35847495e-06 m; max cell error 0.00311260367, endpoint-derived bound 0.00622714825.
Depth identity: presented=True, backend=RayTracingPipeline, source=5febb0ded2f5da0c930dc8a2adb1a2e38622ad34, active SPIR-V SHA-256=06b14261b8b5457e5c246db656dc0fbd2aae5971d6753e25e9b0e56968e434f6.
Common ordered loop equivalence: 288 deterministic randomized comparisons across aperture-selected interval sets and schedules 2/6/8; this is not a physical source model.
GPU interval endpoints and GPU per-cell coverage remain unmeasured; the older union-length readback cannot distinguish displaced intervals.
