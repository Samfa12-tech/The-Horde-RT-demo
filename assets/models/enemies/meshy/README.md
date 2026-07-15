# Meshy enemy staging

The merged-animation skeleton GLB is the source art for the single animated enemy proof.

- Active source file: `skeleton_biped_merged_animations_v01.glb`.
- It contains a 24-joint rig and 11 named clips; use this file, not the separate static/bind-pose character output.
- The sword remains an independent weapon asset and must not be attached to the skeleton yet.
- The current narrow GLB import path samples `Idle_5` on the CPU, refits one dynamic RT BLAS, and uses a procedural bone material. The embedded 4K texture is intentionally not uploaded yet; inspect and budget it before adding texture sampling.
- Keep the Meshy license gate in `ASSET_LICENSES.md` resolved before release.

The proposed final-room replacement is staged as `lich_placeholder_merged_animations_v01.glb`.

- It is a textured, animated 9,188-triangle Meshy lich with an embedded emissive texture and nine clips.
- It is placeholder source art only: the robe, body, and staff are fused into one biped-skinned primitive with no staff or cloth bones, so staff bending and robe deformation are expected.
- It is not referenced by the runtime or release packaging. See `lich_placeholder_merged_animations_v01.METADATA.md` for the audit and integration gates.
