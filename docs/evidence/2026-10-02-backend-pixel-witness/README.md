# Six backend pixels: finite output-only discriminator

Base/control source03a6870f66dd9cb4bab62d2f521f4fd663368678. Investigation branch
`codex/horde-backend-six-pixel-witness`; no production shader change or promotion.
Retained normal controls are the October1 Mobile13-image RTX pipeline/compute
sets at `C:/Dev/tmp/horde-primary-opacity-20261001/control-windows-{pipeline,compute}`.
Their exact six outlier coordinates/RGB are retained in the primary-opacity record.
Current TLAS integration already preserves those control PNGs byte-for-byte.

Question: do the six pixels disagree at the actual primary hit/material, ordinary
surface shading, fire integration, or subsequent atmosphere/display processing?
Common source alone does not prove identical native hits or arithmetic.

Bounded matrix: compile/validate the real module families once, build a Mobile
Diagnostic native RTX application, capture five affected checkpoints once on
each backend. No phone operation, performance sweep, changed traversal budget,
material/geometry/light change or tolerance. Retain all ordinary ray/shading/
counter work. Five output rows encode48 floats per selected coordinate using
the existing lossless two-RGB-pixel method; the other535 rows remain normal.
This is an observer, not a corrected renderer or image-equivalence candidate.

Before interpreting the witness, require honest RT presentation, exact extent
960x540, twelve settling frames, camera/assets/CPU geometry matching the normal
control and unchanged meaningful diagnostics where checkpoint scope permits.
Compare all non-payload pixels under the unchanged maximum3/fraction0.001 gate.
Record any observer-induced differences: if the original outliers do not recur,
do not label captured field differences as the cause of the normal image failure.

## Completed output discriminator

Exact candidate source: `d64bea152e40d7083643a941be018f7f27d8aa99`.
MSVC Debug executable SHA-256:
`1b984281d5f33ba443b377e8ffe3c5c548eb24b57bfd4bd53d600d5e6e0ea730`.
All sixteen real pipeline/compute modules compiled, optimized, disassembled and
validated. All eight Shipping modules remain byte-identical to the normal
renderer, with zero diagnostic atomics and no diagnostics binding. Frozen
budget file SHA-256 remains
`f2ff4be07c08536d140ea395fb24d1e3d502882449a84fc245df121e2ad89f4d`.
Native five-checkpoint pipeline and compute captures each exit0, honestly present
RT, and preserve camera/state, CPU geometry, complete visibility/diagnostic
records and static allocations. The ten PNGs and manifests are retained here.
No performance claim follows from this Debug observer.

The comparator initially rejected the whole static-asset record because its
four BLAS build-duration observations are nondeterministic. Only those named
timing fields are excluded from immutable identity. All allocation fields and
unknown future properties remain checked; image tolerances are unchanged.
No capture rerun was required. Configuration initially lacked the WebView2 SDK;
explicitly reused the existing pinned SDK. An incorrect build-target name was
corrected to `horde_rt_diagnostic_window`. The actual successful build log is
retained; neither setup failure is reported as a renderer failure.

Decoded result: [witness-comparison.json](witness-comparison.json). Every
non-payload pixel is **byte-identical** to its matching control (maximum0,
different fraction0). All six original outlier RGB pairs recur exactly:

| Checkpoint / pixel | Pipeline RGB | Compute RGB | Same primary instance / triangle / material |
| --- | --- | --- | --- |
| worst-bend 556,378 | 29,15,9 | 33,16,9 | 3 / 6803 / 100 |
| blue 396,262 | 9,13,24 | 125,21,25 | 0 / 625 / 4 |
| red 396,262 | 20,10,12 | 54,130,58 | 0 / 673 / 4 |
| finale-roof 552,395 | 34,16,2 | 38,18,3 | 3 / 7029 / 100 |
| finale-roof 770,526 | 18,21,25 | 13,15,20 | 0 / 809 / 2 |
| two-enemy 564,393 | 118,64,18 | 26,14,8 | 3 / 6999 / 100 |

The first substantial difference is already in ordinary `shadePrimary` output
at all six points, before fire/mist/display processing. Primary identities agree;
small normal/texture or ray-position differences exist. This does **not** yet
prove whether direct shadows or reflected secondary hits amplify them. It does
exclude a different primary triangle/material and a newly introduced fire/mist
effect as the original source of these six differences.

## Completed surface discriminator

Exact source `ee71f98a2e00cddb5c6b88fd5ba78834e06fc32a`; Debug executable SHA-256
`743379cf9fc58a322d762d47847fa9a64dabd1a9cdd7be567e9d117c5a90e17f`.
The second observer adds direct-light, reflected-hit/radiance and fog fields
only (85 total). Changed include dependency provenance required restaging;
all16 real modules compile/validate. The [module integrity receipt](surface/module-integrity.json)
checks actual nonempty SPIR-V hashes: all8 Shipping and all4 High Diagnostic
modules remain unchanged. No production budget or transport/counter work changed.
Both five-checkpoint native runs exit0. Source pixels, camera/state, CPU geometry,
complete visibility/diagnostics and allocations recur. All six original RGB
pairs remain exact. Observer-only non-payload differences are maximum1 for
pipeline worst-bend/blue/two-enemy and0 otherwise; fraction over1 is0 everywhere.
This small quantization perturbation is recorded, not called byte identity.
The original maximum3/fraction0.001 gate passes for observer validity, **not**
for pipeline/compute parity (the original failures remain).

Decoded [surface fields](surface/witness-comparison.json) distinguish:

| Point | Demonstrated split |
| --- | --- |
| worst-bend556,378 | Nearly identical direct light; reflected ray hits the same world triangle226, but changed mapped sword normal alters direction/position and reflected texture/radiance. |
| blue396,262 | Nearly identical direct light, same bounce direction; secondary hit changes from triangle627/material4 to641/material5 at approximately0.1082997m. |
| red396,262 | Nearly identical direct light, same bounce direction; secondary hit changes from triangle675/material4 to689/material5 at approximately0.1082997m. |
| finale552,395 | Nearly identical direct light; same reflected world triangle811, changed sword normal/direction and reflected texture/radiance. |
| finale770,526 | Identical bounce hit/radiance; direct sky visibility1 versus0 and skyDiffuse0.6990025 versus0.35445005. |
| two-enemy564,393 | Nearly identical direct light; reflected skeleton hit changes from triangle7109 to2259, with strongly different reflected illumination. |

The finale receiver position is z=-12 exactly on pipeline and z=-11.999999 on
compute. Existing `activeSkyLight` closed-room comparison uses z<=-12 and therefore
selects the open-finale aperture versus ordinary moon direction for the same
authored boundary face; finale open progress is1. This is a selector-boundary
discontinuity, not proof of two inconsistent visibility queries for the same ray.
No rounding-room-boundary fix, scene/material edit or tolerance relaxation is
accepted yet. Five other points are dominated by bounce differences, so one
blanket precision or shadow change is not supported.

Next unfinished step: inspect the blue/red implicated authored faces for actual
coplanarity/overlap and retain exact geometry/ray evidence; separately design a
bounded numerical correction for the demonstrated closed-room light selector.
Do not recapture the completed observers, redo normal controls or repeat rejected
precision/phone measurements. A later actual fix must earn its own image gate.
No main merge, release or publication. Normal worktree/configuration unchanged.

Audio/haptic manual revalidation:NO: output-only diagnostics, semantic playback
inputs unchanged. Existing owner music-focus/S24 checks remain untouched.
