# Task 7 — Gothic chest and reward lantern evidence

Date: 2026-08-28

## Delivered scope

- Imported the authoritative reviewed Meshy source records without a new generation or credit spend, then retained deterministic Blender processing metadata and CC BY 4.0 attribution.
- Added rigid generic static-mesh/BLAS routes for `ChestBase`, `ChestLid`, `GripRing`, and `LanternBody`. The transforms use `ChestLidHinge`, `RewardLanternHingeSocket`, and the lantern `Hinge`/`Flame`/`Light` sockets; they occupy TLAS slots 5-8 while the fixed 20-instance ceiling and slots 17-19 remain intact.
- Added fixed strict-ASTC PBR texture-array layers for chest wood/iron and lantern iron. The generic material route carries all four assets; no object-specific shader path, baked flame, orange opaque pane, hand, floor, or background was imported.
- Added the two deterministic renderer checkpoints: `lantern-chest-unlock` (114) and `lantern-glass-production` (115).
- The lantern body retains six distinct closed 7 mm glass panes. Each pane is a separate 12-triangle cuboid, so the generic dielectric material accounts for exactly 72 triangles with transmission 0.94 and IOR 1.52.

## Final validation evidence — Fix Round 4

- `kRouteFloorWorldY=-0.95` is the single corridor-floor authority consumed by route geometry and `kProductionRewardChestStageWorldFromBase`; the renderer and production-prop behavior test consume the same stage. Chest full world Y is `-0.95..-0.57`, the reward hinge is `+0.33`, lid origin is `-0.61`, flame/light are `-0.1605/-0.1335`, and unchanged `0.90` lantern composition retains `22.5 mm` base and `177.899 mm` lid clearance.
- Focused fresh Debug and Release CTests passed 4/4. Full fresh Debug and Release CTests passed 28/28 each. Shader generic/legacy and ABI freshness passed.
- Fresh RT-presented and directly inspected Windows captures are `reports/task-7-fix-round-4-final-114/capture-manifest.json` and `reports/task-7-fix-round-4-final-115/capture-manifest.json`; exact final hashes/counters are tracked in `docs/evidence/TASK_7_FIX_ROUND_4_WINDOWS_CAPTURES_2026-08-28.json`. Every strict dielectric failure/open-stack/absorption counter is zero. Earlier Task 7 capture families are historical.
- Android `assembleDebug`, unsigned `assembleRelease`, and `lintRelease` passed in one 97-task build; `tools/test-held-item-package-contract.ps1` passed against the rebuilt Debug APK. Debug/unsigned-Release SHA-256 are `094ef6c34c68ffebd44c818926a58e13e6138d947adaeb5b16eb81f6db9072a8` and `7b7ee35f7326af5b9feda6e725ae9c99b52b3f83656b002e64014c73967c1cde`.
- `adb devices -l` returned no attached device. Android evidence is therefore build/package/lint only; no installation, performance, lifecycle, visual, thermal, or owner review is claimed.

Audio/haptic manual revalidation required: NO — asset generation, processing, rendering, and deterministic visual checkpoints add no audio cues, event transport changes, spatial source changes, or haptic behavior.
