# Desktop skin-work attribution and one candidate

Continuation of `efcd303ec138999888f948cf4e514f46bbf824b8` in the existing WP2
worktree and draft PR 27. No phone operations. The earlier phone scale receipt
and its 12.06 ms skin-stage median are preserved; desktop results do not certify
Android benefit. No shader, quality, cadence, animation, AS policy or save change.

Six appended optional CPU stages split player pose evaluation, world vertices,
viewmodel vertices, character clip/palette evaluation, character vertices and
character copies/grounding. Existing indices 0–13 and the existing Skin sum are
preserved. New child phases are subsets of their parent, not extra totals to add.
The Android route CSV appends the same completed-owner phases, counts and output
bytes, plus the existing BLAS/TLAS command-record stages. Failed/aborted work
does not acquire a completed owner. Disabled asset observation reads no clocks.

## Work identified

| Work | Concrete scope |
| --- | --- |
| Player pose | Clip sampling, palette and IK solve; inside PlayerSkin. Laptop fixture mean 0.0122 ms. Simulation animation/gameplay remains inside SimulationStep. |
| Player world vertices | 34,304 unique vertices; mean 1.280 ms in the standalone Release fixture. |
| Viewmodel vertices | 15,855 unique vertices sharing the solved pose; mean 0.504 ms. |
| Player pose copies | Existing world/viewmodel static-PBR repack, outside Skin. 50,159 vertices × 64 bytes = 3,210,176 output bytes per joint refresh. |
| Kit | 26,945 unique vertices, 24 joints. Two output-copy passes total 3,017,840 representation bytes: textured output plus grounding/bounds/static-PBR repack. This is not bus traffic or GPU upload volume. |
| Upload | Existing map/copy/flush/unmap timer, outside Skin; actor output publishes only after successful preparation. |
| AS | Existing CPU command-record timings, separate from QueueSubmit and FrameFenceWait. GPU AS execution is not measured here. |

The player fixture runs 300 poses with the existing 15 mm grip gate. Its old
view-space defaults were invalid for its model-space targets; after correcting
the fixture using already-tested anatomical targets, maximum socket error is
0.010 mm. Failed attempts remain recorded. Its 30 Hz comparison reports 173 mm
maximum motion difference and selects `review`; **no cadence change is adopted**.
These synthetic player means are descriptive, not matched game-frame medians.

The native CPU benchmark uses the actual Kit runtime GLB, 30 warmup poses and
240 measured poses per case. Instrumented/unobserved outputs are compared byte
for byte on every pose; baseline/candidate/control output digests also agree.
Thread CPU time is captured for the complete verification batch, including the
second pose and digest work. It is not per-phase CPU utilisation or a full game
thread/scheduler profile. No owned build/capture ran during A/B/C, but external
scheduling, CPU temperature and frequency were not controlled or measured.

## One bounded candidate

Player solved poses already cache inverse-transpose normal factors per joint.
The generic character path still recalculated those factors per vertex influence.
The candidate reuses the existing exact-arithmetic helper there, once per joint.
It retains the positive-influence singular-transform rejection, tangent handling,
all vertices and refresh rules. Kit needs 24 temporary transport entries (about
1 KiB by source layout); no GPU allocation or geometry count changes.

| Kit case | Opening control vertex ms | Candidate vertex ms | Closing control vertex ms | Candidate full pose ms | Closing control full pose ms |
| --- | ---: | ---: | ---: | ---: | ---: |
| Idle | 1.73235 | 1.55670 | 1.69810 | 1.91425 | 2.06845 |
| Walk | 1.74535 | 1.56480 | 1.69890 | 1.93370 | 2.06335 |
| Half blend + acting | 1.80455 | 1.58950 | 1.70425 | 1.98255 | 2.08590 |

All table entries are medians. Vertex reductions versus the faster closing
control are 8.33%, 7.89% and 6.73%; full-pose reductions are 7.45%, 6.28% and
4.95%. Closing control vertex drift is −1.98% to −5.56%; full-pose drift reaches
−9.06%, so opening-control gains alone overstate the useful comparison.

The predeclared idle target was **1.55 ms**. The candidate's **1.5567 ms misses
it**; the one-iteration goal is **exhausted**, not passed or relaxed. Retain this
single candidate and its measured partial gain for PR review; no further tuning
loop, frame-FPS improvement, mobile admission or release claim follows.
[Summary](summary.json) retains all sample counts, minima/maxima, means, medians,
nearest-rank p95, raw-file hashes, thread CPU context and exact binary identities.
Raw timing arrays and native media remain under ignored
`reports/skin-attribution-20261022/`; no binaries/OBJ/media are committed.

## Visual and functional evidence

Fresh Debug Diagnostic/High RT captures on the RTX 5050 Laptop GPU use the same
960×540 dispatch/output, authored frozen poses and selected shader pair. Lookout
203 and close Kit 207 PNGs are independently byte-identical before/after; both
pairs were inspected. Their manifests confirm successful RT-produced frames.
Settings and foreground focus remain unchanged; all four owned captures exited.
These are frozen image controls, not animated GPU or sustained performance runs.

Four initial focused checks pass after the explicit golden-field update. The
broader selection is **7/8**, with inherited CharacterRenderSlotSmoke string
contracts still red: six shader normal-decode strings, one water-distance string
and one Android tuning-snapshot string. Relevant token presence is unchanged
from the base and no shader changed; [provenance](inherited-contracts.json).
No assertions were removed. The added actual-asset observation/equivalence check
was rebuilt and passed separately in [the final Kit check](kit-profile-final-test.txt).
Original golden failures, candidate compile error and player fixture failures
are retained in [failures.txt](failures.txt). [Broader output](host-tests.txt).

Windows Debug and Release applications compile/link. Android arm64 native
compilation passes; no APK was installed or phone queried. The existing player
rig tests, analytic tangent/reflection tests, normal-transform boundary tests,
benchmark serialization and submitted-frame ownership checks remain relevant.
Audio/haptic manual revalidation required: **NO**; semantic inputs are unchanged.
Existing Kit listening acceptance remains separately open.

## AS and next checkpoint

At the base source, ordinary player world/viewmodel and Kit pose work already
use BLAS UPDATE. A third-actor owner replacement can BUILD. The rescue rope tail
still causes an unconditional UPDATE of its base rescue-world BLAS; it does not
refit the immutable forest dressing BLAS. TLAS updates once per frame, rebuilding
only when instance definitions change. Existing enemy refresh/death caches remain.

A **separate earlier saved phone frame**, serial 1532/tick 7193, records four
BLAS calls at 0.030104 ms and one TLAS call at 0.009011 ms of CPU command creation,
20 allocated BLAS and 28 instances. It is not the 12.06 ms cohort or GPU-AS time.
[AS receipt](as-audit.json) pins its source and identity. Existing optional GPU
stage probes could provide combined AS/dispatch/transfer timing; they were not
captured for this comparison. Per-owner mode/primitive attribution remains open.

Next physical window: owner-confirmed access, exact candidate/control builds,
same admitted live 20/33/40% hold and original settings, new per-phase columns,
warm/context notes and owning submission joins. Keep the unresolved closing 33%
phone slowdown and 20% image-quality gate. No reconnection is assumed.

Next useful desktop gameplay slice: correct Kit's live source/listener anchoring
before in-game listening/mix acceptance. Keep PR conflicts, inherited CI/source
contracts, the Debug route timeout, natural Keeper/full-journey recovery and the
unspecified flask policy open. Avoid another unchanged optimization loop.
