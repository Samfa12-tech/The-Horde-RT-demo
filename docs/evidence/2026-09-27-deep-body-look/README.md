# Bounded anatomical camera look: implementation, not body acceptance

**Superseded by owner clarification on 2026-09-27:** the near-vertical diagnostic
extension is not the first-person acceptance boundary. Restore the established
gameplay range; retain this evidence only as an investigation record. No further
geometry work should target rendering the camera-inside-body extreme itself.

Parent `66f1149225c3cbb7547d2f7ad4c5cf2e5b2d52af`; the accompanying commit
contains this source slice. The phone-tested anatomical mounting/profile and
unchanged candidate meshes remain the working direction accepted as looking good
by the owner. No new asset, grip roll, weights, stretch, light position, held-item
placement or primary/secondary ownership is introduced here.

## Change and evidence

The old pitch parameter is the vertical component of the camera forward vector,
not a true Euler angle. Its -.32 minimum looks only about 20 degrees down. The
explicit AnatomicalBody profile now permits -4.0 (about 76 degrees down); Legacy
retains -.32. Touch, controller, mouse, JNI publication, simulation construction,
reset and live movement agree. Physical held-pose evaluation still uses its
existing [-.32, .28] range. Host tests prove that deeper look leaves the exact
low lantern, sword, IK targets and held-light transforms unchanged.

New development checkpoints 147/148 require AnatomicalBody. Both shared staging
and Android request resolution reject incompatible profiles; the latter checks
immutable configuration, not simulation state from the JNI thread. This prevents
an accepted request from labelling a rejected stage as a successful deep capture.

The previously shader-unused push-constant float at byte 68 is repurposed from
heldPropDepth to minimumCameraPitch. The packed size stays 128 bytes and all other
offsets stay fixed. All pipeline, compute and compatibility SPIR-V modules were
regenerated. Do not mix old modules with the new CPU declaration. Legacy camera
bob retains its old clamp; no instance flag or per-pixel metadata lookup is added.

- Windows build succeeds. Four affected CTests pass (animation, development
  checkpoints, simulation, controller); two additional ABI/render-slot tests pass.
- Native RTX 5050 Laptop GPU portrait captures 147/148 complete with genuine
  RayTracingPipeline presentation, matching requested/actual pitch and dedicated
  world-remainder/viewmodel masks. Images and manifests are retained here.
- Ordinary anatomical checkpoint145 remains byte-identical to the liked mounting
  baseline: PNG `93601b600892f1be8e666446a75bb2345bc693a19b76e6e96aca2796fb442e1f`.
- The matched four-region Legacy control also remains byte-identical:
  `6682ff63384c0e147fad79944434ea249577e74a64c02daf6bb8bcb40ac668ba`.
- Android ARM64 Debug builds, with Mobile pipeline and compute modules contained
  in the actual packaged ELF; spirv-val/disassembly checks pass. Windows executable
  containment/validation also passes. These do not prove backend image parity.
- Frozen shader budgets are unchanged. Pipeline Diagnostic generic/opaque sizes:
  226988/505272 bytes; Shipping: 219208/501036 bytes. Compute Shipping:
  219460/501296 bytes. Shipping variants retain zero diagnostic atomics/binding22.
  This is a camera/ABI change, not a measured performance improvement.
- A serialized read-only catalog check passes against a fresh eight-key shader
  compilation; the earlier concurrent attempt is excluded as described below.

The retained precommit Android artifact is
`C:/Dev/tmp/horde-deep-look-android-20260927-d/HordeLanternRT-1.6.1-viewmodel-debug-arm64.apk`,
SHA-256 `3cce3c6115a247c3e35b88049b5af57b6ba4ae968093c976d6803822957b93b7`.
It is **not installed**. The owner-liked phone APK remains `bd9b348c...` / runtime
259bd26. Earlier -1.5 exploration was too shallow for useful body inspection and
was not installed either. One malformed shell Gradle argument was corrected;
the subsequent actual build passed. A read-only catalog check was invalidated by
concurrent evidence-file creation and must not be counted as a passed check.

## Visual finding and remaining gates

The new view exposes upper-torso surfaces and substantial self-occlusion. It
does **not** establish acceptable torso/legs or closed collar/armpit surfaces.
Feet are not clearly readable in these images. The scene-only captures deliberately
retain that result rather than hiding geometry, moving props for the camera, or
loosening an existing image gate. Natural physical occlusion itself is permitted.
The unchanged view/world geometry and shared pose isolate this as a newly exposed
body-surface/visibility question, not evidence that the accepted arm mounting or
the already-fixed signed-projection IK must be retuned.

Next: inspect the near-face/body boundary against exact posed geometry, fix only
a demonstrated surface/partition defect, then validate continuous deep/shallow
look, walking, attacks/parries and high/low lantern transitions on SM-S948B.
Do not replace the owner-liked installed candidate just to exhibit an already
known unfinished torso. Phase3 remains open; frozen captures are not live-motion
or owner acceptance. Glass correctness/Shipping timing and the rest of 1.6.1
remain separate open gates.

Audio/haptic manual revalidation required: **NO additional check for this camera
slice**: event-time listeners still use unchanged X/Z/yaw, while physical held
transforms retain their clamp. The prior anatomical-profile positional-feedback
check remains open and is not waived.
