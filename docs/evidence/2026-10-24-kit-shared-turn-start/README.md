# Opt-in shared Kit turn/start prototype

Follow-up: the [single coupled retiming comparison](../2026-10-24-kit-turn-retiming/README.md)
retains a 0.50-second diagnostic candidate with a 2.217-second large departure.
Normal gameplay remains OFF; this original 0.75-second result stays preserved.

24 October 2026. **Implemented for isolated diagnostics; OFF in normal gameplay,
unadmitted.** The actual route-context trial removes the retained departure
alignment defect, but its **3.217-second large-turn delay is too deliberate to
recommend for ordinary following**. Keep pacing/owner review open. No normal
binding, original asset, save schema, tether, route, cruise speed or worker-default
change is admitted by this checkpoint.

This follows the [scoped plan](../../superpowers/plans/2026-10-24-kit-shared-turn-start-scope.md)
and [visual reassessment](../2026-10-24-kit-partial-contact/VISUAL_REASSESSMENT.md).
The earlier measurements, withdrawn recommendations and failed experiments are
retained. There is no new small-angle asset or numerical slip tolerance.

## What changed

`CompanionRouteFixture(true)` opts into Idle / Turning / Starting / Walking.
`GameSimulationConfig::diagnosticCompanionTurnPrototype` is false by default,
including the production profile. No settings/UI/JNI or checkpoint field enables
it. The existing four-slot callers and accepted legacy route remain unchanged.

The shared owner settles into Idle zero, completes a latched signed cycle of at
most 45 degrees, re-evaluates latest intent at the planted endpoint, then ramps
to the existing 1.8 m/s ceiling. Joints and external logical yaw use the same
owned 46-key profile and partial amplitude. Walk phase still comes from actual
accepted travel / 1.56 m; turns never translate the root. Collision/support,
six-metre tether, waypoint/tolerance and accepted bookkeeping rules are retained.
Transient clocks freeze on pause/inactivity/readiness loss; reset and canonical
load discard them. Speech timing and durable dialogue state do not change.

The additive loader accepts at most six named bindings while keeping the old
four-slot overload and combat IDs. TurnLeft/TurnRight occupy new IDs 4/5; combat
holes remain unmapped. The pure snapshot recipe feeds the existing one-pass
blend/skin route, with complete diagnostic motion identity in preparation and
upload keys. The separate Debug inspection actor reuses the original GPU lane.
Original source/runtime/seven clips and owned derivative are byte-identical.

## Actual RT result and review

One existing hidden, nonactivating Windows fixture ran **33.313 seconds**:
**578 checks, zero failures, 138 render calls and 131 controller images**.
It uses actual admitted terrain and sequential shared-controller fixed steps.
The campaign stays paused; proof is an explicit fixture input. This is not a
naturally earned journey, live input/voice test or an engine-FPS measurement.

RTX 5050 Laptop / NVIDIA 610.47, hardware RayTracingPipeline, diagnostic High
pair, actual internal/output **1232x803**, DRS off. Every captured snapshot owns
a unique completed submission, RT dispatch, swapchain copy and successful
presentation. Exact executable/settings, source hashes, pipeline hashes and
per-snapshot identities are in [receipt.json](receipt.json).

- Legacy first-moving heading error: **152.933 degrees** on the same terrain.
  Prototype: **0.000027 degrees**, first step at **0.15 m/s**, after **193 fixed
  steps / 3.216667 seconds**. The initial retained wait faces approximately
  155 degrees away; the familiar 153-degree number is the legacy first-moving
  tick after its capped yaw update. No heading was seeded into this controller.
- Mid-turn pause: root/yaw/gait/motion clocks and two RT images are identical;
  resume advances once. Opposite intent completes the current 20-degree cycle
  before its subsequent opposite 30-degree cycle. Neither translates the root.
- Actual slope travel and the first sharp route corner retain nonzero accepted
  walk phase; corner turn/start begins aligned. All four corners/endpoint also
  pass the bounded host route test. Actual RT corner coverage here is the first
  corner only, with sampled joins rather than a continuous corner movie.
- Speech metadata produces/clears the existing acting layer; no listening is
  claimed. Canonical load/reset is also tested through real `GameSimulation`.
- Clearing the candidate restores a byte-identical original RT image, releases
  the inspection actor before scene retirement, and returns to EntryMenu.

Review the [1x departure](shared-controller-departure-1x.mp4),
[departure sheet](departure-contact-sheet.jpg) and
[context sheet](context-contact-sheet.jpg). Original crops are not relit;
the night view limits fine contact judgment. I inspected the actual timestamped
frames, not an interactive foreground video player or human thumb/comfort test.

At departure times **0.100/0.200s**, the nonzero Idle settles before root yaw.
The turns change facing with visible weight/step motion at **0.400/0.683s**;
the successive planted joins around **0.950/1.700/2.450s** show the repeated
four-shuffle rhythm. No gross sole skid, inverted foot or abrupt joint pop is
identified at this gameplay distance. At **3.217s** Kit is aligned and starts;
**3.350/3.483/3.617s** show the ramp into the existing walk. The heading is
better, but waiting over three seconds is a material responsiveness concern.
That is a timed presentation judgment, not a new contact metric or a claim that
the candidate has passed owner motion acceptance.

The MP4 reconstructs the **60Hz fixed-step timeline**, holding the last actual
RT image until the next sample (normally every two ticks), without motion
interpolation. The resume adds one active tick; that real gap is preserved.
It has 0.5-second lead/end holds: first travel is at video **3.716667s**, total
278 encoded frames / **4.633333s**. Output encoding rate is not measured FPS.
[Timeline](timeline.json) preserves every stage/root/recipe and source tick.

## Checks, custody and remaining gate

Debug builds: shared simulation, native app, intake, route/dialogue/checkpoint
tests and scene-resource inventory passed. Focused CTests: **3/3**, 24.70s total
(route 4.33s, dialogue 2.19s, checkpoints 18.16s), unchanged deadlines. Route fixture
reports 2,332 checks; canonical checkpoint suite 1,058. Optional named binding
check: **60/60**, including nonloop endpoints, unmapped holes, roster bounds and
legacy/raw/PBR Idle/Walk/turn byte parity. Original intake: **7 clips / 35 poses**.
Scene resource-inventory executable exits 0. Previous documentation-head
`1434c356` CI 40059726798 is terminal with all six jobs successful; this new
implementation needs its own exact-head CI result, reported separately.

The 151 parked owner files, original/source/candidate assets, isolated settings,
source autosave/manifest and foreground stayed identical. No campaign writes,
phone, focus activation, new GPU owner, public release or merge occurred.
Worker stays OFF; dynamic worker/phone parity and overall performance are not
established by this serial Debug trial.

Next review: decide whether to compare **one modest coupled retiming of the
existing joint/yaw cycles**, preserving complete foot exchanges, then check the
actual large departure and retained-phase corner. Do not silently adopt the
three-second delay, widen the tether, enable production bindings, add a solver
or start another asset loop. Owner pacing/comfort, fuller moving-target and
corner continuity, dynamic worker/phone cost and natural campaign acceptance
remain open. This checkpoint provides a reproducible opt-in seam, not a release
admission.

Local reproduction in the existing WP2 folder uses the unchanged hidden fixture
workflow and fresh isolated `reports/horde-campaign-fixtures-*` state:

```text
cmake --build --preset windows-x64-debug --target horde_rt_diagnostic_window horde_rt_companion_locomotion_tests horde_rt_kit_source_intake_tests
ctest --test-dir build/presets/windows-x64-debug --build-config Debug -R "^horde_rt_(companion_locomotion|chapter_dialogue|campaign_checkpoint)_tests$" --output-on-failure
horde_rt_kit_source_intake_tests.exe --named-turn-bindings assets/models/characters/source/kit-step-turn-proof-v1/native/kit-turn-proof.glb
HordeLanternRT.exe --lookout-validate-hidden --lookout-inspect-kit-controller --campaign-test-root <fresh canonical isolated ForestEndpoint fixture>
```

The existing 90-second/180-render caps and strict filename/state guards remain.
Full local run/raw PNGs/process custody are retained under
`reports/kit-shared-turn-start-20261024/` and
`reports/horde-campaign-fixtures-kit-shared-controller-20261024/`;
only compact sanitized evidence is backed up.
