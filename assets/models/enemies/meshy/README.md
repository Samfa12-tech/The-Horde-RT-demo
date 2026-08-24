# Meshy enemy assets

The current 1.5.2 release contains two sequential enemy GLBs. The opening encounter supports at most two simultaneous skeletons with two pose buckets; the later lich remains singular and never renders concurrently with them.

- Active source file: `skeleton_biped_merged_animations_v01.glb`.
- It contains a 24-joint rig and 11 named clips; use this file, not the separate static/bind-pose character output.
- The sword remains an independent weapon asset and must not be attached to the skeleton yet.
- The skeleton path samples `Idle_5`, `Walking`, `Attack`, and `Dead` on the CPU, refits the selected dynamic RT BLAS, and uses a procedural bone material. The embedded 4K texture is retained but not sampled.
- Preserve the Hotstrike/Meshy credit and the pending public raw-source/history permission gate in `ASSET_LICENSES.md`.

The final-room placeholder is active as `lich_placeholder_merged_animations_v01.glb`.

- It is a textured, animated 9,188-triangle Meshy lich with an embedded emissive texture and nine clips.
- It remains visibly placeholder source art: the robe, body, and staff are fused into one biped-skinned primitive with no staff or cloth bones, so staff bending and robe deformation are expected.
- The runtime selects it after the skylight gate but never renders/refits it concurrently with the skeletons. It passed `SM-S948B` device validation and remains packaged in 1.5.2; the exact signed 1.5.2 APK itself was not installed at publication. Its CC0 evidence is retained as `lich_placeholder_source_licence.png`.
- The shared reader has UV-bearing vertices and configurable clips. Presentation uses `Idle_02` and `Dead`; the distorted walking clip is deliberately avoided and whole-instance hover/orbit supplies movement. Its optional attack slot remains unmapped because the source contains no cast animation.
- Deterministic raw/ASTC texture derivatives and the selective emissive-mask audit preview are under `assets/textures/meshy/lich_placeholder_v01/`.
