# Asset Licenses

The running RT scene uses the five Poly Haven material sets recorded below. Meshy model release gates remain separate and unresolved.

## Asset rules

- Every asset must be commercial-safe.
- Every asset must have a source URL or source note.
- Every asset must have a license recorded before use.
- Prefer glTF/GLB for models where practical.
- Use high-quality PBR textures when visual work begins.
- Meshy assets are allowed later, but Meshy models must be textured before export.
- Do not import untextured Meshy models and call them complete.
- Do not release a Meshy asset until the account plan and resulting license status have been confirmed below.

## Asset manifest

| Asset | Type | Source | License | Imported by | Notes |
|---|---|---|---|---|---|
| Gothic arming sword, player right-hand v01 | Source GLB plus 12,358-triangle LOD1; embedded PBR textures; 2K sidecar maps | Meshy-6 text-to-3D task `019f48d9-df47-7c7a-8341-20e8a11adb8b`; PBR refine task `019f48dd-bbe8-7d0c-9450-c1c13c0c7f06`; see `assets/models/weapons/meshy/gothic_arming_sword_rh_v01.METADATA.md` | **Pending account-plan verification.** Paid/private Meshy output may be commercially used under Meshy terms; free output requires CC BY 4.0 attribution. Do not ship until this is resolved. | Codex, 2026-07-10/11 | Source GLB is 49,439 triangles. `gothic_arming_sword_rh_lod1.glb` was welded and simplified with glTF Transform/meshoptimizer to 12,358 triangles while retaining its material and four embedded 2K maps. The live first-person right-hand sword is currently a tiny procedural metal proof; LOD1 waits for static GLB/PBR upload. The skeleton is unarmed. |
| Skeleton biped, merged animations v01 | Skinned GLB with 11 animation clips; embedded 4K texture | User-provided `Meshy_AI_SKM_Skeleton_Var_1_biped.zip`; merged animation GLB audited on 2026-07-10; see `assets/models/enemies/meshy/skeleton_biped_merged_animations_v01.METADATA.md` | **Pending account-plan verification.** Treat as the same Meshy license gate: paid/private output may be commercially used under Meshy terms; free output requires CC BY 4.0 attribution. Do not ship until confirmed. | User asset handoff, runtime-integrated by Codex, 2026-07-11 | One enemy is packaged with Android and uses the exact `Idle_5`, `Walking`, `Attack`, and `Dead` clips through CPU skinning and dynamic RT BLAS refit. The proof uses a procedural bone material; the embedded 4K texture is not uploaded. The sword remains separate and is not attached to `RightHand`. |
| Medieval Wall 02 | 1K diffuse, OpenGL normal, packed ARM | https://polyhaven.com/a/medieval_wall_02; Rob Tuytel | CC0 | Codex, 2026-07-12 | Dry corridor stone, runtime array layer 0. |
| Cobblestone Floor 08 | 1K diffuse, OpenGL normal, packed ARM | https://polyhaven.com/a/cobblestone_floor_08; Rob Tuytel | CC0 | Codex, 2026-07-12 | Wet stone floor base, runtime array layer 1. |
| Mossy Stone Wall | 1K diffuse, OpenGL normal, packed ARM | https://polyhaven.com/a/mossy_stone_wall; Amal Kumar | CC0 | Codex, 2026-07-12 | Mossed masonry, runtime array layer 2. |
| Damp Sand | 1K diffuse, OpenGL normal, packed ARM | https://polyhaven.com/a/damp_sand; eye-candy.xyz | CC0 | Codex, 2026-07-12 | Puddle-edge/damp-ground blend, runtime array layer 3. |
| Rusty Metal 04 | 1K diffuse, OpenGL normal, packed ARM | https://polyhaven.com/a/rusty_metal_04; Amal Kumar | CC0 | Codex, 2026-07-12 | Aged metal, runtime array layer 4. |

Android runtime derivatives for the five CC0 rows are strict KTX2 arrays using ASTC 6x6 for diffuse/AO-roughness-metal and ASTC 4x4 for normals. The retained raw RGBA arrays and original 1K JPGs remain the provenance/source chain; layer order is unchanged.

Poly Haven's asset license states that its assets are CC0 and may be used commercially without required attribution: https://polyhaven.com/license. Attribution is retained here as project provenance.
