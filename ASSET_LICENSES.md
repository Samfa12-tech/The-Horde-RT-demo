# Asset Licenses

Two external assets are staged for future integration. The current running Phase 1C scene remains procedural and does not yet load either asset or any third-party texture set.

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
| Gothic arming sword, right-hand v01 | GLB with embedded PBR textures; 2K sidecar maps | Meshy-6 text-to-3D task `019f48d9-df47-7c7a-8341-20e8a11adb8b`; PBR refine task `019f48dd-bbe8-7d0c-9450-c1c13c0c7f06`; see `assets/models/weapons/meshy/gothic_arming_sword_rh_v01.METADATA.md` | **Pending account-plan verification.** Paid/private Meshy output may be commercially used under Meshy terms; free output requires CC BY 4.0 attribution. Do not ship until this is resolved. | Codex, 2026-07-10 | Staged only; not runtime-integrated. GLB verified: 1 mesh, 1 material, 4 embedded PBR textures, 49,439 indexed triangles. This exceeds the intended Android RT budget and needs remesh/LOD before use. |
| Skeleton biped, merged animations v01 | Skinned GLB with 11 animation clips; embedded 4K texture | User-provided `Meshy_AI_SKM_Skeleton_Var_1_biped.zip`; merged animation GLB audited on 2026-07-10; see `assets/models/enemies/meshy/skeleton_biped_merged_animations_v01.METADATA.md` | **Pending account-plan verification.** Treat as the same Meshy license gate: paid/private output may be commercially used under Meshy terms; free output requires CC BY 4.0 attribution. Do not ship until confirmed. | User asset handoff, staged by Codex, 2026-07-10 | 24-joint rig, 9,402 indexed triangles, 11 correctly named clips. Staged only; not runtime-integrated. The sword remains separate and is not attached to `RightHand`. |
