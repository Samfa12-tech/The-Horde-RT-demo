# Android touch follow-up, 8 October 2026

This is Debug physical-device input evidence, separate from owner touch feel,
visible blade contact, the earlier completed-frame route captures and sustained
performance. It adds a bounded observer and fixes a newly reproduced missing
Dodge control. It does not change combat, vitality, death or graphics defaults.

## Exact inspection artifacts and build checks

Runtime `7368e2c72099c47548076981f313f832de4e83f0`, tree
`56c1e8b0a1dfdb5e0397287532506ab571301ae0`:

- Four-ABI Debug APK: 138,462,724 bytes, SHA-256
  `c04d0f12b4fa507f5171b16355f21fa646f99b3d1e043b7fb249b0e7fc11691c`.
  Actual installed base hash matches. Closed asset admission, packaged
  orientation/configuration and 16 KiB alignment pass.
- Unsigned Release APK: 120,007,832 bytes, SHA-256
  `a5bcacb8cb94334ac542c00be034ce3d216e4755db17604761f1b34f9922db9b`.
  Closed asset admission passes. All four actual native payloads exclude
  `input-presentation` and `combat-timing` markers and match the preceding
  `09ef0500` unsigned Release native payloads. No production signing or installation.
- Unchanged Windows executable SHA-256
  `c639e713084437f4c4bdb3d481f7bbf5e69284ff6dff5a30a000065923f0cdb3`;
  the Dodge fix changes Android Java only.
- All 177 Android unit tests in 30 classes pass; Debug/unsigned Release builds
  and lint pass. The new regression initially failed with
  `dodgeButton must recover when native becomes ready expected:<0> but was:<8>`;
  the other four touch tests passed in that initial five-test run.
- Source `7368e2c7` passes all 12 aggregate CI jobs in push
  `37655935915` / PR `37655958874`. The preceding observer/test/documentation
  source `09ef0500` also passes all 12 in `37654324073` / `37654335156`.
  Documentation after either seal has separate CI.

The immutable packages and private admission/stop/analysis receipts are retained
under `task-4/integrated-162-7368e2c7-20261007`. The directory's historical date
suffix is not the collection date. These are inspection artifacts, not a final
release candidate or an independently completed Eric audit.

## Reproduced startup visibility defect

Closing the menu while the native surface is still warming hides all three
action buttons. The ready-state poll restored Swing and Parry but omitted Dodge,
leaving Dodge absent after ordinary Debug Enter-the-ruin navigation. The actual
pre-fix hierarchy has Menu, Swing and Parry, with no visible Dodge; the private
injector correctly refused to target the missing control.

The ready-state poll now restores Dodge in the same existing alive/HUD/gameplay
branch as Swing and Parry. A regression exercises warming-to-ready twice,
representing initial Play and surface recovery. Existing press-down action,
cancelled-release, menu and lifecycle tests remain passing. This does not change
input eligibility, accepted Dodge distance/duration, or the underlying mechanics.

## Observer and input method

`decb2194` adds the shared snapshot's consumed movement axes and a separate
256-row input observer, enabled only by the existing explicit Debug RT Lab
inspection admission. Changed publications and idle observations at intervals
of at least 250 ms carry actual consumed action counters, pose tick, position,
look, pause/Dodge state and the owning RT record/submission identity. Failed,
stale, repeated or mismatched presentation identities do not emit rows.
Movement cannot consume the existing combat trace's separate quota.

These rows bind the simulation snapshot to a successful queue-present call.
They do **not** certify fence completion, scanout, displayed FPS or touch latency;
`displayTimeMeasured` remains false. The affected observer/combat trace and
simulation timing/gameplay host tests pass. Release native payloads exclude
the log markers.

The private shell-only UiAutomation helper sends actual multi-pointer MotionEvents
through Android's UI dispatch, with fresh owned-package/window/bounds guards,
bounded event/gesture counts and explicit release or whole-gesture Cancel.
It preserves accessibility services and removes only its nonce-owned temporary
files. It is not shipped in the APK. This is synthetic input on a physical phone,
not a person's manual comfort test.

Allocated model: SM-S948B, Android 16. Portrait output 1440 x 2980; actual traced
extent 720 x 1490. Font scale 1.7, density 560 and portrait rotation settings are
unchanged. The saved custom profile remains 50%, Mobile water/fire, Glass On,
Current shadows, cap30, Mist On. Fresh/reset Glass Off remains authoritative.
Two ordinary three-second forward gestures leave the original opening encounter
before stationary checks; no gameplay checkpoint or damage-rule override is used.

## Accepted diagonal gesture matrix

Each backend admits 16 cases: both movement-first and look-first orders for
plain Cancel, Swing/Parry/Dodge press and normal release, all three actions held
through whole-gesture Cancel, and Menu/Resume with the two movement/look pointers
held. Native initialization identifies Pipeline as backend 1 and hardware
RayQueryCompute as backend 2; the later Pipeline batch and every Compute batch
also retain current capability reports with the expected backend, presented RT
scene and 720 x 1490 trace extent.

| Batch, four cases each | Pipeline run suffix / input rows | Compute run suffix / input rows |
| --- | --- | --- |
| Plain Cancel and Dodge release, both orders | `20261007170534-aa4522624636` / 166 | `20261007171439-e6f05a5a6f3d` / 170 |
| Swing and Parry release, both orders | `20261007170816-8fc7cedf8428` / 172 | `20261007171532-f7a0508d9050` / 176 |
| Swing and Parry held through Cancel, both orders | `20261007171024-0af8c6857d23` / 170 | `20261007171626-d65639fa33bb` / 171 |
| Dodge held through Cancel and Menu/Resume, both orders | `20261007171302-f85c1c0734ec` / 177 | `20261007171718-df7af97f5961` / 176 |

Read-only analysis admits 685 Pipeline and 693 Compute rows with ordered current
PID/tick/publication/RT identities. Every case observes simultaneous nonzero
forward/strafe input and changed yaw; pitch changes or reaches the existing
0.28-radian clamp. After release/Cancel, both axes are zero and position/look are
stationary in the later idle interval. Each selected action is consumed exactly
once during press-down, before release/Cancel, with no later duplicate.
Menu cases observe paused zero axes, then unpaused stationary return with no
leaked action, followed by new gestures. An already accepted Dodge is allowed
to complete before the stationary interval; cancellation does not rewrite its
existing gameplay rule.

Every accepted run stops its owned app and preserves every preference entry.
An earlier additional Pipeline four-case axial startup check also passes,
with 165 rows; it is separate from the 32-case diagonal matrix above.
These checks establish routing/consumption and cancellation, not successful
combat hits/parries, owner perceived latency or sustained rendering performance.

## Held-touch Home return

The corrected Pipeline run `20261007174555-0c2237b27346` and Compute run
`20261007175153-805a99777db9` add four cases each, with 199/201 accepted input
observations: Home with movement-first, a fresh diagonal
move/look/Parry gesture, Home with look-first, and a fresh diagonal gesture with
Parry held through Cancel. Both Home cases retain the same Activity instance and
PID; the actual RT scene epochs are 2 to 4 and 4 to 6 on each backend. Before any
synthetic pointer cleanup, sampled later native idle intervals have zero axes, stationary
position/look and no consumed-action leak. Subsequent fresh gestures are
accepted and consume Parry once during press-down. Installed package identity,
backend, stopped-app and unchanged-preferences checks pass. The Compute run
retains initial, both returned and final actual `RayQueryCompute` capability
reports; the Pipeline run's reports and initialization remain backend 1.
These eight additional cases are separate from the 32-case matrix above.
The sampled stationary spans are approximately 1.6–1.9 seconds; they do not
continuously certify every instant of the surrounding wait.

Home leaves the private injector's two-pointer stream held. The later test
receipt records its original `downTime`, IDs and coordinates; after the retained
post-Home zero-input observations, the helper sends exactly one owned-window
Cancel to finish that synthetic stream. It sends no event or UI query to a
foreign window after Home. The stream cleanup is separate from the preceding
lifecycle cancellation evidence and is not counted as a game cancellation pass.
The helper remains outside the application and public repository.

## Retained failures and remaining gates

At pre-fix observer APK `09ef0500` / SHA-256
`25acf25afbfcdbebf606cfdad2bd8d51a86e4b432e06af1682cbe3c57e9c3b5f`:

1. The first attempt fails before injection because the private wrapper selected
   an older UI snapshot helper without the already-established null-root admission.
   The wrapper is corrected to the verified bounded null-root helper.
2. A four-second forward lead is too short to leave the opening encounter.
   Native axes clear after Cancel, but the subsequent player death fails the
   required live idle check. The replacement uses two bounded ordinary gestures.
3. Four forward/look and Swing/Parry Cancel cases complete before the next
   missing-Dodge preflight refuses injection. The missing control is reproduced
   in the regression and fixed in `7368e2c7`; no guard is weakened.

The first held-finger Home attempt on `7368e2c7` returns to the exact same Activity
and PID, observes a fresh RT scene epoch 2 to 4 and zero axes/stationary state.
Its next synthetic DOWN is rejected by Android's injector. That whole run remains
failed; accepted Home action/new presentation alone does not prove fresh gesture
recovery. A separate one-shot owned-window cleanup diagnostic admits the retained
pointer set but lacks the old receipt's original `downTime`; it is expressly not
a coherent-stream or lifecycle pass. New receipts retain that timestamp. The
complete follow-ups above succeed with known stream timing. No injected
cleanup is sent to a foreign window, and every app stops with preferences unchanged.

The first Compute Home follow-up, `20261007174855-c85a8175e488`, completes four
injected cases but fails its final backend guard. Its return command omits the
explicit Debug Compute requirement; `MainActivity.onNewIntent` correctly consumes
the new intent and absence selects Pipeline. Native initialization and the
retained report identify backend 1 after return, so this is not a controlled
Compute pass. The private return command now repeats the explicit backend
requirement and validates the actual backend after each Home. The failed run is
retained without changing application backend semantics or weakening guards.

## Owner touch comfort

After returning, the owner requests the intended build be opened because three
variants are installed. The exact installed Debug APK at source
`ccd70d3815e0ed08946f9f5ffc52a156bb41ed63`, SHA-256
`7c637c0b15b95964d9303dd93d69f3a7be77fd4962b791e0acea2af0ad743197`,
is hash-checked and launched normally without scenario extras. Foreground PID
2056 is handed to the owner; automated phone use/stopping is suspended during
their playtest. Asked how Swing/Parry/Dodge feel while moving/looking, the owner
replies "they feel great". **Owner native touch comfort: accepted on this build.**
This is qualitative acceptance, not a measured latency value, a second synthetic
matrix, Windows controller validation or proof of every combat/hitch condition.

Windows controller comfort, successful-parry/riposte feel, changed damage
audio/haptic acceptance, moving blade contact calibration, equipment production
activation, same-device 50/40/33 play quality/performance and Eric's independent
audit remain open. Earlier `ab69537a` waterfall/Keeper/pause/Home evidence retains
its exact packages in the [motion](ENGINEERING_1_6_2_ANDROID_MOTION_2026_10_08.md)
and [pause-work](ENGINEERING_1_6_2_PAUSE_WORK_2026_10_08.md) records.
