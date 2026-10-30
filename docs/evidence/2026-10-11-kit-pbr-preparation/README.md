# Kit native PBR preparation, 11 October 2026

Prepared against `3c819c4ffcc88797e9313318b2ab35388c0c2380` in PR27.
This milestone preserves the supplied Warden and equips native indexed skinning
for his existing PBR maps. **Kit is not yet visible in the game; the chapter is
incomplete.** Tree-generator and replacement-canopy work remains deferred.

## Reproduction and correction

The original ordinary clip-skinning API provided positions/normals/UVs but no
animated tangent frame. The new `SkinUniquePbr` evaluates the existing clip and
joint palette without player IK assumptions, skins unique vertices, and produces
orthogonal tangent frames. An independent three-vertex fixture reproduced a
mirrored-transform handedness error ([failed run](analytic-red.log)); skinning
the bitangent now preserves the correct reflected frame. Rotation by 90 degrees
and rejection of the singular midpoint also pass. Legacy non-indexed consumers
still expand through the original indices; unique texture callers avoid that
unnecessary expansion. No shader, descriptor, rendering limit or quality change.

The runtime derivative retains all 464 non-image buffer views byte-for-byte,
including 24 joints, 26,945 unique vertices, 77,904 indices and seven clips. Four
embedded 1K image payloads become routing thumbnails; actual pixels are retained
through reproducible extraction and separate native texture compilation. ORM
uses AO=1 because the original has no AO map. No retargeting or remeshing.
[Exact identities/tool versions](identities.json), [runtime recipe and rights](../../../assets/models/characters/runtime/kit-warden-v1/README.md).

All seven clips at five phases match source/runtime PBR output byte-for-byte.
Expanding that output through the runtime indices also matches the existing
expanded textured skinning result. These are native data checks, not RT visual
acceptance. [Khronos validation](gltf-validation.json) reports zero errors and one
inherited non-root skinned-node warning; native source/runtime correspondence
passes without rewriting the supplied hierarchy.

## Checks and remaining integration

- Debug: [7/7 passed](debug-tests.log), 109.41 seconds.
- Release: [7/7 passed](release-tests.log), 16.63 seconds.
- These cover Kit, node-pose batching, player animation, skinned-character smoke,
  character-render-slot state, rescue attachments and adversarial recovery.
- Android: all four native ABIs and Debug APK built; lint and Java unit tests
  executed successfully. Runtime package/attribution contract passed. APK
  SHA-256 `5b8a6944398f44a1e26485499d9e43ec14e5e67b0226f9e043a8dff5ddd67ad9`.
  No installation or phone validation.
- Eight private 1K/full-mip texture arrays compiled and passed KTX validation:
  Windows RGBA8; Android strict ASTC 4x4 normals and 6x6 other maps.
  [Exact encoded hashes/sizes](texture-receipt.json). They are not packaged yet.
- Re-running the extraction in a fresh directory produced the same runtime SHA.
- The original untracked tomb `.blend1` remains unchanged at
  `0dae1e8fb803336cef2d045f48c5f4c9195a6463f45161cb0b6c7434350769fb`.
- Initial missing local validator-module failure and the pre-tangent red run
  remain under `reports/`; validation succeeded using the existing primary
  checkout validator, without installing another dependency.

The actual chapter atlas is 17 base / 14 normal / 14 ORM layers, including two
terrain additions. Kit's single family can reach 18/15/15 within the ceiling;
this count is capacity arithmetic, not measured GPU admission. The existing
row-25 legacy character shader path would overwrite a reused PBR material;
resolve generic routing before activating Kit. Geometry, textures, descriptor
ownership, fence-safe Keeper/Kit handoff, actual terrain-following behaviour and
real two-way GPU zone retirement remain required. This milestone introduces no
new shader feature and does not claim that those lifetimes are implemented.

Audio/haptic manual revalidation required for this milestone: **NO** (no cue,
event or playback change). Prior chapter listening/phone gates remain pending.
No device installation, interactive game launch, merge or release occurred.
