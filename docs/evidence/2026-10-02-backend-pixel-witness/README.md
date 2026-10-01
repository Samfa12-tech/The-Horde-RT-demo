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

Current state: source probe written; shader compilation/build/capture/decoding
NOT RUN. Next unfinished step: stage actual investigation modules with the
existing compiler/publication functions and unchanged frozen budgets.
No completed normal control, rejected precision or phone measurements rerun.
No main merge, release or publication. Normal worktree/configuration unchanged.

Audio/haptic manual revalidation:NO: output-only diagnostics, semantic playback
inputs unchanged. Existing owner music-focus/S24 checks remain untouched.
