# Splash refraction: useful attribution, no admitted phone preset

Four same-APK Android routes completed on 31 October; final custody verification
was **1 November 2026 00:00:00 UTC**. The private candidate switched only the
existing geometric contact droplets between their authored refractive material
(A) and an opaque RT-lit material (B). It preserved the waterfall, droplet count,
geometry, lights, simulation, resolution, all 189 packaged assets and all 16
shader payloads. This is an appearance trade-off, not a shader efficiency fix.

**Decision: no production adoption.** B consistently avoided the generic
transmission strategy and improved the predefined waterfall interval, but the
closing control drifted beyond the declared limit. The candidate also failed
the declared 3% whole-route improvement gate. Its visual route did not produce
contact droplets. The experimental material switch was archived privately and
removed from runtime source. Only tested immutable contact telemetry remains.

## Exact cohort

Source base `c961f4940337ca4f4205aa3ab97d60e75147cce9` plus the archived probe;
APK SHA-256 `5754b6d580ed7a6907c67170a2613cb652cf1dc92b6240943eaef68b9218f51a`.
SM-S948B / Android 17 / Adreno 840 / Vulkan 1.4.295 / driver 2150932499.
Shipping/Mobile RayTracingPipeline, saved 33%, internal 475x980/output 1440x2971;
Mobile water/fire, Current shadows, Glass/Mist Off, Low dust. These are
instrumented Debug generated-input routes, not sustained ordinary play or
physical-input latency. [Receipt and retained input hashes](splash-receipt.json).

Order A1/B1/B2/A2. Every case had 45 seconds of preparation, a newly acknowledged
checkpoint in the same process, equal starting-state fingerprints and no timed
image readbacks. All sampled framework thermal statuses were 0; fresh-start
AP/BAT/SKIN temperatures differed by at most 1.5C. There were 3,140 strictly joined
completed RT frames, stage rows and actual presentation timestamps.

| Case | Whole route images/s | Water window images/s | Water interval median / p95 ms | Generic frames |
| --- | ---: | ---: | ---: | ---: |
| A1 refractive | 19.100 | 14.195 | 75.000 / 104.167 | 20 |
| B1 opaque spray | 18.295 | 18.409 | 50.000 / 83.333 | 0 |
| B2 opaque spray | 17.958 | 17.341 | 50.000 / 91.667 | 0 |
| A2 refractive | 17.527 | 13.061 | 87.500 / 116.667 | 18 |

The water window was selected before collection: approach simulation seconds
15–18. Whole routes lasted approximately 43 seconds. Counts and rates in these
windows are not reciprocals of GPU durations. Both A runs selected generic
transmission precisely on frames with active droplets; B retained active
droplets without that selection. A's largest dispatches were 138.682/158.441 ms;
B's water-window dispatch maxima were 75.960/83.263 ms.

The closing whole-route rate fell **8.234%**, exceeding the 5% stability gate.
B was slower than A1 overall, despite the local improvement. The unaffected
pre-water region also slowed (a post-hoc descriptive check, not a new admission
gate). Do not average the controls into a saving or claim stable 30 FPS, an outdoor
gain, or a solved phone default. GPU clock/power traces were not collected; a
single idle `thermal_pwrlevel` read after cleanup returned 0 and cannot explain
the timing drift.

## Visual and implementation limits

The separate full-reference `torch-drench` route completed and generated 16 native
RT captures, but every captured state contained zero active droplets. Two exact
frame-bound captures were recovered and inspected; neither validates the changed
spray. No B visual run followed this unsuitable route. The host capture script
also incorrectly compared the native manifest's legacy `1.6.2` label with the
separate detailed provenance ID. Installed APK hashes, rather than that legacy
label, establish artifact identity. No wrong-APK launch was found.

Candidate checks: two C++ CTests, two focused Java intent tests and the offline
arm64 Android build passed. After removing the probe, both affected CTests and
the offline Android build passed again. Source restoration preserves the six
original runtime/test files and removes only the three new probe files. The
retained ledger records active-droplet count/in-stream/wet-ground from the
immutable snapshot; its regression proves later mutation cannot rewrite a row.

No new hosted CI is claimed. Backups use `[skip ci]` while the explicit no-LFS-
downloads restriction remains; the calendar changing to November does not prove
quota reset or authorize a new download. Raw APKs, reports, captures, source
snapshot and scripts remain in the ignored local experiment directory. The
receipt provides hashes; it is not a claim that raw artifacts are in Git.

## Next phone-default decision

The owner reports never noticing the spray. Treat that as owner observation,
not visual parity proof. Under the approved phone-preset scope, next consider
**contact droplets Off** while retaining water and the full-quality option.
Validate an actual active-contact view, gather clock/power evidence where
available, and compare representative opening/combat/outdoor presentation and
tails. Existing explicit saved choices must survive. This local finding alone
cannot establish a genuinely playable default or the cost of the future forest.

The unmeasured fire-light-range probe remains separate and unadmitted. No new
default, preset, shader, forest or actor-worker activation follows from this
checkpoint. The baseline Debug APK was restored and stopped; original
preferences and all 15 campaign files were verified unchanged. Temporary display
settings were restored; production/worker packages were untouched. Phone,
monitors and transfers were released.
