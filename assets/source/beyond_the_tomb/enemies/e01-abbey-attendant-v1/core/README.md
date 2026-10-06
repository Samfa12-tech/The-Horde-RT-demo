# E01 attendant: hand orientation correction (v3)

This revision corrects the two hands seen in the v2 review. It supersedes the v2 hand-orientation and thumb-side claims.

## What was wrong

The generated rest pose has open palms facing upward. The earlier animation preparation added 30 degrees of axial roll to each forearm and another 30 degrees to each hand, mirrored on the left. In the delivered Idle pose this put the palms outward and the thumbs backward. The hand geometry itself is not mirrored. Removing those additions exposes a palm-forward pose; it does not by itself produce a relaxed palm-inward stance.

The v2 mallet reversal was also based on misidentifying the thumb. Multi-angle views now identify the broad thumb-index web and all four separate curled fingers. In v2, the head extended from the little-finger side. That reversal is undone.

## What changed

- Replaced the original right forearm/hand +30-degree additions with -45 degrees each, and left -30-degree additions with +45 degrees each. This is an approximately 150-degree net change from v2, distributed across the forearm and hand. A literal 180-degree hand-joint flip was tested and rejected because it visibly pinched the wrists.
- Restored the mallet socket's original thumb-side direction in both its static transform and every animated clip. The unchanged mallet follows RightGrip directly; do not add a runtime flip.
- Removed the obsolete death-only right-wrist axial correction.
- Added a smooth 35-degree left-wrist extension during 40%–70% of Dead so the corrected open fingers do not enter the floor.
- Restored Dead floor contact with at most 9.461 mm of upward root compensation. No timing, horizontal root trajectory or other body animation channels changed.

## Preserved

The body geometry, triangle indices, normals, tangents, UVs, skin weights, embedded textures, materials, skeleton and separately authored mallet are unchanged. No paid generation, finger-bone addition or engine code change was used. The core clips remain Idle (4.0 s), Walking (1.066667 s), Attack (1.5 s) and Dead (2.966667 s). All eight actions remain in the editable Blender source; the four extra actions are source-only and are not covered by the core-clip motion acceptance.

## Files and attachment convention

- E01_AbbeyAttendant_core.glb: body, rig and four core animations.
- E01_AbbeyAttendant_editable.blend: editable body, all eight actions and constrained mallet assembly.
- E01_maintenance_mallet.glb: unchanged separate static tool.
- E01_Hands_Before_After_v3.jpg: identical Idle timing/cameras for both versions.
- E01_Hand_Orientation_Contact_Sheet_v3.jpg: sampled views from the final GLB.
- evidence/: source-level checks and review scope.

Use RightGrip directly with the mallet's identity Grip in glTF coordinates. Mallet +Y points toward its head; X is the head width. Keep the existing rig 0.01 scale and helper compensation. For Blender fresh import, use bone_heuristic='BLENDER'. The separately imported mallet matrix is armature.matrix_world @ RightGrip.pose_matrix @ RotationX(-90 degrees). The saved Blender assembly already handles this conversion.

## Verification and honest limits

The exact final GLB was freshly imported for multi-angle hand/wrist inspection and full-clip motion rendering. The original engine's standalone native CPU reader evaluated every core clip at 60 Hz. Blender and GLB evaluated positions reconcile within 0.014 mm across 22 sample poses; socket matrices also reconcile. Geometry and skin data are byte-identical to v2. Joint transform determinants stay positive, with no non-finite vertices or newly zero-area tested hand/forearm triangles.

The mallet head has no body-triangle crossings at the sampled 60 Hz frames. The existing rigid grip still has intentional palm/handle contact. One long proximal-palm triangle, which blends into the wrist, touches the handle in part of Walking and briefly in Dead (up to three triangle-pair crossings). It is included in the conservative non-hand count because one corner has under 50% RightHand weight; this is not hidden or treated as a full collision pass. It does not involve the mallet head. See clearance and contact-detail evidence.

The open left hand remains rigid-fingered because this rig has no finger joints. In the final death pose the fingers sit slightly raised; this is visually plausible but less relaxed than a hand-authored finger animation. The repair corrects orientation, not the generated model's entire anatomical detail.

Discrete source checks do not establish continuous collision freedom, gameplay blending, GPU skinning, Vulkan/RT presentation or target-device performance. Those runtime stages have not been run. The game-dev packaging CLI is absent in this cloud environment; the supplied manifest, direct GLB checks and native CPU proof are manual evidence, not a game-dev canonical-package receipt.

## Provenance

RIGHTS.md, PRODUCTION_PROVENANCE.json and MALLET_PROVENANCE.json preserve the earlier source attribution. REVISION_PROVENANCE.json records this correction and final hashes. Earlier v2 review claims about the thumb and hammer direction are superseded. Original inputs remain untouched. The owner visually approved v3 for source archival. This approval does not imply runtime acceptance.
