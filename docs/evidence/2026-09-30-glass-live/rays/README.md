# Exact live RTX continuation witnesses — investigation only

Base source: `fec73b88f22c9223ca7075cb5cf76f5cdcdd8d37`, including the accepted
corner, candidate, surface-position and geometric-shadow fixes. No player,
material, asset, physical interface budget or medium capacity was changed.
This proves three RTX numerical defects, not a universal transport pass or an
Adreno diagnosis. Ordinary production sources/modules are restored after probing.

## Exact replay before instrumenting

The existing gameplay-owned `lantern-reveal-sequence-v1` is staged twice:
600 fixed 1/60 s warm-up steps, then 231 measured steps. Simulation tick833,
walkTime3.849997 and the unmodified lantern pendulum snapshot are frozen.
The paused zero-delta update must not reset that pendulum. Extent1232x803,
Diagnostic/High, RTX5050 Laptop GPU; pipeline and compute each reproduce all41
ordinary counters of their respective live row230/submission831, including
three primary volume failures and three certified reason-bit16 recoveries.
This is not the authored finale checkpoint or a replacement for live motion:
`11-finale-roof.png` is only the existing capture filename. Manifests explicitly
mark the investigation and exact live frame/tick. Failed initial probes that
selected a nonexistent checkpoint or reset the pendulum are not passing evidence.

Ordinary replay EXE SHA-256:
`23efb5e4086965ac076e34590b55fc5c60e26bce1aafd829ca7c075050a9960e`.
The marker isolates three failing pixels, not an image acceptance gate.

## Hardware ray records

The final probe captures the committed triangle's loaded vertices, barycentric
surface point, object-to-world matrix, native query object ray origin/direction
and **raw** intersection t. Ordinary `nextHit.t` also includes origin advance;
it must not be used as raw query t. Records use lossless RGBA bytes in the first21
storage-image rows. Those invocations are reserved, so whole-frame shadow/body
counters and those rows are not ordinary parity evidence. The three selected
failure paths, volume failures3, TIR2460 and certified recoveries3 still reproduce.
Only pipeline ran this final probe; compute's ordinary replay is separate.

Probe EXE SHA-256:
`ce25031de583ccb80606b4c2714382979c8f64047c16e1f207a22e6e3e2391d1`.
Probe SPIR-V SHA-256:
`611a5abc86e3a0035c3f9cd58f61952748b3c0474c33e7d8ae8e6c67a378ce86`.
Actual compilation/validation/disassembly:63049 words,62 functions,222 calls,
two query sites,41 diagnostic atomics. This instrumentation exceeds ordinary
footprint limits and was never admitted or installed on the phone. Full temporary
CPU/GLSL source patch, module, disassembly, statistics and build log are retained.

| Pixel | Ordinary path while medium remains open | Proven native object-ray condition |
| --- | --- | --- |
|576,574 |entry25 -> TIR24 -> entry45 |Inside pane4; exit63 at native0.681um is discarded by1um tMin.|
|504,577 |entry19 -> entry54 |Spawn maps outside adjacent face54 by2.159um locally; entry54 at2.197um is genuine for that misplaced query origin.|
|515,652 |entry62 -> entry3 |Spawn maps outside adjacent face63 by0.396um locally; zero tMin alone cannot recover an exit.|

The same uploaded triangle coordinates match the immutable runtime GLB
bit-for-bit. Its six closed cuboid components are0–5 plus36–41,6–11 plus42–47,
12–17 plus48–53,18–23 plus54–59,24–29 plus60–65,30–35 plus66–71.
GLB SHA-256:`34a2522f2027d3fb04b77480cc929d36c5a19c6e0f33bf3fa0f0ae0959c99ec4`.
These component half-space tests are justified for these convex panes, not for
every closed-manifold asset. Generic certification does not prove disjointness.

Independent double intersections using the **captured native object ray** agree
with the returned native triangle/t. Ideal double inverse-transforming the world
ray does not: it misses the float traversal-transform residual. The old world-ray
calculation alone was only a hypothesis; it is not evidence of a BVH defect.

Six bounded alternatives per witness vary stock/2um/zero normal bias and1um/zero
tMin without changing the main path's state or physical budgets.576 needs its
sub-minimum exit;504's2um/1um route hides a15.9nm entry but does not prove correct
geometry;515 needs a different origin as well as a sub-minimum exit. Therefore
one global bias/minimum knob is **not** a demonstrated fix. Do not ship these
alternatives, filter actual pane interfaces or suppress the failure counters.

## Reproduce the offline analysis

From repository root, use Python with Pillow for decoding and NumPy for analysis:

```powershell
$rayEvidence = 'docs/evidence/2026-09-30-glass-live/rays'
python "$rayEvidence/probe/decode-volume-path.py" "$rayEvidence/volume-objectray-capture/11-finale-roof.png"
python "$rayEvidence/probe/decode-volume-path.py" "$rayEvidence/volume-objectray-capture/11-finale-roof.png" --alternatives
$lanternGlb = 'assets/models/props/runtime/reward-lantern-body/reward-lantern-body-lod0.runtime.glb'
python "$rayEvidence/probe/analyze-volume-geometry.py" $lanternGlb "$rayEvidence/volume-objectray-capture/paths.json"
python "$rayEvidence/probe/analyze-volume-geometry.py" $lanternGlb "$rayEvidence/volume-objectray-capture/alternatives.json" --object-rays
```

The outputs match retained JSON. `artifacts.json` hashes the retained inputs;
the exact executables remain local immutable artifacts, not redistributable game
releases. The probe embedding helper is investigation-only; it temporarily writes
a single catalog/module and must never substitute for production Freeze/checks.
No tolerance was loosened or historical buggy pixel restored. Remaining RTX/phone
failures, real Mobile interface exhaustion, certified/contact cases, unknown
initial shadow medium, backend parity and matched Shipping performance stay open.

Production restoration: fresh eight-key pipeline Freeze and independent catalog
check pass; all eight compute variants validate/disassemble and match. Generated
modules/catalogs and the five probe source files have no production diff.
Native High Debug rebuild passes; current MSVC focused tests4/4 pass (dielectric
math, character slot smoke, benchmark report and owning run). Fresh sourcefec73b8
push/PR CI logs confirm45 portable +11 Vulkan-host tests. These are not a full
final-candidate matrix or new phone acceptance. Logs are retained in `probe/`.

Audio/haptic manual revalidation required:**NO**; no semantic feedback changes.
