# Skeleton biped, merged animations v01 - asset audit

## Source and selected file

- User-provided archive: `C:\Users\sam_s\Downloads\Meshy_AI_SKM_Skeleton_Var_1_biped.zip`
- Selected source inside the archive: `Meshy_AI_SKM_Skeleton_Var_1_biped_Meshy_AI_Meshy_Merged_Animations.glb`
- Excluded companion file: `Meshy_AI_SKM_Skeleton_Var_1_biped_Character_output.glb`; it only contains a short bind/static clip (`Armature|clip0|baselayer`).
- Staged destination: `skeleton_biped_merged_animations_v01.glb`

## Verified GLB content

- glTF 2.0 / GLB v2; one skinned mesh primitive, 9,854 position vertices, 9,402 indexed triangles.
- 24-joint armature including `RightHand` (node 12); no weapon is attached.
- One double-sided material with an embedded 4,096 x 4,096 PNG.
- The delivered material uses that image as base color and emissive. It has scalar roughness (`0.4101`) and a specular extension, but no normal, metallic-roughness, or AO maps. It is HD-textured, not a complete conventional PBR map set.

## Animation clips

| Clip | Duration |
|---|---:|
| `Attack` | 2.800 s |
| `Dead` | 2.967 s |
| `Idle_5` | 1.867 s |
| `Idle_Turn_Left` | 0.933 s |
| `Idle_Turn_Right` | 1.100 s |
| `Running` | 0.633 s |
| `Walk_Backward_While_Shooting` | 1.267 s |
| `Walk_Backward` | 0.900 s |
| `Walk_Turn_Left` | 2.033 s |
| `Walk_Turn_Right` | 2.200 s |
| `Walking` | 1.033 s |

The names match the Meshy UI screenshot. The two backward-walk labels were merely truncated in the UI; no clip-name repair is required.

## Integration and release gates

- Build a GLB animation/PBR import path and a dynamic-skinned RT BLAS/TLAS path before attempting runtime rendering.
- Keep the sword separate until an explicit attachment and animation pass is requested.
- Confirm the Meshy account plan and update `ASSET_LICENSES.md` before public distribution.
