# Meshy enemy staging

The merged-animation skeleton GLB is the source art for the single animated enemy proof.

- Active source file: `skeleton_biped_merged_animations_v01.glb`.
- It contains a 24-joint rig and 11 named clips; use this file, not the separate static/bind-pose character output.
- The sword remains an independent weapon asset and must not be attached to the skeleton yet.
- The current narrow GLB import path samples `Idle_5` on the CPU, refits one dynamic RT BLAS, and uses a procedural bone material. The embedded 4K texture is intentionally not uploaded yet; inspect and budget it before adding texture sampling.
- Keep the Meshy license gate in `ASSET_LICENSES.md` resolved before release.

The final-room placeholder is active as `lich_placeholder_merged_animations_v01.glb`.

- It is a textured, animated 9,188-triangle Meshy lich with an embedded emissive texture and nine clips.
- It remains visibly placeholder source art: the robe, body, and staff are fused into one biped-skinned primitive with no staff or cloth bones, so staff bending and robe deformation are expected.
- The runtime selects it after the skylight gate, but never renders/refits it concurrently with the skeleton. The Android debug APK packages it for compile continuity; phone validation remains pending. Its CC0 licence evidence is retained as `lich_placeholder_source_licence.png`.
- The shared skinned-character reader now has a configurable lich clip set (`Idle_02`, `Walking`, and `Dead`) plus a separate 48-byte position/normal/UV std430 record. Its optional attack slot remains deliberately unmapped because the source contains no cast animation.
- Deterministic raw/ASTC texture derivatives and the selective emissive-mask audit preview are under `assets/textures/meshy/lich_placeholder_v01/`.
