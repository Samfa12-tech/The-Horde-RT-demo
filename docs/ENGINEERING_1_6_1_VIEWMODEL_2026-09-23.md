# Dedicated RT viewmodel implementation

Phase 3 is in progress, following the accepted Phase 2 technical gate.
This record separates asset admission, shared pose ownership, GPU integration
and physical-device/owner acceptance. It does not retire the normal block-arm route.

## Arms-only contract

`PlayerAssetRole::Viewmodel` is explicit; a two-primitive count never implicitly
changes a world-body asset into a viewmodel. The two required named primitives
are `ViewmodelSleeves` and `ViewmodelGauntlets`, in either order, using the existing
Body and Gauntlet texture groups. Both are primary-visible; the world body owns
shadow/reflection visibility. Unknown, duplicate, missing or world-body semantics
are rejected by manifest parsing and actual GLB loading. Direct validation APIs
also reject the opposite role and invalid role values.

The existing four-part world contract remains unchanged. Legacy manifests with
world declarations remain supported; generic non-player assets require no player
role. This slice changes no shader ABI, production route or GPU resource ownership.

Fresh MSVC Release focused verification: 5/5 passed (7.64 seconds):
`player_primitive_contract_tests`, `player_manifest_agreement_tests`,
`static_gltf_asset_tests`, `skinned_character_smoke` and
`player_skinned_semantic_fixtures` (all use the `horde_rt_` prefix). The last
fixture exercises the actual four-part world GLB, not the new arms candidate.

## Remaining gates

- Reproducible arms-only runtime with provenance and real static/skinned admission.
- One gameplay-derived animation/IK/grip authority, consumed by separate world-body
  and viewmodel geometry. No arms-only boot-grounding requirement.
- Independent dynamic buffers and BLAS ownership, named TLAS semantics, correct
  primary/secondary masks, shared immutable texture ownership and bounded updates.
- Native RT sword/torch/reward-lantern checkpoint and live-motion matrix, including
  extreme pitch, retraction and grip agreement; no full-body-primary substitute.
- Windows RTX and exact phone evidence, then owner phone acceptance before retiring
  block arms. Offline geometry and host tests do not satisfy these gates.

Audio/haptic manual revalidation required: NO for the contract slice; no feedback,
playback, event timing or listener/source semantics changed.
