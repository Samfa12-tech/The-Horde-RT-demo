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

## Reproducible geometry candidate

`tools/process-player-viewmodel-runtime.py` derives the two existing modelled
arm partitions from the accepted rig/source pipeline, retaining the original
26-joint rig and Idle/Walking tracks. Head, torso and leg faces are absent.
No paid generation, source replacement or licence change occurred.

Two successful clean single-thread Blender 5.2.0 LTS exports are byte-identical:
SHA-256 `1e3b041ee7aa896a84fe462c182f6012d026b4a577b2d4ddf59d764b823f2538`,
1,725,820 bytes, 23,162 unique vertices and 14,370 triangles (5,532 sleeves,
8,838 gauntlets). Both world-reference exports match the admitted world hash
`e8737f10e7669b284e04109d9c3acdf537df284093a21511656bf191a70450fd`.
The initial material-slot clearing attempt failed its nonempty-part gate; it was
not admitted. Material indices are now preserved across slot replacement.

The candidate lives under `assets/models/player/viewmodel/`, outside existing
package allowlists. Its manifest, receipt and regeneration command are recorded
there. The registered `horde_rt_player_viewmodel_admission` CTest verifies actual
static/skinned addressing, finite textured poses and exact authored hand/grip
transforms at three phases of both clips. Its first MSVC Release run passed.
The focused Vulkan CPU-host CI lane now includes this ninth test and its LFS input.
This is offline/host evidence only; it is not a rendered or production arms route.

## Shared solved pose

The existing player animation/IK evaluation now produces an opaque
`SkinnedPlayerPose`. `PlayerRenderSlot` retains it and skins the world geometry
from it; a viewmodel consumer can skin its own geometry from the same palette
and grips without a second animation clock or IK solve. Gameplay snapshot,
held-item ownership and the existing cadence/imported-same-tick rules are unchanged.
The old combined skinning API remains a wrapper for existing callers.

Cross-mesh binding requires exact ordered joint names and inverse-bind matrices.
The immutable rig identity survives moves; a checked compatible binding is cached,
not inferred from matching joint counts or raw asset addresses. Reload clears
that cache. A failed pose evaluation invalidates the old palette; failed skinning
clears output instead of exposing partial geometry. Normal successful updates
reuse output storage rather than clearing and zero-initialising the whole mesh.

New malformed-fixture coverage rejects changed inverse binds, a renamed non-hand
joint, a genuine hierarchy cycle, invalid animation-channel target, NaN position
and NaN weight. The admission test also checks exact shared-versus-independent
vertices/tangents/grips at six clip/phase combinations, pose moves and invalid
time/IK input. Finite geometry/rig/channel admission checks happen at load time;
the solved palette and produced vertices are checked before use.

This is the CPU ownership seam, not completed GPU viewmodel ownership. Separate
buffers/BLAS/TLAS and native-rendered acceptance remain the next integration work.

Fresh validation of the shared-pose source before its commit:

- MSVC Release: 5/5 passed in 13.25 seconds: skinned-character smoke, world semantic
  fixtures, viewmodel admission, viewmodel pose fixtures and character-slot smoke.
- Android unsigned Shipping/Mobile RelWithDebInfo: four ABI build succeeded
  (`arm64-v8a`, `armeabi-v7a`, `x86`, `x86_64`), 35 seconds. APK SHA-256
  `cc03de6974aee6e836df81839edbda783c77cc01a52ee1da6207c69c6521fd1e`,
  86,060,003 bytes. ZIP inspection confirms all four native libraries and the
  existing world asset; the offline viewmodel is not accidentally packaged.
- No new phone installation, device evidence, native image capture, frame-time
  improvement or owner acceptance is claimed by these checks.

Preceding geometry commit `6a93eb7` has fresh green push/PR runs
`35789772367` / `35789776592`: 43 portable tests and 9 focused Vulkan CPU-host tests.
That CI evidence does not certify subsequent shared-pose edits.

## Remaining gates

- Native rendering admission of the reproducible arms-only runtime candidate.
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
