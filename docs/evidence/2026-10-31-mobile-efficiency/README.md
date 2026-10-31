# Current S26 baseline and closed scratch-initialization trial

**Decision: no runtime optimization admitted.** The source-only scratch candidate
passed its collision checks but did not establish a repeatable useful whole-frame
gain. Its runtime and temporary test edits were restored to `a89362dc`. No renderer,
quality, resolution, gameplay or forest default changed. This is a first bounded
performance checkpoint, not completion of the mobile optimization task.

## Exact configuration and method

Source `a89362dcea2e394f2ce78ac1f2622ab7eb9c8d8c`, draft PR37. Baseline
[CI 41188188172](https://github.com/Samfa12-tech/The-Horde-RT-demo/actions/runs/41188188172)
passed all six jobs. The new evidence-only backup skips CI because the owner
reported exhausted LFS bandwidth and prohibited new LFS downloads; it does not
claim a new CI pass or change workflow coverage.

Galaxy S26 Ultra **SM-S948B**, Android17/API37, OS `S948BXXS4BZIG`,
Adreno840, driver raw `2150932499`, Vulkan1.4.295. Debug package code10 /
`1.6.2-debug` contains current development source, not the published 1.6.2 build.
Both local offline arm64 APK builds passed. Native code uses `-O2`; the actual
completed-frame witness identifies the **Shipping/Mobile RT Pipeline** shader
pair. Route recording and GPU frame-stage timestamp queries were enabled;
shader diagnostic counters and dispatch clock probing were off. The full APK,
native library, build-input and actual shader hashes are in [receipt.json](receipt.json).

Unchanged saved settings: **33%, 475x980 internal / 1440x2971 output**, Mobile
water/fire, Glass Off, Current shadows, Mist Off, Dust1. Saved preview cap30
paces menus/previews, not Showcase gameplay; these gameplay runs had no cap sleep.
Worker and failed full480 forest remained OFF. All APK assets were byte-identical
between control and candidate. No Windows foreground was acquired.

The existing generated-input harness starts in an isolated staged campaign and
records ordinary simulation motion, fixed held views, authoritative resource
rows and actual completed-image presentation timestamps. It is **not** a natural
chapter playthrough, sustained FPS, active combat stress test or touch-latency
measurement. No image capture occurred inside timed motion. Each run was stopped
after its terminal receipt. Raw reports, binaries and logs remain private/local;
their hashes and availability are recorded, not substituted for uploaded bytes.

## Observed baseline

| Run | Joined frames / intervals | Actual presented images/s | Median / p95 interval | Maximum interval |
| --- | ---: | ---: | ---: | ---: |
| Opening to Keeper / torch transition | 777 / 776 | 17.916 | 50.0 / 87.5 ms | 204.2 ms |
| Chapter traversal A2, ordinary forest | 902 / 901 | 14.864 | 58.3 / 116.7 ms | 120.8 ms |
| Scratch candidate B1 | 904 / 903 | 14.865 | 58.3 / 116.7 ms | 125.0 ms |
| Restored baseline A3 | 786 / 785 | 11.661 | 70.8 / 150.0 ms | 250.0 ms |

The opening held-unlit and held-lit dispatch medians were **40.839 / 60.937 ms**;
actual interval medians were 50.0 / 70.8 ms. Authoritative light-state joining
passed, but the camera origin moved slightly and encounter state changed. This
is lighting-associated evidence, not an isolated causal estimate of light cost.

On baseline A2, the two held exterior views spent **99.480 / 100.260 ms** in
shader dispatch, **0.548 / 0.553 ms** in AS work and about0.12 ms in transfer.
Lower/passage dispatch medians were38.905 / 47.146 ms. Total skin CPU work was
about6.4-9.7 ms across those views; simulation about0.2-5.1 ms. These overlapping
stage medians must not be added into a promised saving.

Tracked resource ranges reached approximately250.4 MB device-local and53.7 MB
host-visible at a transition; ordinary exterior values were160.7 / 48.3 MB,
31 TLAS instances and22 BLAS, briefly23 BLAS. These are recorder-visible resource
facts, not total driver memory, exact peak VRAM or future dense-forest cost.

## Why the candidate was closed

`TriangleSupportSurface` initialized a512-entry temporary interval array for
each footprint query although consumers only read fully assigned `[0,count)`.
The candidate removed unused-tail initialization without changing collision math
or count bounds. Full/overflow/empty/sparse/stationary/step/crack repeated-query
checks passed on both headers. Two affected Release CTests passed on the candidate
and again after restoring production source. Host microbenchmark ABBA checksums
matched, but timings varied and were not phone evidence.

A2/B1/A3 started at framework thermal status0, with AP/BAT/SKIN readings within
1.5 C of A2. A2 ended at status1; B1 and A3 stayed0. The closing control slowed
mid-run despite lower subsequent temperatures. No GPU clock/power trace was
captured, so its cause is unknown. Thermal start admission alone did not control
the full trajectory. Small generated-route position differences also remain.

B1 versus A2 changed whole-route presented-image rate by only about0.009%, with
unchanged median/p95 intervals, far below the3% useful-gain criterion. The
repeat-ascent simulation median fell5.132 ->2.920 ->5.081 ms, a possible CPU-local
effect, **not** an established whole-frame gain. Do not average in the slower
closing control to manufacture a favorable result. Preserve the patch privately;
reopen only with new evidence that this cost limits a controlled target workload.

Two earlier attempts were setup-only: opening used an obsolete completion
sentinel, and outdoor unnecessarily awaited all benchmark windows despite a
valid ready acknowledgment. Both stopped safely; neither contributes timing data.
A selected chapter-journey host target was absent from the portable Release
preset, so that command failed before CTest. The two actual affected tests were
then run separately. These are not unexplained all-green claims.

## Ranked next work and custody

1. Reconstruct the early fast-phone feature/performance timeline from existing
   Git and receipts, separating scene, resolution, driver and thermal changes.
   Owner clarification explicitly includes historical attribution and selectable
   lower-cost effect options, while retaining added features and full-quality modes.
2. Measure current fire, lighting, shadows and material work using controlled
   changes. Fire is an owner hypothesis, not an identified cause. Distinguish
   unchanged-quality efficiency from an explicit selectable visual trade-off.
   Do not repeat rejected first-blocker, opacity or player-lookup mechanisms.
3. Audit common skinning against its6-10 ms CPU contribution; worker stays OFF.
   Require whole-frame evidence, not a skin-only win.
4. Improve bounded clocks/power observability where supported before another
   comparison. No fixed-FPS release veto follows.

Broad AS rewriting is low priority for the measured sub-millisecond stage.
Texture LOD/reconstruction remain hypotheses with separate visual prerequisites.
No change to authored forest scope or dense-forest cost acceptance follows.

Phone released **22:07:40 UTC**: the verified baseline APK
`a9a2930a64c58c83de3e0399ee2dc31ee6f0e55c813746cfaaca83ce62496307`
remains installed in the dedicated Debug package and is stopped. Preferences
are byte-identical; all15 original campaign files and151 parked owner files are
unchanged. System brightness/rotation settings were never changed. Production
and actor-worker packages were untouched. No owned phone monitor, transfer or
Windows game process remains. No merge, publication, report submission, LFS
download or security change occurred.
