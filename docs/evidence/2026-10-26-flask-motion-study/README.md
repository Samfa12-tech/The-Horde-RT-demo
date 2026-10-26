# Flask equip/use/return - source candidate, unadmitted

[Editable source, intent and authoring tools](../../../assets/models/player/source/flask-use-study-r1/README.md)
now include complete and cancelled use actions on the unchanged player rig.
Original body/viewmodel/sword bytes, bind matrices and guard/combat source hashes
are retained. The original Idle/Walking assets stay intact. Production flask is
OFF; no gameplay, native binding, cap, texture, save or phone state changed.

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

Windows native visual execution remains blocked and deferred by the owner; no
retry/security change occurred. Phone stays with Reef. This checkpoint does not
complete flask controls/upgrade presentation, recovery-path or owner playtesting.
