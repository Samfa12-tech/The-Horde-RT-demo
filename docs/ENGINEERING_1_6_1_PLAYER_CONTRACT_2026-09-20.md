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

## Second slice: manifest and loader enforcement

Both runtime and clip manifests now declare all four semantics. The generic
manifest parser owns material strings and distinguishes an absent optional field
from an explicitly empty field during parsing. A supplied field must contain the
exact four valid declarations; production player loading explicitly requires it.
Other assets remain compatible without player metadata. Programmatically supplied
nonempty declarations are revalidated by the static loader.

Static loading validates actual per-primitive material references, not just the
GLB material table. The skinned player slot validates its primitive names before
deriving grip sockets and clears a semantically invalid loaded asset. These are
load-time checks; no per-frame work or presentation switch was added.

New regressions exercise owned-string copying, parser presence/type/duplicate-key
handling, malformed declarations, 24 manifest orders, 24 independently reordered
four-primitive GLBs, actual missing/duplicate/unknown geometry semantics, existing
primary mask bits, and runtime/clip manifest agreement. The shipped player GLB
also passes the existing static/skinned stream and grip/animation smoke.

Fresh MSVC Release build succeeded. The first CTest attempt passed 3/4; the skinned
smoke could not locate repository assets from the external build directory. Its
CTest working directory is now the source directory. Final targeted run passes
**5/5** (3.31 seconds): contract, manifest agreement, animation, static glTF and
skinned-character smoke. This is host validation, not new phone/visual evidence.
No asset binary or shader was regenerated in this slice.

## Remaining gate

Replace first-seen texture allocation with explicit named groups; exercise real
texture permutations and skinned malformed fixtures; regenerate into a clean
validation destination from existing licensed sources; verify generated agreement
and GCC/MSVC. The current host checks do not establish generated-asset agreement
or a fresh GCC/Android pass. Phase 2 remains open.

Dedicated viewmodel geometry/resource ownership and owner phone acceptance follow
this gate. Preserve current primary masks and do not substitute the full world body.

Audio/haptic manual revalidation required: **NO**. This slice does not alter
feedback events, transport, timing, spatialisation, playback or haptic semantics.
