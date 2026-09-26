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

## Lantern-arm stiffness: isolated mechanism, fix under investigation

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
