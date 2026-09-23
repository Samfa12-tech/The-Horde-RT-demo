# Hand orientation investigation — not asset admission

The owner reported upside-down-looking hands and possible left/right reversal in
the Phase 3 images. This record follows checkpoint `8277df2`. The accepted world
and opt-in viewmodel GLBs remain unchanged; normal gameplay still uses block arms.

## Findings

- Named Left/Right chains and grip targets are not swapped. The model-to-world
  basis is a proper 180-degree Y rotation, not a reflection. The source gauntlet
  is anatomically left-handed despite its historical `right-` filenames; the
  processor explicitly mirrors the right copy and reverses its winding. Do not
  repair an apparent reversal by swapping bones or changing gameplay item poses.
- Player-side inspection of the exact posed gauntlets shows a bad cuff direction.
  A controlled 180-degree roll of both authored Grip bones changes that direction
  while retaining the grip origins, local handle axes and gameplay prop authority.
  This is evidence for a grip-frame roll mismatch, **not** final anatomical or
  visual acceptance of that calibration. Thumb/palm/cuff orientation still needs
  a stronger semantic mount contract and inspection throughout live motion.
- The gauntlets remain rigid: maximum triangle-edge stretch is about 1.0001,
  with no degenerate faces or negative geometric/supplied-normal alignment in the
  two measured poses. Sleeves already deform badly in the exact CPU upload,
  before GPU vertex decoding. UV/normal split vertices are not proof of holes:
  position-welding the bind sleeves gives two physical components.
- Roll correction alone makes sleeve stretching worse. Moving only the dedicated
  viewmodel sleeve Hand weights into the same-side ForeArm reduces that problem,
  but the combined native RT images still contain unacceptable broad/angular
  panels. Neither experiment is admitted. Remaining extreme triangles are at
  Arm/ForeArm blends, not Hand weights. Bind normals have zero negative alignment;
  the combined posed sleeves have 148/5,532 negative alignments in each pose.
  These are geometry diagnostics, not a complete explanation of final pixels.
- OBJ coordinates are model-space. Comparing them directly with the world-space
  capture camera is invalid; near-camera causality remains open until the exact
  instance transform is applied. No such distance claim is accepted here.

## Controlled experiments

All native shots use one Debug Diagnostic/High executable, SHA-256
`78d5d5d7e2ba6621b86dc27f28d37dc9cbc7dd8205c8ca4a7cbb1776d0c271fa`,
RTX 5050 Laptop GPU, RayTracingPipeline, 960x540 at 100%, twelve settling frames,
fixed animation time zero and raw RT storage pixels without overlays. No shader,
resolution, gameplay grip transform or physical transport change was made.
These frozen captures are not live-motion, Shipping-performance or phone evidence.

| Sleeve result | Original grips / raised lantern | Roll only | Roll plus sleeve weights |
| --- | --- | --- | --- |
| Maximum triangle edge, metres | .12324 / .13709 | .20223 / .20337 | .09180 / .09169 |
| Triangles with maximum-edge stretch over 3x bind | 33 / 72 | 93 / 120 | 2 / 23 |
| Maximum stretch ratio | 5.183 / 5.168 | 6.366 / 5.855 | 4.014 / 3.967 |

The metric is per-triangle maximum posed edge divided by its maximum bind edge,
not a material quality or image acceptance score. The paired GLBs pass the
existing addressing/shared-pose/vertex/tangent/grip/invalid-pose admission smoke;
that test does not certify anatomy or attractive sleeves.

Candidate identities:

- Original world: `e8737f10e7669b284e04109d9c3acdf537df284093a21511656bf191a70450fd`.
- Original viewmodel: `1e3b041ee7aa896a84fe462c182f6012d026b4a577b2d4ddf59d764b823f2538`.
- Roll-calibrated world: `a08643bc0d798c185b826e5079b1d6ef53e616ce5fb0076c3babc505f1a12a91`.
- Roll-only viewmodel: `b8ee4d1e8e93d821ea0af4db1a7af076975820e67c7ef43e14e03a13a30ec709`.
- Combined viewmodel: `f3c1c1c718c07fcc467e325cdeccfa9a721bd171e92f614cfc311304d27ac416`.

The roll-only export was repeated with identical world/view hashes. Both flags are
opt-in investigation tools in `tools/process-player-viewmodel-runtime.py`; outputs
must be new directories. The processor first reproduces and verifies the admitted
world reference. Grip changes require the paired world and view rig because their
shared pose contract includes inverse binds. Sleeve reweighting happens **after**
the paired world export and changes only the independent viewmodel.

Local candidate/capture roots (not release artifacts):

- `C:/Dev/tmp/horde-grip-calibration-20260923-a` and `-b`: repeat roll-only exports.
- `C:/Dev/tmp/horde-grip-sleeve-calibration-20260923-a`: combined export.
- `C:/Dev/tmp/horde-viewmodel-weight-ab-20260923-baseline`: matched original shots.
- `C:/Dev/tmp/horde-grip-calibration-rt-20260923-a`: roll-only grips/raised lantern;
  `C:/Dev/tmp/horde-grip-calibration-rt-20260923-matrix`: other six frozen poses.
- `C:/Dev/tmp/horde-grip-sleeve-calibration-rt-20260923-a`: combined two-pose shots.

The [retained evidence](evidence/2026-09-23-hand-orientation/) contains the two
combined native images/manifests, stage receipt, comparisons and processor receipt.
OBJ geometry remains local, with exact SHA-256 in each native manifest; it is
reproducible from the recorded source/candidates, not a GPU readback claim.

## Diagnostic seam and next gate

Debug viewmodel captures now emit the exact current CPU-upload positions, normals,
UVs, source-order indices and named material groups as OBJ, with a hash in the
native manifest. This does not solve another pose or add a Shipping readback path.
Non-current/missing uploads and existing output paths fail explicitly.

Fresh validation: Debug native build and Release/Shipping native build succeeded;
the Debug resource-inventory/capture regression passed 1/1 (3.70 seconds), including
exact attributes, primitive-local reordered indices and refusal to overwrite.
Its first run caught a Windows trailing-separator error in the test's temporary
directory guard; that fixture was corrected and rerun. The default processor
re-export reproduced the original viewmodel SHA above. Both retained native
PNG/OBJ pairs match their manifest hashes. These focused checks do not replace
the eventual complete candidate matrix or certify the unadmitted art.

Next: finish anatomical grip calibration and sleeve deformation/shape with real
geometry, then inspect the eight poses and live transitions. Keep image tolerances,
gameplay grip authority and independent world/viewmodel ownership unchanged.
Android candidate packaging, native-device validation and owner phone acceptance
remain open. No asset replacement, paid generation, licence change, phone install,
signing or publication occurred in this investigation.

Audio/haptic manual revalidation required: NO; playback and semantic inputs are unchanged.
