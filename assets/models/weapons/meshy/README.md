# Meshy weapon staging

Meshy-generated weapon GLBs are staged here before runtime integration.

- Staged asset: `gothic_arming_sword_rh_v01.glb` with embedded PBR textures and 2K sidecar maps.
- Target: static first-person right-hand prop; bottom-origin pivot; PBR textures.
- Current source mesh: 49,439 indexed triangles. The reviewed `gothic_arming_sword_rh_lod1.glb` is 12,358 triangles and is used only by the Debug `pbr-sword-closeup` loader/render proof.
- The audited derivative and validator/budget reports are under `runtime-development/`; packaged Release builds must exclude that directory and the staged source assets.
- Record the exact Meshy task, prompts, account/license status, and imported date in `ASSET_LICENSES.md`.

The generic GLB/PBR route is intentionally enabled only by the development checkpoint. It is not evidence that the unresolved Meshy account-plan licence is shippable.
