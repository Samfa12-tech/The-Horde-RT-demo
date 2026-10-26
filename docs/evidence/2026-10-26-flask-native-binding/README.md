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
renderer timer, retains authored right-arm motion and applies the existing
left-light IK to one shared solved palette. It rejects invalid clocks and
conflicting movement, combat, dodge or grounded stance before cadence reuse.
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
