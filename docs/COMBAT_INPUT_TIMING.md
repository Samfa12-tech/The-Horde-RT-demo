# Combat input edge timing

`GameSimulation` remains the 60 Hz gameplay authority. Attack, parry, and dodge
publishers increment their existing monotonic counters, then append the edge,
counter sequence, monotonic nanosecond time, and coherent dodge stick direction
to `InputSnapshot::combatEdgeHistory` before publishing that snapshot through
the existing mailbox.

Platforms may pass the matching steady-clock sample as the optional final
argument to `AdvanceFrame`. The first timestamped frame establishes an anchor
and uses the existing legacy edge path. Later complete metadata batches map an
edge's fraction of the raw owner-clock interval onto the runner's accepted
frame contribution and old accumulator. The edge executes at the first fixed
tick boundary at or after that phase. Simulation time is still determined only
by the supplied frame delta and `FixedStepRunner`'s 100 ms cap.

At a shared target tick, existing fixed-step order remains unchanged: dodge is
consumed in the movement update, then attack is considered before parry in the
encounter update. The global edge order is diagnostic provenance and does not
reorder those established rules.

An edge older than the preceding owner sample, newer than the current sample,
or inside a clock-regressed interval is late and runs on the next tick. An edge
in the portion of a raw interval dropped by the 100 ms cap runs on the newest
tick accepted by that frame, or the next tick if no tick is accepted. If an
unconsumed counter sequence is absent from the retained history, or a batch is
malformed or over capacity, it falls back to the existing monotonic-counter
behavior and increments the bounded timing diagnostic. Ring overwrites of
already-consumed records are reported separately and do not cause fallback.
No counter delta is expanded into an unbounded loop.

Pause, death, world reset/retry, lifecycle synchronization, and timing reset
discard scheduled combat edges and advance the consumed sequence floor so a
stale input cannot reappear after resume. Direct `StepFixed` and callers that
omit the owner timestamp retain the existing deterministic counter behavior.
The immutable snapshot exposes a bounded edge-to-target/actual-tick trace; a
successful parry links its source parry sequence to the semantic event sequence
and tick. This trace is diagnostic provenance, not a substitute for rendered
contact, device input-latency, or owner-feel evidence.

The Windows and Android publishers use the same native steady clock as the
owner interval. Android timestamps JNI receipt, not the hardware touch event;
Java dispatch delay remains part of the unmeasured physical input latency.
Benchmark and deterministic capture paths retain their recorded timing policy.

Debug builds emit at most 128 accepted-present diagnostic rows per renderer
session. These join the command, native receipt, publication, target/consumed
tick, shared sword matrix, semantic feedback and actual submitted frame identity.
The recorded pose tick must match that frame. Queue-present return is observed;
display scan-out is explicitly unmeasured. The shared matrix is not labelled as
the final skinned blade. Release builds omit this logging path.

Current host validation (2026-10-07): the Windows Debug app builds and all seven
affected native suites pass, including timing, gameplay, mailbox, trace reporting,
held-item transition, clearance and final rig sockets. The timing suite covers
15/30/60/120 FPS, hitches, late input and a late catch-up parry that correctly
misses. Logs are retained in the task's external evidence directory as
`post-reset-foundation-native-build-20261007-05/06.log` and
`post-reset-foundation-native-tests-20261007-03.log`. Earlier stack, pose and
compile failures remain recorded; the duplicate trace ring and stale nominal
pose fixture were corrected. Android native packaging, real input/present traces,
rendered blade-contact calibration and owner feel remain separate open gates.
