# Upper-torso partition repair candidate

**Acceptance clarification:** the owner subsequently confirmed the existing
gameplay pitch range, not the extreme147/148 study, defines acceptance. The
material-partition fix is retained, but do not engineer torso/legs specifically
for the withdrawn near-vertical camera. Normal-range seam/visibility and live
checks remain required.

2026-09-27, parent `11ca3836ef3f6a87bce79daaa1af98abc5a0d028`.
Phase3 remains in progress. No production asset replacement or phone installation
is claimed by this offline/native slice.

## Proven defect and bounded repair

The old NearFace band masks connected shoulder/torso cloth between 79% and 86%
of the 1.8m source height. The deeper native view exposes that cut. A read-only
ray test through exact CPU-upload geometry finds 1,204 of 32,400 sampled rays
that miss the retained player but intersect this band. Skin-accessor inspection
also shows it is predominantly shoulder/arm/spine weighted, not face-owned.
These measurements localize a partition error; they do not prove all apparent
dark regions are holes or establish live-motion acceptance.

Opt-in `--retain-upper-torso` moves the old 1,116-triangle NearFace band into
BodyRemainder. The **entire previous head mask stays masked**: its lower hood/
neck below the authored Head origin (1.619081m) becomes NearFace, while its upper
part remains Head. This retains all five semantic roles without inventing a
dummy primitive, changing visibility declarations, or exposing the full body.
No camera-dependent cut or skin-weight threshold was introduced.

Actual GLB comparison proves unchanged expanded triangle positions, normals,
tangents, UVs, joints, weights and winding. Head1813 = newHead1345 + NearFace468;
newRemainder11592 = oldRemainder10476 + oldNearFace1116. Sleeve5532 and
gauntlet8838 triangles are exact; the viewmodel GLB remains byte-identical.
Shared gameplay pose/grip authority and independent GPU ownership are untouched.

## Reproduction and checks

From the repository, run Blender5.2 with `--background --threads 1
--python-exit-code 1 --python tools/process-player-viewmodel-runtime.py -- NEW_DIR
--gauntlet-source-hand Right --grip-roll-degrees 105 145 --blend-elbows
--gauntlet-scale .099 --reconcile-sleeve-seams --body-remainder --retain-upper-torso`.
Existing output directories are rejected. Source assets are not overwritten.
The no-option reference export in this run still reproduced the admitted world.

Candidate: `C:/Dev/tmp/horde-upper-torso-20260927-a`.
World SHA256 `58c8033850d9783c95c04e4040e015d1492d92437a6407642609633cd5bfa31b`;
view SHA256 `6f06d77e754d7e2d9017b84c1204879aba2be302b69c077c5f84578408d5166b`.

- 21 Python tests pass, including three new cutoff/invalid-landmark tests.
  The new helper tests run in CI. Actual artifact comparison is separately run
  with `tests/PlayerBodyPartitionArtifactCheck.py OLD_WORLD NEW_WORLD OLD_VIEW NEW_VIEW`.
- Actual C++ world static/skinned admission and paired world/view shared
  pose/vertex/tangent/grip tests pass.
- glTF validation: zero errors, one existing NODE_SKINNED_MESH_NON_ROOT warning.
- Four native RTX portrait captures147/148/145/146 pass existing gates. The
  low/down145 control remains exact PNG `93601b60...`. No image gate was weakened.
- All198 matched world/view boundary edges in low/down have zero measured posed
  separation. This does not certify every unmatched garment boundary.
- Native player pixels at147 rise from334600 to353746, consistent with retaining
  the measured cloth band. Deep lantern148 records2 glass transport overflows;
  preserve that separate open correctness gate. No performance claim.

## Remaining acceptance

The missing upper-body band is restored in native images, but the extreme downward
view still exposes a collar/interior appearance issue and feet are not clearly
readable while idle. Natural body occlusion is permitted. Do not call torso/legs,
wrist/armpit surfaces or continuous motion accepted from these captures alone.
Next use live walking/look/attack/parry/high-low checks on the paired candidate;
inspect whether moving legs are naturally visible before changing body mounting.
Preserve the owner's liked normal-view direction. The phone still holds
`bd9b348c...` / runtime259bd26 until a later recorded installation.

Audio/haptic manual revalidation required: **NO new check for this partition**;
geometry positions, event/source/listener data and playback are unchanged. The
prior anatomical-profile positional-feedback check remains open.
