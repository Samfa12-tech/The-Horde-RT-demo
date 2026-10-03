# OpaqueFast compiler-treatment investigation — not a Shipping candidate

October1 local exact **SM-S948B**, Android16/Adreno840/driver2150932499.
Uncooled75%/1080x2235, strict ASTC/Mobile/native RayTracingPipeline, unchanged
scene/geometry/materials/lighting. Not physical-glass, backend-parity, owner-quality,
sustained-performance or release acceptance.

## Treatment and actual artifacts

Detached source `eafbf8262442a82e0835edf5cf5306d718e64633`; both compile identical
143,863-byte preprocessed Shipping OpaqueFast SHA-256
`eaa0b8a9f7b1a87fce83e34ab8568c524b1a8ed4160009bf35fc1a89e728713f`.
Probe changes glslang flags/SPIR-V pass group together, not shader maths, C++
strategy, budgets,41-counter CPU/GLSL ABI or assets. Individual pass causality
is not isolated. Static representation is not dynamic ray work or measured speed.

| Shipping opaque module | Bytes /words | Instructions | Functions /calls | Static query sites |
| --- | ---: | ---: | ---: | ---: |
| Normal `66e39df9…bb9c4b` |501,012 /125,253 |27,536 |1 /0 |23 |
| Probe `14509c27…249d3` |212,452 /53,113 |12,595 |60 /192 |2 |

Actual installed/pulled-back Shipping APKs: control
`ab3e2261fd081f87e667a6e4967e2476077fa96554702330e6aa49baa8133eae`, probe
`4827a3c2e26d3328ed53608e15c77afe12a34077e96dcd0564c8d65532e21ff4`.
Diagnostic probe
`1a515220ff77f33b89f675b6f96de64b55e33331cde2286758d39dbc2baeb069`.
Recipe/patch/build receipts and independent actual ELF scan retained; no APK/ELF
or full source snapshots duplicated. Scan validates/disassembles all8 modules:
Shipping zero atomics/image reads/binding22; Diagnostic opaque30 static atomics
versus normal5. Generic/compute unchanged.53 assets agree:50 exact,3 formatting-only.
Frozen-production catalog check and standard package compute/pipeline strategy
parity correctly remain RED. Independent byte inspection does not replace guards.
Focused MSVC bundle1/1 and unchanged compute1/1 pass, not hardware/image acceptance.

## Complete Shipping A/B/B/A — descriptive only

Strict analysis admits1,838 owning/presented/GPU-valid rows and160 opening rows per
run:7,352/640 total. Exact workload/quality/APK/module/completion joins and null
compiled-out diagnostics checked; warmup excluded. Full reports/context/parser
negative fixtures accompany frozen comparison SHA-256
`e2e57576eb1e234dab5c6454199a4080cace9c7e6b32aab0da375f96855a0da2`.

| Run (suffix `20261001`) | Cycle median /p95 ms | Route GPU median ms | Opening GPU median /p95 ms | Battery C start→end | GPU power levels |
| --- | ---: | ---: | ---: | ---: | --- |
| control-a1 |60.9232 /80.6117 |49.9261 |66.419686 /73.752134 |27.2→34.1 |0–1 |
| profile-b1 |44.0113 /58.3418 |33.5008 |46.043827 /51.347603 |31.9→36.5 |0–1 |
| profile-b2 |50.1984 /65.3972 |39.5027 |51.478931 /60.740415 |32.0→35.2 |0–5 |
| control-a2 |66.8472 /88.7201 |56.1994 |72.1915605 /86.566248 |30.0→35.4 |0–2 |

Thermal status0 throughout context, but battery/power states differ and are not
frame-aligned. B2 reuses B1's process with verified new benchmark ownership.
Mean-of-run-statistic changes: cycle−26.27%, route GPU−31.21%, opening GPU−29.64%:
**not causal gains**. All640 opening GPU intervals exceed33.333ms. Native timings
do not measure compositor display pacing. Probe misses30FPS; no lantern timings.

## Image gate remains failed

Diagnostic opening/Home-resume `20261001-003239` passes honest presentation/modelled
ownership. Authored scene and all41 counters agree with normal Debug389d6954.
Separate Diagnostic image gate FAIL: maximumRGB14 (limit3),125/4,492,800 pixels>1
(fraction0.0000278223, limit0.001). No tolerance relaxation or Shipping/Diagnostic
inference. Original PNGs and full difference receipt retained. Sparse analysis:
68 eight-connected components, largest6pixels. This does not establish causality
or turn failure into a pass. Approximate weapon/hand/fire image locations are not
native primitive attribution. Next use actual pushed inputs/ray/hit/shading
witnesses; compiler roundoff versus hit divergence is unproven. Do not retune
accepted players or restore buggy transport pixels to conceal differences.

Normal Shipping restored/pulled back beforeA2. Normal Debug389d6954 then restores/
pulls back exactly: opening/Home-resume `20261001-010627` PASS, PNGd3ec98a8 equals
the prior same-artifact control. Optional Play Protect upload prompt declined;
no global security setting changed. Stable app/data unchanged. No treatment promoted.
Physical glass budget/recovery/live/full backend/exact S24/S25/final gates open.
Audio/haptic manual revalidation: **NO** — semantics/playback unchanged.
