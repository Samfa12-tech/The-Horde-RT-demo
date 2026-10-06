# E05 Bellkeeper: original body rig and bundled locomotion QA

## Recommendation

Keep the original rig as the sound body-rig starting point and preserve bundled walking/running as reference clips. Neither motion is the final H2 heavy rigid-cuirass guardian gait. Build the deliberate heavy walk and restrained, short-stride urgent movement on this skeleton; bind and review the rigid shell separately.

- Walk: recognizable natural, relaxed biped stroll. Broad alternating arm swing, fairly long stride and about 7.8 cm pelvis bob read light/ordinary for this role. Source cadence is 112.5 steps/minute (two steps per 1.066667-second cycle).
- Run: athletic sprint/jog mechanics, large bent-arm pump and very high trailing heel lift. Mesh foot minima reach 73–84 cm during swing. Source cadence is 180 steps/minute (two steps per 0.666667-second cycle). It is a poor unchanged match for the heavy guardian.
- Both close their source loops and remain in-place. Neither provides locked world-space feet independently of gameplay actor motion. Merely reducing actor speed would introduce skating.
- The original rigged_character clip is a static bind clip ending at 1/30 second, not a finished Idle performance.

## Rig and scale

- Original source is unchanged. Body is one mesh, one skin, one material: 7,071 exported vertices, 9,072 triangles and 24 joints. Input was 7,037 vertices with the same triangles; the rig operation introduced 34 seam splits.
- Native rest height is exactly 2 m; original positions span Y=0..2 m. Original Armature node is scaled 0.01. Blender consequently shows 0.01 world scale on the imported rig and body while actual deformed geometry is correctly 2 m tall. Do not apply another 0.01 to positions. A future unit-transform normalization must keep skin inverse binds and animation translations consistent.
- Float32 positions/normals/UV0/tangents/weights, uint8 joint indices and uint16 indices match the observed native reader. Four stored influences maximum, no zero-weight vertices, nonnegative weights, maximum weight-sum error 1.79e-7. No required compression extension, sparse accessor, or matrix-only node blocker.
- Named hierarchy: Hips; Spine02 / Spine01 / Spine / neck / Head / head_end / headfront; LeftShoulder / LeftArm / LeftForeArm / LeftHand; right equivalents; LeftUpLeg / LeftLeg / LeftFoot / LeftToeBase; right equivalents. No finger bones and no authored Grip sockets.
- Icosphere is an import-time Blender bone custom shape, used by all 24 pose bones, in glTF_not_exported. It is absent from the one-mesh GLB and excluded from this QA. Its helper bounds are recorded in blender-motion-metrics.json. Cube/Light/Camera in the initial diagnostic were Blender factory defaults; the actual QA uses an empty factory scene.

## Geometry, UV and PBR preservation

- Expanded triangle POSITION and TEXCOORD_0 are exactly equal in corner order to the accepted input. Paired position/UV values are exact, so there is no observed UV relocation or changed triangle geometry.
- All three embedded 1K PNG images (basecolor, tangent normal and ORM) are byte-for-byte identical to the accepted core. Material and texture-link JSON is also exactly preserved.
- Tangents were added by the provider; input did not contain them. Normals differ slightly, with maximum normal angle delta 0.344 degrees and max component difference 0.004895. This is not a detected severe PBR or geometry fault.
- Fresh Blender Cycles offline views show the preserved quilted cloth, boots/gloves and face details. This is offline PBR source evidence only.

## Root motion and feet

All lengths below are metres. Blender coordinates use Z up and -Y forward; native glTF uses Y up and +Z forward.

- Unchanged native host skin reader passes bind, walk and run. It samples all vertices at >=60 Hz, verifies finite posed positions/normals/UVs and reads named transforms.
- Native walk pelvis range XYZ: (0.064740, 0.078539, 0.053854); run: (0.020559, 0.079520, 0.083339). End-minus-start translation is effectively zero. Root is in-place but pelvis sway/bob is intentional within-cycle motion.
- Dense Blender samples show toe/foot endpoint closure within 0.5 micrometres. No accumulating translation is observed.
- Walk sole minima: left 18.86 mm, right 19.66 mm above the original floor. Run: left 22.83 mm, right 15.80 mm. There is a small source contact gap to address if preserving these clips; there is no sampled floor penetration.
- Contact-speed estimate uses the longest interval within 15 mm of each foot's lowest mesh point, then fits ToeBase backward travel. Approximate compensating actor speeds: walking 1.47–1.65 m/s, running 5.70–5.82 m/s. These are diagnostics, not selected game speeds. In-place stance sweep alone is not a rig defect.
- At fitted speed, toe forward-axis line-fit residual is ~5 mm left / 49 mm right for walk and ~19 mm left / 4 mm right for run. The right walk contact proxy spans the cycle boundary and its initial one-frame hold; toe rotation and contact choice affect these figures. Use foot-lock/contact evaluation with the actual actor velocity for final animation acceptance.

## Timing and visual evidence

The first source key is at 1/30 second, not zero. Native clip durations are maximum key time, so the source has an initial held pose from zero to the first key. Subtracting Blender action start from end would shorten walk/run by 1/30 second. All final metrics, quarter-cycle renders and movie samples use t=0 through native maximum key time.

- walk-quarter-cycle-contact-sheet.jpg: front and side at 0%, 25%, 50%, 75%, correctly labeled with source seconds.
- run-quarter-cycle-contact-sheet.jpg: same for running.
- bellkeeper-original-cadence-comparison.mp4: front/side, walk above run, 30 fps, 5.333333 seconds; five walk and eight run cycles. No retiming or shell.
- quarter-cycle-poses/: individual Cycles frames, including static bind front/side.
- cadence-video-receipt.json and cadence-video-ffprobe.json: authored timing and independently decoded media properties.

## Exact evidence

All outputs are within this original-rig-qa directory.

- rigged_character-inspect.json, walking_glb-inspect.json, running_glb-inspect.json: static GLB accessors, counts, materials, joints, animations, format gates, source hashes.
- input-core-inspect.json: reference static input. Its skinned runtime gate intentionally fails because it is an unrigged, unanimated source; this is not a new defect.
- pbr-uv-geometry-lineage.json: image hashes and input/output correspondence.
- skeleton-weights-normal-delta.json: hierarchy, per-bone vertex influence counts, normalization and normal change.
- rigged_character-native-probe.txt, walking_glb-native-probe.txt, running_glb-native-probe.txt: original host-reader outcomes.
- blender-motion-metrics.json: 121 samples per clip; only the actual skinned body is measured.
- foot-contact-root-summary.json: bounded contact proxy, loop closure and actor-speed estimates.
- native-probe-build.log, native_skin_probe and source-receipt.json: executable/source provenance.
- render_measure_bundled.py, render_original_cadence.py, compare_source_lineage.py and assemble_visual_evidence.py: reproducible QA scripts.

## Boundaries

No original source, shell source, engine source or Git state was modified. No paid calls. game-dev is not installed/on PATH, so its doctor/capability/validation workflow was not claimed; QA uses the inspected reusable Python/Blender tools and the unchanged native C++ skin reader. Native repository commit: e6cf2e97e5577aa051c5a18014477598762d7b75.

NOT TESTED: rigid-shell clearance and attachment, authored combat motions, held-tool grip, native GPU skinning, native RT material rendering/presentation, gameplay integration, device performance or package release certification. Original body importer success is not GPU/PBR runtime certification.
