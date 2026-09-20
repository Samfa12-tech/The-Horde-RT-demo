# Phase 2: player semantic contract

## First slice: typed four-way contract

The processor and current GLB already emit BodyPrimaryVisible,
HeadPrimaryMasked, NearFacePrimaryMasked and GauntletPrimaryVisible. The clip
manifest still declares only three; loader enforcement and texture routing remain
open. This slice does **not** close Phase 2 or enable modelled first-person arms.

- Added a shared, name-keyed four-way semantic/visibility/texture-group contract.
  Identities are independent of GLB order and are not GPU or TLAS indices.
- Renderer visibility and existing head/near-face material-bit assignment now use
  this contract. Shader bits 128/256 and normal presentation remain unchanged.
  Invalid enum values fail closed. Texture groups are declarations only in this
  slice; actual layer allocation is not yet repaired.
- Added a portable contract test for all 24 orders, missing/extra/unknown/duplicate
  names, conflicting visibility, named texture groups and diagnostic clearing.
  Expanded the existing animation test with gauntlet and invalid-enum coverage.

Validation on 2026-09-20: observed initial missing-header RED; implementation and
final defaults/null-guard rebuild passed the three targeted MSVC Release CTests:
`horde_rt_player_primitive_contract_tests`, `horde_rt_player_animation_tests`,
`horde_rt_static_gltf_asset_tests` (3/3, final run 4.09 seconds).
GCC/Clang and WSL were unavailable locally; no fresh GCC or remote CI pass is claimed.
No shaders, asset bytes, ray masks, gameplay or feedback inputs were changed.
Current GLB SHA-256 remains
`cd64a8447097f59f825620ef43e2425f9286ff075f5bfdb48c25c5d96f2c479d`.

## Remaining gate

Synchronize manifests; enforce the four-way declaration in the asset loader;
replace first-seen texture allocation with explicit named groups; exercise real
loader/texture permutations and malformed fixtures; regenerate into a clean
validation destination from existing licensed sources; verify generated agreement
and GCC/MSVC. Manifest storage must own strings: the contract's declaration views
are borrowed validation inputs, not persistent parsed storage.

Dedicated viewmodel geometry/resource ownership and owner phone acceptance follow
this gate. Preserve current primary masks and do not substitute the full world body.

Audio/haptic manual revalidation required: **NO**. This slice does not alter
feedback events, transport, timing, spatialisation, playback or haptic semantics.
