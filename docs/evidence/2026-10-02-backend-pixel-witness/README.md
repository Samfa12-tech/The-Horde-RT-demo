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

Next finite step: retain these completed results, add only direct-light versus
reflected-bounce output fields at the same six points, compile changed modules,
and capture the same five checkpoints once per backend. Require original RGB
recurrence and byte-identical non-payload output again before interpreting it.
Do not repeat normal controls, rejected precision or phone measurements.
No main merge, release or publication. Normal worktree/configuration unchanged.

Audio/haptic manual revalidation:NO: output-only diagnostics, semantic playback
inputs unchanged. Existing owner music-focus/S24 checks remain untouched.
