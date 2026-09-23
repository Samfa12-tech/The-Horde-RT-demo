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
role. The initial contract slice (`6404976`) changed no shader ABI, production
route or GPU ownership; the later native integration is recorded below.

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
That initial `6a93eb7` evidence was offline/host only, before native integration.

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

At `fff4649`, this established the CPU ownership seam; separate buffers/BLAS/TLAS
and native evidence followed in the GPU integration checkpoint below.

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

Shared-pose commit `fff4649` subsequently passed fresh branch/PR runs
`35790685040` / `35790688872`: 43 portable tests and 10 focused Vulkan CPU-host
tests, including the newly registered malformed-pose fixtures.

## Shared immutable texture ownership

`StaticRtAssetRegistration::textureSource` explicitly reuses an earlier,
self-owning provider's named texture groups. Geometry and material factors remain
independent. Default registrations preserve their previous per-asset allocation.
Unknown/forward providers, alias chains, inconsistent repeated sources, missing
groups, conflicting texture-category presence and textured ungrouped aliases are
rejected. This is a reusable registration contract, not a player-name shader branch.

Self-contained host fixtures check layer reuse/counts, separate geometry/materials
and negative cases. The actual world/viewmodel GLBs also pass registration checks:
viewmodel materials resolve to the matching Body/Gauntlet groups without adding
atlas layers. The existing production atlas remains unchanged (ten base/normal/ORM
layers and one emissive layer). At `051fcde` no renderer registration used the
alias yet; the GPU integration below activates it. Neither is a measured
production performance gain.

Fresh MSVC Release: 4/4 affected CTests passed in 6.93 seconds (static GLB tests,
skinned smoke, actual viewmodel admission and malformed viewmodel pose fixtures).

## Native GPU integration checkpoint

The shared texture slice was committed as `051fcde`; fresh push/PR CI runs
`35791581940` / `35791587978` passed 43 portable and 10 Vulkan CPU-host tests.
The subsequent integration adds three explicit vertex streams: Static,
PlayerWorldBody and PlayerViewmodel. Dynamic world vertices are removed from the
static allocation, not duplicated. Indices, materials and immutable texture arrays
remain shared; viewmodel materials reuse the named world texture groups without
adding atlas layers. Each player mesh owns its vertex buffer, updatable BLAS and
scratch resource, consumes the same solved pose, and refits only when needed.
Viewmodel skinning does not apply the world-body boot-grounding step.

`RtGeometryRole` occupies the former reserved metadata word at byte offset 24;
the 32-byte instance ABI is unchanged. Primitive vertex offsets are role-local,
with separately validated per-primitive vertex counts for AS addressing. The
fixed bindings are 23 (world) and 24 (viewmodel); 22 remains Diagnostic-only.
Descriptor preflight checks the actual non-contiguous Shipping roster and rejects
unknown/duplicate/missing bindings, allowing only the unused legacy held-light
binding 20 to be optimized out. Device storage-descriptor limits are checked.
No descriptor-indexing extension or additional frame-in-flight was introduced.

The earlier proposed reuse of TLAS slot 10 is deliberately superseded. Normal
hybrid rendering still uses the procedural BLAS at 10-13; registering PBR
viewmodel metadata at 10 would decode the legacy BLAS incorrectly while inactive.
The registry now has nine asset slots and 21 TLAS/metadata slots, preserving all
original indices and adding named viewmodel index 20 beside world-body index 4.
Modelled primary rays include `0x40`, outside legacy `0x04` screen-Y culling;
secondary masks exclude it and world-body `0x10` supplies shadows/reflections.
All procedural arms are disabled only in the opt-in modelled route. Missing
optional viewmodel assets fail explicit requests without a full-body fallback.

Both shader backends compile and validate with fixed streams. Triangle-level
buffer selection was compared with three per-vertex helper calls. Generic is
692 bytes / 37 instructions / 4 branches larger, with two fewer function calls;
Opaque is 720 bytes / 126 instructions / 72 branches smaller. The retained
triangle helper selects once per triangle and reduces Opaque divergence. These
are compiler statistics, not measured driver speedups.

| RTP variant (High/Mobile same counts) | Bytes | Instructions | Functions/calls | Ray-query sites | Atomics |
| --- | ---: | ---: | ---: | ---: | ---: |
| Diagnostic Generic | 227084 | 13381 | 60 / 193 | 3 | 32 |
| Shipping Generic | 219304 | 13012 | 60 / 193 | 3 | 0 |
| Diagnostic Opaque | 505660 | 27733 | 1 / 0 | 23 | 5 |
| Shipping Opaque | 501424 | 27554 | 1 / 0 | 23 | 0 |

Compute adds 252 bytes / 16 instructions for Generic and 260 bytes / 19
instructions for Opaque. All eight Shipping modules across both backends have
zero diagnostic atomics and no binding 22; 23/24 remain present. Frozen numeric
budgets were rebaselined for the explicit stream-selection feature (roughly
1.1% Generic / 2.5% Opaque byte growth), not to claim an optimization. Ray-query
site counts, bounded loops, instrumentation, quality and physical-strategy guards
remain unchanged. The compatibility includes and both catalogs were regenerated.

The raygen publisher now recognizes only the exact eight compute counterparts
beside its eight owned artifacts. It preserves those files byte-for-byte and
still rejects unknown files; a portable regression test covers this boundary.

### Fresh implementation evidence

- MSVC Release focused suite: 13/13 passed in 59.56 seconds. The stale-shader
  negative fixture originally failed because its obsolete hardcoded hash made
  mutation a no-op; it now derives the current hash, proves mutation and verifies
  rejection. The later live BLAS-count/inventory move test passed separately
  (1/1, 1.80 seconds). Both player buffers, viewmodel scratch/backing and the 17th
  optional BLAS are included in actual inventory rather than a fixed report count.
- Native Debug Diagnostic/High Windows build succeeded. Nine frozen RTX captures
  passed presentation/ownership/grip gates, including all eight viewmodel poses.
  Actual camera state is recorded separately from requested checkpoint state;
  legal pitch endpoints are -0.32/+0.28 radians and a host regression checks all
  eight authored requests against staged simulation state.
- The old world-body control passes the unchanged image tolerance: maximum RGB
  delta 2, one pixel over 1, fraction 0.0000019290123456790124.
- All 16 variants passed actual compilation/SPIR-V validation. Shipping remains
  free of Diagnostic atomics/binding 22; this is not backend pixel-parity proof.
- Android unsigned Shipping/Mobile final four-ABI build succeeded in 34 seconds. Exact APK
  SHA-256 `323710dd1628708ae28045bce911d017070d4f2701c575eb95b3a3d17df39016`,
  86,300,051 bytes. ZIP inspection confirms all four native libraries and the
  existing world asset. The viewmodel remains outside the Android package
  allowlist, so no Android viewmodel-rendering or device result is claimed.

See the [native evidence bundle](evidence/2026-09-23-viewmodel-rt/README.md) for
exact executable/image hashes, controls, reproducer and explicit limitations.

### Broader validation follow-up

GPU integration `d8b1b7f` is pushed and remote-verified. Its fresh branch/PR runs
`35797498248` / `35797806653` pass all ten Vulkan CPU-host player tests but expose
six stale portable expectations (38/44). Provider word counts/hashes now bind to
the reviewed frozen catalog; fixture arguments must agree with it and actual
SPIR-V bytes must still match. Containment/negative controls remain intact,
including rejection of a valid-format but wrong hash. The development checkpoint
test now checks all eight new stable names/IDs and exclusion from the release route.

Fresh out-of-tree MSVC Release non-Vulkan build succeeded. The full 59-test run
passed 56 and exposed three further test issues: the shader helper-order assertion
still looked in the old launcher instead of shared `rt_frame.glsl`, compatibility
artifact pins predated the reviewed ABI change, and spatial-audio source lookup
assumed an in-repository build directory. All three were fixed and rerun
successfully (the two shader scripts directly, audio through CTest). No runtime
audio logic, image tolerance, physical shader behavior or diagnostic gate was
relaxed. Both actual eight-mode shader compile/validation paths are freshly covered.
This is a full run plus focused repair reruns, not a claim of one clean 59/59 run.

Follow-up `6b219832514b3e36bda3792ad6639119894675c2` is pushed and remote-verified.
Fresh branch run [35799096847](https://github.com/Samfa12-tech/The-Horde-RT-demo/actions/runs/35799096847)
and PR integration run [35799100216](https://github.com/Samfa12-tech/The-Horde-RT-demo/actions/runs/35799100216)
both pass 44/44 portable and 10/10 Vulkan CPU-host tests (logs inspected).
PR #15 is MERGEABLE/CLEAN at that source. The earlier failures remain honest
history, not relabelled green. The following handoff-only commit changes no runtime.

## Remaining gates

- The [hand-orientation investigation](ENGINEERING_1_6_1_HAND_ORIENTATION_2026-09-23.md)
  supersedes the initial shape hypothesis: exact CPU uploads confirm sleeve
  deformation, and an isolated Grip-roll candidate changes cuff direction without
  swapping named Left/Right chains or changing prop authority. Combined reweighting
  improves stretch but the RT result is still unacceptable. Accepted runtimes are
  unchanged; finish anatomical mounting and sleeve shape before admission.
- Fix the visibly angular/open-looking sleeve/shoulder surfaces and near-camera
  composition with model/skin investigation and geometry changes, not visual cheats.
- Live-motion transitions, legal extreme pitch, retraction, grip agreement and
  final owner phone acceptance. Frozen native capture success is insufficient.
- Package the opt-in candidate, validate actual Android RT presentation/resources,
  and retain normal block arms until the replacement passes owner acceptance.
- Investigate the one retained raised-lantern transport overflow; do not waive it
  because the ownership capture completed. Glass correctness/performance and
  Shipping/Diagnostic versus pipeline/compute parity remain separate gates.
- Justified persistent mapping/device-local static memory and reusable renderer
  extraction remain measured follow-on work, not claims of this ownership slice.

Audio/haptic manual revalidation required: NO for the contract slice; no feedback,
playback, event timing or listener/source semantics changed.
