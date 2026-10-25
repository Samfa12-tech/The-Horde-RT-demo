# Held guard and guard-to-attack — development checkpoint

Implements Sam's25October approval in the existing shared1.7 combat profile.
Future stabs, directional-dodge combinations and parry-break progression are not
part of this change. No shader, geometry, speed, tether, maximum health, volume,
personal preference or rejected asset experiment is changed.

## Behavior

Fresh press retains the existing40msstartup/220msactive parry window and successful
parry/stagger/riposte benefit. Continued hold enters Guard after that window;
repeating held publications/edges cannot reopen it. The current blockable frontal
skeleton contact loses half a heart; rear contacts and the Keeper's red lightning
retain full damage. Dodge and existing hit immunity remain authoritative. This
is a trial balance, not an assertion that combat is finished.

Attack starts directly from Guard and ends mitigation on that tick. Original
attack/contact/recovery and existing continuation timings are preserved. Holding
guard through legitimate recovery returns to Guard without a fresh parry window;
releasing during the swing prevents that return. Pause/lifecycle, dodge, stow,
reset and death require release before rearming. Campaign restore also waits for
a fresh neutral publication with guard released.

The existing raised end-active-parry pose (normalized0.52) supplies guard. A shared
pose weight blends hand/arm and rigid sword into the actual attack curve over
0.12seconds, then toward guard during the legitimate recovery tail if still held.
Visual guard pose never grants mitigation during an attack/recovery. Zero weight
preserves original pose outputs. Left light ownership and successful-parry jolt
are unchanged; no renderer-only correction or new clip is added.

Android aggregates touch, Q and controller LT, publishes a fresh edge atomically
with held state, and releases/cancels through existing ownership/lifecycle rules.
Windows aggregates existing Q/LT; reserved right mouse remains unchanged. The
shield icon stays. Existing teaching explains fresh parry, held half damage,
release/repress and red-attack dodge. Physical multi-thumb comfort remains open.

## Exact health and saves

Health uses exact integer half units, retaining `vitality` as ceil hearts and
`halfHeart` as the missing half of the highest displayed heart. `3,true` means2.5;
six guarded hits kill, not zero damage from rounding. Existing invulnerability
and lethal event ordering remain. Both platform HUDs display half hearts and
accurate accessibility text; maximum health stays3.

Schema1's142-byte payload/golden bytes remain unchanged. Schema2 appends one
canonical boolean (143bytes), with strict schema/length validation and rejection
of unknown newer schemas. Legacy loads clear stale fraction state; there is no
migration-on-read or owner-file rewrite. Capture/restore, boundary equality and
independent slots retain the exact fraction. Flask code is not included.

## Checks and retained failures

- Focused vitals, codec, campaign restore, pose and direct combat checks pass.
  The new pause test first caught stale published Guard after internal
  cancellation; publication is corrected, and all710foundation checks pass.
- Windows controller/teaching correction selection passes3/3. Existing coherent
  mailbox stress includes held guard:125,000publications/four readers, unchanged
 30seconddeadline, passes in3.22seconds.
- Android focused selection passes115tests. Initial touch failures were an
  unresumed Robolectric fixture, correctly rejected by the new lifecycle gate;
  setup now explicitly models resumed/focused play and adds rejection coverage.
  Original count/cancellation assertions and first failed XML are retained.
- **Local storage execution remains blocked:** even the unchanged schema1
  create-only write fails `SetFileInformationByHandle(FileRenameInfoEx)` with
  `PublishFailed`/Win32ERROR_ACCESS_DENIED(5), after preparation and unchanged
  target readback. The target is disposable
  `<system temp>/horde-campaign-storage-test-<suffix>/slot-a.sav`; its full random
  path was not logged. Storage implementation has no diff. Half-slot failures
  also occurred, but their individual native codes were not logged. No retry,
  alternate temp path, permission change or altered publication semantics was
  used. Hosted Windows acceptance must be read separately at this exact head.

## Actual hidden Windows RT sequence

The existing supported frozen capture path owns manual60Hzinputs; strict before/
after snapshot, successful present/completion, dedicated primary-arm pixels,
fixed position and unchanged vitality checks remain. No foreground activation.

First invocation selected `player-body-forward`, a legacy comparison route, and
was rejected before frames because its mount profile was LegacyViewRelative.
The corrected existing production `player-viewmodel-forward` route passes:
202frames,60images, exactly one parry window, original draw/attack, guard attack
through full recovery, held return and release. Settings, owner campaign and
foreground custody were unchanged;151parked owner-file hashes are exact.

Representative [held guard](guard-held.png), [attack entry](guard-attack.png) and
[held return](guard-return.png) show coordinated sword/hand poses. The blade can
extend above the view as in the existing raised parry; positive arm pixels alone
are not full-blade framing or wall/ceiling clearance acceptance. Torch only;
lantern/body/reflection/shadow breadth, physical feel and interruptions still
need their relevant actual-play checks.

[Silent sequence](guard-real-speed.mp4) uses actual `motorTick/60` sample durations,
including irregular action-edge captures. It is authored simulation-speed video
of unpaced RT frames, not realtime wall performance or measured scanoutFPS.
The raw completed-frame stream/60PNGs remain in ignored
`reports/held-guard-20261025/guard-rt-r2/`; compact index, receipts and original
failure evidence are retained here. No natural journey or phone acceptance is
claimed from this fixture.

Next: verify exact-head CI and the built Android package, then a bounded device
guard/input check with custody/restore. Owner balance/two-thumb feel and natural
earned journey remain explicit. Continue the approved flask after this coherent
checkpoint; pre-flask schema2 must preserve half health and receive the same
approved empty base flask as other older records, never an automatic refill.
