# Android moving RT evidence — 8 October 2026

This slice closes the lack of an Android moving inspection adapter. It reuses
the shared 60 Hz simulation, input schedule and motion ledger, with genuine
Pipeline or required RayQueryCompute output on the allocated SM-S948B / Android
16. It does not close production equipment activation, owner feel or performance.

## Method and isolation

Debug-only launch admission selects one named scenario and fresh bounded run ID.
The render owner waits for a current completed, presented RT frame, applies one
accepted checkpoint seed, then drives ordinary movement/look axes and timestamped
combat edges. There is no phase import, second simulation, fixed frame delta,
extra submission counter or saved-preference write. Actual owning fence/idle
completion binds every RT row and raw RGBA image to its input/simulation row.

Foreground loss, external input, profile/settings changes, overlapping capture,
replay or RT Lab work, unexpected resource scope changes and receipt failures
reject the run. The normal world-command path handles retry. Terminal ledgers and
manifests use atomic replacement before the UI terminal notification. UI/game
input latches are reseeded on release. Automated SFX/haptics/music are muted and
queued scenario cues discarded; this is not listening or haptic evidence.

The scenario has a 30-second arming deadline, 120-second wall deadline and 64-image
limit. The collector reserves 36,395,904 host bytes, and milestone image readbacks
wait for the actual graphics owner. These costs make this an inspection lane,
not a sustained FPS, power, memory-saving or scanout benchmark. Native UI remains
separate from the traced-image captures.

## Exact checkpoints and retained failures

Initial immutable `90abb92946469605a290c509eb4b8eeb7db93ddc`, tree
`a97bedfc7a3ada10ef1d6811f4c16e29350a28ab`, four-ABI Debug APK SHA-256
`76599ad9d170068c0f9633327414333206922936d5be982ba7725dd3a3b133b6`
(138,462,724 bytes), installed pullback matched. Its unchanged Windows executable
retains the earlier `868691fc` evidence and foreground-arm gap.

| Physical scenario | Pipeline | Required Compute | Scope |
| --- | --- | --- | --- |
| Torch / rear opening / low passage | 605 completed RT rows, 50 semantic events, 14 images; pass | 626 rows, 50 events, 14 images; pass | 24.4 simulation seconds, ordinary axes; damage disabled |
| Active-torch waterfall shaft | 376 rows, 18 events, 10 images; pass | 388 rows, 19 events, 10 images; pass | Approximately 17.9 simulation seconds; pre-drench torch |
| Keeper death/retry/reward | Fails at retry: 248 completed RT rows | Fails at retry: exact failed ledger retained | Adapter wrongly expected retry to advance scene epoch |

Every run stops its owned app and preserves every preference entry. The initial
runner attempt also stopped immediately because its foreground parser expected
the wrong device report field; it produced no completed moving pass. The parser
now checks the exact `topResumedActivity` component. This invocation failure is
separate from native scenario failures.

The initial two-second image cadence captured only idle player combat poses,
despite observed live swing/parry states. `156157d7` adds captures at the actual
windup/active/parry transitions. It also admits exactly the existing retry's
measurement-generation increment with retained scene/surface/extent; other
changes remain rejected. Known appended draw/attachment/waterfall events are
admitted without changing their IDs, ordering or duplicate rejection.

The three valid-event regressions fail before correction, then pass. A stale
first test binary hit a Debug bounds assertion in the new fixture and was
stopped; the guarded fixture then reproduces the three intended admission
failures. A mistaken native build target is retained as an invocation failure,
followed by the correct diagnostic-window target's successful build.

Exact `156157d7b3113a49bc30a4be0ab2f3347cbdc960`, tree
`2ea69705bf4b1b41e328bc3bfdcb32bc977ed13e`, APK SHA-256
`ae5757d00e1ff827fa24e2aaa2fc8e3f01bafb7b65ff89cc75d438c9c2b5d4db`,
passes the retry scope change on both phone backends, then fails the shared
observer's pause check: the ordinary zero-delta retry legitimately resets its
reveal clock while the new measurement output is pending. Pipeline/Compute
retain 248/247 completed RT rows and two declared resource scopes. Both apps
stop with preferences unchanged. Neither is a complete Keeper pass.

`4e9e5bb7` admits only the consumed, safe, zero-clock ordinary retry reset; normal
pause still freezes reveal/recognition. Host regressions reproduce the old
failure at 15/30/60/120 FPS and reject subsequent clock advancement while paused.
The authoritative gameplay, lifecycle, contact and timing definitions are unchanged.

## Corrected immutable checkpoint

Source `4e9e5bb7696f4d5d3864835eb35e29f4bce42d8e`, tree
`93df3ea4a5d34f84511f28450fb6806394a49c90`. Four-ABI Debug APK:

- SHA-256 `68741e3d3407d124b6f3ba95f7e84e991d701070fe396922c90014b3ed6e80b7`, 138,462,724 bytes.
- arm64 native SHA-256 `980f70a037ffd8d9423fc0a73d18ef22efa3c4de1a7b4448193eba12a0dd139f`.
- Windows Debug executable SHA-256 `62e80f9541d02b0dff58e3f83acbcc77ebf5cc18f8ae9f9474d0d265f0259498`.
- Closed asset admission, four ABI roster, orientation/configuration manifest and 16 KiB alignment pass; installed pullback matches.
- Affected host policy/scenario checks pass 2/2 (7.28 seconds), including all four schedules at 15/30/60/120 FPS and paused ordinary retry. Diagnostic-window native build and four-ABI Debug assembly pass.

The corrected package passes these actual moving runs. Every state/RT/image join
was checked against its saved ledger; all apps are stopped and every preference
entry is unchanged:

| Scenario | Pipeline | Required Compute |
| --- | --- | --- |
| Torch / rear opening / low passage | `m-20261007135800-7db26b10bd6e`: 577 completed RT rows, 50 events, 16 images | `m-20261007140031-26d18b216ea1`: 627 rows, 49 events, 16 images |
| Keeper death/retry/reward/re-entry | `m-20261007135338-b83d82fc66b1`: 657 completed RT rows, 53 events, 33 images | `m-20261007135638-a781d860b05d`: 669 rows, 53 events, 33 images |

Torch images include actual windup, SwingActive and ParryActive states. Keeper
records one player death, one ordinary retry, one initial awakening and warning,
two combat-ready transitions, three EnemyHit events, one defeat, one chest
unlock/open, one claim and one finale completion. Its two scopes retain surface
generation 1 / scene epoch 2 while measurement advances 3→4. Each terminal ledger
has zero pending submissions. Native input/tick/pose/present-call traces remain
alongside PID logs; display time is explicitly unmeasured.

Selected Pipeline RGBA frames were converted losslessly to PNG and inspected:

- Torch ParryActive, state row 373 at 15.5333 simulation seconds: PNG SHA-256 `87842dfd3d7d172b35eaf87c10f0bcb2880c37081a3d7f3ed9417f1c45742d09`.
- Keeper first accepted-hit SwingActive, state row 302: PNG SHA-256 `4a438d15f1f96eb5989e0bf5ca89c1b5adef954012145e14e9607b7334c26348`.
- Keeper late death pose, state row 388: PNG SHA-256 `65ad4e7cafa19581eeb411b140fde9ea8cf1ff07cdfb080cfc171250ef7c3ff3`.
- Keeper reward-stage pose, state row 402: PNG SHA-256 `eccfc1394b31a6d175065db8c2f5c5a88ed0e83c849d28cea1493a2a3c046573`.

These captures do not establish blade/target contact calibration or comfortable
framing. The early active-hit pose, near-camera windup arm and close portrait
Keeper view require further measurement/owner inspection; scenario completion
does not imply visual acceptance. Original RGBA hashes and exact state/RT
identities remain in each checked capture receipt.

All 12 aggregate source checks at `4e9e5bb7` pass in push `37631996493` /
PR `37632008946`. Four-ABI unsigned Release assembly and lint pass; lint remains
0 errors/62 warnings. Actual unsigned APK SHA-256
`5598d7cfc1892caf18cc92f34660817c8d733e00c165015deddbdf7c712711b0`.
All four actual Release payloads exclude harness markers; exported JNI names
remain inert stubs. This artifact is preserved, unsigned and unpublished.

Earlier source `90abb929` has all 12 aggregate CI checks successful in push
`37628203328` / PR `37628210982`; those results are not assigned to later code.
Full Java 176/176 in 30 classes belongs to the earlier adapter build; Java has not
changed in these corrections. The four actual unsigned Release payloads at `53f75d1d` also exclude
the harness markers; JNI names remain inert stubs. No production package is signed
or published. Private original ledgers, raw images, hashes, native PID logs,
install/stop/preference receipts and invocation failures remain under `task-4`.

## Remaining gates

The saved profile in these runs is explicit 50%, Mobile water/fire, Glass On,
Current shadows, cap30, Mist On; custom preferences are preserved. It differs
from fresh/reset Glass Off. Native output is 1440×2980 with newly traced 720×1490.
This is not the accepted 50/40/33 performance comparison or historical 75% baseline.

Production sword-stow/waterfall flags remain off. Moving draw/attachment,
body/shadow/reflection inspection, collision route activation, contact/range
calibration, physical move/look/action and controller checks, owner visual/audio/
haptic/comfort, sustained phone quality/thermal/power evidence and Eric's
independent audit remain open. Prior checkpoint/route/rotation/Graphics ACK and
closed negative experiments retain their exact original source and limits.
