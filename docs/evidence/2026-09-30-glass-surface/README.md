# Triangle-surface dielectric hit reconstruction

September 30, production correctness candidate on base `f707bc4` (which retains
the accepted `632322d` implementation). Accepted player geometry, animation,
mounting, materials, ray masks, physical traversal limits and diagnostic reporting
are unchanged. This is not a Phase 4 completion or performance acceptance.

## Proven cause and repair

The preceding [exact path probe](../2026-09-30-glass-recovery/README.md) demonstrates
one raised-lantern exit lost because `origin + direction*t` reconstructs the hit
outside the adjacent pane. Barycentric reconstruction reaches glass57 at1.924um
with the **unchanged** normal bias and1um minimum. Changing bias alone does not
repair the recorded path.

`rt_hit_decode.glsl` now reconstructs the surface point for static transmissive
triangle hits only: object-space barycentrics, then the linear instance transform,
then translation. `precise` preserves that arithmetic ordering. Ray distance,
UVs, normals, Fresnel, IOR, attenuation, query bounds and materials are unchanged.
OpaqueFast compiles this branch out: all four pipeline module hashes remain
identical. No error-bound constants or universal hardware robustness claim are
borrowed from the [NVIDIA solver reference](https://developer.nvidia.com/blog/solving-self-intersection-artifacts-in-directx-raytracing/).

The geometry regression fixture retains the exact measured float32 positions and
adjacent plane. The source-contract guard was run red without the repair, then
green after restoration. This fixture is not a substitute for GPU traversal;
the actual phone query evidence remains in the preceding bundle.

## Exact artifacts and checks

| Artifact | SHA-256 |
| --- | --- |
| RTX Diagnostic/Mobile EXE | `23080638f0d0fd2c4db9c75e04eba736aba10526ce0cdea354ad551066bff69e` |
| RTX Diagnostic/High EXE | `28e98bfb67bafb5557b8660325690a00e72b456d936a7b7bfe7622e63e8e4cf5` |
| SM-S948B ARM64 Debug APK, installed/pulled back identically | `457641fdec90e3cfda220934060c34413014e8aeafe64f8b50c0969c465e3570` |
| ARM64 unsigned Shipping Release APK, not installed/published | `c02b08150bbbef42577624e7c4075f0770578dfc6fa4f016e8bd390e7d37bc84` |

MSVC focused CTests3/3 pass: pipeline variants, dielectric math, render-slot smoke.
Both native quality builds and Android ARM64 Debug/unsigned Release builds pass.
Fresh pipeline8-key compilation/catalog/generated-header/compatibility checks
pass; compute8-key generation and a fresh8-key artifact/catalog check pass.
Actual packaged Debug and
Release stripped libraries match their APK bytes; SPIR-V disassembly/validation
passes. All four packaged Shipping modules have **zero diagnostic atomics and no
binding22**. These are compile/package facts, not Shipping performance evidence.
Successful logs and containment reports are in `checks/`.

The precise reconstruction adds255 words to each generic pipeline module
(Diagnostic56705->56960, Shipping54760->55015, about0.45/0.47%). Four generic
**module-footprint** ceiling rows are explicitly revised to the measured
size/instruction/branch/selection counts. Opaque ceilings, loops, functions, call
sites, ray-query initialization counts and diagnostic atomic ceilings stay fixed.
The physical Mobile4/High8 interface budgets and pixel tolerances are not changed.
This is a reviewed correctness footprint cost, not an optimisation claim.

## Exact SM-S948B evidence

`phone/` retains run080757, Android16/Adreno840/driver2150932499, native pipeline RT,
strict ASTC,75%/1080x2235, all7 captures and Home/resume complete. Ordinary Debug
only; stable and owner-accepted viewmodel packages/data are untouched. Compared
with exact632322d APKea1c6265/run064746:

| Scene | Primary transport overflow | Certified recovery before->after | Reason mask before->after |
| --- | ---: | ---: | ---: |
| Isolated lantern | 1->1 | 80->80 | 2->2 |
| Raised/look-up lantern | 0->0 | 528->527 | 3->2 |
| Low/parry | 0->0 | 0->0 | 0->0 |
| Millimetre closed pane | 0->0 | 0->0 | 0->0 |
| Grazing/floor-contact fixture | 0->0 | 30081->30703 | 1->1 |
| Tinted and fire transport | 0->0 | 0->0 | 0->0 |

All7 shadow overflow counts remain0. The raised view's opaque-terminal reason
disappears, consistent with the demonstrated position-layer repair. Remaining
mask2 recoveries are genuine fixed-budget paths, not an accepted complete pass.
The grazing increase is retained openly: different near-edge paths can result
from corrected positions, but counters alone do not establish contact correctness.
One verified floor contact does not certify all opaque terminals. No S24/S25,
phone compute parity, sustained performance or live visual acceptance is inferred.

## RTX images and distinct parity gates

`windows/` retains24 focused native captures: Mobile/High x632322d baseline/new
pipeline/new compute x isolated/high-look/low-parry/grazing. All complete with
honest RT presentation; all24 report transport/shadow overflow0. Recoveries:

| Quality/scene | Baseline pipeline | New pipeline | New compute |
| --- | ---: | ---: | ---: |
| Mobile isolated | 20 | 20 | 20 |
| Mobile raised/look-up | 109 | 109 | 109 |
| Mobile low/parry | 0 | 0 | 0 |
| Mobile grazing | 1806 | 1878 | 1886 |
| High isolated/raised/low-parry | 0 | 0 | 0 |
| High grazing | 1828 | 1893 | 1894 |

The standard13-capture run completes. **12/13 images are byte-identical**.
Finale differs at24/518400 pixels (>1), max89, within lantern bounds
x382..448/y378..477. Thus the unchanged max3/fraction0.001 equivalence gate
**fails**. Corrected transmissive hit positions are not required to reproduce
buggy transport pixels; the image change remains explicitly recorded, not relabelled
as a passing equivalence result. The peak point(424,378) is on the lantern.
No unrelated standard image changes. Standard timing is not matched Shipping A/B.

Focused before/after images also fail equivalence. Pipeline/compute is a separate
comparison: isolated passes(max2); raised(max12), low/parry(max5) and grazing(max38)
fail at both qualities. Their sparse differences and recoveries remain open for
backend investigation, not blamed on/hidden by the correctness image update.
Shipping/Diagnostic parity is **not** established by this matrix.
`checks/glass-barycentric-pixels.json` includes exact hashes, bounds and peak pixels;
phone comparisons are ADB screenshots, not raw cross-backend byte parity.
The small read-only comparison helper records the unchanged tolerance; it does
not replace or weaken the standard foundation comparator.

## Remaining gates / next work

- Real isolated-lantern interface exhaustion and other budget recoveries remain.
  TIR consumes a real interface; neither budgets nor diagnostics may be relaxed.
- Grazing opaque/contact recovery remains unaccepted. Classify physical boundary
  handling without globally accepting every open-volume opaque terminal.
- Current one-query shadow attenuation still uses authored-thickness estimates;
  geometric path length and finite-endpoint/origin-inside contracts need repair.
- Live glass motion and per-frame failure evidence remain required; frozen images
  and the earlier600-frame reveal are not a replacement.
- Only after correctness: matched Shipping performance and separate backend parity.

Audio/haptic manual revalidation required: **NO**, RT hit position only;
feedback authority, events, assets, gain and timing are unchanged.
