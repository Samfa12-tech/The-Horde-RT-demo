# Meshy enemy staging

The merged-animation skeleton GLB is the source art for the single animated enemy proof.

- Active source file: `skeleton_biped_merged_animations_v01.glb`.
- It contains a 24-joint rig and 11 named clips; use this file, not the separate static/bind-pose character output.
- The sword remains an independent weapon asset and must not be attached to the skeleton yet.
- The current narrow GLB import path samples `Idle_5` on the CPU, refits one dynamic RT BLAS, and uses a procedural bone material. The embedded 4K texture is intentionally not uploaded yet; inspect and budget it before adding texture sampling.
- Keep the Meshy license gate in `ASSET_LICENSES.md` resolved before release.
