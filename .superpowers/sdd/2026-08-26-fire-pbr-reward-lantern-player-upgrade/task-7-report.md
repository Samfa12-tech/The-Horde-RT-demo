# Task 7 — Gothic chest and reward lantern evidence

Date: 2026-08-28

## Delivered scope

- Imported the authoritative reviewed Meshy source records without a new generation or credit spend, then retained deterministic Blender processing metadata and CC BY 4.0 attribution.
- Added rigid generic static-mesh/BLAS routes for `ChestBase`, `ChestLid`, `GripRing`, and `LanternBody`. The transforms use `ChestLidHinge`, `RewardLanternHingeSocket`, and the lantern `Hinge`/`Flame`/`Light` sockets; they occupy TLAS slots 5-8 while the fixed 20-instance ceiling and slots 17-19 remain intact.
- Added fixed strict-ASTC PBR texture-array layers for chest wood/iron and lantern iron. The generic material route carries all four assets; no object-specific shader path, baked flame, orange opaque pane, hand, floor, or background was imported.
- Added the two deterministic renderer checkpoints: `lantern-chest-unlock` (114) and `lantern-glass-production` (115).
- The lantern body retains six distinct closed 7 mm glass panes. Each pane is a separate 12-triangle cuboid, so the generic dielectric material accounts for exactly 72 triangles with transmission 0.94 and IOR 1.52.

## Final validation evidence — Fix Round 5

- `horde::gameplay::kRouteFloorWorldY=-0.95` now lives in neutral `ShowcaseRoute.h`. Torch default/fall/settle gameplay, active skeleton placement/recoil, renderer corridor geometry, and `kProductionRewardChestStageWorldFromBase` all consume it. A cross-system behavioral test proves a settled torch, resting active skeleton, and production chest stage resolve to the same floor.
- The exact-literal audit classifies the remaining source/test `-0.95` values as unrelated atmosphere-mist coordinates, a dielectric direction fixture, and a player IK X pole. Chest full world Y remains `-0.95..-0.57`, the reward hinge is `+0.33`, lid origin is `-0.61`, flame/light are `-0.1605/-0.1335`, and unchanged `0.90` composition retains `22.5 mm` base and `177.899 mm` lid clearance.
- Focused fresh Debug and Release character/gameplay/prop tests passed. Full fresh Debug and Release CTests passed 28/28 each. Shader generic/legacy and ABI freshness passed.
- Fresh RT-presented and directly inspected Windows captures are `reports/task-7-fix-round-5-final-114/capture-manifest.json` and `reports/task-7-fix-round-5-final-115/capture-manifest.json`; exact final hashes/counters are tracked in `docs/evidence/TASK_7_FIX_ROUND_5_WINDOWS_CAPTURES_2026-08-28.json`. Their PNG bytes match Round 4, and every strict dielectric failure/open-stack/absorption counter remains zero. Earlier Task 7 capture families are historical.
- Android `assembleDebug`, unsigned `assembleRelease`, and `lintRelease` passed in one 97-task build; `tools/test-held-item-package-contract.ps1` passed against the rebuilt Debug APK. Debug/unsigned-Release SHA-256 are `329f8714fc90411a57baa02df16204c4765a66592169f74c61c084a699f4738c` and `5c3788e12a262349ade16146643ddcd183fa99739ef6f74baaa1fd10aef63253`.
- `adb devices -l` returned no attached device. Android evidence is therefore build/package/lint only; no installation, performance, lifecycle, visual, thermal, or owner review is claimed.

Audio/haptic manual revalidation required: NO — route-floor authority relocation, asset rendering, tests, and deterministic visual checkpoints add no audio cues, event transport changes, spatial source changes, or haptic behavior.
