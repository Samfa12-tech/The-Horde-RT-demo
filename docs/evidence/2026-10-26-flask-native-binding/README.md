# Explicit native flask binding candidate

The [separate editable candidate](../../../assets/models/player/source/flask-native-binding-r1/README.md)
preserves original geometry, bind/scale data and Idle/Walking, appending FlaskUse
(1.25 seconds) and FlaskCancel (0.566667 seconds). The latter starts only from
Use at 0.60 seconds; it does not support arbitrary cancellation. Original source
and runtime assets remain unchanged. The five-file source folder is excluded
from runtime packaging. Its GLB is 2,719,368 bytes, 150,860 bytes larger than the
original body, not an APK/GPU/resident-memory increase for normal play.

The first source/native comparison failed: Blender's quaternion-component
Bezier curves crossed hemispheres and produced up to 0.469 metres of subframe
hand error. A separate editable derivative uses continuous quaternion signs and
linear right-arm TRS curves, preserving every authored key. The already
shortest-path native GLB did not need changing. Actual native loader comparison
of 220 keys/midpoints then measured maximum position error below 0.001 millimetre;
both unsolved Grip matrices match named nodes exactly. Hips and left-hand values
are exact original Idle. The source receipt preserves hashes and both outcomes.

`PlayerRenderSlot::LoadUtilityCandidateAsset` explicitly binds the appended
clips; existing `LoadAsset` and all production callers retain the original
bindings. `PrepareUtilityCandidatePose` samples the supplied phase without a
renderer timer and uses one shared solved palette. Its initial stationary
whole-clip proof passed at `f1b029c7`; the right-arm-only follow-up below avoids
that proof's frozen-Idle limitation. Invalid clocks and conflicting combat,
dodge or grounded stance reject before cadence reuse.
Changing or clearing the request invalidates stale same/odd-tick data. Normal
requests preserve the existing no-copy input path and exact original vertices.

The native consumer test covers 18 Use/Cancel samples with a nonzero left-light
solve: right Grip error and left-solve difference are both zero. The unchanged
viewmodel skins that same solved palette. The flask attachment uses the actual
native prop Grip and its inverse exactly once. Its independent source prop-root
curve drifts by up to 26.77 mm between keys and must not drive carried placement.
This algebra does not prove hand/head/mesh contact or a visible join.

Debug utility/dodge/crouch rig tests pass (3/3, 30.16 seconds total, unchanged
individual deadlines). Release utility test passes (1.28 seconds). The Vulkan
CPU-host lane explicitly hydrates only this source candidate GLB and adds its
build/CTest entry; no existing test, timeout, compiler/platform coverage or
resource ceiling is removed or increased. Hosted exact-head CI is separate.

Normal flask remains OFF. This checkpoint does not wire a simulation caller,
grant healing, change saves/input/accepted guard transitions or admit the
fixed-seed Cancel as a general return. Remaining gates are phase-correct
arbitrary cancellation and entry/exit joins, the existing equipment lease before
equipment advance plus final post-damage readiness, actual RT hand/contact and
simultaneous ring/flask lifecycle/restoration, platform controls and physical
feel. No native game launch, Computer Use retry, phone access or security change.

## Right-arm-only target follow-up

Moving must cancel healing without freezing the player's walking body. A bounded
12-case native probe showed the existing arm solver can reuse the source Grip
and elbow targets over original Idle/Walking. It measured maximum Grip error
0.00047 mm, unchanged left-light solve/hips/23,900 non-right vertices and both
foot groups. Walking feet really moved relative to Idle. Observed chain stretch
was 1.0; this is evidence, not a new exact-1 intake rule. The initial deeply nested
MSVC configure failed with FileTracker FTK1011 before compiling; the unchanged
probe passed from a short staging build path. No security denial was retried.

The explicit candidate consumer now keeps actual Idle/Walking as its base.
It samples utility RightShoulder/RightForeArm/RightGrip, rebases at the current
locomotion RightShoulder, derives the elbow pole and supplies existing right IK
at weight1. It does not rebase at animated RightArm or introduce a new solver,
arm-length allowance, asset, second palette or skin pass. Normal production
callers remain unchanged. This is presentation, not permission to keep drinking
while moving.

The existing consumer test adds nine walking samples, reading actual joint
weights to identify every non-right vertex. Those bytes, hips and left solve
must match the original walking base; an Idle sensitivity control rejects frozen
legs. The unchanged viewmodel consumes the same solved pose. Debug passes in
3.44 seconds and Release in1.26 seconds. The compact
[probe receipt](right-arm-target-proof.json) retains exact inputs and all twelve
case results. Exact-head CI remains a separate checkpoint result.

The next small controller seam must retain the pre-cancellation Use sample before
`HealingFlask::Cancel()` clears it, cancel healing immediately, and let normal
movement continue while the right hand returns. A source-derived return can
reverse the lift toward0.30s, leave the pose-identical mouth hold into0.70s,
continue the existing belt return, and request owned sword Restore only after
flask release. This is a proposed path, not implemented or visually accepted.
Do not snap every cancellation to the fixed0.60s Cancel, restart an existing
Restore, advance equipment twice, or rearm guard/parry from the return. Actual
join/contact/cadence, interruption/save/load/reset and RT gates remain.

The subsequent [live owner checkpoint](live-owner.md) implements the bounded
phase-aware simulation/equipment seam behind a separate default-OFF gate. It
does not admit native scene joins or platform play.
