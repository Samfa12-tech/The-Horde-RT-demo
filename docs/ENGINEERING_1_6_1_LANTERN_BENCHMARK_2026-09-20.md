# Production lantern benchmark workloads

The old route benchmark does not obtain the reward lantern. Its `finale` zone
name does not certify the reward/reveal workload. These additional cases address
that coverage gap without changing the historical route or renderer quality.

Implementation: shared scenarios/reporting `6335049`, Windows launch `efb04d1`,
Android launch/export `7ccb753`. This is development measurement tooling, not a
glass optimization or completion of the audit's performance/image gates.

| Workload | Simulation behavior |
| --- | --- |
| `showcase-route-v1` | Unchanged 13-waypoint route; 1,838 measured frames |
| `lantern-held-high-v1` | Frozen authored held-high production lantern |
| `lantern-held-low-v1` | Frozen authored held-low production lantern |
| `lantern-grazing-v1` | Frozen angled pendulum/lantern view |
| `lantern-motion-extreme-v1` | Frozen extreme-swing state, **not** a live motion test |
| `lantern-reveal-sequence-v1` | Live ten-second 60 Hz raise/reveal/skylight/dawn sequence |

Each lantern case runs 600 warm-up frames followed by 600 measured frames.
Both laps import the same gameplay-owned initial state. Frozen cases advance
simulation by zero delta so they cannot drift into a cheaper completed finale;
the live case advances the real simulation. Reports name both the versioned
workload and frozen/live policy. A lantern case reports `routeTraversalComplete`
false rather than claiming it traversed the corridor. Report each case separately.

`StageLanternBenchmark` uses the existing simulation checkpoint/import interfaces,
held-light state, hand/hinge resolution and pendulum transform. It does not expose
arbitrary Debug checkpoints, change camera-space presentation, select glass-only
geometry, override tuning, alter ray masks or lower resolution/physical transport.
All ordinary production scene geometry/shading remains in the renderer. Benchmark
completion must not grant progression/unlock state or open the ending overlay
before the final measured submission is drained.

## Commands

Windows Release:

```powershell
.\HordeLanternRT.exe --benchmark-showcase C:\path\to\unique-output `
    --benchmark-workload lantern-held-high-v1
```

Android uses the isolated non-debuggable Shipping/Mobile validation build described
in [the build guide](ANDROID_BENCHMARK_VALIDATION_BUILD.md). After verifying the
device and exact installed APK, use a unique run ID:

```powershell
adb -s VERIFIED_SERIAL shell am start -W --user 0 `
    -n com.samfa12.hordelanternrt.benchmark/com.samfa12.hordelanternrt.MainActivity `
    -a com.samfa12.hordelanternrt.action.BENCHMARK `
    --es horde.benchmark.run_id UNIQUE_RUN_ID `
    --es horde.benchmark.workload lantern-held-high-v1
```

The fully qualified Activity class is necessary for suffixed application IDs.
No `-S` allows a subsequent Activity to reuse the existing process; record actual
PID/launch state, case order and thermal context rather than assuming sustained
equivalence. After completion, pull that run's app-specific external directory.
The exported marker must be complete and the requested ID/workload must match.
Check 600 expected/completed/CPU-accepted rows, explicit GPU status, exact owning
completion joins and zero rejected/cancelled/outstanding samples. A failed or
interrupted run is invalid, not a zero-time or partial performance pass.

## Fresh development checks

- Shared Debug scenario test passed; fresh Release selection passed 5/5:
  lantern scenarios, launch parser, old route, evidence owner and report tests.
  Scenarios assert real held-light/chest/finale ownership, finite hinge/body
  transforms, exact warm-up/measured state sequences, frozen-state stability and
  coverage of every live reveal phase.
- Windows Release built. Exact EXE
  `cfff415877611f224e2fa89f2ec7240bc7b8acc7604415b6bbbd097000320b37`
  completed all five lantern cases, each with exit 0, 600 completed rows and
  600 valid GPU samples. Reports:
  `reports/lantern-benchmark-wip-20260920/verified-*/`. These overlapping-build
  development runs are functional evidence, not matched performance results.
- An earlier same-renderer held-high run was visually inspected through the
  native Windows window; the real lantern was present. That EXE was
  `4cca40eebe3d6648f73139a2865725149a7dfca2ba05d80dbd830ae67d494cf6`,
  before the final HUD/report-label adjustments. Do not substitute this for
  Shipping/Diagnostic image parity or final player/viewmodel acceptance.
- Android Debug built all four ABIs. The actual non-debuggable benchmark build
  also built all four ABIs and passed 11 focused Java tests (8 export, 3 Intent).
  Final packaged ARM64 SPIR-V validation/disassembly passed with four expected
  Shipping/Mobile modules, zero diagnostic atomics/binding 22 and real ray queries.
  Stripped/packaged ARM64 hash:
  `b8d58ed70d710974966e0d557c50ae394d6fd6106aa7d9e73f5fd5eafba84b86`.

## First physical Shipping observation

The development-signed APK
`0f30a72535a532f770d59f817b23934a2152f087588de95e8982698c60d80a95`
was installed in the separate `.benchmark` package on exact model `SM-S948B`
(Android 16) and pulled back byte-for-byte. Stable/Debug packages were not replaced
or cleared. Its runtime source is now committed as `7ccb753`; compilation occurred
before that commit, so this is an exact hashed development artifact, not a fresh
clean-checkout release build. A later progress-label-only patch is not in this APK.

`lantern-high-20260920-01` completed and exported all 600 measured rows with valid
CPU/GPU joins and no rejects/cancellations/outstanding samples. Strict ASTC,
RayTracingPipeline, honest presentation, Adreno 840 and Vulkan 1.4.295 are recorded.
Independent current `cmd gpu vkjson` reports driver integer 2150932499
(512.842.19). The captured image shows the actual held lantern.

The unchanged default was **100%, 1440x2980**. Overall median was **130.469 ms**,
mean 130.5828 ms, P95 132.0279 ms and slowest-one-percent-derived 7.4435 FPS.
This is approximately 7.7 FPS by median, below the descriptive 30/50/60 FPS bands.
It establishes that this glass view is expensive; it is not an improvement or a
comparison with an equivalent 1.6.0 lantern run. Observed Android thermal status
and GPU thermal power level were both 0; battery temperature during the run was
29.8 C. These spot observations are not a sustained thermal trace.

The remaining cases completed in the same PID, in high → low → grazing → frozen
extreme → live reveal order, each with 600 measured rows and exact completion
joins. Activity/native rendering contexts were recreated between cases; this
is same-process ordered observation, not uninterrupted gameplay.

| Case | Median ms | P95 ms |
| --- | ---: | ---: |
| Held high | 130.4690 | 132.0279 |
| Held low | 118.4801 | 120.1086 |
| Grazing | 103.5469 | 112.7632 |
| Frozen extreme swing | 100.8550 | 109.6016 |
| Live reveal | 146.9541 | 152.7008 |

The later three cases have periodic context records: grazing thermal status 0–2,
GPU thermal power level 0–1; extreme status 2, power 0–2; reveal status 2, power
0–3. Direct GPU clock reads were permission-denied. Different cases and changing
thermal/governor state cannot establish a causal optimization comparison. Several
`am start -W` calls hit their ten-second Activity wait timeout, but the same live
process continued and subsequently produced verified complete exports; none was
restarted because of that observation timeout.

Evidence: `reports/phone-shipping-lantern-20260920/` contains exact candidate and
installed APKs, `held-high.png`, `vkjson.json`, all five exported runs, later-case
context logs and a per-row verifier. Interruption/lifecycle checks, sustained
thermal-matched A/B, image parity and eventual viewmodel owner acceptance remain
separate gates.

## Interruption regression and fix

Home during `lantern-home-20260920-01` correctly exported only an invalid marker,
but surface teardown could bypass the queued cancel/reset command and leave the
temporary scene in the process. `6576950` moves restoration into the existing
owning-render-thread cancellation helper; its normal cancel caller no longer
resets twice. A shared test now asserts cleanup restores spawn/torch/non-finale
state. The targeted test and four-ABI benchmark build passed.

Cleanup APK `faf42c587379238796bfae987382386ea31028a643a9a28f6081827aadfc0eb1`
was installed and pulled back byte-identically. `lantern-home-fixed-20260920-01`
again produced only the invalid marker, with no stale benchmark JSON/text. Normal
relaunch then honestly presented RT and entering the ruin showed the ordinary
spawn/torch scene rather than the held reward case (`cleanup-resumed-scene.png`,
`cleanup-logcat.txt`). The phone was returned Home. This is a targeted lifecycle
pass for the cleanup APK, not a remeasurement of its five-case performance.

## Shipping/Diagnostic image and counter check

Fresh Debug-host captures of Diagnostic/High and Shipping/High-override executables
passed all 13 historical checkpoints, plus `lantern-held-high` and
`lantern-glass-transmission`. The lead independently reran the retained
`reports/parity-20260920/compare.py`: all 15 pairs satisfy the existing RGB
max-absolute tolerance 3 (largest difference 2). Both lantern pairs are PNG
byte-identical. Manifests bind actual camera/state/extent and PNG hashes.

- Diagnostic EXE: `93141582fdfd34b3a5109bde11611b064851e194f999bc3c1b324af61d415be7`.
- Shipping-override EXE: `58f1a8b56c16d7e52ec4319cb75e153e0c722cd5ee5a169d6065b5c37f2c742e`.
- Both exact executable scans pass external SPIR-V validation/disassembly with
  four expected RTP/compute modules. Diagnostic modules retain binding 22 and
  5/32 atomics per strategy; Shipping modules have neither binding nor atomics.
- Diagnostic counters remain available. They are **not all acceptable**: held-high
  records one transport overflow, one pane-stack failure and one primary volume
  budget event. Transmission records 34 secondary dielectric terminations. These
  are unresolved glass findings, not erased by Shipping's compiled-out counters.

Authoritative targeted captures are the `*-v3` directories under the parity root;
`pixel-comparison.json` holds all results. These are Debug-host shader-equivalence
checks, not Release timing, Mobile-driver parity or general glass correctness.

Capture-command incident: an incorrectly quoted delegated `Start-Process` output
argument wrote `116-lantern-held-high.png`, `118-lantern-glass-transmission.png`
and `capture-manifest.json` under the pre-existing
`C:\Users\sam_s\Documents\the` folder. The 118 PNG was newly created there; the
116 PNG and manifest were overwritten. No exact pre-run manifest backup is known.
Older captures elsewhere are not proven restoration copies, so no restoration or
deletion was attempted. Correctly quoted, isolated reruns provide the authoritative
evidence above; the incidental folder is not used as validation authority.

Audio/haptic manual revalidation required: **NO** for this harness. Normal gameplay
events, playback and haptic routing are unchanged; scenario staging is explicitly
benchmark-only. The later adaptive music integration has its own listening gate.
