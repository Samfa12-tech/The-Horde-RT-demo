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


## First physical guard attempt — held pose not accepted

The compatible APK above was temporarily installed on SM-S948B with fresh original
APK/save/settings/rotation custody. A fresh isolated namespace entered through
normal Enter the Ruin, without a checkpoint or completion flags. One observed
Swing press drew the sword; a stationary2.5second shield touch and release were
captured. The shield visibly pressed, but the [held capture](phone-held-unverified.png)
and [release](phone-release-unverified.png) did not establish the raised held pose.
This is retained as unverified evidence, not a device pass. Legacy constructor
rules were checked and do not explain it; timestamped input consumption versus
continuous held intent is now being tested separately. No speculative runtime
change or repeated phone sequence is justified by images alone.

Both original APKs,18owner save/lock files, preferences and system settings were
verified restored, zero new save paths and no renderer/transfer remaining. The
receipt records exact release time; no phone calls followed release. Two-thumb
attack/guard, damage/balance, earned progression and broader lifecycle remain open.

Full Android testing additionally exposed two genuine overlong visible-label
layouts (453tests,2failed,6pre-existing skips). Compact PARRY text is restored;
full semantics stay in accessibility/teaching. Both unchanged layout suites now
pass. Original failed XML and full-roster failure remain retained locally; the
corrected full roster and new exact-head CI still need completion.


## Timestamped input correction and complete Android roster

The real phone attempt remains unverified; its exact cause is not asserted from
images. A separate shared regression reproduced a real timestamped-input defect:
continuous held intent can arrive on catch-up tick 1 while its85ms parry command
correctly targets tick 6. The old fresh-held token expired on an earlier tick,
so telemetry reported the command consumed without an actual parry action.

The token now survives until that command is consumed, not until the next
unrelated fixed step. Both accepted and busy-rejected commands consume it; release,
interruptions and reset still clear it. Corrected tests preserve targetTick=actualTick
and prove guard entry, no repeated held windows and release/repress rearming.
A second real timestamped attack case rejects the busy press at tick7, retains
full recovery, then rejects three further held commands without a fresh window.
Timing, foundation and direct combat pass3/3; the final busy-case target passes.
Original failing-first receipt is retained, not weakened or hidden.

The full Android roster now passes453tests with the same6pre-existing skips.
The compact visible label fixes both hosted layout failures. One additional stale
accessibility assertion was updated to require the new dedicated guard description;
no layout bounds, line counts, input assertions or test roster were removed.
Exact corrected-head CI remains separate. Previous6c5e2fb9CI had five successes
(including hosted Windows native tests) and the two known Android layout failures;
this does not retroactively validate its timestamped input behavior.

A corrected APK must be built from the new source before a phone retest. The
first phone APK/receipt stays pinned to the earlier checkpoint; do not relabel it.

## Corrected build and second allocated phone attempt

The corrected frozen APK was built from `69e46426`, with assemble/lint passing
and the complete Java roster passing (453 tests, six existing skips). Its exact
[artifact receipt](apk-timing-correction-receipt.json) describes the build-time
state before installation. All six jobs in
[exact-head CI 40171242117](https://github.com/Samfa12-tech/The-Horde-RT-demo/actions/runs/40171242117)
subsequently completed successfully, including Android and MSVC. This supersedes
the pending build/CI statements above; it does not close physical guard acceptance.

A second allocated device attempt installed that exact compatible APK, entered
the normal isolated route and observed gameplay readiness. It **did not issue
Swing or guard input**: an initial startup focus observation race and a duplicate
receipt filename consumed the action budget. The 267-second safety check then
stopped input rather than rush the test. Original failures remain under ignored
`reports/guard-phone-69e46426-r2/phone/`; no staged checkpoint or progression flags
were used. This is setup/custody evidence, not a guard retest pass.

Restoration completed **07:28:34 UTC**: both original installed APK hashes,
18 prior save/lock files, preference bytes and system settings verified exact;
zero added save paths, no owned renderer or transfer. Horde released its device
reservation. No new phone access is authorised until another slot is allocated.
Before requesting it, the host driver needs an offline startup/receipt dry run
and a realistic setup/test/restore budget.

## Clearer devlog animation preview

The source-pinned `11c13659` hidden Windows RT capture produced an 11.55-second,
960x540 silent MP4, now delivered through Library as
`libfile_1d733e64d39081919fa7c8a70124ab57`. The compact
[video receipt](devlog-video-receipt.json) retains its exact source/executable,
hash, timing, Library identity and limitations; the video and raw frames remain
outside Git. Its brighter existing skylight view shows one fresh parry, sustained
guard, attack directly from guard, held return and release. Encoded key frames
were inspected for the action order and readable hand/weapon presentation.

This is explicitly labelled **RT animation preview / simulation-timed / silent**.
The 354 actual RT images retain their original 60Hz simulation timestamps, with
no interpolation, artificial speedup or repeated padding. Rendering was unpaced
and took 105.9 wall seconds; it is not an 11.55-second real-time recording or FPS
claim. Incoming damage is disabled in this staged diagnostic encounter. Lighting,
materials, exposure, projection and the production guard pose are unchanged;
the raised blade tip can naturally leave the original first-person frame.

Save/settings/foreground and the 151 parked owner files passed the capture's
unchanged-custody check. No phone or X posting was used. Library upload succeeded;
the unchanged official local metadata helper failed because native Windows Python
lacks `os.setxattr`. The returned Library identity is retained in the receipt,
but local extended attributes were not applied or bypassed.

## Latest phone candidate: partial acceptance, restored and released

Frozen source `6dbc43fd` built a four-ABI debug APK with469 passing Android tests
(six existing skips), assembly/lint success, the expected compatible certificate,
and no new runtime asset or source/save entries. The exact
[phone receipt](phone-6dbc43fd-receipt.json) pins its hash and scope.

The first finite attempt reached the actual PLAY menu, while the old harness
waited for ENTER THE RUIN; no gameplay input was issued and originals were
restored. The failure remains in ignored `reports/guard-phone-6dbc43fd-r1/`.
One corrected attempt used observed PLAY and Begin in Slot1 in a fresh supported
isolated namespace, with no seeded completion flags or checkpoint. It completed
in90.812seconds. Actual RT images show the sword raised during a shield touch
hold and returning to its original rest pose after release. The new pause icon
is visible at its portrait anchor. Raw full-resolution captures and input logs
remain under `reports/guard-phone-6dbc43fd-r2/phone/`.

The additional mixed keyboard/touch trial does **not** establish held return:
Android's saved input log has key-up events before Swing, despite the intended
duration argument and a still-running shell child. Its return image is Idle.
This is an unadmitted held-input witness, not proof of a production regression
or a two-thumb test. No multipointer injector was introduced, and screenshot
completion is not used as acceptance. Damage/balance, true simultaneous touch,
landscape and owner comfort remain open; no performance claim is made.

At **09:29:31.855UTC** both original installed APK hashes,18 save/lock files,
preference bytes and system settings were verified restored; no added save paths,
owned renderer or transfer remained. Horde released the phone. The candidate was
not left installed or described as ready for recording. Spin Vector's subsequent
slot is respected; later menu wording work is offline and not part of this APK.
