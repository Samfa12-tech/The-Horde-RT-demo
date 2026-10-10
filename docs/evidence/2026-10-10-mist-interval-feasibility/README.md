# Mist interval feasibility correctness slice

Final outcome: **DEFER / static admission and strict CPU cell gate fail**. The
final measured ledger is `full-metrics.json`; the final correctness result is
`correctness-final.json`. High Generic remains 140 bytes over plus structural
excesses; High Opaque remains one static query site over. No production adoption
or further eight-mode/GPU performance pass occurred.

The expanded CPU tests inspect 12 actual frozen ray/aperture pairs, every union
endpoint and the projected per-cell vectors for all 2/6/8 schedules. Three
aperture-one cell checks fail the original fixed 2e-5 discriminator. A larger
endpoint-derived roundoff bound explains the discrepancy but is **not** used to
turn that failure into acceptance. See `methodology-and-failures.json`, including
the honest absence of raw intermediate reports that were overwritten before the
runner was changed to preserve numbered outputs. The 288 ordered scattering
schedule comparisons and six synthetic source/union/capacity fixtures pass.

This bounded CPU slice checks interval placement, not only total covered length. It mirrors the insertion sort, overlap union, and cell-overlap calculations in `../2026-10-10-mist-interval-prototype/reproduce/intervals.glsl` using IEEE float32 arithmetic. The independent oracle uses double precision. It also consumes the frozen prototype rope triangles, both finite light positions and cone planes, the frozen camera, and the exact `current-depth-3` depth capture. Six predetermined pixels are evaluated against both aperture sources, then across the original 2/6/8 sample schedules. The capture's presentation record, active SPIR-V identity, and frozen-state bytes are checked before its depth image is used.

Additional deterministic fixtures cover both aperture identities, source filtering, unordered overlapping and disconnected intervals, a thin interval, equal-length displaced unions, and the exact 64/65 raw-interval and 512/513 callback limits. The weighted-scattering comparison runs 288 deterministic CPU float32 cases across 2/6/8 schedules, both aperture-selected interval sets, and disjoint, overlapping, and thin interval patterns. It compares the old fixed-offset call schedule with a common ordered loop while preserving sample positions and accumulation order. The fixed extra-light term is only a control; this does not model all physical source illumination.

The frozen-ray CPU AABB candidate counts range from 6 to 48 across the chosen pixels. These are direct ray/AABB counts from the saved bounds; they do not stand in for hardware BVH callback counts.

Run from the repository root:

```powershell
python docs/evidence/2026-10-10-mist-interval-feasibility/reproduce/verify.py
```

The machine-readable output is written to `reports/mist-interval-feasibility/correctness.json`. It records the source input hashes and CPU endpoint/cell errors. Cell coverage is compared using the float32 sample-position construction and projected cell bounds from the shader; the report also bounds coverage deltas from measured endpoint error. Passing proves only agreement between the CPU float32 mirror and independent CPU geometry/arithmetic on these frozen inputs. It does not execute or compile the production shader, establish Vulkan callback order or GPU numerical behavior, or validate GPU interval endpoints. The old GPU readback compared union lengths and therefore could not distinguish equally long displaced intervals. A future exact GPU discriminator should emit predetermined sorted interval endpoints and per-cell coverage values for both apertures; no GPU capture or harness changes are included in this slice.

The script reads the already-generated ignored report artifact `reports/mist-interval-prototype/captures/current-depth-3`; it does not rebuild or launch anything. It needs Pillow to read the saved float depth image. The cases preserve the prototype's semantic limits: at most 64 retained raw intervals and at most 512 candidate callbacks. Callback overflow is checked before source identity filtering, matching the shader. Reaching either limit is valid; exceeding it rejects the result explicitly.

The unimplemented GPU endpoint/cell readback design is recorded in [GPU_PROBE_DESIGN.md](GPU_PROBE_DESIGN.md).

Static reproduction (requires the exact previous prototype resolved sources in
ignored reports, obtainable with its retained prepare recipe):

```powershell
python docs/evidence/2026-10-10-mist-interval-feasibility/reproduce/compile_feasibility.py
```

It refuses an existing output directory. The retained `static-artifacts.zip`
contains resolved source, raw and optimized SPIR-V, validation logs, disassembly
and exact static query-site attribution for review. These are experimental
artifacts outside every runtime package. Compiler strategy, tool binaries and
all production maximum/exact fields are unchanged.
