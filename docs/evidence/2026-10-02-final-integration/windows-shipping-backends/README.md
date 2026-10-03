# Bounded Windows Shipping backend completion

Previous goal turn: PROGRESS (Windows focus repair and owner/CI acceptance).
Reuse the original integration matrix and accepted player/audio work. This is
its unfinished Windows runtime continuation, not a repeated Host audit.

## Current candidate and validity problem

Parent source `3281bce66eca18651acc75e5de1d3ce28d214dea`, plus a Windows-only
cancellation-observation patch. No cancellation predicate, workload, shader,
simulation, geometry, audio or quality change. All existing UI cancellation
call sites require an explicit trigger; staging/render/recreation failures also
write one bounded stderr observation, never per-frame output or foreign-window
identity. Initial build caught an incorrect snapshot member name; corrected to
`tickIndex` before either candidate build/test passed.

Fresh actual Windows Debug/Shipping builds PASS, selected benchmark/evidence/
launch/lantern/music-focus contracts6/6 each: Debug0.33s, Release0.14s.
Candidate Shipping SHA256
`173e1875899b53752c9fc5b8f490ade35f3fa3b307529204a73f73c1c5c4eb7e`.
Stage: `C:/Dev/tmp/horde-windows-shipping-backends-20261002`, unchanged accepted
world46a88dac/vieweaa0db3a assets and canonical music. Fresh defaults are High,
100% RT scale. Actual output identity/extent must be checked, not assumed.

The older218ab102 route cancellation has0 measured frames and no trigger.
One instrumented attempt is justified to distinguish lifecycle/startup failure
from completed RT performance. If cancelled, retain it and act only on that
demonstrated trigger; do not repeat an unchanged artifact without a validity fix.

## Finite run matrix — complete

| Backend | Workload | Status |
| --- | --- | --- |
| RayTracingPipeline |showcase-route-v1 |run02 PASS: exit0,1838/1838 completed/expected/valid GPU rows,2/2 laps and26 waypoints. run01 remains retained with unrecorded exit. |
| RayQueryCompute, explicitly required |showcase-route-v1 |run03 PASS: exit0,1838/1838 completed/expected/valid GPU rows,2/2 laps and26 waypoints. |
| RayTracingPipeline |lantern-reveal-sequence-v1 |run04 PASS: exit0,600/600 completed/expected/valid GPU rows; live sequence complete. |
| RayQueryCompute, explicitly required |lantern-reveal-sequence-v1 |run05 PASS: exit0,600/600 completed/expected/valid GPU rows; live sequence complete. |

For each row require actual backend, honest swapchain presentation, complete
expected/fence-owned GPU rows, exported reports and clean exit. These are runtime
checks/short workload timings, not matched phone performance, unchanged-tolerance
image parity or owner live visual acceptance. Do not close High physical edges.

run01 PID5744 completes without cancellation stderr on the exact173e1875 binary.
It does not demonstrate the older cancellation's cause, and no cancellation
predicate is changed. Its report retains long acquire/outer-cycle tails; median
alone does not certify frame pacing. The launch observer did not retain the
child's exit code. run02 repeats only that invalidated receipt with a live parent
process handle and explicit exit capture; run01 remains retained, not discarded.

All four required rows report `result/status=complete`, `workloadComplete=true`,
`presentedEveryFrame=true`, complete frame evidence, and zero rejected/cancelled/
outstanding/error/missing rows. Each stderr is empty. Reveal correctly does not
claim route traversal: it completes the separate live sequence. Exact backend is
`executionBackend`, not the coarse `rtMode` field, which reports
RayTracingPipeline even for the explicitly required hardware RayQueryCompute.
No cancellation reproduced; the earlier0-frame failure's cause remains unknown.
No change to cancellation behavior is justified by these results.

[Exit receipt](process-exits.txt) records the live parent-handle observation,
not an exit inferred from a report or process disappearance. Raw JSON/text/stdout/
stderr/capability receipts are retained in each named run folder. Capability
snapshots written at first present may have pending timestamps; the completed
benchmark report has the authoritative full-denominator final evidence.

## Exact RTX identity and timing limits

All four: NVIDIA GeForce RTX5050 Laptop GPU, Vulkan1.4.341, MAILBOX,
1232x803 internal/presentation,100%, High Shipping, RGBA8 raw fallback plus raw
RGBA8 KTX2 lich. Pipeline module hashes remain66e39df9/79e80a48; compute modules
5002afa7/f0e10d5a. Full identities are in the raw reports. Accepted cuff/music
assets are unchanged. These are sequential short fixed-step harness runs, not
matched A/B or sustained foreground gameplay.

| Run | CPU cycle median / p95 (ms) | GPU median / p95 (ms) | CPU cycle slowest1% mean (ms) | Acquire slowest1% mean (ms) |
| --- | --- | --- | --- | --- |
| run02 Pipeline route |6.4623 /8.7898 |2.8073 /4.4985 |243.6858 |233.8844 |
| run03 Compute route |6.5287 /8.2120 |2.8267 /4.2962 |37.1724 |25.3565 |
| run04 Pipeline reveal |7.8181 /10.2018 |4.1393 /6.3904 |410.0352 |401.1546 |
| run05 Compute reveal |8.4359 /9.6844 |4.5124 /4.6093 |12.6032 |0.0738 |

The retained [audio-gate log](background-audio-gates.log) observes
`foreground=0`, with the accepted music policy suspended; do not relabel this as
full foreground gameplay/audio acceptance. Acquire/cycle tails are aggregate
summaries, not aligned per-frame attribution or a proved OS/driver cause.
Per-frame evidence retains identities/GPU duration, not individual CPU-stage
samples. FPS values are inverse-duration proxies, not achieved presentation
cadence. No performance gain, pacing acceptance, phone evidence, image
equivalence or High physical-defect closure follows from these functional passes.

Next unfinished Windows gate: foreground/live visual acceptance of the explicitly
required compute backend on this artifact. If pacing is investigated, require a
specific foreground/matched validity improvement; do not repeat this completed
matrix or add a general telemetry framework. Exact-device final-candidate gates
and the recorded High physical limitations remain open.
Audio/haptic manual revalidation required:NO; cancellation observations do not
alter accepted feedback/playback. No phone use, merge, signing, release or publication.

Review note: the five literal emitted text reports contain `Run ID: ` with an
empty ID and trailing space. The full staged whitespace check reports those
five raw-output lines; they are preserved as evidence, not silently reformatted
or hidden by a whitespace suppression. The separate authored C++/Markdown/exit
receipt check passes. Benchmark identity also includes its unique filename and
timestamp; no nonempty run ID is invented.
