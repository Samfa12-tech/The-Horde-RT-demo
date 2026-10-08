# 1.6.2 bounded mobile quality measurements

**Ordinary experimental choices — 9 October:** source `40677738` adds clearly labelled 33%/40% choices to Android and Windows Graphics for owner manual play. Fresh/reset phone default stays 50%; saved/custom preferences and Use/native ACK/Keep/Restore remain authoritative. 252 Java tests, lint, both builds, seven affected host suites and 12/12 runtime CI jobs pass. Exact normal-phone installation and both preview/Restore paths retain saved 50/custom effects and menu mix. The build is reopened for manual play; sustained performance and any upscaling decision follow feedback. [Exact artifacts and limits](ENGINEERING_1_6_2_EXPERIMENTAL_SCALES_2026_10_09.md).

**Motion comparison and owner decision — 9 October:** exact `a6439e0b` Shipping/Mobile APK completes matched-start 50/40/33 runs on SM-S948B/Android 16, real Pipeline, output 1440×2980, with native traced extents 720×1490 /576×1192 /475×983. Short ~6.5-second actual-presentation spans measure 24.288 /31.205 /46.171 images/s respectively, including milestone-readback overhead; these are not sustained FPS. Three separate encoded moving clips are shown, and the owner accepts **33% appearance for further sustained testing**. The phone default remains **50%**. Isolated historical 75/Mobile/Glass On is restored through normal ACK/Keep; primary settings/menu mix are byte-identical, owned apps/recorders stopped. Earlier thermal refusal remains recorded. Runtime CI is 12/12; sustained ordinary-play pacing/thermals, Keeper/reward overlap and final profile acceptance remain open. [Exact cohort, hashes and limits](ENGINEERING_1_6_2_MOTION_PRESENTATION_2026_10_09.md).

## First warm 50% route observation — 8 October, same 2a408613 APK

One newly traced mobile-default-profile probe completes both laps/26 waypoints,
with all 1,838 final owning frames joined to 3,677 actual-presentation records,
zero unresolved/loss/error counters, genuine Pipeline/MAILBOX, traced **720×1490**
and output **1440×2980**. Graphics requested/effective/saved and exact native
acknowledgement/Keep are observed before the cold benchmark launch: 50%, Mobile
water/fire, Glass Off, Current shadows, cap30 (menu/preview scope), Mist On, Dust
Off. This changes only the isolated benchmark; the primary app is untouched.

The 82.137502104-second measured span supplies 1,837 intervals: median
**41.666667 ms**, p90 **58.333333 ms**, p95 **75 ms**, max **91.666719 ms**;
1,782 exceed the explicit 33.333 ms threshold and none exceeds 250 ms.
Observed presented-image rate over that span is **22.364936/s**, below the target
in this short controlled route. This is not ordinary-play/sustained FPS, a
thermal plateau or a final profile decision. The existing per-course-frame
simulation convention and CPU/GPU timing definitions are preserved.

The phone was already warm after live Graphics work: typed current HAL AP
44.4→45.2°C, battery 37.6→38.2°C, skin 39.7→39.9°C, Android thermal status 1
at both sampled endpoints, USB powered. This is an ordered warm observation,
**not a thermally matched 50/40/33 cohort**. Do not infer scale-only savings
against the 75% collector run: glass and initial thermals also differ. No 40/33
result, normal-play motion/quality decision, power availability, Keeper/reward
overlap or sustained acceptance follows. No quality/default reduction is made.

Raw report SHA-256 `8fbc2b85f080cea34524256c62037aa5bebdd7360cb5df3377ad4050df593825`;
[sanitized analysis](evidence/2026-10-08-present-timing/scale50-warm-route01-analysis.json)
and [exact package/receipt](evidence/2026-10-08-present-timing/receipt.json).
The original isolated 75% / Mobile / Glass On / Current / cap30 / Mist On / Dust
Off save is restored through exact acknowledgement and Keep; owned app stops.
The first attempted reset correctly refuses an offscreen target without touching;
bounded ordinary scrolling reaches it. That harness guard is not a settings failure.
Analyzer/tooling head `67bb4d47b412d15ad946b84fafa6dc5ef4e20765` separately completes
12/12 aggregate CI jobs. Later documentation-head CI remains separate.

**Earlier presentation-timing checkpoint — 8 October:** runtime `2a408613668a4936dd1592d79cd141e1d6baf5dc`, isolated Shipping/Mobile benchmark APK `faf35f482b78150b7157335b56c3a0e5c499ec1d2205b9dc6a82bd90ce82992f`. Optional actual image timestamps join all 600 lantern and 1,838 complete-route measured frames on SM-S948B / Android 16 at the benchmark’s preserved 75% profile. Collector/retirement fixtures, admission/build/package checks and 12/12 runtime CI jobs pass; nine offline analysis tests pass. Ordinary builds remain Off, assets/defaults/custom saves are preserved, and owned apps are stopped. This establishes measurement, not 50/40/33 or sustained-30-FPS acceptance. [Exact subject, intervals, retained failures and remaining gates](ENGINEERING_1_6_2_PRESENT_TIMING_2026_10_08.md).

The new 75% collector proof remains separate from the historical baseline. Its complete course has a matched 142.608337028-second measured span; observed image rate is 12.881435/s, with 70.833437 ms median and 141.666719 ms p95 intervals. Those actual-display intervals do not replace existing CPU/GPU definitions or establish any 50/40/33 result. Power remains unavailable and sampled thermal rise is not a plateau.

Current authority, 9 October: fresh/reset Android defaults are **50%, Mobile
water/fire, Glass Off, Current shadows, cap30 and Mist On**. Desktop stays
100%/High water+fire/Glass On/Current/cap30/Mist On. Saved/custom preferences
remain authoritative. Ordinary Graphics now admits discrete experimental 33%/40% choices; fresh/reset 50% and native UI resolution stay unchanged. The owner's target is practical sustained 30 FPS at an accepted
profile, with a same-device/backend/output/integrated-route 50/40/33 comparison
and an explicit owner decision. No 33% default or acceptance is inferred.

The numerical 75%, 50%, 40% and 33% results below describe their historical
artifacts and workloads. Their then-default 75% and fixed-75% target do not
override the current goal. Historical sustained fixed-75% performance and the
earlier near-60 recollection remain unmet or unproven; they are preserved
separately from the current target.

## Integrated comparison artifact prepared after recovery

Source `ebffb4f0412f76b46ee1beb735475a145207e58a`, tree `2aec05aff62a3f582501e326407a8875b70a6506`; isolated Shipping/Mobile benchmark APK SHA-256 `d2d9d62f064119ac46bea10f8fd66336cdf65d0225a5c5c165a5dbcb950f5322`, 120,571,024 bytes. Four-ABI assembly/vital lint passes in 2 min 46 s; actual models/caches/compile commands, stripped payloads, non-debuggable package/development certificate and 16 KiB alignment are verified. All 91 runtime assets match the owner-tested ac468f51 Debug package. This subject includes normal waterfall guards, production stowed start, accepted wide equipment, Dust support/default Off and same-generation recovery. Native Debug motion/checkpoint markers remain absent; ordinary min50/default50 stays unchanged. This is build/package preparation, **not installed, not a 50/40/33 play or sustained-performance result**. The earlier 7a subject below is historical and is not used as the integrated cohort.

The then-current Debug motion adapter for that artifact admits only ordinary min50. Sending 40/33 to it would be rejected/clamped and cannot establish newly traced lower-resolution evidence. Keep the existing isolated Shipping/Mobile min33 admission rather than weakening ordinary build limits or presenting image resizing as tracing. The existing route benchmark supplies exact owning CPU/GPU distributions but does not become free play, actual compositor cadence or sustained performance; those gaps remain explicit.

## 8 October preparation and pacing-method limits

An isolated four-ABI ShippingMobile benchmark package is prepared at source
`7a628fe6e9143c3d6ea2f2bca44ec12a10b22002`, SHA-256
`6102925dde3ced29b2f54d103e221d8d41400f2da294c2f1679bb6fd83654446`,
120,028,712 bytes. Build passes in 1 minute 57 seconds. Actual native build
models, resolved caches and compile definitions admit min33 only through the
explicit benchmark opt-in; ordinary minimum/fresh default remain 50%. All four
actual stripped payloads match the package, diagnostic drench/input markers are
absent, closed assets and 16 KiB alignment pass. The non-debuggable `.benchmark`
package uses the Android Debug development certificate, without production
signing. It is **not installed or performance-tested**. Production stow/waterfall
flags remain off; it is not the final integrated quality-comparison subject.

The existing in-app showcase benchmark has one warmup and one measured lap,
13 waypoints and 1,838 owning measured rows. Its CPU whole-frame cycle and GPU
RT distributions, accepted presents and trace extents are useful exact-workload
facts. They are not live-input, compositor/display intervals, sustained free
play, or a substitute for expensive combat/Keeper/reward overlap. The original
timing definitions stay unchanged. The existing historical memory collector's
strict 75% identity is not silently repurposed for a 50/40/33 cohort. Existing
thermal/governor fields do not provide watts; earlier battery-current access
was denied, without a permission or security workaround.

A read-only owned native SurfaceView pacing feasibility probe uses the earlier
exact `408126d4` Debug APK, with package/PID/foreground guards and unchanged
preferences. Two initial attempts retain their layer-selector failures. The
third identifies the actual Android 16 `RequestedLayerState` surface name and
queries that exact layer four times. Every latency response contains only the
8,333,332 ns display-period header, **no timestamp triples and zero valid actual
present samples**. There is no FPS result; nominal display refresh is not the
game's displayed frame rate. Owned apps stop. No counter clear, TimeStats
enable/disable, screen recording, resolution or system-setting change occurs.

[Android's frame-rate measurement guidance](https://developer.android.com/games/optimize/framerate)
describes per-layer TimeStats present-to-present histograms. A later read-only
dump finds historical Debug Activity/splash statistics, without a new owned
native SurfaceView sample. Those counters are not this run's RT frame cadence.
Prior collection is preserved without global clearing or changing its state.
Actual usable native presentation pacing, sustained thermals, reliable power/
memory and the controlled integrated comparison therefore remain open. Closed
selective-opacity, Lower-indirect and mirror-to-stone negatives are not rerun.

## Historical exact artifact and workload

Runtime source is `95eb08070354d4b3b775e6d2368cca8688f9cfe6`; the installed, pulled-back benchmark APK SHA256 is `269ac11a129cf66b8f170bceb3b5b77670aa567480faec4764e5ab0322367790`. SM-S948B/Android16 used genuine RayTracingPipeline/MAILBOX, ShippingMobile O2/NDEBUG, Mobile water/fire/dielectrics and Glass On. Presentation extent was 1440x2980. The exact selected opaque/generic module hashes are `cbbea6b92696ee710a34fa6d15a579a7b9c67a2f737716dea87861b6bd3f0cc5` and `4f60824853e357bfab745b124613377c0db6afda268d467ad7de95108f4e81a9`.

Each ordinary UI-started showcase route completed one warmup lap and one measured lap over 13 waypoints per lap (26 visits total). Each summary has 1,838 expected/completed/CPU-accepted/GPU-valid owning rows; rejected, cancelled, outstanding and GPU-unavailable populations are zero. CPU cycle measures completed owning render-entry-through-present; GPU measures completed owning RT duration. Neither is display scanout FPS or a sustained free-play benchmark.

| Historical ordered route | Internal extent | CPU cycle median / p95, ms | GPU RT median / p95, ms | Battery temperature start -> end |
| --- | --- | --- | --- | --- |
| 40% | 576x1192 | 28.634323 / 37.904479 | 18.486874 / 26.648437 | 43.8 -> 43.3 C |
| 33% | 475x983 | 24.065652 / 30.603333 | 13.728281 / 19.448125 | 43.9 -> 43.5 C |

These are single ordered warm/hot runs with USB connected and cooling unknown. Battery temperature comes from the current dumpsys header, not its historical entries; endpoint polling lag is at most 20 seconds. Typed report thermal availability remains not-collected/status unknown. There is no thermal parity, repeated cohort, causal saving, whole-game or fixed-75% performance acceptance. At 40%, CPU p95 exceeds the 33.33 ms frame budget. The 33% distribution alone does not establish sustained ordinary-game performance.

The reviewed literal summaries are 10,310 bytes at 40% (SHA256 `96efa605a0ea4e4f20ec2c3573536787f2e4d9bbb629770713d521b9da986b8d`) and 10,318 bytes at 33% (`23b6158110a7d73ec99d31b8532cf3e17cd47cbbca21423ba9d93dcbc265ab58`). Preparation was explicitly consented; optional hardware inclusion stayed off. Private literals, identifiers, screenshots and device logs remain outside Git.

## Materials preview is a different workload

After a 20-second preview warmup with a 30 Hz menu cap, actual settled Details reported:

| Historical Materials preview | Successful RT presents/s | CPU loop/render, ms | GPU RT, ms | Graph transitions |
| --- | --- | --- | --- | --- |
| 40% | 15.1 | 66.19 | 63.13 | 0 |
| 33% | 23.1 | 43.30 | 40.92 | 0 |

The closed-glass Materials scene differs from the moving route and Mobile lantern configuration. Its counter is successful RT presentation, not scanout. Counter epoch/count were not exported. These observations support reporting the preview's actual cost, but do not isolate glass, CPU, fire or shadow cost, or establish matched ordinary throughput. Tracked preview device-local allocations were 19.64 MiB/18.94 MiB respectively and host allocations 2.19 MiB; these are tracked allocation counts with overlapping owners, not total VRAM residency.

The historical 50% course used a different APK (`e339da58`): CPU cycle median 37.585808 ms/GPU RT 26.934999 ms, internal720x1490, battery41.9 -> 42.8 C. It remains historical evidence, not the third member of a same-artifact controlled cohort. Its earlier 40% observer never started an accepted course; stationary dead-player runs and second-Activity command-line attempts are rejected. Failed preview/navigation attempts remain recorded privately without being promoted to successful courses.

## Later returned75% review

The owner reallocated the same SM-S948B/Android16 on4October. The same benchmark269 APK was freshly pulled back and matched without installation. [Returned-device review](ENGINEERING_1_6_2_RETURNED_DEVICE_REVIEW.md) records a separate ordinary75% course with1,838 owning CPU/GPU-valid rows, CPU median/p95 64.160860/83.593542ms and GPU53.590051/73.118071ms. Settled75% Materials reported5.8 successful RT presents/s, CPU173.43ms/GPU170.11ms/zero transitions. Course battery header40.2 -> 42.5C/USB/unknown cooling and native thermal not-collected do not make this a balanced or thermally matched cohort with earlier40/33. Initial33.3C preceded Preview and is not the course baseline. Live original requested/effective values match; Draft require Apply does not prove a new save gate.

Separate Debug Replay/capture/Home correctness is now sealed by receipt `ca226dcb663e3b7571947aaf63bc1f2a5d8eff9dcd14bcb02eb4eb01f1196bb3`: both Replay routes complete13/13 and new completed owning resume packets pass on both backends. These Diagnostic runs have empty timing CSVs/stationary captures and add no Shipping performance distribution or phone owner approval. Ordinary Debug Preview/Home retains the original confirmed tuple/pending=false without quality edits or Apply/Keep. Earlier saved-pending packets remain historical limits, not upgraded by later proofs. Validation apps are stopped/phone Home; live Windows owner review remains open.

## Disposition and restoration

Retain explicit experimental choices in the isolated benchmark only. Do not silently reduce ordinary quality or infer an upscaler/DRS benefit. The rejected player-lookup experiment remains rejected; its implementation was restored. Low-fire/shadow investigation has a separate [bounded design outcome](ENGINEERING_1_6_2_LOW_FIRE_SHADOW_INVESTIGATION.md), with no independent control or measured saving claimed.

After both courses, ordinary preview Apply/Keep restored 75%/Mobile water/Mobile fire/Glass On/30 Hz. A separate fresh benchmark process showed both requested and effective original values. The restore-time preview numbers are configuration evidence, not a matched 75% performance baseline. Fresh Debug restart recovered vitality3 and returned to the main menu; its saved schema2 original tuple/pending=false was collected. Both validation apps were stopped; production and user data were preserved.

The independent quality receipt SHA256 is `bf46ad55dbff5412eab54242bad0be74adfbd056d08855edc98540572e5679bc`: 47 checks and362 closed files stable, all13 CPU-stage denominators and10 zone populations/weighted-mean joins pass. Actual selected shader words match across all four ABIs and the sealed APK/installed pullback. Per-course APK identity relies on the recorded uninterrupted installation/process context, rather than atomic per-course attestation. Screenshot counters14.3/23.8 were sampled at different times from the table's settled Details15.1/23.1. Final-state audit `cb3f0d24ba7185647694e78363a4b327fdb2d4d3bf5b3b519bc8a33e630d518c` seals20 files and verifies cold original settings, Debug restart/menu and preferences. Additional artifact admission identities are in the [current candidate](ENGINEERING_1_6_2_FINAL_CANDIDATE.md). Owner appearance/audio/haptic acceptance and sustained fixed75% performance remain separate gates.
