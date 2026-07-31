# Horde Lantern RT - Showcase Alpha 0.1.3

Package version: `0.1.3-alpha.1`
Android version code: `4`

## What changed

- Rebuilt the player from a broad box torso into a layered low-poly travelling coat, shaped shoulders, belt, collar, coat tails, articulated capsule limbs, pelvis, boots, and a reflection-only head.
- Added animated walking with opposed stride, knee bend, foot lift, toe roll, pelvis bob and sway, and restrained torso counter-rotation.
- Added three-point player vitality, bounded hit invulnerability, damage feedback, a short death hold, encounter retry, full-route restart, and quit flows on Android and Windows.
- Extended the lich defeat into a complete ending: the death animation finishes, the roof opens, dawn warms the ruin, and an epilogue offers Continue, Begin Again, or Quit.
- Preserved native `vkCmdTraceRaysKHR` presentation, phone-safe ray-query shading, one frame in flight, 18 TLAS instances, eight BLAS, and one active skinned enemy.

## Compatibility and scope

- Android remains intended for explicitly validated Vulkan hardware-RT phones; unsupported devices receive diagnostics rather than a fallback renderer.
- Windows requires a Vulkan hardware-ray-tracing GPU and current driver.
- Simultaneous enemies, water, broader AI, block/dodge, and the staged textured sword remain outside this release.

See `ASSET_LICENSES.md` for asset provenance and licence terms.
