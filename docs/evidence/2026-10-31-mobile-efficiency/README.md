# Current S26 baseline and closed scratch-initialization trial

Latest follow-up: [1 November Mobile-water reduction and pause handoff](MOBILE_WATER.md).
That scoped quality change is retained; the scratch trial below remains closed.

**Decision for the scratch trial: no runtime optimization admitted.** The source-only scratch candidate
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


## Follow-up: history and existing Fire Low comparison

The [historical investigation](../../ENGINEERING_1_7_MOBILE_COST_TIMELINE_2026_10_31.md)
recovered July phone timing bytes and traced later water, mist, fire/visibility,
Keeper-light and world additions. Historical CPU window averages are not present
rates and cannot be compared directly to the current chapter.

A same-APK Mobile / Low / Mobile sequence produced **19.319 / 18.203 / 17.125
actual presented images/s** over approximately 43 seconds per run. Native budget
witnesses confirm Mobile four versus Low two volume steps, both one reflection
sample; direct fire-light visibility is unchanged. The closing control drifted.
**No reliable Low-fire gain or regression is established**, and no runtime change
is admitted. [Receipt and raw hashes](fire-quality-receipt.json).

All fresh-cohort thermal samples were framework status 0 and admitted starting
AP/BAT/SKIN readings were within 1.5 C. That is insufficient control: clocks and
power are unknown, and prewarm differed (45 / 10 / 10 seconds). Source review
confirms the Keeper motion arm resumes the live world rather than resetting it.
Cross-run world equivalence must be established before another comparison. The
next quality hypothesis is bounded distant fire-light contribution, preserving
near RT occlusion, visible flames and the existing full-quality option; it is
not yet implemented or accepted.

This follow-up ended **22:39:34 UTC**, before the owner's reported Reef use at
22:43-22:45. Baseline APK identity was reread, the owned Debug app stopped, all
15 original campaign files and original preferences verified unchanged. Fixed
brightness 72 was temporary during each case; each prior brightness value and
automatic mode were restored. Subsequent automatic brightness may vary normally.
Rotation, production and actor-worker packages were untouched. No owned phone
monitor or transfer remains. Earlier 22:07 custody statements above describe the
first released block, not the later temporary brightness adjustment.

## Follow-up: light-range stop and waterfall attribution

The [23:11 checkpoint](WATERFALL_AND_LIGHT_RANGE.md) records a compiled private
light-range probe with **no admissible candidate timing**, two valid baseline
runs using corrected equal preparation/fresh-start acknowledgement, and the
explicit thermal stop rules. Production source and the installed baseline were
restored; the phone was released with saves/preferences unchanged.

Separately, all three retained baseline routes reach their maximum GPU dispatch
on the first approach switch to GenericDielectric near the waterfall. The
2,396-frame analysis and source trace identify splash transmission as a useful
next controlled comparison. They do not establish an isolated cost, a causal
gain, or that resident water slows every frame. [Compact receipt and private
artifact hashes](reach-waterfall-receipt.json). No runtime optimisation is
admitted; full-quality options and the existing renderer contracts remain.

## Follow-up: splash material comparison

The [same-APK A/B/B/A comparison](SPLASH_REFRACTION.md) completed3,140 joined
frames. Simpler spray avoided the generic strategy and improved the selected
waterfall interval, but the closing control drifted8.234%; it did not pass the
whole-route keep gate. An unsuitable visual route produced no active droplets.
The material probe was archived and removed; tested contact telemetry remains.
No phone preset/default gain is claimed. The owner now identifies a playable
phone preset, including measured OFF options, as the deliverable and reports
never noticing spray. Full quality and explicit saved choices remain protected.
