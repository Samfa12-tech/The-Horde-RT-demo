# Meshy enemy staging

The merged-animation skeleton GLB is staged here as source art only.

- Active source file: `skeleton_biped_merged_animations_v01.glb`.
- It contains a 24-joint rig and 11 named clips; use this file, not the separate static/bind-pose character output.
- The sword remains an independent weapon asset and must not be attached to the skeleton yet.
- Before runtime use, add a narrow GLB animation/PBR import path, inspect the 4K texture cost, build the enemy BLAS, and profile on laptop and phone.
- Keep the Meshy license gate in `ASSET_LICENSES.md` resolved before release.
