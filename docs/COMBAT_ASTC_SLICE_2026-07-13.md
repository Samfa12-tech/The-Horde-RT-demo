# Combat and ASTC Runtime Slice - 2026-07-13

## Outcome

- Added the first bounded game loop: one skeleton approaches, attacks, can be hit by a timed player sword swing, plays its authored `Dead` clip, and respawns.
- Split the procedural sword from the torch into its own static BLAS and fourth TLAS instance. Both held props remain on mask `0x02`, so they stay visible to primary/reflection rays without self-shadowing the torch direct-light estimate.
- Expanded the narrow skeleton reader to exactly `Idle_5`, `Walking`, `Attack`, and `Dead`. Walking loops; attack and death clamp instead of wrapping to their first pose.
- Added `SWING` on Android and right mouse/Space on Windows. The Android attack is an atomic event latch rather than a transient continuous-control value.
- Added a red edge pulse when the enemy attack connects. Gameplay decisions live in `src/gameplay/SwordCombat.h`; rendering consumes an immutable snapshot.
- Added separate output exposure. Android retains `0.92`; Windows uses `0.62` to calm the bright desktop presentation without weakening the shared one-bounce lighting terms.

## Compressed mobile materials

- Android now packages three strict KTX2 arrays instead of the raw RGBA arrays:
  - diffuse: `VK_FORMAT_ASTC_6x6_SRGB_BLOCK`, 591,680-byte GPU payload;
  - normal: `VK_FORMAT_ASTC_4x4_UNORM_BLOCK`, 1,310,720-byte GPU payload;
  - AO/roughness/metal: `VK_FORMAT_ASTC_6x6_UNORM_BLOCK`, 591,680-byte GPU payload.
- Total runtime texture payload is 2,494,080 bytes, down from 15,728,640 bytes (84.1%).
- The runtime accepts only KTX2 arrays with the exact identifier, format, dimensions, five layers, one face, one mip, no supercompression, and exact level payload size.
- Selection also requires sampled-image, linear-filter, transfer-destination, extent, and array-layer support for every requested format. ASTC LDR is enabled at logical-device creation only when the physical device exposes it.
- Android intentionally has no packaged raw fallback. An unsupported device receives an explicit diagnostic instead of silently defeating the mobile memory goal. Windows retains the raw RGBA fallback because RTX hardware is not assumed to support ASTC.
- `tools/compile-material-textures.ps1` regenerates and validates the committed arrays from the established raw source arrays.

## Validation completed

- Final review caught and fixed the turning enemy's skinned and geometric normals remaining in model space; both now follow the committed RT instance transform for correct lighting and ray-origin offsets.
- Regenerated embedded raygen SPIR-V: SHA-256 `D68F46AA2F0515F3D1106FAE3EC5282DBB76685C2D9EA0A0028E0E7CAF7DF8D5`.
- Windows Debug capability probe and diagnostic window built successfully with MSVC.
- RTX 5050 capability probe still selected `RayTracingPipeline`.
- Windows RT window remained running during an eight-second presentation smoke test and logged `PBR material encoding: RGBA8 raw fallback`.
- Android `assembleDebug` succeeded for `arm64-v8a`, `armeabi-v7a`, `x86`, and `x86_64`.
- APK audit found exactly four runtime assets: the live skeleton and the three ASTC KTX2 arrays. No `.rgba` array is packaged.
- Debug APK size is 46,793,835 bytes. ZIP compression had already made the raw arrays inexpensive on disk, so APK size is effectively unchanged; the material win is app-private staging and GPU memory, not archive size.
- `horde_rt_combat_smoke` passed swing, player hit/death, respawn, and enemy damage-pulse state transitions.

## Open phone gate

No Android device was connected during this pass. Fresh install, current-shader visual comparison, ASTC selection log, RT-present log, hands-on controls/combat, and the cold 126-interval benchmark remain mandatory. Preserve the 50+ FPS median gate before adding another enemy or the textured sword LOD.

The desktop brightness comparison must also account for build age: the latest ACES/sRGB seam-and-moon shader had not previously been run on the phone. Android keeps its existing exposure until the exact current build is compared on target hardware.
