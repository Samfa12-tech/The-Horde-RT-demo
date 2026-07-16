# Lich placeholder, merged animations v01 - asset audit

## Status and provenance

- User-supplied file: `Meshy_AI_Meshy_Merged_Animations.glb`.
- Staged destination: `lich_placeholder_merged_animations_v01.glb`.
- Role: active placeholder for the coloured-light route's final emissive enemy in the Windows-first showcase goal. It is included in the debug Android compile package but has not been device-validated.
- Source size: 14,940,452 bytes.
- SHA-256: `049979A83ACA55358F54AF8D3AF1F7D518607BEF634474A4EF015BFDFF947A42`.
- glTF 2.0 / GLB v2, exported by Blender glTF I/O 4.2.57.

## Verified content

- One skinned mesh named `char1`, one triangle primitive.
- 11,873 position vertices, 27,564 indices, and 9,188 triangles.
- Approximate bounds: 1.25 m wide, 1.81 m tall, and 1.08 m deep.
- Vertex data includes position, normal, UV, joints, and weights. There are no morph targets.
- One 24-joint skin named `Armature` using a conventional biped hierarchy.
- One double-sided alpha-blended material named `Material_1`.
- Two separately encoded embedded 2,048 x 2,048 RGBA PNGs whose decoded pixels are identical (canonical top-left RGBA SHA-256 `5765579E0CE081C3BD2CDC62DF0900C7E92FAC679CB53DE9F1F6CA527CA464A1`). The alleged emissive image is therefore not a usable authored emissive map.
- The material declares an emissive texture with factor `[1, 1, 1]`. It has no normal, metallic-roughness, or occlusion maps. Its specular extension contains the unusual colour factor `[2, 2, 2]`, which must be normalised or deliberately remapped during runtime import.

`tools/prepare-lich-textures.ps1` extracts the embedded base colour and derives a deliberately selective violet mask for the bright staff crystal and eye/gem candidates. Windows uses raw RGBA8 KTX2; Android selects strict ASTC 6x6 KTX2. The runtime UV audit identifies 40 emissive staff-crystal vertices and averages their animated positions, so the bounded violet light follows the real skin weights despite the missing staff bone.

## Animation clips

| Clip | Duration |
|---|---:|
| `Dead` | 2.967 s |
| `Fall_Dead_from_Abdominal_Injury` | 3.500 s |
| `Fall_Down` | 4.700 s |
| `Head_Hold_in_Pain` | 3.733 s |
| `Headache_Relief` | 4.833 s |
| `Idle_02` | 2.333 s |
| `Running` | 0.633 s |
| `Walk_Turn_Right` | 2.200 s |
| `Walking` | 1.033 s |

All nine clips key translation, rotation, and scale on all 24 joints. There is no dedicated attack, cast, or staff animation.

## Known placeholder limitations

- The character, robe, and staff are fused into one skinned primitive.
- There is no staff bone, rigid accessory node, separate staff mesh, cloth/skirt bone, or cloth morph target.
- The staff is therefore likely weighted to ordinary hand, arm, or body joints and can bend or distort during biped animation.
- The robe is driven by the hidden biped legs and can stretch, collapse, or expose unnatural motion. The runtime cannot add cloth physics to this source without a new cloth/simulation design, and cloth physics is not part of this showcase phase.
- Correcting these faults cleanly requires source separation and re-rigging: a rigid staff mesh parented to an appropriate hand/accessory bone, plus authored robe deformation or skirt/cloth bones. This is not a material or shader fix.
- Restrained idle and walking clips are the safest initial candidates. Every selected clip needs visual review before it is mapped into the one-enemy loop.

## Integration gates

1. Source licence is CC0. The user supplied a Meshy workspace screenshot showing this exact lich asset with `Change License: CC0`; it is retained as `lich_placeholder_source_licence.png`, SHA-256 `6094E4D9A27A25022A1426C297F069DB60F779CC77526CFE6B154421F6DB96EE`.
2. The derived mask and textured runtime are now active on Windows. Replace the thresholded placeholder with an art-authored mask if the staff/eye selection needs polish.
3. Map only visually acceptable clips. The current asset has no direct replacement for the skeleton's attack clip.
4. The 9,188-triangle model runs at a 30 Hz skin/BLAS-refit cadence on Windows. Measure it on the target phone before certifying the Android path.
5. Keep it in the shared one-active-skinned-enemy slot. The roster remains plural/configurable, but skeleton and lich are not rendered together in this goal.
6. If staff or robe deformation remains distracting in the final reveal, replace or re-rig the asset rather than expanding the runtime around a placeholder defect.

## Proposed credit if shipped

`Placeholder lich character created and animated with Meshy.`
