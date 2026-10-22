# S26 Ultra outdoor scale checkpoint - 22 October 2026

The live, stationary lookout is expensive at33/40%.20% reduces GPU work but
remains **diagnostic only**, with unacceptable/unadmitted image quality. No
engine optimization, new default, earned journey or sustained-performance pass
is claimed by this checkpoint.

## Exact scope

SM-S948B, Android17/API37, Adreno840. Development-signed arm64 Debug APK
`712f070996124014832b9cf6bf4941e7fbac2d448fabfac926297be8d9d942a2`,
Shipping/Mobile shaders and explicit private outdoor probe. Base source
`bb507d2461b8feaeae630ea08a9bec62d18b7376` plus the compiled-source hashes in
[receipt.json](receipt.json). Existing LOD1, hardware RayTracingPipeline,
water1/fire0/cap30/glassOff/shadow1/MistOff/dust1 are fixed. Saved scale33 and
all owner settings remain unchanged. Private20 is transient and is neither a
normal UI/storage value nor a Release setting.

The already existing checkpoint203 stages a post-Keeper lookout; the hold does
not grant new progression or teleport. Each40-second run advances the normal
60Hz simulation, excludes10 seconds of warmup, then samples30 seconds. Immutable
submission/completion/presentation ownership, stationary camera, ordinary
residency and zero mid-run readbacks are checked. A first conditioning run
precedes33/20/33/40/33. Complete raw reports stay local under ignored
`reports/horde-outdoor-scale-20261022/`.
The existing Debug motion path suppresses speech/SFX delivery and suspends music;
this is an explicitly muted noncombat workload, not audible play acceptance.

Native window/surface output is2981x1440 landscape, swapchain1440x2981 with
pre-rotation2. Dispatch is288x596 at20%,475x984 at33%,576x1192 at40%.
The full display capture is3120x1440 including its139px cutout strip. UI remains
at display resolution. All retained runs pass exact extent/transform/tuple and
completed-frame/presentation joins.

## Measured medians

| Order | Scale | Post-warm frames | GPU ms | CPU frame cycle ms | Actual image presentation interval ms |
| --- | --- | --- | --- | --- | --- |
| A |33%|619|35.511|47.748|50.000|
| B |20%|955|16.083|31.911|33.333|
| C |33%|617|35.486|47.857|50.000|
| D |40%|467|48.205|64.711|66.667|
| E |33%|499|43.933|58.279|58.333|

The complete ranges, p95 values, sample counts, CPU stages, call counts and
thermal traces are compactly retained in the receipt. Actual image intervals
come from joined VK_GOOGLE_display_timing rows; they are not scanout, photons,
video frame gaps or reciprocal CPU/GPU FPS. CPU/GPU/fence durations overlap.

A and C GPU medians differ0.07%;20% is55% below their GPU time. The closing33
GPU median rises23.8% and CPU22.1% relative to A. This exceeds the15% regression
investigation threshold and remains **unexplained**, not discarded or averaged
away.40% therefore has a qualified comparison against a drifting closing
control. All starting current-HAL AP/BAT/SKIN temperatures remain within the
predeclared1.5C band and framework status0; this does not establish equal GPU
clocks, power or a prolonged steady thermal state. Neither clocks nor power
were measured. The finite cohort does not close sustained warm acceptance.

At20%, median total skin CPU is12.060ms, pose copy0.815ms, upload0.519ms,
submit0.162ms and fence wait16.433ms; CPU frame cycle remains31.911ms despite
GPU16.083ms. At the first33 control skin is8.258ms, with per-stage
values retained rather than substituting old8.7ms claims. Calls and live ticks
prove normal animation, not a frozen pose. Stable20BLAS/28TLAS instances,
160,578,496 device-local and48,277,888 host-visible bytes exclude a growing
resource roster in this workload. These totals do not attribute AS versus
shading cost. Next performance work should inspect measured fixed animation/
copy/submit costs and actual ray/material/geometry costs; existing skin caches,
enemy refresh and BLAS UPDATE must not be proposed as missing.
These are per-stage elapsed CPU wall timers and invocation counts, not OS CPU
utilisation or on-core/thread sampling. Animation-only, BLAS/TLAS-stage, thread
scheduler and CPU-frequency attribution are absent. Frame cycle includes GPU
fence wait/pacing and must not be presented as CPU busy time.

## Retained failures and staging repair

The first live hold rejects before recording states/frames. A diagnostic build
`2e412bfbbf0f815fe7f5ca243247b711c35bdb2ecb2c8bfb6c9989b2e0724247`
identifies a nonzero semantic event overflow. Real imported-terrain CPU staging
reproduces48 overflows at tick2757, endpoint13.3049/36.2046, one claim and one
deployment. The diagnostic preparer had accumulated thousands of route ticks
without draining its64-event queue. Its music worker also stopped; that earlier
run cannot establish normal music/mix acceptance.

Only diagnostic exterior staging now accumulates evidence and drains each
observed step. It preserves event sequences, totals, combat staging and overflow
rejection; it does not reset a failure counter. The same real-terrain route now
ends at the same tick/position/claim/deployment with0 overflows.
[Red](actual-terrain-red.log), [green](actual-terrain-green.log) and
[three focused host registrations](host-checks.log) are retained. A separate
synthetic route staging attempt failed at the landing with overflow0 and did
not prove the phone cause; its ignored raw failure remains available. Seven
profile Java tests and the Android build also pass. No phone combat occurred.

## Image quality and custody

A separate20% screenshot and12-second original device clip are local under
`quality20-visual/`; their hashes are retained, and recording/screenshot overhead
excludes that entire case from timing comparisons. Inspection shows coarse
lantern/building edges and noisy ground. The camera is stationary; the clip does
not accept camera-motion ghosting or disocclusion.20% is not recommended from
speed alone. See the [reconstruction readiness audit](../../design/HORDE_1_7_UPSCALER_READINESS_2026_10_22.md).

Phone released at19:44 UTC, before19:55 target: ordinary Quit, launcher in front,
no Horde render surface, no install/capture/transfer active. All18 original save/
lock hashes and settings bytes match; zero new save paths. Original font1.7 and
rotation0 restored, production APK unchanged. Android retains only a cached
owned Debug PID with no activity/render surface. The final UI APK differs from
the timing APK and has separate, incomplete acceptance; see the current checklist.
