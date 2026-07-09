# Meshy weapon staging

Meshy-generated weapon GLBs are staged here before runtime integration.

- Staged asset: `gothic_arming_sword_rh_v01.glb` with embedded PBR textures and 2K sidecar maps.
- Target: static first-person right-hand prop; bottom-origin pivot; PBR textures.
- Current mesh: 49,439 indexed triangles, which is above the Android RT target. Do not put it in the BLAS or render it until it has a reviewed remesh/LOD.
- Before runtime use, inspect scale, normals, UVs, texture links, materials, polygon count, and map dimensions.
- Record the exact Meshy task, prompts, account/license status, and imported date in `ASSET_LICENSES.md`.

This repository does not yet have a GLB/PBR asset-loading path, so staging an asset here does not make it render in-game.
