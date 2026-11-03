# Waterfall attribution and light-range probe, 31 October 2026

**No new optimisation is admitted.** The light-range probe compiled, but neither
candidate attempt reached an admissible timing run. A separate analysis of
existing baseline traces found a repeatable waterfall-area spike coincident with
the first switch to the general transmission shader. That is a strong lead for
a controlled comparison, not an isolated cause or a measured saving.

The [receipt](reach-waterfall-receipt.json) records exact APK/native/shader
identities, 40 private input hashes, failed attempts and custody. Baseline runtime
source remains `a89362dcea2e394f2ce78ac1f2622ab7eb9c8d8c`; the intervening
`a7e7935f` / `d48c5ef9` commits contain evidence only. The existing [profile](README.md)
is unchanged: SM-S948B, Android 17, Adreno 840, 33%, Shipping/Mobile Pipeline,
real hardware RT, native Debug `-O2`, timing instrumentation on. Worker and
rejected 480-tree forest remain OFF. These are generated-input measurements,
not owner playability or shipping-performance acceptance.

## Waterfall findings from existing runs

Strict motion/presentation and submission/tick/stage joins passed for all
**2,396 frames** in three baseline routes. No new water intervention or phone
operation was used for this analysis.

| Baseline run | Player X at route dispatch peak | Dispatch at peak | Actual inter-present interval | AS at that frame | Active strategy |
| --- | ---: | ---: | ---: | ---: | --- |
| opening-a2 | -2.16881 m | 192.911 ms | 204.167 ms | 0.595 ms | First approach GenericDielectric frame |
| reach-a1 | -2.11661 m | 138.767 ms | 150.000 ms | 0.400 ms | First approach GenericDielectric frame |
| reach-a2 | -1.90957 m | 166.068 ms | 183.333 ms | 0.443 ms | First approach GenericDielectric frame |

Player Z is approximately -15.20 m. The authored falling streams are centred at
X -2.32, Z -15.26. These recorded positions are **player origins**, not measured
optical-eye positions or water pixel coverage. The table assigns the interval
ending with that completed frame; stage durations are not additive counters.

The waterfall and the 31-instance / 22-BLAS scene remain resident before and
after the transition. AS work stays sub-millisecond at these peaks. A torch
wetting transition and changing flame strength occur nearby. Neither the water
surface nor the splash/pipeline transition has been isolated experimentally.

There is also a useful later observation after uploaded fire count reaches zero.
For post-hoc moving samples with X between -5 and -3.3 m, Z within 0.2 m of the
waterfall lane, and phase `approach`, dispatch medians by strategy are:

| Run | OpaqueFast median / samples | GenericDielectric median / samples |
| --- | ---: | ---: |
| opening-a2 | 38.756 ms / 1 | 87.961 ms / 9 |
| reach-a1 | 32.409 ms / 3 | 68.702 ms / 10 |
| reach-a2 | 33.268 ms / 2 | 74.641 ms / 9 |

Small populations, moving views, changing contact state and uncontrolled clocks
prevent treating these ratios as a predicted gain. They do justify isolating
transmission strategy selection before blaming fire alone or increasing an AS
optimisation budget.

## Mechanism supported by source

- [WaterContact](../../../src/gameplay/effects/WaterContact.cpp) emits bounded
  droplets on stream entry, periodically in the stream and on wet footsteps.
  Their lifetimes are 0.45-0.65 seconds; the pool is capped at 24.
- [WaterContactRenderGeometry](../../../src/vulkan/raytracing/WaterContactRenderGeometry.h)
  gives these small geometric droplets a shared thin-wall transmissive material.
- [PresentableTinyRtScene](../../../src/vulkan/raytracing/PresentableTinyRtScene.cpp)
  marks that instance transmissive whenever admitted droplets are active and
  Water is not Off. `HasActiveGenericTransmission` then selects the general
  transmission pipeline for the **whole frame**, independently of primary-view
  coverage. It does not perform a frustum test here; reflections/shadows still
  need correct contributor handling.
- The waterfall itself uses [shared thin-water shading](../../../shaders/raytracing/include/rt_dielectric_common.glsl).
  Mobile retains actual transmitted RT and shared secondary shading; High adds
  traced reflection work. The same resident waterfall can coexist with the
  OpaqueFast strategy. Its mere presence does not prove global generic cost.

The logs identify the chosen shader, but do not record individual active generic
contributors or droplet counts. Source strongly supports the splash mechanism;
this receipt does not upgrade that inference into an isolated device result.
The retained-function generic strategy and fully inlined opaque strategy are
deliberate existing driver/budget contracts. Blindly forcing full inlining is
not a tested fix and is outside this checkpoint.

## Light-range probe and improved preparation

A private Mobile-only candidate fades direct fire-light contribution between
6 and 8 m. It retains visible fire and nearby RT visibility, and preserves all
eight High SPIR-V payloads byte-for-byte across Pipeline and RayQueryCompute.
This intentionally changes distant lighting; it is not an invisible efficiency
change, a new selected default or a player-ready setting.

The first expression exceeded an existing shader selection-merge budget and
was rejected. A revised expression reused the existing zero-contribution exit;
all 16 variants compiled and validated without raising limits. An initial
Android build caught a stale generated Pipeline catalog adapter. Regeneration
followed by an offline arm64 Debug build passed. Only the native library differs
in decoded APK entries; Java/resources and all 189 asset entries match baseline.

Preparation now uses equal 45-second warmup, then a fresh acknowledged import
of the same starting checkpoint on its owning native thread. A1/A2 verified one
new import in the same process, matching recorded world/settings fingerprints,
and a current RT-presented generation before motion timing. The offline checker
also rejects five changed/missing-state cases. This corrects the earlier live
world/prewarm mismatch; it does not control GPU clocks or power.

| Attempt | Actual outcome |
| --- | --- |
| reach-a1 baseline | 847 joined frames, 19.619 images/s, p95 70.833 ms; fresh start AP/BAT/SKIN 41.3/32.2/35.8 C, status 0. |
| reach-b1 candidate | Compatible installation/start, then rejected before motion: 43.0/34.2/37.5 C exceeded the declared 1.5 C start delta. No candidate timing. |
| reach-a2 baseline, new warmer cohort | 772 joined frames, 17.799 images/s, p95 83.333 ms; fresh start 41.8/34.4/37.1 C, status 0. |
| reach-b2 candidate | Passive cooldown ended after the bounded 90 seconds at BAT 31.1 C, above the declared 31.0 C target. No installation, launch or timing; closing C1 not started. |

A1 and A2 belong to different preparation/thermal cohorts and must not be pooled
as matched controls. Candidate B1's retained latest report is stale baseline data,
not a completed candidate run. Terminal stills show the Android menu, so they do
not establish the proposed lighting's in-game appearance. All 23 changed source
and generated files were restored and rehashed. Exact candidate source/APK
remain private for a future justified comparison; there is no production patch.

## Next bounded test, not an admission

Prioritise splash transmission over another unchanged light-range loop. Prepare
one instrumented comparison that reports active droplet count and generic
contributors, preserves water/geometry/lights/scale, and changes only the splash
material treatment. Compare current refractive droplets with a deliberately
lower-cost, opaque RT-lit spray candidate; keep full quality available. This
would be an explicit visual-quality option if accepted, not fake RT or removal
of the waterfall. It is not implemented by this checkpoint.

Use fresh equivalent state and short fixed views before, within and just beyond
the stream, including looking away while the same contact state remains active.
That distinguishes global shader selection from water screen coverage. Follow
with a moving traversal and a representative exterior control. Require closing
controls, actual-present tails, at least the existing 3% useful whole-frame
criterion, bounded thermal waits and inspected images. Keep state/light changes
out of the isolated comparison. An existing Water Off diagnostic can measure a
combined upper bound, but cannot by itself isolate splash cost or justify
shipping water removal. Stop the cohort if controls drift; do not relax its
thresholds after seeing results.

## Release and backup boundaries

Phone released **23:11:42 UTC** with baseline APK
`a9a2930a64c58c83de3e0399ee2dc31ee6f0e55c813746cfaaca83ce62496307`
installed and stopped. Original preference bytes and 15 campaign files match.
Automatic brightness mode 1 was restored (automatic value last read 173);
rotation is unchanged. Production and actor-worker packages were untouched.
No owned process, monitor or transfer remains. No Windows foreground was used.

This is an evidence-only backup. Runtime source is unchanged, and no new CI or
LFS download is needed solely for these documents. The baseline's six-job CI
remains its own evidence; it is not a new run for this receipt or candidate.
