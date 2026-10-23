# Exclusive actor preparation worker — desktop experiment

23 October 2026, based on `e2e93858` plus the recorded source hashes. **Opt-in
experiment, default OFF.** No Android gain, overall FPS gain or sustained-play
acceptance is claimed. No phone operation occurred in this checkpoint.

## Change and ownership

`SkinnedPbrActorWorker` is one persistent, exclusive CPU lane for one independently
owned actor. A bounded request owns names/rotations, clip/blend, tick identity and
generation. Results own diagnostics/metrics. Player and Kit use distinct asset and
scratch objects; this is not parallel skinning through the shared skeleton model.

The opt-in scene dispatches Kit preparation before independent player work and
joins before reading Kit's vertices, uploading or recording its refit. A stack
ticket joins on every intervening early return. Reload, move (both populated
destination and source) and Destroy retire the worker first, including the
device-null path. The worker performs no Vulkan, simulation, input, save or
telemetry-accumulator calls. Only the owner publishes joined metrics and advances
pose keys after successful upload. CharacterSkin worker elapsed overlaps PlayerSkin;
these intervals must not be summed as critical-path time.

Frame-in-flight count, the pre-preparation GPU fence, input age, GPU buffer owners,
animation cadence and shader bank are unchanged. Thread startup/stack/RSS cost is
not measured here. First use creates one worker; subsequent jobs reuse it.

## One bounded desktop comparison

MSVC Release, actual Kit runtime rig plus actual player world/viewmodel CPU
preparation, 30 warmup and 240 sampled changing poses per run. The harness requires
an actual player update each tick. Start-to-join timing includes dispatch and
join, not just worker duration. Hashing/verification occurs outside that timer,
equally in both modes. No samples/outliers were discarded.
The timed player section is `PlayerRenderSlot::PreparePose` plus
`SkinPlayerPoseUniqueTextured` for the viewmodel. Request/shoulder-target setup,
static-PBR publication packing and uploads are excluded; this is not all owner
work or a full `UpdateDynamicInstances` measurement.

| Milliseconds | Opening serial | Worker | Closing serial |
| --- | ---: | ---: | ---: |
| Combined CPU critical-path median | 3.81365 | 2.23565 | 3.77895 |
| Combined p95 | 4.4344 | 2.9343 | 6.3137 |
| Combined maximum | 6.3245 | 4.1134 | 24.7347 |
| Kit pose median | 1.99425 | 2.07515 | 1.99035 |
| Player preparation median | 1.79165 | 1.84245 | 1.76825 |
| Dispatch median (serial column includes Kit work) | 1.99425 | 0.00490 | 1.99035 |
| Remaining join median | 0.00000 | 0.32755 | 0.00000 |

Observed combined median reduction is **40.84% (1.5433 ms)** versus the faster
control; control medians differ by -0.91%. Kit itself is slightly slower on the
worker; overlap provides the component gain. Closing-control tail drift remains
visible. This is a short CPU component experiment without measured CPU clocks,
thermal matching, upload, AS execution, GPU work or present pacing. It is not a
predeclared goal pass or a phone utilization result. No tuning loop followed.

All runs produce the same output digest `9b031ba6f6fb24a9`. The separate actual-rig
contracts perform byte-for-byte vertex and grounded-bounds comparisons across
idle/walk/blend/acting, owned-request mutation, duplicate admission, early return,
failure and stop/reload. **663 checks pass** in Release; Debug CTest passes in
4.95 seconds. The first fixture failure (incorrectly assuming one copy) is retained;
the corrected gate compares actual serial/worker work counts and bytes.

Reproduce with target `horde_rt_actor_pose_worker_tests` in Release:

```powershell
.\build\presets\windows-x64-release\Release\horde_rt_actor_pose_worker_tests.exe
.\build\presets\windows-x64-release\Release\horde_rt_actor_pose_worker_tests.exe serial 240
.\build\presets\windows-x64-release\Release\horde_rt_actor_pose_worker_tests.exe worker 240
.\build\presets\windows-x64-release\Release\horde_rt_actor_pose_worker_tests.exe serial 240
```

## Actual runtime proof and admission limit

Fresh hidden Windows hardware RT captures use the same chapter-lookout checkpoint,
960×540 output/internal dimensions and owner settings. Manifest records grounded
Kit generation 1 with complete Keeper dissolve; a fresh scene requires its pose
before upload. The ON binary contains the worker-only diagnostic marker; OFF does
not. Both honestly presented frames are byte-identical PNGs, SHA-256
`a303359e63eeebedd9cf4b77c3a2ef26a8fd3ea32b34f4ef714fb15328ef79c3`.
The candidate frame was visually inspected. Frozen captures prove output, not
steady skin/frame cost. Debug scene retirement CLI and the complete affected
resource-inventory CTest pass (17.87 seconds), including prior resource contracts.

All native capture processes exited. Foreground/settings stayed unchanged; the
owner campaign path remained absent. Owner settings SHA-256 remains
`ed609b868fd2951698c46bb17b98ffbad647bf30ca7cfacfa5d64f5342fd57c3`.
Candidate/control binaries and raw captures remain in ignored
`reports/actor-worker-20261023/`. A normal default-OFF application rebuild restores
serial behavior after the trial; its final hash/status is recorded separately.

The new test is additive to the existing Vulkan-host CI roster; no gate or timeout
is weakened. Exact-head hosted execution is pending backup approval. Android
worker flag admission/build, dynamic RT continuity, cold startup/memory, comparable
device critical-path and separately bounded warm-play performance remain required
before adopting it. Existing phone baseline and thermal-mismatched field data are
unchanged. Do not substitute the old private shader bank or claim every scheduler
sleep is a GPU fence.

Next feature task is final caption/icon overlap inspection and one justified touch
layout correction, followed by the recorded bounded forest view proposal; do not
stack actor-range jobs, pre-fence redesign or texture-LOD changes into this trial.

Compact raw [samples](desktop-samples.csv), [receipt](receipt.json),
[Debug contracts](debug-contracts.txt), [resource gate](candidate-resource-inventory.txt)
and [retained initial failure](initial-fixture-failure.txt) accompany the source.
