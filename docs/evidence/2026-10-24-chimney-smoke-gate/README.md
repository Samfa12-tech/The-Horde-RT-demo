# Chimney smoke: rejected shader expansion, retained open-flue source

The bounded shared High/Low prototype is **unadmitted**. The normal renderer,
generated ABI, embedded shader bank, v4 town loader and settings behavior are
restored to `bed6afe7c851f88aedef62dd7eab1478f8a5901e`. No candidate was loaded,
no static budget was raised, and no High/Low runtime image or GPU cost is claimed.
Chimney smoke remains unfinished required 1.7 work. New outdoor mist, shafts,
fireflies and visual wind remain explicitly deferred; existing effects and
mobile Mist OFF preferences are unchanged.

All six previous-head hosted jobs passed in
[40135203801](https://github.com/Samfa12-tech/The-Horde-RT-demo/actions/runs/40135203801).
The evidence-only checkpoint's CI must be evaluated at its own exact head.

## What compiled, and why it was rejected

The parked patch reuses shared bounded world-space transport. A 96-byte medium
record follows the exact existing binding-26 quality/dust prefix. Its host packer
validates finite bounded inputs; Off is zero. Both preparation paths initialize
the tail; existing prefix writes and fire/light counts remain unchanged. The
candidate uses physical extinction/scattering, primary and local reflected /
transmitted segments, and density-only receiver shadow attenuation. Low/High use
4/8 primary samples, 2/4 secondary/shadow samples and 1/2 hardware-RT sky anchors.
This constrained, distant, disjoint plume is not general overlapping-media proof.

Canonical `compile-raygen.ps1 -Freeze` compiled Diagnostic High Generic first:
264,096 bytes, 88 functions, 280 calls, two RT query sites. The existing Generic
limits are 83 functions and 264 calls. Freeze then stopped at Diagnostic High
Opaque's combined legacy-shape gate, before bank publication. A single-variant
diagnostic compile retained SPIR-V/disassembly for inspection and failed the same
unchanged gate. Both generated modules pass Vulkan 1.2 SPIR-V validation; that
does not establish driver creation, runtime acceptance or affordability.

| Diagnostic High Opaque metric | Pre-smoke control | Candidate | Existing limit |
| --- | ---: | ---: | ---: |
| Bytes | 564,208 | 799,624 | 595,520 |
| Functions / calls | 1 / 0 | 1 / 0 | 1 / 0 |
| Ray-query initialization sites | 29 | 39 | 29 |
| Instructions | 31,274 | 44,216 | 33,067 |
| Branch operations | 4,322 | 6,090 | 4,615 |
| Loops | 84 | 132 | 84 |
| Selection merges | 1,716 | 2,449 | 1,860 |
| Diagnostic atomics | 5 | 5 | 5 |

The error label said “lost the fully inlined legacy strategy shape.” Inspection
corrects that initial interpretation: **inlining survived**. That combined flag
also checks query-site count. Static growth is demonstrated; a measured GPU
slowdown, driver failure or mobile result is not. Mobile and Shipping candidate
variants were not reached. Full stats/hashes are in `shader-gate-diagnosis.json`.

`unadmitted-smoke-implementation.patch` preserves 14 source paths in 26 KB,
normalized to LF. It passes `git apply --check --ignore-space-change` against the
restored source; it has NOT been applied. Generated ABI/bank output and temporary
capture/geometry hooks are intentionally excluded. A future reviewed application
must regenerate the ABI and full canonical bank and pass all unchanged gates.

## Native control, fixture failure and custody

Two initial captures exited 1 with no frame records. Strict paused-pose checks
caught missing viewport-aspect initialization: `RenderFrame` correctly called
`SynchronizeSimulationViewportAspect`, whose zero-time aspect rebind changed
held-item/player animation after the fixture took its baseline snapshot. The
fixture was corrected to synchronize first; assertions were preserved. Original
failure stderr and compact receipts remain. This was a setup mismatch, not a
demonstrated gameplay regression.

The corrected hidden Windows Diagnostic High control completed 188 RT-produced
frames (48 warmup, 120 measured, 20 post) with four captures. It used the isolated
open-flue geometry and an appended **unused** ABI tail with original shading,
960×540 internal and output, 100% scale, DRS off, RayTracingPipeline, fixed 60 Hz
idle inputs at authored `chapter-forest-lookout` checkpoint 203. It is not a
natural earned journey or byte-identical production build. Each record requires
successful RT presentation and owning final-idle completion. Forced external
idle and sparse capture overhead make it unpaced, not scanout FPS or CPU usage.

Measured medians: whole RT GPU **8.22384 ms**, `RenderFrame` CPU **52.2971 ms**.
There is no candidate comparator and no thermal/clock attribution: neither value
establishes effect cost or a gain. The exact executable SHA is
`dcb84e45e0c45fb28b0c5aa8fd64114f1abfa14b6792c3850c8e33bdaa079510`.
The capture body, compact frame CSV, settings/dimension metadata and one actual
RT image are retained here. The temporary wrapper hooks were restored; only
their hashes, not complete source bodies, were retained, so this is historical
control evidence rather than a turnkey reproducible comparison build.

Owner settings, campaign existence/content, foreground window and all 151 parked
owner-file hashes were unchanged across each capture. No phone, listening,
foreground activation, controller or human-play acceptance was attempted. Raw
binary, full frame stream, disassembly and failed receipts remain locally under
`reports/chimney-smoke-20261024/`; `raw-evidence-hashes.json` pins them without
committing binaries, private save contents or large generated artifacts.

## Independent source result and next gate

The [owned B01 derivative](../../../assets/models/world/source/chimney-smoke-study-r1/README.md)
passes the separate geometry checks: 982 triangles / 1,990 vertices / two
materials, open mouth and coherent rim/well edges. Its close Blender source
preview visibly shows the opening. Original v4, owner v5, crown files and normal
runtime remain unchanged. No new collision region, material, texture or town
gameplay is admitted.

The present implementation path is rejected rather than budget-adjusted. The
smallest proposed next investigation is a bounded per-frame **live hardware-RT
incident-light preparation** for one/two medium anchors, then shared density
integration using that current-frame radiance. It could avoid duplicating sky
queries across inlined surface paths, but is a separate material renderer change:
it needs explicit BLAS/TLAS identity, pipeline/buffer/barrier ownership, reset and
quality behavior, plus whole-frame cost and direct/reflection/shadow checks. It
is not baked lighting or permission to add a broad framework, and may still fail
the existing code/loop budgets. Return this proposal for review before code;
neither moving work nor reducing samples alone proves a benefit.
