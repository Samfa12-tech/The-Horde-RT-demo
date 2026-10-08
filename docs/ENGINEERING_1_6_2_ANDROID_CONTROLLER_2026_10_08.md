# Android controller and wider-view checkpoint, 8 October 2026

The owner's queue addition is implemented for host review on existing PR18.
Physical Android controller support is **not yet accepted**. The owner is away
with the phone: no ADB, installs, device waits or owner testing occurred in this
checkpoint. Continue the existing tomb finish goal and preserve prior results.

## Current integrated follow-up — 454d58ee

- Runtime source: `454d58ee0f71bdb837a75ce486d6c139e03e0b02`; tree
  `28d7b7467305821f7e1b31fa8a9f50893c334d25`.
- Windows Debug executable: 11,553,280 bytes; SHA-256
  `b538d71a559b2c9bec3ebd764e565f880220c02bcdf634b0fb63ecb897e9caec`.
- Four-ABI Debug APK: 138,462,724 bytes; SHA-256
  `12dfe45d1903bce5a6c11ee01a522603f2657d4a248b346e1ae3a7082f14e973`.
  Both packages were built at this exact source and sealed in
  `task-4/integrated-162-454d58ee-20261008`. No installation or phone test occurred.
- Native APK payload SHA-256: arm64-v8a
  `a64b35dc1caca9c0e37d5704636f6bc73ea7bd25fcb9ee8fedaa75fa025a9905`;
  armeabi-v7a `9c46a08ee0baa9b07919e11916f219ee2ee50e75a25ae2b3478ce75efdc0b26d`;
  x86 `d989f237f3b4a063a4d7146fba4cbaae4be15de423ce2062397aea837761e08d`;
  x86_64 `4724341a0296c22822202d82c11b52904577dbf79da064bb80f937c2f4bc5f7c`.
  All four native libraries changed after the shared pose extraction; all 91
  asset entries remain byte-identical to a6/c2. Manifest and SDK 37 16 KiB
  alignment checks pass with the same orientation/version settings.

A native dialog could previously keep polling a held navigation control after
its own Window lost focus. The activity's focus flag alone does not describe a
focused dialog. The follow-up tracks each owned dialog's actual Window focus,
gates key/joystick/poll delivery on it, neutralizes held input on loss and requires
fresh releases/neutral axes on return. Dismissal and parent focus restoration
preserve native callbacks, consent, graphics serial/generation guards and the
exact acknowledgement timer. Two new regressions fail against the a6 activity
and pass after the fix. Robolectric fixtures dispatch both View and
ViewTreeObserver focus notifications, matching the actual framework route;
physical Android Window delivery remains pending.

`SkeletonRenderPose.h` now owns the skeleton clip/time/root transform used by
both `CharacterRenderSlot` and the actual-mesh contact diagnostics. Removing the
diagnostic's copied mapping avoids a second animation convention. The sampled
old/new renderer comparison passes 12/12 cases within 1e-6; retained contracts
also cover zero/negative death duration and action/animation precedence. This
extraction preserves pose behavior and changes no combat range/cone, contact
pulse, parry window or damage rule. It supplies a reliable input for the remaining
moving-target calibration; it does not resolve that calibration by itself.

Current affected checks:

- **213 Android tests in 35 classes**, zero failures/errors/skips, and lint pass
  in `android-controller-dialog-full-tests-lint-20261008.log`. Four-ABI Debug
  assembly passes separately in 8 s; Windows Debug build passes.
- Shared pose edge contracts pass; the earlier full socket check passes in
  17.41 s. Renderer smoke passes in 4.88 s after correcting only its old parry
  expectation to the already-implemented torch-clearance pose.
- Test-only commit `03fe9ae209c4b6b540c5b8df183b22db99ba745b` registers the finite
  shared pose contract command in CTest. Its targeted CTest passes in 0.02 s.
  The two-line registration does not rebuild or relabel the sealed runtime.
- Eight fresh frozen captures from 454: both real RT backends, both recorded
  dimensions and both inspection poses. All eight PNGs, viewmodel meshes and
  player-world-body meshes are byte-identical to a6. Exact completed/presented
  RT submission joins pass, zero synchronization-validation markers, all eight
  owned process IDs verified absent. This retains the corrected a6 appearance,
  not continuous motion, complete secondary views or phone acceptance.

Retained failures include the initial attempt to compile the Vulkan renderer in
a portable fixture (missing `vulkan/vulkan.h`), the stale renderer smoke parry
expectation, and initial focus fixtures that omitted the Window observer event.
The authoritative corrected two-case baseline reproduces both defects against
a6; the focused 18-case and final 213-case fixed runs pass. No failed run is
relabeled. Exact private logs include
`android-controller-dialog-observer-corrected-before-20261008.log`,
`android-controller-dialog-focus-observer-after-20261008.log`,
`aspect-render-plan-shared-helper-equivalence-20261008.log`,
`aspect-render-pose-helper-edge-contracts-20261008.log`,
`aspect-render-pose-helper-edge-smoke-ctest-20261008.log`,
`skeleton-pose-contract-ctest-20261008.log`, and
`controller-pose-final-captures-20261008.log`; the seal contains the hash joins.

Runtime 454 push [37712231672](https://github.com/Samfa12-tech/The-Horde-RT-demo/actions/runs/37712231672)
and PR [37712236068](https://github.com/Samfa12-tech/The-Horde-RT-demo/actions/runs/37712236068)
completed successfully: **12/12 aggregate jobs** (push 6/6 and PR 6/6).
Test-registration 03fe has separate push 37712764483 / PR 37712770661 CI,
completed successfully at 12/12. Earlier documentation f000 completed 12/12.
Documentation 6e67be51 first attempt has 11/12 aggregate jobs successful: the
push MSVC native report-form Unicode edit times out at its existing two-second
deadline, while the matching PR job passes. Its single failed-job retry completes
successfully, giving 12/12 current aggregate jobs; the original failed JSON/log
remain preserved. Test-only 22920c19 separately passes 12/12 in push 37714363918 /
PR 37714368236. Documentation a65c03da first attempt passes 11/12 in push
37714583032 / PR 37714590338: the PR Windows job hits the same Unicode-note
timeout. Its original failure is preserved and one failed-job retry is pending.
No code or deadline is changed, and successful retries do not establish the
cause of these intermittent native UI failures. Results are not assigned to
other source or package identities. Build correctness, physical acceptance and
sustained performance remain separate.

## Earlier a6 candidate and preserved evidence

- Candidate source: `a6c19938994db95f9764b7fcf3eaa888811ecc1f`.
- Candidate tree: `4d19032a2ed55b2b4ce085a97699027a00a8c7dc`.
- Windows Debug executable: 11,553,280 bytes; SHA-256
  `fa5102bff6acfdbfdfdc2a2990fa3c8898b381d2b75369230eebd9a0d008f313`.
- Four-ABI Debug APK: 138,462,724 bytes; SHA-256
  `71eafd106f46ccc32ae6ca7c47c0da25dc177b3a254692158d7bd8bf906f5bda`.
- APK build source: `cb7c80f6c8d9a525594c088376617e2bc3b4b1d0`. It is reused byte-for-byte:
  the later candidate changes only Windows capture aspect delivery and its host
  regression. Android/runtime assets, shaders, shared gameplay and Android build
  inputs are unchanged between those sources. Do not label it rebuilt or installed.
- Native APK payload SHA-256: arm64-v8a
  `ab0e6d9f3cef1f02e0bd380bef70eac3a7b6619d1d963399d11d3ba4757276f0`;
  armeabi-v7a `6e5790a0007efc7cd00eb05e5b6bda45a7d43ecfac0c8a40e0e5dc0a59adaa97`;
  x86 `fa3035cde818189e2d78e1731b3c2fe18de16d96fadd79abfa70e47c5c8e3b90`;
  x86_64 `5a7527ebb27e1d6b0cee3ee6b0209b85bb5d5b7f9bba3d241ef6d27ecc642a3c`.
- All 91 APK asset entries are byte-identical to the earlier c2 candidate's
  closed asset admission. Manifest remains 1.6.2-debug/code 10, fullUser (13),
  orientation|screenSize (0x480). SDK 37 zipalign verifies 16 KiB alignment.
- The earlier `c2de5d7d` sheathed-hand/parry correction, its exact packages,
  ten native checks, 183 Java tests and 12/12 CI retain their identities.
  [Earlier equipment record](ENGINEERING_1_6_2_ANDROID_MOTION_2026_10_08.md#parry-hilt-and-gauntlet-clearance-candidate).
  Prior owner touch/torch/low-ceiling acceptance on `ccd70d38` and smaller-dust
  still acceptance on `998137c9` do not certify this new input or pose candidate.

Private artifacts and exact logs are retained under
`task-4/integrated-162-a6c19938-20261008` and the preceding cb7 seal. No release,
production signing, merge or independent final audit is claimed.

## Android input and native navigation

`AndroidControllerInput` maps standard Android controls rather than Windows
button numbers. A is Interact/Confirm, B Dodge/Back, X Swing, Y lantern,
L2 Parry, R2 Swing and Start/Menu Pause. Keys act on fresh press-down edges.
Digital and analog trigger reports share one latch; analog press/release
thresholds are 0.50/0.35. D-pad keys and hat input share navigation state.

Actual MotionRange bounds/flat values normalize sticks with a radial dead zone
of at least 0.12. Right Z/RZ is preferred, with RX/RY fallback. Neutral/drift
samples neither take over touch nor steal the active device. Four bounded
inactive candidates may become active only after neutral readiness and meaningful
input. Suspend/reconnect require fresh key releases and neutral axes; repeated,
stale and duplicate routes do not create a new action.

`MainActivity` publishes through the existing ProbeBridge coherent mailbox.
The existing shared 60 Hz simulation continues to own gameplay and timestamped
semantic edges. The UI callback integrates look into the same absolute view
controls, clamps delayed callbacks to 50 ms and resets its clock on suspension.
There is no second controller gameplay simulation.

Controller navigation uses real enabled native controls and callbacks, visible
focus and scroll-to-focus. Left/right adjusts SeekBar through its native user
change route. Native spinner/Graphics choices use focused labelled buttons;
Graphics uses the same presented option, serial, generation and busy guards.
Use/Keep/Restore and the confirmation starting after exact native acknowledgement
are unchanged. Native dialogs route keys/joystick navigation and restore a still
visible parent dialog after dismissal. Menu confirm cannot leak into combat.

Touch action/menu visuals hide only after meaningful controller use. Health,
required context and generic Android prompts remain subject to the saved Show HUD
and interface preferences. Connection alone never changes mode. An intentional
first touch restores actual hit targets before dispatch and neutralizes old axes
and held touch roles. No preferences are rewritten. Prompt names describe the
standard mapping; they do not identify or certify a Backbone model.

Focus loss, Home/pause, rotation and destruction neutralize axes, look and held
actions. Actual active-device removal also pauses gameplay and exposes touch
recovery. A device property-change callback reseeds state rather than treating
a still-present gamepad as disconnected. Native scroll pages resize within
existing insets in both landscape directions and portrait without rebuilding
focused controls or rewriting settings. Existing Vulkan pre-rotation/recovery
contracts remain in place.

## Shared equipment aspect input

`logicalViewAspect` comes from logical output extent and its presentation
transform, independently of render scale and platform. Square and portrait add
zero separation. Smooth interpolation reaches 3 cm outward per hand at 16:9 and
caps there. The offset enters shared held-item targets before anatomical reach,
forward/wall and overhead clearance; the arm IK, item Grip and light consume
those same solved targets. Released torch world trajectories are unchanged.

The owner-thread aspect setter refreshes presentation snapshots at zero delta,
including frozen/paused checkpoints. It does not advance gameplay, damage,
contact timing or reach rules. Host checks sample named idle, swing, upward slice
and parry action states at their default phase times; they are **not a full
animation-time sweep**. Existing bounded torch/wall/walk and sword-roof fixtures
also exercise wide views. Moving equipment and secondary views remain pending.

## Checks and retained failures

- Final Android run: **211 tests in 35 classes**, zero failures/errors/skips;
  `testDebugUnitTest`, `lintDebug`, four-ABI `assembleDebug` pass in 33 s.
  This includes 10 controller policy, 16 activity integration and two native
  navigation tests; existing touch, Graphics, lifecycle and settings regressions
  remain in the full roster.
- Policy coverage includes mappings, actual axis ranges/dead zones, repeats,
  digital/analog deduplication, D-pad/hat union, device/drift ownership and
  suspension rearming. Activity coverage includes move/look/actions, HUD/prefs,
  first-touch fallback, Start/pause, Entry/More/Settings/Play, Graphics, disabled
  controls, native dialogs, death/retry/ending, disconnect/reconnect, focus,
  rotation/page resize and keyboard separation. Native slider callbacks and
  scrolling/focus repair have focused tests. These are synthetic host checks.
- Three affected native CTest targets pass: held-item clearance (5.39 s), socket
  (22.16 s), Windows controller input (0.04 s), total 27.61 s. The Windows-only
  frozen-order regression and diagnostic/socket rebuild pass afterward.
- Eight fresh RTX 5050 Laptop frozen captures: Pipeline and RayQueryCompute,
  landscape 960 x 540 and portrait 540 x 960, sheathed and parry poses. Actual
  completed/presented RT frames, PNG hashes and both uploaded mesh hashes join;
  zero synchronization-validation markers, eight owned processes exited.
  Portrait PNG/viewmodel are byte-identical to cb7/c2; wide PNG/viewmodel change
  on both backends. [Original wide images](evidence/2026-10-08-controller-aspect/README.md).
- Earlier cb7 frozen captures were all byte-identical to c2. They correctly
  presented RT, but did **not** prove the new wide spacing: frozen Windows
  checkpoints skipped the live controls path that received aspect. Candidate
  `a6c19938` supplies aspect before checkpoint import and common frame snapshot
  consumption; fresh evidence above closes that delivery gap only.
- The first 10-case integration run had one assertion failure: native dialog
  click callbacks are posted asynchronously in Robolectric. Waiting for the
  main looper fixes the test; actual native callbacks remain unchanged. A later
  focused 10/10 and final 211/211 run pass; the failed log is retained.
- A first editing command used the Android directory for a source-relative file
  and failed before editing; its earlier build is not the final implementation
  proof. Capture comparison initially used an incorrect prior directory name
  and stopped before remaining launches; corrected paths and all eight captures
  pass. SDK 34 rejects `zipalign -P`; only SDK 37's successful check is alignment
  evidence. No failed attempt is relabelled as a pass.

Exact private logs: `android-controller-final-build-tests-20261008-02.log`,
`controller-aspect-native-ctest-20261008.log`,
`aspect-capture-followup-build-20261008.log`,
`aspect-capture-followup-test-20261008.log`,
`aspect-frozen-runtime-build-20261008.log`,
`aspect-fresh-wide-capture-20261008.log`,
`aspect-fresh-remaining-captures-20261008.log`, and the sealed
`artifacts.json` / `native-inspection.json` / per-case launch and native manifests.

Source `cb7c80f6` passes **12/12 aggregate CI jobs** in push
[37708858800](https://github.com/Samfa12-tech/The-Horde-RT-demo/actions/runs/37708858800)
and PR [37708865790](https://github.com/Samfa12-tech/The-Horde-RT-demo/actions/runs/37708865790).
Windows capture follow-up `a6c19938` has separate push
[37710046348](https://github.com/Samfa12-tech/The-Horde-RT-demo/actions/runs/37710046348)
and PR [37710051071](https://github.com/Samfa12-tech/The-Horde-RT-demo/actions/runs/37710051071)
CI: **12/12 aggregate jobs successful** (push 6/6 and PR 6/6).
Documentation `f000753d` has separate push 37710860642 / PR 37710864776 CI;
those runs completed successfully, 12/12 aggregate jobs.
Green CI is build correctness, not physical controller or owner acceptance.

Audio/haptic manual revalidation for this input/pose-only slice: **NO**; shared
feedback semantics are unchanged. Earlier changed combat/audio/haptic gates
remain open. This does not establish physical latency, controller comfort,
continuous geometry clearance or a performance result.

## Physical acceptance and final review

| Exact phone/controller checkpoint | Status |
| --- | --- |
| Phone model, Android version, controller identity and exact installed APK/pullback | Pending owner return and intended-device verification; no installation here |
| Cold launch, hot-plug, controller changes; neutral/drift behavior | Pending |
| Move/look/Swing/Parry/Dodge/Interact/lantern/Start together | Pending |
| Entry, More, Settings, sliders, scrolling, dialogs and full Graphics Use/Keep/Restore ACK route | Pending; synthetic routes do not prove Android Window delivery |
| Death/retry and ending/reward with clear prompts | Pending |
| Both landscape directions, portrait and rotation in menu/Graphics; large-font/cutout clarity | Pending; host geometry cannot certify actual presentation |
| Disconnect while moving/holding, safe pause, touch first-interaction fallback, reconnect | Pending |
| Home/resume and focus loss without stuck/replayed input | Pending |
| Idle/moving/swing/parry/low-roof/stow/draw equipment, full body/shadows/reflections | Pending current-candidate motion and owner review |

On return, coordinate one coherent hands-on matrix on the identified device and
controller; one Backbone model cannot certify all Backbone hardware. Keep final
mobile sustained thermals/performance/quality, dust motion/cost, production route
activation, owner audio/haptics and Eric's independent final audit separate.
Fresh Android remains 50%/Mobile/Glass Off/Current/cap30/Mist On; desktop remains
100%/High/Glass On/Current/cap30/Mist On. Custom preferences, historical 75% and
closed experiments remain intact. Dust stays optional/default Off; shafts deferred.

Recommendation: **keep this checked implementation for physical acceptance**,
retain the wider poses for moving review, and defer support certification and
production flags until their exact gates close. No merge, signing, tag or release.
