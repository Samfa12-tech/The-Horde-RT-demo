# Hand orientation investigation — not asset admission

**Latest owner correction:** the supplied source is a right hand. The earlier
left-handed identification is withdrawn. The new explicit Right-source export
mirrors the mesh for the torch's Left hand and preserves the source for Right;
it does not swap named bones. Earlier roll candidates are not accepted fixes and
must be rechecked with corrected chirality. See the final section below.

**Follow-up correction:** the large rectangular panels in the opening captures
are authored world gallery swatches and a wall mirror, not sleeve geometry.
The later isolation below supersedes that part of the initial visual attribution.
The measured CPU sleeve deformation and unresolved hand orientation are real,
separate findings; neither image interpretation alone admits the candidate.

The owner reported upside-down-looking hands and possible left/right reversal in
the Phase 3 images. This record follows checkpoint `8277df2`. The accepted world
and opt-in viewmodel GLBs remain unchanged; normal gameplay still uses block arms.

## Findings

- Named Left/Right chains and grip targets are not swapped. The model-to-world
  basis is a proper 180-degree Y rotation, not a reflection. The source gauntlet
  was previously classified as left-handed; that identification is now withdrawn
  following the owner's correction. The historical processor mirrored Right
  instead of Left. Do not
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

## Continuous elbow weights and world-space isolation follow-up

The new opt-in `--blend-elbows` field is a smoothstep centred on the actual elbow,
along the bisector of the bind upper/lower arm, with half-width one quarter of
the shorter bone. It changes only sleeve weights, not geometry, UVs, the rig,
gauntlets or gameplay authority. Two independent exports produce the same
viewmodel SHA `293dbce6ecbeddc47f2dfb42162c45763c8785f3dc274636ad14266fed15b0b6`.
The paired world remains `a08643bc...a12a91` (the prior Grip-roll experiment).

Compared with the preceding roll-plus-wrist-weight candidate:

| CPU sleeve metric | Grips, before → after | Raised lantern, before → after |
| --- | --- | --- |
| Maximum edge, metres | .09180 → .06333 | .09169 → .08432 |
| Maximum triangle stretch | 4.014 → 1.954 | 3.967 → 2.796 |
| Triangles over 3x | 2 → 0 | 23 → 0 |
| Negative geometric/supplied-normal alignment | 148 → 110 | 148 → 118 |

All 1,309 coincident-position sleeve vertex groups remain coincident after the
grips pose (maximum gap zero); UV/normal splits are not open cracks. Residual
normal disagreement and live-motion/art gates remain open. This is not a
performance improvement or final sleeve acceptance.

The Debug OBJ now records the exact row-major 3x4 model-to-world instance matrix,
with move/reset ownership and a nonidentity regression fixture. The grips matrix
is `[-1,0,0,-.002399676; 0,1,0,-.949749291; 0,0,-1,1.54908919]`.
Camera rays reconstructed from the unchanged `rt_frame.glsl` intersect gallery
swatches 3/4 at pixels (180,400)/(230,350), distances 1.809/1.992 m, and the right
wall mirror at (820,330), distance 2.571 m. They miss the viewmodel there. Actual
sleeve hits at (280,480)/(650,485) are about .482/.480 m away. This numerical
localization is joined to native isolation, not offered as a substitute renderer.

Temporarily masking the viewmodel preserved those world panels while removing
the actual arms. The normal capture gate correctly **failed** with zero primary
player pixels; its negative manifest is retained, not labelled a pass. A separate
chest-mask isolation also preserved the panels. All temporary mask overrides were
removed; the restored native grips PNG is byte-identical to the pre-isolation
image, SHA `3a04b82a2872f31f85baca48837da0954984ddbe2bb927fc8ccec5831ec0fe61`.
Do not remove or reshape scene swatches/mirrors as an arm fix.

An independent proposed cuff-axis sign flip did not reproduce in the actual
Blender processor: its reconstruction mixed glTF Y-up with Blender Z-up. A guard
rejected that experiment before any replacement export; the unsupported change
was removed. No anatomical axis flip is accepted from that hypothesis.

Current local roots: `C:/Dev/tmp/horde-elbow-blend-20260923-a` and
`horde-elbow-blend-repeat-20260923-b` (exports), `horde-elbow-blend-rt-20260923-a`
(two native poses), `horde-elbow-frame-restored-20260923-a` (restored frame evidence),
and `horde-no-viewmodel-isolation-20260923-a` (expected-negative isolation).
Restored Debug executable SHA is
`6cc233238b8d98ee48d512410e9f5dfcdf2ccd0658dccfdde4c74deaa1a06652`.
The focused Debug regression passes 1/1 after restoration (3.68 s).
The Release/Shipping native executable also rebuilds successfully. Six further
frozen native checkpoints completed and were visually inspected in
`C:/Dev/tmp/horde-elbow-matrix-20260923-a`, with unchanged masks/grip gates:

| Pose | Maximum triangle stretch | Negative sleeve normal alignment |
| --- | --- | --- |
| Forward | 1.382 | 0 |
| Downward cut | 2.143 | 121 |
| Upward slice | 1.898 | 61 |
| Look up | 1.766 | 0 |
| Look down | 1.954 | 110 |
| Lantern low | 2.839 | 118 |

Together with grips/raised lantern, none of the eight poses has a triangle over
3x stretch. This is a useful deformation improvement, **not** an art-quality
threshold or a waiver of remaining fold/normal/motion/physical-phone gates.

Checkpoint `f584be2` also has fresh successful branch/PR CI `35802962127` /
`35802965472`: logs show 44/44 portable and 10/10 Vulkan CPU-host tests in each.
Those jobs do not certify the subsequent source edits described here.

## Right-source correction and mirrored UV integrity

The owner explicitly identified the torch/source hand as Right. The corrected
candidate uses `--gauntlet-source-hand Right --blend-elbows`, with zero added
Grip roll so chirality is checked separately from the prior roll experiments.
Left is a real offline reflection of the right-hand source; Right is unmirrored.
Both retain scale 0.090 and rigid named Hand weights. No gameplay transform,
animation authority, source asset, accepted runtime or licence was changed.

The audit of this path also found that the old mirror reversed face winding but
left UV corners in their original order. Corrected exports reverse both together,
preserving each source vertex's texture coordinate. The pure geometry helper has
six passing regression cases, including Right→Left mapping, corner correspondence,
unchanged Right, malformed input rejection and explicitly isolated legacy output.
The portable CI job now runs those tests separately from its CTests.

World candidate SHA: `5d4e07a6f92e82b4cda35c528bfc15f50ed68a482cabecbfe55bee7f26a8661c`.
Viewmodel candidate SHA: `7d364d5ed0cb335f523796c2c7691527325e836ae04d1f869fe080454d504f13`.
Two independent exports reproduce both hashes exactly. Each first reproduces
and verifies the unchanged admitted world reference before making the candidate.
The corrected UV association reduces candidate unique gauntlet vertices from
18,527 to 11,220 while preserving all 8,838 triangles. Total viewmodel vertices
are 15,855 (sleeves remain 4,635). This is an actual deduplication result, not a
measured frame-time claim or reduced geometry quality.

Existing static/skinned addressing, shared pose/vertex/tangent/grip agreement and
invalid-pose rejection pass for the paired candidate. Native grips and upward-slice
captures complete on the unchanged Debug RTX executable. Their source is
`C:/Dev/tmp/horde-right-source-rt-20260923-a`; generated files are in
`C:/Dev/tmp/horde-right-source-20260923-a`. These checks do not establish final
orientation, sleeve shape, live motion or phone acceptance. Corrected source
classification does not make previous roll choices automatically valid.

The source's actual +X end was visually verified as the cuff with a labelled
Blender geometry preview (−X is the finger end). Do not invert cuff direction
based on an unlabelled extreme-vertex or hilt-removal boundary cluster. Blender
inspection remains separate from native RT evidence and owner acceptance.
The [corrected-chirality evidence](evidence/2026-09-23-hand-orientation/right-source-correction/)
retains the processor receipts, two native images/manifests, exact stage roster
and labelled source preview. No old failed experiment is relabelled an accepted fix.
