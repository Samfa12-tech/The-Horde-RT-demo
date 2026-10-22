# Automatic crouch and analogue movement checkpoint

Source continuation of `026bceda8924cff7356337ae14b83cc1cbaf0352` in the existing
`.worktrees/horde-1.7-wp2-vertical-proof`, branch
`codex/horde-1.7-wp2-vertical-proof`. Desktop-only work after the owner's phone
disconnect; no device query, install, capture, reconnect or owner-save access.

Ordinary movement now requests the existing crouch transition when a standing
body sweep fails but the full crouched body can pass. Movement stays blocked
until the actual transitioning body fits. Standing resumes only with full head
clearance. The same imported terrain, raised step support, authored roofs,
readiness and side-collision checks retain authority. Detection uses existing
collision results on the normal clear standing path; additional height sweeps
are limited to blocked movement or an already lowered body. Dodge, knockback,
frozen captures and authoritative poses do not trigger automatic entry.

Touch/controller deflection now scales to the existing 3.2 m/s run cap, with no
Run latch. Keyboard WASD retains walking, Shift and the existing Caps Lock run
toggle. Windows gives held movement keys priority over controller axes and uses
the same arbitration for directional Dodge edges. Android publishes axes and
analogue mode in the same locked mailbox update. Ordinary manual Crouch/Run
buttons and controller stance bindings are inactive/hidden; help describes the
new behavior. The low-level stance API remains for deterministic rig fixtures.
Historical motion/capture/benchmark input explicitly clears analogue mode.

## Verified locally

- Seven selected Release CTests pass: chapter step movement, shared gameplay,
  motion scenarios, waterfall production route, mailbox stress, crouch rig and
  desktop controls. See [selection/result](host-ctest.txt) and
  [native output](host-test-output.txt). The final desktop source-binding checks
  were rebuilt and run separately in [desktop-final.txt](desktop-final.txt).
- 73 chapter-step checks use actual imported terrain plus disposable test-only
  roof/root geometry. Forward entry takes 55 ticks with two blocked transition
  ticks; forward exit 75 ticks with none; backward and strafe crossings take 172
  and 152 ticks, each with two blocked ticks. The tests bound each posture change
  to 40 mm/tick, check head/roof clearance and raised foot support, hold safe posture
  while stationary beneath the roof, and stand after exit. Solid walls and roofs
  below full crouch height reject movement without requesting crouch.
- Existing save rejection while lowered, pause freeze and load/neutral-release
  checks pass. No save schema or healing policy changes. This is a restored
  post-Keeper fixture, not a natural victory/full-journey acceptance run.
- Analogue tests cover 25/50/100% deflection, diagonal cap, centred stop and
  recovery after attack/Dodge without recentering. Legacy keyboard timing remains
  asserted. The 125,000-publication/four-reader stress test now includes the new
  mode bit; its original 30-second deadline is retained.
- Existing 288 native CPU crouch-rig samples pass. This does not certify rendered
  animation or GPU cost. The existing waterfall production-route assertions pass
  unchanged; the former no-manual-crouch approach blocker is closed locally.
- 97 Android unit/Robolectric tests across eight affected classes pass, zero
  failures/errors/skips; [suite counts](android-tests.json). These cover input,
  controller/touch handoff, retired controls, menu, death and surface lifecycle.
- Windows Release application compiles/links, including the earlier outdoor
  speech-domain change. Android Debug compiles/packages all four configured ABIs:
  [build result](android-build-tail.txt). Exact source-file and artifact hashes are
  in [receipt.json](receipt.json). The APK was not installed.

The first new entry assertion fails against the previous implementation:
[retained red result](red-step.txt). An initial candidate assertion also fails
because its arbitrary 40-tick boundary ends 1 mm short despite safe progress and
200 mm lowering: [retained diagnosis](entry-diagnosis.txt). It was replaced with
complete bounded crossings and per-tick clearance/support assertions. Existing
production route assertions and timeouts were not weakened.

## Remaining acceptance and next slice

No physical input, native RT image/animation inspection or new performance claim
is made here. The fixture's two-tick entry pause and stand/turn feel need actual
play acceptance. Final portrait/landscape captions and touch layout remain
unaccepted on the phone; cold 207 missing-terrain remains open. The last installed
phone APK is the previous UI candidate, not this source checkpoint.

Audio/haptic manual revalidation required: **YES**, limited to movement-driven
footstep cadence at the new analogue speeds; no cue, mix or haptic changes here.
Kit listening remains a separate prior acceptance gap.

Next useful desktop slice: split the existing skin-stage wall timer into actual
animation, vertex-copy/skin and upload/AS work, retain wait/submit separation, then
measure one shared-code candidate at matched settings. The phone's current
12.06 ms skin-stage median is a target for investigation, not Windows evidence or
CPU utilisation. Prepare an exact-build comparison for the next owner-confirmed
phone connection. Keep the closing 33% slowdown, 20% quality gate, Debug route
timeout, PR conflicts and pending recovery policy explicit.
