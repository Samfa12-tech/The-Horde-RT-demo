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
failure at 15/60/120 FPS; the corrected checks also cover 30 FPS and reject
subsequent clock advancement while paused.
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

## Secondary native UI navigation

The same installed `4e9e5bb7` Debug APK was used for ordinary portrait
entry → More → Controls / Credits / Report navigation on SM-S948B, preserving
the owner's large font setting. Private run
`secondary-ui-20261007142105-042113c7d35e` contains 20 owned hierarchy snapshots
and eight screenshots. Ordinary scoped touches and vertical scrolling reached
each Back control and returned to entry. Report consent stayed unchecked and
the remote send control was absent before preparation. No report was prepared,
exported or sent. PID 305 stopped; every preference entry remained unchanged.

Inspected screenshot SHA-256 values:

- Controls top: `40c52808ffebb470aef3013cc0a6de2dd8af34035dc5af5c14062feee749a45c`.
- Report bottom: `12fa176cc73051fe2b2d7801f6bea55c8c7fe9a4fdfee5bca5626e8db39c6fc2`.

These are native navigation/readability checks, not moving touch, owner visual
acceptance, report transport or loading-error coverage. Documentation head
`a18404c1c489ff0cc294b370ad3ee1e5c7316ccf` separately passes all 12 aggregate
checks in push `37634256848` / PR `37634262651`.

## Measured player contact follow-up

Runtime `ab69537a46248d551420df16f4c3a1f781de6bc5` moves each player
contact sample into its existing stroke: 100 ms after downward-active entry,
50 ms after upward-active entry. The imported production sword has 11,424
expanded vertices; the imported skeleton idle mesh has 28,206. Sampling uses
the final anatomical player Grip and the actual sword mesh. In the frontal
1.2 m fixture, the old downward pulse at tick 11 had a 355.3 mm blade gap;
the new pulse at tick 17 has a 2.13 mm gap. The upward pulse moves from tick 27
(116.4 mm gap) to tick 30 (13.1 mm gap). The 20 mm reporting band is a
measurement threshold, not a new gameplay collision rule.

New regression assertions failed against the old contact timing, 0/2. The first
post-fix run passed 6/8 affected targets; the two remaining failures were old
pulse-tick expectations and 10 ms float accumulation at 100 ms. Their corrected
rerun passes 2/2, including actual blade proximity and one pulse per stroke.
Additional smoke coverage passes zero-delta calls, 120/60/30 Hz direct updates,
50 ms updates, irregular partitions and immediate riposte without early contact.
Android Debug/unsigned Release and Windows Debug app builds pass; lint is
0 errors/62 warnings. Existing frame-rate/hitch/parry tests also pass.

Health, damage amounts, death, parry eligibility, combo durations, 60 Hz
simulation and late-input policy are unchanged. Range 1.72 m and cone dot .52
remain uncalibrated: several admitted idle-target samples never approach the
blade, and the measured left/right coverage is asymmetric. Walking/attacking
target geometry, input-to-presentation timing and owner feel remain open.
**Audio/haptic manual revalidation required: YES**, because damage-event timing
changes. Automated muted inspection does not close this gate.

### Dynamic contact diagnostic

A subsequent test-only diagnostic runs ordinary `SwordCombat` AI with frontal
starts at 1.28 / 1.50 m and two 1.28 m starts at +15 / -15 degrees. It requests
the player cut during enemy windup, then skins the imported target and resolves
the final anatomical player Grip. Target action/clip/instance mapping is an
explicit diagnostic copy of `CharacterRenderSlot`; it is not execution of that
renderer translation unit. The pre-hit sample uses the previous immutable
enemy snapshot at windup 1.1167 s; the player uses the current pulse snapshot.
The post-hit sample uses the actual published Dead pose at 0.0167 s. This
one-tick distinction is retained, not described as simultaneous mesh contact.

Full indexed blade-triangle / target-triangle distance checks include edge
intersections, both vertex-face directions and edge-edge distances through a
conservative target BVH. The blade filter retains 6,905 triangles whose three
Grip-local vertices exceed 0.15 m, excluding the hilt. These ten instantaneous
samples find no actual intersection:

| Target sample | Blade / target triangle gap |
| --- | --- |
| Idle frontal 1.20 m, old tick 11 | 355.3 mm |
| Idle frontal 1.20 m, current tick 17 | 2.13 mm |
| Attacking frontal start 1.28 m, pre / post hit | 189.4 / 261.5 mm |
| Attacking frontal start 1.50 m, pre / post hit | 164.0 / 292.9 mm |
| Attacking +15 degrees at 1.28 m, pre / post hit | 184.4 / 444.2 mm |
| Attacking -15 degrees at 1.28 m, pre / post hit | 248.5 / 178.0 mm |

The 1.50 m start approaches to root distance 1.3347 m before attacking. At +15
degrees the earlier full-sword point gap was 18.3 mm, but the blade gap is
184.4 mm; hilt proximity must not be called blade contact. The target mesh
center also shifts with the imported Attack clip. Neighboring point samples
are diagnostic only; no inter-tick sweep or collision rule is inferred.

The held-item target build and expanded `--combat-geometry` invocation pass.
The preceding default held-item CTest pass remains separate; it was not
repeated for these expanded-only helpers. This evidence preserves the useful
idle timing correction while exposing the unresolved moving-target mismatch.
Range/cone and any bounded sweep still need calibration against actual poses;
there is no general combat-engine replacement or widened parry window.

The focused `--combat-dynamic-neighborhood` mode then retains only the frontal
and +15-degree 1.28 m cases over pulse -3 through +3. A parallel ordinary
`SwordCombat` instance receives no player attack, keeping its target alive and
advancing the Attack clip after the other instance is killed. That unhit control
is explicitly counterfactual after damage; its previous-tick target pose is
paired with the attacked instance's current player pose. Same-root/facing Idle
controls use clip time zero. The 28 full triangle-distance samples pass in about
90 seconds, with no general range matrix or gameplay change.

| Unhit control / player sample | Pulse | +1 tick | +2 ticks | +3 ticks |
| --- | --- | --- | --- | --- |
| Frontal live Attack gap | 189.4 mm | 103.7 mm | 17.4 mm | 0 mm (intersection) |
| Frontal same-root Idle gap | 47.0 mm | 34.9 mm | 75.2 mm | 106.6 mm |
| +15-degree live Attack gap | 184.4 mm | 109.2 mm | 40.9 mm | 7.77 mm |
| +15-degree same-root Idle gap | 279.3 mm | 299.9 mm | 329.7 mm | 351.4 mm |

Player active time advances from .1033 at the pulse to .1533 s at +3 ticks;
the unhit target crosses windup into active. These instantaneous controls
support investigating a bounded stroke sweep: a single global timing shift
would move contact away from other idle samples. They are not swept-contact
proof, simultaneous renderer execution or owner acceptance.

## Waterfall equipment moving inspection

The same immutable runtime has tree `f49720cd5c2fb6afaf2a9fab75cb9bfccf3fae1c`.
Four-ABI Debug APK SHA-256 is
`99e012534a5b442846d7aeb724dbf17e9ea9860492724c1ee100bdfa20fd3895`
(138,462,724 bytes); installed pullback matches. Arm64 native SHA-256 is
`ee0fb90dd5c5012466699df629bf53939278247922c5c25f21dad90fbc41829c`.
Windows executable SHA-256 is
`a5827b1003563c7ec922e15ebe27c754377be86b26f3d5933aa642ca9602e7ee`.
Unsigned Release APK SHA-256 is
`03151b019762a140d419509fa8d1aa58d3a21c82ce790febc6d97f4f9f4ec9ad`;
all four actual native payloads exclude the Debug harness markers. It remains
unsigned and unpublished.

The Debug-only `waterfall-equipment` scenario scopes the existing two flags
around ordinary checkpoint 2, follows a collision-valid two-leg approach, then
observes range/LOS warning, phased draw, attachment, ordinary attack and parry.
The same two guard IDs, count, health and active torch remain authoritative.
End/failure/surface-retirement cleanup restores the previous flags, route and
monotonic input/event floors. Host scenario/parser/policy checks pass 3/3; an
earlier failed diagonal route and incorrect one-guard cleanup expectation are
retained. Production flags remain off.

| SM-S948B / Android 16 run | State / completed RT rows | Events | Images | Simulation time |
| --- | --- | --- | --- | --- |
| Pipeline `m-20261007145711-7b7d2f0dba97` | 169 / 169 | 16 | 13 | 6.5333 s |
| RayQueryCompute `m-20261007145749-c0925cba47cb` | 176 / 176 | 16 | 13 | 6.6167 s |

Both runs complete with zero pending submissions, exact image/state/frame joins,
unchanged preference entries and stopped owned apps. Draw captures retain actual
observed progress, not synthetic skipped poses: Pipeline .2917/.50/.8333;
Compute .25/.50/.75. Both threshold masks are 7. Ordered semantic events are
warning 13, draw 14, attachment 15, swing 16. Collector reservation is
36,658,048 bytes, plus intrusive milestone readbacks; timings are not sustained
FPS or isolated memory-savings evidence.

Inspected images show the blade in hand by completion and changing attack/parry
poses. Intermediate draw silhouettes are obscured or outside the portrait view;
these images do not establish a snap, clearance, blade contact, world-body,
shadow/reflection or owner framing acceptance. The bright gold water curtain
also obscures a guard. Water mesh/shaders are unchanged from earlier sources;
without a matched prior capture, its appearance is not declared accepted or a
new renderer regression. The cumulative socket-distance telemetry includes the
intentionally released right hand during stow/reach (up to 811,801 micrometres),
so it is not a current fully engaged Grip-error measurement.

Initial `ab69537a` CI failed because selected host LFS lists omitted the existing
skeleton GLB required by the new geometry fixture. The bounded fetch correction
`ebeb9117ad5e51a07868ce3c9fe699de74c380de` passes all 12 aggregate checks in
push `37641619811` / PR `37641674418`; runtime assets/binaries are unchanged.
The original CI failures and exact private receipts remain preserved.

The affected Keeper sequence was also rerun on this exact APK because contact
timing changes damage delivery. Pipeline run `m-20261007150742-116f2d5a8448`
passes 661 state/completed-RT rows, 53 events, 33 images and two resource scopes
over 47.25 simulation seconds. Compute run `m-20261007150925-301ab0337e8b`
passes 668 rows, 53 events, 33 images and two scopes over 46.9333 seconds.
Both cover genuine death, ordinary retry/recognition, three accepted hits,
defeat, reward/ending and re-entry. All joins pass, zero submissions remain
pending, preferences are unchanged and owned apps stop. These muted scripted
runs do not establish comfortable physical input or presented blade contact.

### Active-draw Home interruption

The same APK also rejects an actual Home interruption during active Draw and
returns to a newly presented RT surface on both backends. Pipeline run
`i-20261007152259-30c7bc3675cd` interrupts at progress .4583; Compute
`i-20261007152412-cba9895ea529` at .7083. Both native logs bind resume to actual
request/presentation generation 3, with the same owned PID, unchanged
preferences and stopped apps afterward. This is native presentation evidence,
not a new completed-frame image binding or physical multitouch check.

The original runner incorrectly required generation 2. Its failed invocations
and stop receipts are preserved; read-only analysis instead joins the latest
actual request to its successful presentation marker. Earlier attempts also
retain a trigger that arrived after draw completion, and an active-draw
interruption reported as ownership loss before surface retirement. Rejected
interruption ledgers can contain one in-flight submission and are not claimed
as complete accepted motion ledgers. No lifecycle bypass or source correction
was needed for these checker failures.

## Remaining gates

### Closed coarse contact-proxy probe

A further test-only `--combat-capsule-feasibility` mode reuses the same two
seven-tick live-Attack neighborhoods and same-origin Idle controls. It fits 24
actual imported joint/node segments; each triangle is assigned to the segment
requiring the smallest covering radius, fitted from its three skinned corners.
The 672-byte per-pose cover preserves authored root/clip transforms and queries
the actual 6,905 blade triangles. No runtime collision or damage rule changes.

At frontal pulse69, live-Attack capsule gap is .1245 m versus retained exact
triangle gap .189441 m; +15° is .1545 versus .184408 m. The frontal Idle proxy
already reports zero while the real blade remains .0469822 m away. Frontal
Attack proxy intersects at tick71, one tick before the exact tick72 intersection;
+15° proxy intersects at tick72 while the exact blade is still .00777264 m away.
Fitted radii reach .293 m. This coarse covering profile is too loose for
trustworthy visual contact and is **not adopted**. It does not rule out a tighter
surface profile or the authorized bounded sweep.

Host fitting costs approximately 212–250 ms/pose and the deliberately unoptimized
6,905-triangle ×24-capsule query 432–492 ms/pose. These diagnostic costs are not
a proposed runtime cost. Exact triangle rows are reused from the earlier matching
oracle; no new full-oracle pass is claimed. The private successful probe log is
`combat-capsule-feasibility-run-20261008.log`. Do not repeat this unchanged coarse
cover to claim progress or tune parry eligibility around it.

### Later CI checkpoint

Test/documentation head `51472d21` passes all 12 aggregate CI jobs in push
`37647368275` / PR `37647377943`; runtime remains the sealed `ab69537a` here.
Earlier `22338811` has 11 successful jobs and one cancelled MSVC PR job:
the Windows report-form Unicode-edit test first times out after its existing
two-second message deadline, then the overall ten-minute job is cancelled.
Five focused local repeats pass (2.69 s total). No report source or timeout is
changed and the transient cause is unproven; the failed test/job is preserved
separately from the later passing CI checkpoint.

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
