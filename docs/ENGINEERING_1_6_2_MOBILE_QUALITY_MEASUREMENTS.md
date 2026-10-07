# 1.6.2 bounded mobile quality measurements

Current authority, 8 October: fresh/reset Android defaults are **50%, Mobile
water/fire, Glass Off, Current shadows, cap30 and Mist On**. Desktop stays
100%/High water+fire/Glass On/Current/cap30/Mist On. Saved/custom preferences
remain authoritative. The ordinary minimum remains 50%; native UI resolution
is unchanged. The owner's target is practical sustained 30 FPS at an accepted
profile, with a same-device/backend/output/integrated-route 50/40/33 comparison
and an explicit owner decision. No 33% default or acceptance is inferred.

The numerical 75%, 50%, 40% and 33% results below describe their historical
artifacts and workloads. Their then-default 75% and fixed-75% target do not
override the current goal. Historical sustained fixed-75% performance and the
earlier near-60 recollection remain unmet or unproven; they are preserved
separately from the current target.

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
