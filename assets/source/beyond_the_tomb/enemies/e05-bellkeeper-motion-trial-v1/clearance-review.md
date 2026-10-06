# E05 Bellkeeper bounded motion-trial QA

## Decision

Retain **Idle, ExposureOpen, ExposureHold and RecoveryClose** as the mechanically reviewed **source-trial subset**, subject to the existing fitting caveats below. Retain **Attack and Dead as drafts**. This review does not accept any clip for gameplay or establish engine readiness. No further art-polish loop is needed to close this bounded trial.

## Direct visual review

Inspected all 27 provided JPEGs in `../authored-v02/poses/`: front, side and oblique for Idle 0.50 s; Attack 0.65, 0.85 and 1.15 s; ExposureOpen 0.65 s; ExposureHold 0.75 s; RecoveryClose 0.90 s; Dead 1.80 and 3.30 s.

- The short maul follows the curled right hand in all inspected poses. Its shaft remains plausibly gripped. The frontal views sometimes superimpose the maul over the cuirass; side and oblique views resolve this as projection rather than gross maul/cuirass penetration.
- Idle has a coherent neutral armoured silhouette with feet planted. Cuirass, helmet and shoulder plates retain rigid shapes.
- The three Attack poses communicate raised windup, forward/down strike and lowered follow-through. No gross detached tool or obvious exposed plate breakthrough is visible in these renders. A small hidden hardware contact was found by mesh evaluation, so the full clip is not cleared.
- ExposureOpen and ExposureHold expose the front torso with shutters opening forward beside it. Arms and hands stay outside the door sweep in the inspected views. RecoveryClose ends back at the neutral pose.
- Dead's side views visibly show suspended knees with boot contact farther behind. It reads as an unsupported kneeling pose. The head droop also reduces the helmet/gorget space; mesh testing confirms new surface crossings.

## Evaluated-mesh evidence

`evaluated-clearance.json` records read-only Blender 4.3.2 evaluation of the original modular blend at 127 times: 10 Hz through all six clips, plus exact requested times and final frames. The source SHA-256 is recorded there. No source, assembly, engine or repository files were edited.

### Validated subset and mechanism checks

- Tool/shell: no sampled triangle-overlap candidates between the maul and any shell mesh in any of the six clips.
- Tool/body: sampled maul/body crossings are confined to body triangles dominated by RightHand weights, consistent with the intentional grip. No sampled maul contact was found on forearm, torso, legs or helmet.
- Grip stability: the maximum deviation of tool vertices relative to the RightGrip transform, converted back to world metres, is 0.0000006925 m, or about 0.00069 mm. This supports the v02 prop-follow fix at the sampled times. It does not approve v02's socket-scale format; any v03 format correction needs its own equivalence evidence.
- Shutter/body and shutter/tool: no sampled candidates throughout opening, holding or closing, including intervening 10 Hz poses. Shutter/hinge-cleat overlaps occur during rotation at the attachment area and are kept separate from limb-clearance findings.
- Idle and exposure sequence feet: both sole-region minima stay about 2 mm above the z=0 render ground, and foot-bone positions are unchanged throughout each of these clips. This is consistent with the supplied grounding offset and stable standing contact.
- Attack feet: supplied 60 Hz foot-bone samples show at most 2.08 mm fore/aft excursion on the left and 1.45 mm on the right; there is no gross standing-foot slide. Sole minima remain close to the 2 mm grounding offset.

### Attack: keep as draft

The right upper arm intersects `FixedHingeCleat.R.1` in sampled frames at 0.80, 0.85 and 0.90 s. There are respectively 10, 12 and 10 intersecting triangle pairs. None are present in Idle at 0 or 0.50 s, Attack 0.70 s, or Attack 1.00 s.

This is a new moving arm/hardware contact, unlike the pre-existing armour fitting overlaps. `refined-intersections.json` confirms all of these candidates by testing triangle edges against the opposing triangle in both directions, excluding segment endpoints within 1 micrometre. The contact is small and mostly obscured in the supplied full-body renders, but it prevents an unqualified clearance pass. Keep the current strike timing/shape as a proposal without repeating art work now.

### Dead: keep as draft

At both 1.80 and 3.30 s, the lowest vertices in fixed source knee neighborhoods are 39.3 mm above ground on the left and 35.9 mm on the right. The regions use vertices within 105 mm of the rest knee-bone heads and contain 39 and 53 vertices. The feet are lower: approximately 9.2 mm on the left and 2.0 mm on the right. These measurements corroborate the visible knee gap; they are not claims of whole-leg contact-patch analysis.

The supplied 60 Hz foot-bone trajectories also move backward by roughly 331 mm and 321 mm during the collapse. That movement alone would not reject a kneeling motion, but it does not establish grounded knee support.

New rigid-component crossings are confirmed by independent triangle-edge tests:

- Visor/gorget: first sampled crossing at 0.50 s; 52 triangle pairs at 1.80 s and 76 at 3.30 s.
- Rear helmet skirt/gorget: first sampled crossing at 1.50 s; 21 triangle pairs at 1.80 s and 27 at 3.30 s.

Both pairs have zero crossings in the tested Idle baseline. These are pose-induced helmet/neck-armour contacts, not static helmet construction seams. The death contact pose and head clearance need a later dedicated revision if this clip is pursued.

## Existing overlaps and limits

The baseline already contains hidden cuirass/body, gorget/body and shoulder/body surface intersections, plus component overlaps within the helmet, shoulder lames and attachment hardware. Some triangle-pair identities change under motion or numerical precision. Their presence means this is not a collision-free fitted assembly, and their counts must not be represented as new pose regressions merely because individual triangle IDs differ from Idle 0.

The four retained clips passed only the bounded visual/mechanism checks above: stable standing feet, coherent grip, shutter clearance from body/tool, no new cross-part helmet/gorget crossing. Existing rigid-shell fitting intersections remain a known source-model limitation. No volumetric penetration depths, continuous-time collision proof, locomotion gate, gameplay event timing, export/runtime contract or engine import claim is made here. No Walking clip is included or approved.

## Evidence files

- `evaluated-clearance.json`: all 127 sampled contact and mesh-pair results, source hash and method
- `refined-intersections.json`: independent edge/triangle confirmation for the new Attack and Dead contacts
- `evaluate_clearance.py` and `refine_flagged_intersections.py`: reproducible read-only Blender checks
- `evaluate.log` and `refine.log`: Blender run outputs
- `../authored-v02/poses/`: the 27 inspected images

The game-dev CLI was not on the validation environment's PATH. This is independent Blender/visual QA of existing bytes, not a game-dev package-policy validation receipt.
