# Low-carry parry contact — demonstrated pose-layer fix

Owner feedback on phone APK `fce8b40c...`: swings clear the lantern, but parry
contacts the low lantern arm. This native matched comparison reproduces and
removes actual geometry crossings. Phone/live acceptance of this change is
still required; this is not final Phase3 acceptance.

## Reproduction and change

New development checkpoint144 (`player-viewmodel-lantern-low-parry`) stages
one real shared parry command and freezes the ninth60Hz step, action
ParryActive at .11s. It retains low carry, yaw -pi/2 and pitch -.30, with the
same .099 gauntlet/world assets. Both captures use the corrected lighting and
.44 lantern scale. Baseline runtime is `b4b5c82`-equivalent; after differs only
by the shared reward-carry parry pose. See manifests for executable/module and
uploaded geometry hashes.

The prior guard put the right hand at view-X -.16, through the left arm. The
reward-carry guard now uses +.08 and a more upright blade (-.28 rather than
-.62 inward radians), retaining a similar tip blocking region. Startup and
recovery interpolate through the same authored evaluator; success-jolt timing,
Y/depth, combat windows and the ordinary torch guard remain unchanged. The
same pose drives the right hand/arm and sword. The left Grip is not displaced,
and no geometry is hidden.

## Exact uploaded-mesh evidence

The retained read-only Blender diagnostic checks all15,855 vertex/UV entries
and exact per-material triangle membership against the bound GLB. It recovers
RightGrip from5,610 rigid Hand samples, residual below1.6e-7m, then mounts the
actual sword mesh through its authored Grip (runtime socket error below5.4e-9m).
Both asset stages use viewmodel `6f06d77e...` and paired world `baa5b7dc...`.

| Strict triangle-pair crossings | Before | After |
| --- | ---: | ---: |
| Right sleeve / left sleeve |109|0|
| Right gauntlet / left sleeve |363|0|
| Right arm union / left sleeve |472|0|
| Sword / left sleeve |307|0|

The candidate has **zero BVH broadphase candidates**, not merely rejected
triangle intersections. Left/right sleeve partitions contain3076/2456 faces
(all5532 sleeve faces); the right hand contains4419 of8838 gauntlet faces.
Crossings require strict segment/triangle tests; tangent/coplanar overlap is
not classified by the narrow-phase test. `before.obj`/`after.obj` are exact CPU
upload records, not GPU readback. The native RT images provide the associated
rendered evidence; this helper is not an alternate rendering path.

Reproduce with Blender5.2, `--background --factory-startup --threads 1
--python-exit-code 1 --python <this-directory>/check-viewmodel-parry-intersections.py
-- <source-gauntlet.glb> <candidate-viewmodel.glb> <runtime-sword.glb> <before-or-after.obj>`.
Its sibling `trace-gauntlet-source-mount.py` is required. Candidate recipe/hashes
are in the [arm follow-up](../../ENGINEERING_1_6_1_LIVE_ARM_FOLLOWUP_2026-09-27.md).

The new shared-kinematics guard regression failed before and passes after,
covering high/low carry,61 samples in each parry phase and three success-reaction
times (1098 poses), with left Grip/basis invariance. Actual held-item socket
contracts and the focused animation/checkpoint tests pass. New Android capture
guards pass34 positive/negative cases; their addition preserves the old24 attack
cases. The integrated candidate has not been installed on the phone yet.

Still open: live high/low parry and transition acceptance, inner shoulder/arm
opening, wrist join, mirror torso penetration and explicit look-down body.
No broad IK, grip-roll or sleeve-weight tuning was performed. Audio/haptic
manual revalidation required: **NO**; feedback event-time inputs are unchanged.
