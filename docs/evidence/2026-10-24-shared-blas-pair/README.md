# Existing-master shared BLAS pair: bounded desktop proof

**Resource sharing and direct-view equivalence are demonstrated. Normal forest adoption, forest depth, isolated tree reflections and a speed improvement are not admitted.** Production still uses its existing world-baked 64 trees. This explicit Debug fixture uses the same admitted pine at quarter scale inside the existing preview room; it is not a full-size outdoor/root test.

Source basis is `09c629cf`. Its [six hosted checks passed](https://github.com/Samfa12-tech/The-Horde-RT-demo/actions/runs/40089409465). This later proof has its own source, executable, master and shader identities in `receipt.json`; the basis's green CI is not a new-head claim.

## What changed

A named Debug-only hidden pair uses the existing generic PBR loader, atlas subsets, acceleration-structure builder, hit decoder, frame evidence and storage-image readback. It does not add a tree shader or replace ordinary preview roles. Ordinary preview remains seven physical definitions; the baked control adds one (eight total), while the shared candidate adds two referencing one BLAS and one metadata row (nine total). No dummy masks disguise that difference.

The immutable master owner survives initialization and exact-request recreation. Different live requests remain rejected. BLAS build scratch and command/fence ownership are retained until successful completion; destruction uses the existing successful-idle retirement. Normal Release excludes the feature and compiled successfully. The existing generic hidden-preview incompatibility remains; only the explicit Debug pair command is admitted by its parser.

## Measured evidence

RTX5050 Laptop GPU, Windows Debug, current Diagnostic High strategy bank, real `RayTracingPipeline`, 960x540 internal **and** output, 100% scale, exposure0.62. Existing room lighting/quality is recorded in each manifest. Order: baked → shared → closing baked, same executable/master/shaders and five views per process; all three exit0. Frozen pause/reset checks, unchanged gameplay tick/events and same-owner resource recreation pass; scene epoch advances2→3 and tracked allocation inventory stays constant through recreation.

| Pair resource | Baked two copies | Shared master |
| --- | ---: | ---: |
| Vertex payload | 717056B | 358528B |
| Index payload | 120000B | 60000B |
| BLAS allocation | 622592B | 314880B |
| Build scratch allocation | 508160B | 255616B |
| Total tracked device-local allocations | 107004032B | 106278272B |
| Total tracked host-visible allocations | 3150880B | 2732576B |

Host-visible/device-local classifications may overlap; do not add the last two differences. Scratch is not a total startup peak or residency measurement. Both variants have eight BLAS, one TLAS,29 buffers and38 tracked allocations. Actual physical transforms, single-owned-BLAS references and the three canonical texture mappings are in the manifests.

The [baked direct image](baked-direct.png) and [shared direct image](shared-direct.png) were inspected, along with the basis and mirror-context views. Direct changes affect109/518400 pixels (0.0210%), maximum4 levels in an8-bit channel; the basis view changes162 pixels, also maximum4. The apparent geometry/materials and tree shadows agree; these small differences are consistent with CPU-baked versus GPU-transformed floating-point intersections. This is a visual judgment plus retained numerical evidence, not an invented universal threshold. Closing-control PNGs are byte-identical at all five witnesses. The direct image is unchanged by frozen pause/reset/recreation within each variant.

The mirror-context view is **not a clear isolated tree-reflection witness**: foreground trees and existing props obstruct the useful reflected area. Its small numerical differences do not establish tree secondary appearance. Keep that acceptance gate open for the full-size route proof; no further camera search was selected.

Each view retains12 RenderFrame wall-call samples. Medians mostly sit around6.04–6.10ms; the candidate first view is6.29ms and closing post-recreate view6.92ms. Calls include driver/presentation waits; this is not scanout FPS, CPU busy use or an unpaced GPU benchmark. Each manifest retains only the **last owning completed GPU duration**, not a per-view GPU distribution. Those samples vary1.77–3.74ms across the final cohort. Single build-wall observations are about1ms. These data establish **no causal speed gain** and no phone benefit.

## Retained failures and fixes

- Initial baked initialization asserted before presentation: seeking a full provenance range in an empty vector. The mesh contract allows an unclassified suffix as Shared. The diagnostic now preserves that contract; the [failed receipt](retained-provenance-assertion.json) remains. Only its tracked process was stopped after bounded close failed, without focusing or operating the dialog.
- The first recreate trial [failed](retained-recreate-failure.json). `Destroy()` retains device identity and immutable configuration; the new setter incorrectly rejected the identical request. Idempotent exact-request acceptance now changes no live GPU state, and the final three runs prove recreation. Different requests remain blocked.

Targeted Debug app build and four parser/atlas/preflight/resource CTests pass. Targeted Windows Release app compilation passes; no normal interactive Release run was attempted. All raw15 final frames, earlier failures, logs and PID/exit custody remain under ignored `reports/forest-shared-blas-20261024/`; only two original PNGs and compact manifests are committed. No owner settings, campaign files, phone, audio playback, placement contract or normal forest changed. Audio/haptic manual revalidation required: **NO**.

## Actual next implementation boundary

Use shared packed master ranges in the real chapter, account for the fixed metadata/primitive/physical-instance capacities and preserve non-tree geometry sharing Bark/Alder. Reprove static omission provenance and full-size route roots, normals, reflections, shadows and lifecycle before adding one bounded F01 background sector. See the [forest boundary](../../design/HORDE_1_7_SHARED_FOREST_BOUNDARY_2026_10_24.md). This proof does not turn a sparse near-route band into a finished forest.
