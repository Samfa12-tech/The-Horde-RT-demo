# E01 Abbey attendant: corrected hammer grip (v2)

The maintenance mallet was held backwards: its head exited the little-finger side of the fist. The corrected RightGrip reverses the handle axis by 180 degrees about its local X axis, keeping the grip origin and head-width axis fixed. The head now exits the thumb side.

The delivered source had a death-only wrist twist fitted to the backwards hammer. That ramp is reduced from -45 degrees to -25 degrees around RightHand local Y, with its original smooth timing from 20% to 55% of Dead. This is the only changed deform-joint animation. It avoids both floor and skull contact after correcting the hammer direction.

## Preserved

- Same body geometry, skin weights, texture bytes, materials and 25-joint skeleton.
- Same separately authored 516-triangle mallet. No model or animation regeneration.
- Same core clips and timing: Idle 4.0 s; Walking 1.066667 s; Attack 1.5 s; Dead 2.966667 s.
- Same Idle, Walking and Attack deform animation channels. Extras remain source-only; their grip direction is corrected consistently.
- The fixed Blender source retains all eight actions. The compact package includes the four-clip body GLB, editable all-action Blender source and separate mallet.

## Attachment convention

Use the corrected body's RightGrip directly with the unchanged mallet's identity Grip, in glTF coordinates. Do not add another 180-degree flip in game code. glTF mallet +Y points toward its head; head width is X. RightGrip origin is unchanged. Preserve the rig's existing 0.01 scale and the helper's compensating scale.

For Blender fresh import, use bone_heuristic='BLENDER'. To display the separately imported mallet, its Blender matrix is armature.matrix_world @ RightGrip.pose_matrix @ RotationX(-90 degrees). The editable assembly already uses the appropriate local mesh conversion and constraint.

## Review and verification

The comparison film renders the original and corrected final GLBs at identical clip times. The corrected film combines a full-body view and tracked grip close-up. Playback is half-speed; clips have hard cuts. Multi-angle stills examine the fist, backswing, strike/recovery and final fall.

See validation-summary.json for the exact final hashes and measured checks. The final exact GLB is independently checked at 60 Hz across every core clip. Triangle-crossing tests exclude intentional gripping-hand contact. They are discrete surface tests, not continuous collision detection or a promise of perfect hand geometry.

These are source/native-reader assets. Game integration, actual gameplay animation blending, GPU skinning, Vulkan RT rendering and device performance have not been tested. Existing small floor variation of body surfaces is not corrected by this grip-only revision.

## Preservation and rights

The original v1 delivery remains preserved in earlier Git commits; the current source paths contain v2. RIGHTS.md, PRODUCTION_PROVENANCE.json and MALLET_PROVENANCE.json retain the source attribution. No paid generation, game-code change, runtime release or licence change occurred in this correction. This source-only revision is archived on the existing planning branch.

The old v1 pose images, clearance receipts and source hashes describe the backwards-grip version only. They must not be reused as evidence for v2; the new validation summary and manifest supersede them for the revised files.
