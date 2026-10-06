# E05 Bellkeeper bounded modular motion source trial

Status: editable modular source prototype. Not a combat-ready enemy, runtime integration, public release, or device-performance certification.

## Retained source-trial subset
- Idle: 3.0 s closed guard
- ExposureOpen: 0.65 s
- ExposureHold: 1.5 s loop
- RecoveryClose: 0.9 s

These are model/animation source candidates only. Exposure still requires future actual reflected-light mechanism authority; source keyframes do not implement it. Existing hidden armour/body fitting overlaps are disclosed in clearance-review.md. No collision-free claim is made.

## Explicit drafts and omissions
- Attack, 2.3 s: DRAFT. New right upper-arm / FixedHingeCleat.R.1 surface crossings at 0.80–0.90 s. Excluded from retained subset.
- Dead, 3.3 s: DRAFT. Knee surfaces remain 35.9–39.3 mm above ground at 1.8/3.3 s; visor/gorget crossing begins around 0.5 s and rear-skirt/gorget around 1.5 s. Excluded from retained subset.
- Walking: OMITTED. A heavy gait is still needed. Bundled original walk/run are preserved separately as reference motions; neither is relabeled or speed-scaled into an accepted H2 gait.

## Files
- E05_Bellkeeper_modular_motion_trial.blend: authoritative separate body, shell pieces, hinge helpers and separate maul, with six named editable source actions; Attack and Dead remain drafts as described above.
- E05_Bellkeeper_source_subset.glb: one mesh/two material primitives/one skin, with only Idle and the three exposure clips.
- E05_Bellkeeper_host_motion_trial.glb: six-clip host-test derivative, including the explicit Attack and Dead drafts. The adjacent admission manifest governs use.
- bellkeeper_short_maul.glb / .blend: separate original short-maul art proposal, not newly established lore.
- clearance-review.md and clearance-*.json: 27-image review and 127 evaluated-mesh samples.
- native-* and final-host-format-inspection.json: actual unchanged host-reader evidence; no native PBR render claim.

## Verified scope
11,904 body-plus-shell triangles; 33 joints; two material primitives; 1K body and shell maps; finite audited buffers; 60 Hz linear-sampled motions. All six clips load and CPU-skin in the unchanged native reader. Rigid helper witnesses match node/inverse-bind transforms below 0.001 mm. A unit-world RightGrip socket is checked numerically against the Blender source frame after explicit Blender +Z-shaft / glTF +Y-shaft conversion. Separate preview maul pose stays equivalent within 0.013 mm after socket correction.

The existing reader reads meshes[0]/skins[0], iterates primitives and records material names, but does not interpret their PBR image/factor records. Existing enemy rendering is not E05 PBR admission. No gameplay hit/parry/victory, reflected-light behavior, GPU/RT rendering, Windows/Android integration, or sustained device performance was tested.

Final .blend and GLB retain editable/provenance separation. Read RIGHTS.md: mixed generated/original content is not blanket CC0 or newly relicensed.
