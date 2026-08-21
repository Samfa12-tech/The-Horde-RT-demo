# Horde Lantern RT - Showcase Alpha 0.1.4

Package version: `0.1.4-alpha.1`
Android version code: `5`

## What changed

- Replaced platform-owned gameplay orchestration with one deterministic 60 Hz simulation shared by Android and Windows.
- Expanded the opening fight to two stable skeleton enemies with independent deaths, deterministic separation, nearest-target sword contact, and one attacker token.
- Synchronized sword and enemy damage with explicit animation wind-up, active, and recovery phases.
- Added a press-down `PARRY` action on Android and `Q` on Windows. A correctly timed frontal parry cancels one skeleton strike, produces distinct feedback, and visibly staggers that attacker for a normal riposte opportunity.
- Preserved ordered entity-aware audio and haptic events, including event-time listener/source positions and the skeleton impact-to-fall timing.
- Retained native `vkCmdTraceRaysKHR` presentation, phone-safe `rayQueryEXT` shading, recursion depth one, strict Android ASTC, one frame in flight, at most two pose buckets, nine BLAS, and nineteen TLAS slots.

## Compatibility and scope

- Android remains intended for explicitly validated Vulkan hardware-RT phones; unsupported devices receive diagnostics rather than a fallback renderer.
- Windows requires a Vulkan hardware-ray-tracing GPU and current driver.
- The opening encounter is capped at two skeletons and one attacker at a time. The lich remains singular and its ranged staff attack cannot be parried.
- Larger hordes, held guard, dodge, automatic counterattack, fire/water expansion, the staged textured sword, and a general ECS remain outside this release.
- Sustained performance varies with scene and phone governor state. The validated `SM-S948B` candidate retained honest RT presentation; its standard 75% route ranged from about 77-108 FPS across the opening, two-enemy, and environment checkpoints to about 43 FPS in the heaviest lich checkpoint.

See `ASSET_LICENSES.md` for asset provenance and licence terms.
