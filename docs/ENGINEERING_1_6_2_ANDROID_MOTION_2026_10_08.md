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

### Bounded interpolated stroke samples

Test-only source `367ac7a364cab88f5e456e246c436cbc21370d77` adds
`--combat-bounded-sweep` for those same two 1.28 m cases and their same-root,
same-facing Idle controls. It queries all 6,905 Grip-filtered blade triangles
against indexed target triangles at the pulse and +1/+2/+3 ticks, plus .25/.5/.75
of each interval: 13 samples per control, 52 distance queries in total. Intermediate
triangles are linearly interpolated world vertices, not authored/rendered
sub-tick poses or a continuous collision solver. The existing floating-point
triangle-distance oracle reports zero with its own numerical precision and
degeneracy handling; no gameplay proximity tolerance is added.

| Case / control | Gap at current pulse | First sampled zero | Minimum sampled gap |
| --- | --- | --- | --- |
| Frontal live Attack | 189.441 mm | +2.5 ticks, interpolated | 0 mm; exact +3 endpoint also intersects |
| Frontal same-root Idle | 46.9822 mm | None | 30.9447 mm at +.75 tick |
| +15-degree live Attack | 184.408 mm | None | 7.77264 mm at +3 ticks |
| +15-degree same-root Idle | 279.334 mm | None | 279.334 mm at the pulse |

Both root gates remain eligible (1.28 m; cone dot 1 / .965926). Eligibility with
a separated Idle mesh is reported separately from physical blade contact; the
existing range/cone rule does not itself promise literal mesh intersection.
The parallel unhit target remains counterfactual after ordinary damage, and its
previous-tick pose is paired with the current player pose as in the earlier
diagnostic. These results support bounded sweep feasibility, but do not justify
a production tolerance, global damage delay, range/cone change or parry change.
The closed coarse-capsule negative is not rerun.

The focused target build passes in 2.82 s; the reviewed diagnostic passes with
exit 0 in 216.17 s. It checks successful asset/final-Grip/target-pose resolution,
four poses inside the existing downstroke, stable indexed counts, finite
distances, current range/cone eligibility and the 13-sample count. Geometry
outcomes are reported, rather than asserting a preferred collision result.

```powershell
cmake --build build/presets/windows-x64-debug --config Debug --target horde_rt_held_item_socket_tests
build/presets/windows-x64-debug/Debug/horde_rt_held_item_socket_tests.exe --combat-bounded-sweep
```

Private ignored logs are `build/reports/combat-bounded-sweep-reviewed-build-20261008.log`
and `combat-bounded-sweep-reviewed-run-20261008.log`. Executable SHA-256:
`c68478459af49ec366f9f74208530b52ac2958c753718dcba75e63b637db2db4`;
run-log SHA-256:
`c9c7099ead1974510ecf61a5d01d191dd0d167fa950443630cd34246053f2885`.
This explicit host diagnostic is outside default CTest and supplies no phone,
displayed-FPS, input-latency, owner-feel or final integrated-candidate evidence.

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

### Windows CCD waterfall inspection: completed state, limited images

After the owner explicitly agreed to keep the Windows demo foreground,
the sealed `ccd70d3815e0ed08946f9f5ffc52a156bb41ed63` executable
(`d15b5bf005fb243924a07d1a651b805dc8a193a72442c336d65c01bc6acdf764`)
completed `waterfall-equipment` on NVIDIA GeForce RTX 5050 Laptop GPU,
RayTracingPipeline. PID 22924 exited normally in 9.965 wall seconds with
verified owned-window focus and zero Vulkan validation error markers.

The read-only exact-join audit passes 163 state / 163 completed RT rows,
16 ordered events, one resource scope and seven actual 960 x 540 PNGs.
Final simulation tick is 383 (6.38333333 seconds), zero submissions remain
pending, and warning/draw/attachment/swing events are sequences 13/14/15/16.
The ledger contains 11 SwingActive rows and five ParryActive rows.

However, the stage-entry `equipment-attack` and `equipment-parry` images both
join **Idle** snapshots. Draw joins only progress .0416667; the existing
Windows stage/periodic/torch capture policy missed later action and draw poses
inside the same stage. These seven images do not close moving draw/action,
guard-room layout, body, shadow or reflection acceptance. The approach corridor
and water curtain obscure the room. The muted, checkpoint-seeded,
damage-disabled run with intrusive readbacks is not sustained performance.

Private evidence is `windows-waterfall-ccd70d38-ownerfocus-20261008-pipeline/`.
Manifest SHA-256:
`d67e93c5d61d9eb1e468092665d493f76df91a7b842b660070f087650031f8ab`;
ledger SHA-256:
`9bf203b44469019d8074e0ea24566fa8308bb02549f243471ce81d35be8cee09`;
launch receipt SHA-256:
`2c23b260b14215070ed1c15cffba8a0d5954c77df56059e1641d67b61518d685`.
The private audit retains the image hashes and exact owning-frame joins.
Older Windows focus failures retain their original failed classification.

The affected capture-policy regression reproduces eight missed action/draw
cases with the old policy, then passes with explicit observed pose milestones
while retaining stage, periodic and torch capture triggers. Actual inactive
completed draws do not fabricate intermediate captures. The host launch/capture
target passes 1/1; corrected Windows runtime captures remain to be collected.

Documentation head `62b6d40e` has 11/12 successful aggregate CI jobs: push
`37680968970` fails the MSVC report-form UI test (96/97 CTests), while PR
`37680975262` passes all six jobs. The failed Unicode note edit's native
WM_SETTEXT call times out after 2004 ms. This failure is retained separately
from the sealed CCD runtime and earlier `e4cc12f8` 12/12 results; it is not an
installation timeout or a gameplay/capture pass.

### Waterfall room guards: staged Windows preview

The owner's updated direction places the existing pair to the player's right
and left, facing arrival. Immutable Debug source
`3da5f2ddb7050422ccc083a645afc3a84b17aa4a`, tree
`f9bef90b62d06c27dab015640afb234a4f311a3b`, implements explicit shared spawn
poses at (-5.5, -15.95) and (-5.5, -14.45), facing +X. The second guard retains
a .65-second simulation walking-phase offset after waiting outside the room,
leaving/re-entering and reset. The imported walk clip is 1.0333333333 seconds;
the existing .90 sample rate makes the offset .585 clip seconds, about 56.6%
of a cycle. Attack/contact/death clocks remain zero-based. Unauthored layouts
retain their prior animation-clock behavior; identities/count/stats are unchanged.

The first `38f9d48a` room stills completed natively but showed no guards: the
historical SkylightChamber selector chose Lich. Three new room-selector assertions
reproduce that defect. The shared enemy director now accepts an explicit encounter
override only while the opted-in waterfall encounter is inside its room; leaving
retains ordinary route selection. No renderer-specific enemy branch is added.
The affected five host checks pass (21.19 s), the development registry check
passes (6.52 s), and Windows plus four-ABI Android Debug builds pass.

Debug-only checkpoints 153/154/155 stage room entry and six/eighteen actual fixed
ticks, with immutable snapshots while RT settles. Existing checkpoints 100–152
retain their identities; bounded admission rejects more than 30 staged ticks.
The zero-delta authoritative import adds one boundary, so completed simulation
ticks are 1/7/19. Earlier wrong base/count/freeze assumptions and private-access
build failures remain retained as failed harness attempts.

Three NVIDIA RTX 5050 Laptop / RayTracingPipeline stills at 960 x 540 pass exact
manifest, owning completed-submission/presentation, pipeline, launch and PNG
hash joins. Owned PIDs 59624/51732/56180 exit normally, with zero synchronization
validation error markers. All three original images were inspected: both lateral
guards face arrival, timed stills show differing walk poses, and the held torch
partly occludes the left guard. This is staged still evidence, not continuous
motion, scanout, sustained FPS, complete body/shadow/reflection or owner acceptance.

Private set: `waterfall-guards-3da5f2dd-staged-20261008/`; audit SHA-256
`8af3933c980987df99a57a1d73787bb8c77f3b3aed9e14942eef9f2e25070d02`.
PNG SHA-256, entry / early / later respectively:
`6dfe02533ad8a729a829c822c10a4909dfc7468ab1817e8f6e493b5d38b4c2db`,
`231edc403c510f690e9696bb99940bf1c36527ce6c3fad9a9e5ff3e4d51ecfd2`,
`10746825d08baf52b6946b69ab70d8c354807e0016f3044a4187e2ab19da8f0f`.
Failed predecessor `waterfall-guards-38f9d48a-staged-20261008/` remains unchanged.

Sealed Debug APK is 138,462,724 bytes, SHA-256
`4c0829af2d1c890455917fd49740f0f0dfd8ea02de923384f329b26e29622db3`;
Windows Debug executable is 11,352,576 bytes, SHA-256
`d22757047868747d05685318bda42406c599a8e0bb927fa1fc9167a662ed0688`.
Closed Android assets, four native ABIs, manifest and 16 KiB admission pass.
The owner took the phone away; this APK is **not installed or device-validated**.
There is no new Release/signing/owner/performance/audit result. Production stow
and waterfall flags remain off pending moving attachment/body/shadow/reflection
and integrated-route checks. Earlier CCD owner approvals keep their exact APK.

Both `38f9d48a` source CI runs retain six failed / six successful aggregate jobs:
GCC/Clang/MSVC fail the old development-registry size/153-null assertions;
Android/Vulkan/sanitizers pass. Updating the registry to 56 / first absent 156
retains all old checkpoint checks and adds explicit bounded preview assertions.
The local registry check passes; `3da5f2dd` completes **12/12 aggregate CI jobs**
in push `37685777037` / PR `37685784607`, including both MSVC report-form runs.
This does not establish the cause of the earlier intermittent Unicode-edit
timeout. Documentation checkpoint `bcb86b17` has separate CI.

The corrected Windows moving sampler was then attempted on this sealed binary.
PID 52292 exits 1 after 32.941 wall seconds because Windows denies the owned
window's foreground activation. Native arming expires at its existing 30-second
budget, the manifest is incomplete with zero images, and no focus guard is
bypassed. Synchronization validation reports zero error markers; this is a
failed focus/inspection attempt, not successful motion or a renderer pass.
Private set: `windows-waterfall-3da5f2dd-motion-20261008-pipeline/`;
manifest SHA-256 `30b7a9cc5733c2778f8ef952509760501b3de65e49e65b1963d25a2e78741e83`;
launch SHA-256 `e71f93ae5ec72d329fa394c1b0bbcbff0d62ff3381f535d30a4807e1d534a896`.
The owned process has exited; actual moving image coverage remains pending.

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

### Approaching-target range/cone calibration gap

Two new opt-in host modes in `HeldItemSocketTests` investigate fresh approach
conditions rather than replaying the closed Attack/Idle neighborhoods or coarse
capsule experiment. They use the actual imported player rig, final sword Grip,
all 6,905 blade triangles and imported target skins. An ordinary unhit parallel
combat control supplies the pre-update target paired with the existing contact
pulse, matching `ResolveSwordHit` ordering. Renderer clip/instance mapping is
the then-existing aligned diagnostic copy, not direct execution of CharacterRenderSlot.
Later source `525a5d0e` extracts the actual shared `SkeletonRenderPose` helper for
both renderer and diagnostics; the [454 follow-up](ENGINEERING_1_6_2_ANDROID_CONTROLLER_2026_10_08.md#current-integrated-follow-up--454d58ee) records equivalence/edge checks.
The historical measurements below retain their original source and gaps.
Each case samples pulse through +3 fixed ticks (.10–.15 active seconds); this is
four discrete endpoints, not continuous collision or actual presented contact.

`--combat-walking-range-cone` passes 40 queries in 183.59 s. At pulse tick 17,
walking roots are 1.69967 m away and both 0/.65-second gait seeds are sampled.
The present 1.72 m / .52-dot gate admits all frontal/±50° cases and kills the
target, despite positive blade gaps at every sampled endpoint. Minimum gaps
across the four endpoints and two phases are .3765 m frontal, .9769 m at +50°,
and .6685 m at -50°. Relative +62° and 1.78467 m range controls correctly reject
damage. The +62° case uses a legal +50° world path and -12° player yaw so the
ordinary target stays inside the physical corridor.

`--combat-inner-range-cone` passes 20 further queries in 80.47 s, using ordinary
early Attack windup (.25 s at pulse). Attack entry resets the gait clock, so
identical phase variants are not repeated. Current range/cone admits all five:

| Target root | Pulse blade gap | Minimum of four endpoints | Sampled intersection |
| --- | --- | --- | --- |
| Frontal 1.20 m | 257.8 mm | 62.2 mm | None |
| Frontal 1.28 m | 303.0 mm | 100.9 mm | None |
| +15° / 1.28 m | 398.5 mm | 365.2 mm | None |
| -15° / 1.28 m | 272.4 mm | 0 mm | One later endpoint |
| +30° / 1.28 m | 523.0 mm | 516.5 mm | None |

These measurements establish a calibration gap; they do not certify the
current hits as visible contact. A symmetric cone shrink or universal delay
alone is not supported by the different target poses and lateral results.
Production range/cone, contact times, parry eligibility and health/damage rules
are unchanged. A bounded pose-aware hit/short-sweep proposal and affected
moving presentation/owner checks remain required; no general combat engine is
introduced here. Audio/haptic manual revalidation for this test-only change: **NO**.

The first walking fixture fails and is rejected: held input inherited normal
player z=1.85 while combat used z=0, and a +62° world path was outside the
physical corridor. Its 123.33 s run, source patch and failed receipt remain
retained, with no calibration conclusion. Corrected outer test executable
SHA-256 is `2475776d92f578773fe1af27da1d2a321aa2937b9f53027e562384999b7fc7bb`;
source-file SHA-256 is
`05dd0726dc59de0582b37b026a9e957791efb0a0b0031c08c88fedc9f6a5ee9c`.
The subsequent inner-mode source also corrects the unobserved initial facing;
the simulation already faces the player on every tick before measured pulse.
Its executable SHA-256 is
`6bd03f2f12fe22e7529bdd739ba760b9d6f081988c1fb7e7ae5bb9a2de05e37d`;
source-file SHA-256 is
`f5fddb844f3dfaedccde65a85a96f2e8a3db24a91934615cfd728f8507b09316`.
Exact logs/receipts remain under `build/reports/combat-*-range-cone-*20261008.log`
and `task-4/combat-*-range-cone-*-private-20261008.json`. The sealed `3da5f2dd`
runtime/package is unchanged. Documentation `bcb86b17` separately passes all
12 CI jobs in `37686976032` / `37686980419`; this test follow-up has its own CI.

### Bounded preceding-tick contact probe, 8 October follow-up

The opt-in `--combat-prepulse-bracket` uses the actual shared renderer helper,
final imported anatomical Grip and all 6,905 indexed blade triangles. It keeps
the ordinary immediate downward attack, two 1.28 m target bearings (frontal and
-15 degrees), aspect 1 and 16:9, and two existing authored roof origins. A cheap
preflight passes all eight cases before any triangle query. This is diagnostic
pose sampling, not a new playable encounter or proof that the target clears the
lintel/walls.

Five samples span the previous fixed snapshot (tick16) to the pulse snapshot
(tick17): player active clock .0866667–.103333 s, unhit target Attack windup
.25–.266667 s. Both endpoints use post-update render snapshots. A separate
query pairs the pulse blade with the actual pre-update hit-authority target
(.25 s); one further query uses both actual post-update poses at tick18
(player .12 s, target .283333 s). The phase is unchanged throughout, so no
animation transition is interpolated. These are five discrete synthesized
poses over one fixed tick, not continuous collision or presented-frame evidence.

Build and preflight pass; the finite full run exits0 in approximately158.52 s,
completing **56/56 queries**, with damage admission matching the unchanged
1.72 m/.52-dot gate in all cases. Every sampled gap remains positive:

| Roof / aspect | Bearing | Pulse blade vs pre-update target | Minimum of five bracket samples | Actual tick18 pair |
| --- | --- | --- | --- | --- |
| High / 1 | Frontal | 303.0 mm | 275.3 mm | 187.4 mm |
| High / 1 | -15 degrees | 272.4 mm | 234.7 mm | 96.5 mm |
| High / 16:9 | Frontal | 292.9 mm | 265.4 mm | 172.9 mm |
| High / 16:9 | -15 degrees | 277.3 mm | 239.1 mm | 97.6 mm |
| Low lintel / 1 | Frontal | 308.7 mm | 281.1 mm | 190.1 mm |
| Low lintel / 1 | -15 degrees | 277.0 mm | 239.5 mm | 98.9 mm |
| Low lintel / 16:9 | Frontal | 298.4 mm | 271.0 mm | 175.7 mm |
| Low lintel / 16:9 | -15 degrees | 281.9 mm | 243.8 mm | 99.8 mm |

High origin is(0,0), roofY1.35 m. The existing low lintel is at(-29.5,-15.2),
roofY.88 m, yaw pi; target and player/held poses translate together. The low
fixture lowers the torch .470124 m, while the sword needs zero lowering in this
particular active-stroke interval on both roofs. It does not validate earlier
windup/peak ceiling response, continuous geometry clearance or a real route.

These samples do **not support adopting a preceding-tick-only sweep as the
contact fix**. They also do not prove the unsampled continuous path has no
intersection. Preserve the range/cone calibration gap and the closed coarse
capsule negative. Do not move the pulse, widen parry or invent a tolerance to
turn this measurement into a pass. The next bounded candidate needs a tighter
pose-aware eligibility/target representation, measured against the actual-mesh
oracle and moving output before adopting a gameplay change. No general combat
engine or runtime collision path is introduced here. Audio/haptic manual
revalidation for this test-only addition: **NO**; earlier changed-event gates
remain open.

Measurement base is documentation head `6e67be519442f07b07e76674cbeaf10f3ca20f2a`
plus the explicit test-file work in progress. Tested source-file SHA-256:
`af47c1f002af0b0239e8d01d88a1ff50188d9a86b5eec3664d636f5a2febb2d6`;
tested executable SHA-256:
`9ca90b08c64f9406864acb1ffa9c9c9f4d0040fb9ab43134b35cc597d8db7885`.
Final source-file SHA-256:
`fe0ac2a8d75c9f23348615761adabb2d5bf48cc44d4f555f217ded9e7d5dfb67`.
Two whitespace-only boundary fixes after the run were independently reconstructed
and hash-joined to the tested bytes; no expensive run is repeated for formatting.
The original receipt incorrectly labels runtime package base454 as its checkout
head. That receipt remains intact; the root-verified receipt corrects the base
and joins source/binary/56 rows explicitly.

An initial compiler attempt removed two existing `windupTrigger` declarations
through a broad replacement. Both were restored; the final diff only adds the
new diagnostic. The successful rebuild overwrote the original raw failed log;
a separate note preserves the known C2065 diagnostics and this evidence gap.
Private logs/receipt are `build/reports/combat-prepulse-bracket-{build,preflight,run}-20261008.log`,
`combat-prepulse-bracket-receipt-20261008.json`, the initial-failure note, and
`task-4/combat-prepulse-bracket-root-verified-private-20261008.json`.
Runtime454 packages, earlier phone acceptance and earlier measurements retain
their own identities.

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

### 8 October visible Windows waterfall sequence and owner findings

Sealed runtime `998137c94448b28bec1da3f6bf74f875ea26004f`, Windows Debug
executable SHA-256
`3a6af703d83dd27620421f22e7769a0c685673b59f3c1e3bd7fd881ce83d3513`,
on RTX5050 Laptop / real Pipeline, authored High/Current/Mist On/Dust Off.
The first new attempt again fails visible foreground arming (33.389 seconds,
PID62916, exit1, zero images and zero validation error markers). After the
owner explicitly confirms readiness, the retry arms through actual visible
foreground focus, exits normally in10.402 seconds and passes13 capture hashes
plus150 state/completed-RT rows. A requested replay also exits normally in
10.376 seconds with13 captures and zero synchronization-validation errors.
All owned processes are stopped; the failure is retained separately.

For the verified retry, draw milestones .25/.50/.75/1 and Windup/Attack/Parry
images join the exact state and completed-frame tick, correcting the earlier
Idle-image gap. The pair is visible to either side during ordinary approach.
This is a staged equipment-seed inspection, not production encounter/route
activation, sustained performance or complete world-body/shadow/reflection
validation. The owner requests review of apparent right-hand disappearance
while sheathed and parry hilt/hand interference with the torch; the blade itself
is not their current suspected intersection. They permit a small parry-only
torch move if needed, with portrait comfort emphasized. These visual findings
remain open; no owner sequence approval is inferred from harness completion.

### Sheathed right-arm regression and source fix

The imported arm remains in the mesh. A new actual-rig regression at the
waterfall inspection stance reproduces its out-of-view released Grip:
view-local (0.426668, -0.368569, 0.233188) metres, outside the narrow phone
frustum. Releasing the sword previously reduced right-arm IK weight to zero,
so the clip's rest pose controlled the hand.

The shared animation now retains right-arm IK, and the renderer blends the
hand target between the existing free carry and the animated mounted item Grip.
The sword's BodyStow/HandSocket ownership, fixed-tick attachment edge, combat
rules and ordinary torch carry remain unchanged. World body and dedicated
primary arms consume the same solved pose. The fixed regression measures
(0.18, -0.339998, 0.59999) metres in view, with the sword still mounted and
released. Existing full draw/sheath, actual sheath/body/floor clearance and
15 mm Grip assertions remain in force; their CPU reconstruction now consumes
the same arm-target blend as the renderer.

Affected local checks pass: player animation, item transition and socket,
actual skinned character, arm reference, semantic fixtures and viewmodel
admission/pose fixtures (eight unique checks). The initial socket fixture
failure is retained: it still overwrote the released arm with the mounted
item target; updating that reconstruction restores the existing release and
clearance assertions without weakening them. Windows Debug builds. This is
source/CPU evidence; a new sealed package and physical presentation/owner
review are pending. The owner removes the phone again while source work
continues; no device installs, tests or wait loops will run while absent.

Parry hilt/hand clearance remains a separate open finding. The first temporary
13.32 mm prop-distance result used a +0.28 look pitch rather than the captured
-0.04 pitch. The corrected pitch gives contact-scale unsigned prop distance,
which is not a signed mesh-intersection proof. The initial grip-point follow-up
also inverted the item Grip socket incorrectly. Both diagnostic mistakes are
excluded from acceptance; the corrected bounded probe and actual-IK candidate
will determine the adjustment. No parry timing/window changes follow.

### Parry hilt and gauntlet clearance candidate

Exact runtime `c2de5d7d2f8c25e4ed12152b9cd86fe53e2d6cd1` includes the preceding
sheathed-hand fix. Tree `f0a405dad1649e6c0b7423f5f09c58a62628fc22`. The new regression
uses the imported Rag torch, sword and actual production skinned sleeve/gauntlet
triangles at the captured stance (x 0.495964, z-15.143019, yaw-1.561293,
pitch-0.04, walk time 5.950023). It reproduces touching/intersecting surfaces
(0 mm separation) for both the guard region and right arm at high-carry parry
startup end, active and early recovery. The guard selection contains 5,304
hilt/guard/lower-blade-base triangles; the authored right sleeve/gauntlet contains
6,875. This is actual triangle/edge evidence, not an unsigned hand-centre proxy.

Normal-torch parry moves the shared right Grip x from-0.16 to 0.0 m and makes the
blade angle more upright (-0.62 to-0.42 rad). The existing IK moves hand, sleeve
and prop together. Torch carry, its Flame/Light sockets, lantern-specific parry,
attachment edges, combat timing and windows remain unchanged. No camera move or
off-hand offset is required by this candidate.

At the same ten high/low-carry phase samples, the uncapped query measures at
least 49.5863 mm guard-region separation and 69.8668 mm right-arm separation.
The retained CI fixture caps queries at 60 mm, reporting that value as a lower
bound; its 25 mm guard and 15 mm arm assertions remain stronger than contact-only
checks. It additionally tests peak successful-parry reaction for both carries
(12 cases). All pass; surfaces outside 60 mm are not reported as exact distances.
These discrete samples do not certify every interpolated pose or world obstacle.

Affected native checks pass 10/10 unique checks: item transition, full original
Rag torch low-ceiling matrix, animation, item sockets, development admission,
actual player rig, arm reference and semantic/viewmodel admission/pose fixtures.
The first run retains 9/10: the socket check still expected the old authored
parry x/angle. Updating those two exact expectations preserves all existing
left-hand, Grip, ceiling, scabbard/body/floor and transition assertions; the
corrected socket rerun passes 23.29 seconds. The original matrix passes 182.40
seconds. An initial new-admission compile error (string_view versus const char*)
is fixed and its log retained. Windows Debug and all four Android Debug ABIs
build; Java 183/183 in 32 classes passes with zero failures/errors/skips.

| Sealed inspection artifact | SHA-256 |
| --- | --- |
| Debug APK (138,462,724 bytes) | `b1a6c8af7b4983404b869c48dea856f01116dc03e4979aabd11b819194be3f57` |
| Windows Debug EXE (11,551,232 bytes) | `a918a77add10c45cade357980fc427751f8f7fb752ea25411b05071b50ca595e` |
| lib/arm 64-v 8 a/libhorde_rt_probe_android.so | `d2c31f65fded8d5da815f3c89dd50c7e2c7e7a57bc9d947fe25c55b08cea31f9` |
| lib/armeabi-v 7 a/libhorde_rt_probe_android.so | `857c9091894082ec10143266d8245f0f577bc205259af875e3819a6de6cc1c51` |
| lib/x 86/libhorde_rt_probe_android.so | `d976531ffa2affe77e6cb39ff7083e62b9b807ad5ea6610baa585a06e73b7907` |
| lib/x 86_64/libhorde_rt_probe_android.so | `f66e8f4fd6d69c203a24a2aae0f6b821b301b02c04440828f49ede46e3b24a5b` |

Closed Android asset admission, fullUser=13 and orientation|screenSize manifest,
and 16 KiB alignment pass. The APK is **not installed or phone-validated**; the
owner removes the device again. Prior 998137 c 9 phone evidence and preferences
are preserved and do not validate these new poses.

Eight frozen Windows captures on RTX 5050 Laptop (Pipeline and RayQueryCompute;
960 x 540 and 540 x 960) pass actual backend, completed/presented submission, native
PNG and CPU-uploaded viewmodel/world-body geometry hashes. Zero synchronization-
validation errors; all eight owned processes exit normally. Scene defaults stay
High water/fire, Current shadows, Glass On, Mist On; Dust is explicitly Off for
this inspection. The released right hand is visible, and parry hilt/hand-to-torch
separation is visible. The new parry fixture stages ordinary torch carry through
one real shared command, without the separate waterfall equipment seed. This is
not an exact replay of the earlier walk-time frame. Neither backend's frozen
stills imply live motion, scanout, phone-aspect acceptance, complete body/shadow/
reflection or sustained FPS. [Two unmodified portrait images and their hashes](evidence/2026-10-08-equipment-clearance/README.md).

Private immutable evidence: `integrated-162-c2de5d7d-20261007/`, including
`equipment-native-review-private-20261008.json`, audit SHA-256
`9380d12ea029ad2997689e06ca53e14a13e4d2fc1405e7bc098b4f4a6fb83ea0`.
No owned validation game remains running. Audio/haptic manual revalidation
required: **NO for these pose fixes**, since semantic events, listener/source,
cues, playback and haptic routing are unchanged. Earlier changed combat/audio
acceptance remains a separate gate.

Source CI passes all 12 aggregate jobs in [push 37704972028](https://github.com/Samfa12-tech/The-Horde-RT-demo/actions/runs/37704972028) /
[PR 37704977150](https://github.com/Samfa12-tech/The-Horde-RT-demo/actions/runs/37704977150). The preceding df 15 ea 97 hand-fix source separately passes
12/12 in push 37702888309 / PR 37702892974. Owner moving review, current phone
motion, production activation/full route and final body/secondary-view inspection
remain pending. The source fix is ready for review; it is not owner acceptance
or Eric's independent audit.
