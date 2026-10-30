# Backward movement: first-person and action continuity

Accepted for the bounded Windows checks below at runtime `90a65044`. No new
runtime, rig, collision, camera, save or renderer change was required. The only
implementation delta is additive regression coverage in
`tests/PlayerDodgeLifecycleTests.cpp`; existing assertions and time limits remain.

## What was exercised

- Native RT: a fresh current-rules simulation at the portal, ordinary fixed motor
  input, 97 rendered simulation ticks and 18 captured owning frames. Sequence:
  idle, backward, forward, backward, backward dodge, backward, idle. The fresh
  simulation replaces legacy checkpoint staging; no authoritative player pose,
  campaign files or completion flags drive this movement.
- Every captured frame has real `RayTracingPipeline` dispatch, swapchain copy,
  matching simulation tick, completed submission and successful presentation.
  Dedicated primary ownership and the existing visible-arm gate pass 18/18, with
  no outside-frame/ceiling exception. Primary player pixels range 511–6,232.
- Inspected native images 0, 4, 9–14 and 17: torch remains visible through reverse
  travel, accepted dodge weight 1, recovery and idle. The hand reaches the lower
  edge during the low recovery pose (image 13, 511 pixels); it does not disappear.
  This is bounded framing evidence, not a claim of improved motion quality.
- Host lifecycle: real backward landing travel before existing lower/upper
  campaign restore fixtures; accepted dodge into actual rope ascent/descent;
  continuous correction through rope ownership/release; prepare/reject/cancel/load,
  safe-side retry, zero-tick pause and route reset. 55 checks, zero failures.
  The existing legacy clock may advance during rope ownership; its reverse
  correction is retained without an extra phase reset. Fresh load/reset clears
  the transient phase, preserving the save schema.

Five focused Debug CTest gates pass: directional rig, dodge rig, lifecycle,
animation and dodge presentation (35.11 s total). The accepted app was rebuilt
after removing the temporary probe; `DiagnosticWindow.cpp` is restored byte-exact
to SHA-256 `f86f3c6f7e0bec115416cd84298b33b1861ba3e7b82d195e3580cbee3c1dc9eb`.
Settings remain byte-exact and the owner's campaign-v1 directory remains absent.

## Reproduction and limits

`native-probe.inl` records the local-only hook, and `capture.ps1` records its
hidden bounded launch, exact executable/source hashes and arguments. It was
temporarily included immediately before the capture settling loop, after
checkpoint staging, then removed. The nonempty development checkpoint freezes
platform simulation updates while each explicit current-rules motor tick is
rendered; the probe rejects renderer mutation of the published pose.
`frames.json` retains completed RT evidence and PNG hashes; `frames.csv` is its
compact motion index. Raw PNGs remain under ignored
`reports/backward-first-person-20261023/native-motion/` and are not Git assets.
The laptop uses the same RTX 5050 Laptop / Debug / 960×540 / 100% configuration
as the preceding static portal proof: Higher shadows, High fire, Mist On, Dust
High. This unpaced fixed-tick capture is not an FPS or performance measurement.

Rope evidence is CPU motor/actual terrain evidence, not a native RT rope movie or
an earned journey. Natural Keeper victory, hostile death, physical controls,
phone framing/feel, sustained performance and authored backward/run/strafe clips
remain unaccepted. Residual backward foot sliding from the prior checkpoint is
not changed or hidden.

The prior full Debug result remains 226/230 (two route deadlines and two unchanged
shader-hash gates). Exact source-changing head `90a65044` hosted run
[39884452538](https://github.com/Samfa12-tech/The-Horde-RT-demo/actions/runs/39884452538)
completed **cancelled**: five jobs pass; MSVC hits its unchanged 15-minute cap
after build success, with no surfaced final test total and six later offline
checks skipped. Neither local focused success nor this run closes hosted CI.

Next: keep this motion checkpoint backed up, then verify the owner-supplied Drive
music originals and prepare the separately approved outdoor PCM integration.
No phone operations, base integration, merge or release were performed.
