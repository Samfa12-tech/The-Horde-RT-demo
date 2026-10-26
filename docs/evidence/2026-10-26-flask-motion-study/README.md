# Flask equip/use/return - source candidate, unadmitted

[Editable source, intent and authoring tools](../../../assets/models/player/source/flask-use-study-r1/README.md)
now include complete and cancelled use actions on the unchanged player rig.
Original body/viewmodel/sword bytes, bind matrices and guard/combat source hashes
are retained. The original Idle/Walking assets stay intact. Production flask is
OFF; no gameplay, native binding, cap, texture, save or phone state changed.

The exact source checkpoint `d51c815e07aee70609180a4c2c8acb90b4ea569c`
completed [CI run 40309715446](https://github.com/Samfa12-tech/The-Horde-RT-demo/actions/runs/40309715446)
with all six jobs successful. This covers the hosted checks for that source
head; it does not close the native flask presentation or device gates below.

## What was checked

- A complete 1.25-second source action: sword stow, flask pickup/lift, brief swig,
  flask return and sword restore. Existing Stow/Restore durations remain 0.30/0.36.
- A source cancellation at 0.60 seconds, returning the flask over 0.20 seconds
  and restoring the sword over 0.36, with no gameplay side effects.
- Reloaded saved actions, 76 complete and 71 cancellation samples. Coupled grip/
  item transforms and identical start/end right-arm bases pass the playback
  consistency checks. Left-hand matrix difference is zero; original arm lengths
  are retained. These numerical checks do not establish naturalness or skin fit.
- The r2 flask retains 242 vertices/384 triangles, one primitive/material and
  no texture. r1 still regenerates byte-identically; r2 static checks and Khronos
  validation pass with zero errors/warnings and one expected `Grip` information.
- The final four completed-action source contact witnesses show no triangle intersection between
  the selected head surfaces and flask or gauntlet. This is limited source
  evidence, not cancellation contact or full swept/body/renderer collision acceptance.

## Follow-up: complete and cancellation sampled contact

The [expanded receipt](sampled-contact-sweep.json) covers both unchanged saved
actions at every 60 Hz key sample and its midpoint: 151 complete observations
from 0 through 1.25 seconds, and 141 cancellation observations from 0 through
1.166667 seconds. All 292 observations show zero intersections of the 1,813
selected head triangles with the flask or 8,838 selected gauntlet triangles.
The original four witnesses reproduce exactly. Rechecking the retained rejected
short-neck Blend reproduces its original overlap counts, including 16 flask
and 120 gauntlet/head overlaps at the three mouth witnesses. This verifies that
the expanded observer did not simply stop detecting the old failure. Its old
lip distances are not compared with the newer r2 mouth reference.

The local Blender 5.2 background runs completed successfully, without rendering
new frames or writing source assets. Original body, viewmodel, sword, rig,
combat and saved Blend hashes were checked unchanged. The existing cancellation
body frames at 0.80 and 1.00 seconds were inspected for context; these are source
Workbench views, not the native light/guard solve. No authored pose, input rule,
gameplay timer, resource capacity or production default changed in this follow-up.

This is discrete selected-region evidence, **not continuous collision detection
or whole-body/prop contact**. It does not establish native first-person framing,
left-light/sword joins, moving-camera/world clearance, reflection/shadow parity,
or any device acceptance. The source mouth reference's nearest measured distance
is reported as an observation, not promoted to a new clearance tolerance.
Native integration and the specific joint resource decision remain open.

The initial authoring assembly selected a glTF helper mesh instead of the
skinned viewmodel; it was corrected before review. Blender display-bone axes
also proved unsuitable for the body stow mount, so the original raw glTF Hips
frame is used. Straight carrying passed too close to the shoulder's minimum
folded reach; an outward source arc preserves real limb lengths instead.

The first short-neck lip fit intersected the head/gauntlet. Its failed receipt
is retained. One original longer-neck derivative plus a side approach preserves
body/Grip positions, radii and counts while clearing the selected source contact
witnesses. This does not erase the failures or imply native admission.

## Authoring previews only

The MP4s are **silent Blender Workbench animation previews**, 960x540, encoded
at the intended 60 Hz source timeline. Rendering was unpaced. They are not Horde
gameplay captures, measured FPS, RT images, audio or controller acceptance.
No in-between video frames or playback speed change is applied. Inclusive endpoint samples make
the 76-frame complete file 1.266667 seconds (last source sample 1.25); the
71-frame cancellation file is 1.183333 seconds (last sample 1.166667).

[Complete source preview](complete-source-preview.mp4) ·
[Cancellation source preview](cancel-source-preview.mp4)

![Source drinking pose](drink.png)

The body key poses and decoded video frames were inspected. The candidate is a
brisk swig within the approved clock; natural feel and native first-person
framing remain provisional. Do not present this source study as an admitted
gameplay feature or public showcase clip.

## Smallest integration seam, after resource approval

The subsequent [source-only utility lease/clock proof](../2026-10-26-flask-utility-preparation/README.md)
preserves production bindings and confirms that the retained0.89s return cue
misses sword readiness by one60Hz tick in the explicit caller composition.
The proposed53/60s cue meets tick75 only if separately retimed hand art actually
releases the flask then. Current art remains unchanged; do not treat that test
cue or the lease as admitted native integration.

The GLBs supply Idle/Walking, while accepted attack/parry/guard hands are solved
by shared runtime IK. Appending a Blender action cannot replace that authority.
Keep original callers/poses. Reuse the existing immutable snapshot boundary:

1. Derive utility presentation from authoritative flask action/elapsed and
   equipment ownership in `GameSimulation`; rendering grants no healing or timer.
   Current `FlaskContextFor` rejects every active sword transition. It must
   distinguish this action's owned stow/restore from unrelated transitions,
   preserving all other eligibility/cancel checks rather than removing the gate.
2. Feed the appropriate right-hand grip/item basis through shared
   `HeldItemKinematics`/`PlayerAnimationState` and the immutable frame adapter.
   Preserve the left-light solve and original sword/guard poses. Never fabricate
   fresh parry windows, simultaneous mitigation, buffered attacks or readiness.
   Healing/charge commit stays at the authoritative 1.25-second completion;
   animation reaching the mouth early must not consume resources or award health.
   Prove native 60 Hz handoff scheduling and sword readiness by tick 75; source
   continuous phase edges such as 0.89 are not assumed to be runtime tick edges.
   Resolve targets in the current body/head frame while looking, not frozen
   world coordinates or a second camera/platform animation timer.
3. The joint warning28/flask29 resource admission remains pending. Import and own
   valid assets before presentation; preserve stowed sword participation, current
   capacities until approved, retention correctness and one-frame ownership.
   Native world/viewmodel joins, actual camera aspects, wall/ceiling sweep,
   reflection/shadow, pause/death/load/reset and input cancellation gates remain.

At this source checkpoint, automated Windows visual execution was blocked and
phone custody remained with Reef. Later, Sam manually opened the unchanged
forest candidate; the running-window inventory confirmed its exact path, while
Computer Use window selection timed out without producing a screenshot or app
binding. See the [updated launch evidence](../2026-10-26-windows-app-control/README.md).
That manual forest launch does not validate this source-only flask candidate.
Reef subsequently released the phone to Horde, but this follow-up made no
device calls. No retry/security change occurred. Flask controls/upgrade
presentation, native recovery-path and owner playtesting remain incomplete.
