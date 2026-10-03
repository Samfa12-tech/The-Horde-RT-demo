# Phase 3 owner live-play follow-up — 2026-09-27

This is an in-progress acceptance record, not approval of the modelled route.
The owner reports substantial improvement on SM-S948B, with three remaining
issues: sword/lantern intersection, a stiff lantern arm, and undersized hands.
Read-only ADB verified installed isolated Debug APK SHA-256
`51b257c0d50c8b5068a024a54a86ca9774c6050d873f522e1ec27ec48737efda`.
That feedback supersedes treating the eight frozen poses as visual acceptance.

## Sword / lantern separation: candidate gameplay fix

Proven cause: the previous shared sword grip curve used the same inward cut
with either light. Its hand X reached .01–.02 m while the blade leaned inward
.78–.82 radians, crossing the carried lantern's cage. This is independent of
the already-fixed signed-projection IK edge case.

`EvaluateHeldSwordPose` now reserves a right-side cut when carrying the reward
lantern: hand X stays .17–.18 m and maximum inward tilt is .22 radians. The
same gameplay pose drives the sword prop and right-arm IK. The lantern is not
moved, hidden or capture-special-cased. Vertical/depth travel, combat damage
windows and timing, and the ordinary torch arc remain unchanged.

Fresh host evidence:

- Neutral high/low carry: six complete attack phases, 61 samples each (732).
  The new `PlayerAnimationTests` cage-envelope regression failed before the
  change and passes after it.
- An independent Blender triangle-crossing check of the actual sword and
  lantern-body GLBs found 286/732 intersecting poses before, 0/732 after.
- A 1,800-tick shared-simulation sequence includes walking, looking, down/up
  combos, high/low transitions and the actual pendulum body transform. The
  same mesh check found 0/1,800 intersections after the change.
- Fresh Debug player animation, held-item socket and actual skinned smoke:
  3/3 pass. Shared simulation gameplay/timing: 2/2 pass.

Local investigation evidence is in ignored `reports/held-clearance-*` and
`reports/check-held-mesh-intersections.py`; the standalone sampler is at
`C:/Dev/tmp/horde-held-clearance-20260927`. These are not remotely backed-up
artifacts. The committed host regression preserves the neutral-cage control.
This mesh sweep is **not** a continuous GPU or phone result. It does not prove
skinned sleeve/arm separation or parry clearance. Live native RT motion and
owner acceptance remain open; no production-route switch is authorized by it.

## Lantern-arm stiffness: isolated mechanism and candidate

UV-correspondence recovery of the actual captured high-carry skin palette
measures left elbow flexion .612 degrees, reach fraction .99998595 and axial
chain stretch 1.43103. The current stretch policy makes an overreaching chain
only just long enough to reach the grip, leaving almost no elbow bend. A pole
change alone cannot restore compliance at that reach. Grip mounting and
chirality are separate: the corrected candidate has a true mirrored Left and
proper Right, with no second runtime reflection; cuff direction and wrist
continuity were checked against the uploaded vertices.

No renewed signed-projection fix, broad sleeve rewrite, grip-roll search or
weight sweep is justified by this finding. Preserve the secure shared grip
and independent world/view geometry while testing a bounded carry-pose change.

The candidate adds a gameplay-owned preferred elbow-flexion allowance for the
reward carry only: 22 degrees at rest, varying by at most 3 degrees with the
existing gait, 5 with pendulum forward angle and 2 with sword-hand lift. The
pole also follows bounded pendulum strafe. The actual chain keeps the existing
1.75 axial-stretch cap; cosine-law reach reserves the requested bend when
possible. Zero allowance preserves the old denominator exactly. Targets,
Grip basis, shoulder attachment, signed projection and gameplay lantern
physics are unchanged. This is modest additional axial stretching of the
existing rig (1.43103 to 1.45741 at high carry), not a claim of constant-length
anatomical bones or a repair of unsuitable sleeve topology.

Fresh Debug analytic fixture checks cover overreach at .8/1 m, multiple bend
requests, the unchanged cap at 2 m and invalid input rejection. Animation
contracts prove grip invariance, pendulum/sword response and clearing the
allowance outside reward carry. Both tests pass; rebuilt simulation timing,
gameplay and actual skinned smoke pass 3/3. Native high/low captures succeed;
UV correspondence on the new high-carry upload measures 21.99995-degree
flexion and wrist discontinuity below .0000002 m. An initially mistyped upward
checkpoint was rejected (exit 2); the corrected `player-viewmodel-upward-slice`
capture succeeds. Frozen images still do not certify natural live motion.

## Other requirements

- Hand proportion: one explicit +10% asset-scale candidate is being tested
  about the authored Grip origin. Grip-anchor invariance alone will not prove
  surface contact, cuff fit or owner acceptance.
- Android default scale 75% is implemented in `ed6e4bb`; existing saved choices
  and benchmark overrides are preserved. The phone currently stores 50%; no
  app-data reset or preference overwrite was performed. This is an owner-
  requested default, not a measured performance optimization.
- Fast render-scale resize investigation follows Phase 3 acceptance, with
  matched phone timing. It has not been implemented.
- Explicit look-down torso/legs and nonduplicating visibility remain open.
- Audio/haptic manual revalidation required: **NO** for these visual-only
  changes; event-time audio/haptic inputs and transport are unchanged.

## Hand-proportion candidate and reproducibility

Both processors expose a bounded `--gauntlet-scale` parameter (default .090,
allowed .080–.105); no-option exports reproduce the tracked world/view GLB
hashes exactly. One .099 candidate was generated with Blender 5.2.0 LTS
`fbe6228777e7`, single-threaded, without changing rolls, weights, UVs or sources:

```powershell
& 'C:/Program Files/Blender Foundation/Blender 5.2/blender.exe' --background --factory-startup --threads 1 --python tools/process-player-viewmodel-runtime.py -- C:/Dev/tmp/horde-gauntlet-proportion-20260927-a --gauntlet-source-hand Right --grip-roll-degrees 105 145 --blend-elbows --gauntlet-scale .099
```

Use a **new output directory** to reproduce, never overwrite previous evidence.
The paired world is `world-grip-calibrated.runtime.glb`, SHA-256
`baa5b7dc78a180eda94de813c71f890e46a27cc19b1d3122330ba873ec75c23a`;
viewmodel SHA-256 is
`6f06d77e754d7e2d9017b84c1204879aba2be302b69c077c5f84578408d5166b`.
Receipt `sourceWorldSha256` retains its established meaning (tracked source
authority); `pairedWorldSha256` identifies the candidate world.

Existing offline gauntlet geometry tests pass 6/6. Actual C++ viewmodel admission
smoke passes against the paired world (semantic/static-skinned/rig agreement).
The candidate retains 5,532 sleeve and 8,838 gauntlet triangles. Eight native
RTX portrait checkpoints succeed across `C:/Dev/tmp/horde-arm-followup-native-20260927-a`
(grips) and `-b` (remaining seven). A mistyped `forward-attack` name in the
first run was rejected; the existing `player-viewmodel-forward` was used on
rerun. Native image inspection does not replace phone motion/contact/owner
proportion checks. These are isolated assets, not a replacement of admitted
production GLBs or retirement of block arms.

## Phone and CI checkpoint

Installed APK `fce8b40c...` passed fresh 13-waypoint replay, eight modelled-route
captures and Home/resume on SM-S948B. A separate 45-second unfrozen recording
retains high/low attacks and carry/walk/look transitions. Reviewed sequences
show no visible sword/cage crossing, but subjective compliance/proportions and
complete Phase3 acceptance remain with the owner. [Exact artifact, evidence,
limitations and owner checks](evidence/2026-09-27-live-arm-followup/README.md).
Both runtime-source CI runs at `54493cf` pass (push36273603275/PR36273605905):
44 portable and11 Vulkan-enabled CPU tests, including the actual skinned
reference/smoke/semantic/viewmodel contracts. PR15 remains draft and mergeable.

## New owner lighting finding — open

The owner reports excessive yellow wash on the hands/sword, especially backs
of hands/arms expected to occlude lantern light. Treat this as a transport/
emitter/visibility investigation before final lantern and Phase3 acceptance,
not an art-direction request to darken gauntlet materials. First checks should
separate actual emitter placement and direct-shadow rays from indirect,
emissive and participating-medium contributions on the same native capture.

A targeted source check shows the modelled viewmodel is primary-only and the
world body owns secondary visibility (`0x10`); the normal light visibility mask
`0x35` includes that world body and excludes the viewmodel. This alone neither
proves missing occlusion nor certifies correct self-shadowing: compare actual
world/view surfaces and the light/shading paths. No shader was changed and no
cause is yet demonstrated. Preserve genuine RT visibility and useful failure
diagnostics; no scalar fake shadows, screen-space fixes or masking geometry.

### Follow-up: demonstrated lighting correction and latest owner feedback

The unoccluded post-fog bay-light colour term was isolated and removed in
`178b332`. Matched RTX capture geometry is byte-identical, the occluded hand
back loses the flat yellow floor, and the ordinary torch control remains
pixel-identical. Both shader backends were regenerated/validated; Shipping
retains zero diagnostic atomics/no binding22. Push36275529648 and PR36275533059
pass. [Full A/B evidence](evidence/2026-09-27-yellow-wash/README.md).
This correction is not yet installed on the phone.

The owner's subsequent test calls the phone candidate quite good and confirms
swings do not hit the lantern. Keep that as owner-confirmed swing separation.
New remaining details: low-carry parry contacts the arm; mirror swing penetrates
torso (lower priority); lantern size; open inner left bicep/armpit in low carry
while looking down; wrist join in high carry while looking up.

The requested size candidate uses scale **.44 instead of .50 (12% smaller)**.
This is the existing shared physical prop composition: ring/cage/dielectric
geometry and authored Flame/Light socket locations scale together in reveal,
inspection and carry; the GripRing remains anchored to the same hand socket.
It is not a camera-space shrink or benchmark-only change. Source GLBs, UVs,
light intensity and existing gameplay pendulum angular calibration are unchanged.
Performance comparisons must use matching geometry/scale; this owner-requested
size change is not evidence of an engine optimization.

Fresh Debug player-animation, production-prop asset and actual skinned smoke
pass3/3. Native high/low captures succeed in
`C:/Dev/tmp/horde-lantern-size-20260927-a`; the earlier .50 lighting-only images
provide the size control. Phone size/grasp acceptance remains open; do not
replace the installed candidate until the next coherent visual pass.

Read-only sleeve inspection identifies open shoulder and wrist contours despite
existing UV/PBR textures; ordinary backface culling is disabled. The sleeve
hem follows Arm/ForeArm while the gauntlet cuff follows Hand, so their seam is
a separate mount/topology issue. This is evidence for targeted investigation,
not final proof of the exact combined-pose defect. Capture low+maximum-down,
high+maximum-up and low parry explicitly next. Do not retry rejected all-sleeve
caps or broaden into an IK/roll/weight rewrite.
