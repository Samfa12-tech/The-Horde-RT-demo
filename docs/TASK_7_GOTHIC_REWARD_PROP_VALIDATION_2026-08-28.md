# Task 7 Gothic chest and reward lantern validation

Date: 2026-08-28

## Delivered scope

- Imported the authoritative reviewed Meshy source records without a new generation or credit spend, then retained deterministic Blender processing metadata and CC BY 4.0 attribution.
- Added rigid generic static-mesh/BLAS routes for `ChestBase`, `ChestLid`, `GripRing`, and `LanternBody`. The transforms use the authored chest rear-hinge pivot and lantern sockets; they occupy TLAS slots 5-8 while the fixed 20-instance ceiling and slots 17-19 remain intact.
- Added fixed strict-ASTC PBR texture-array layers for chest wood/iron and lantern iron. The generic material route carries all four assets; no object-specific shader path, baked flame, orange opaque pane, hand, floor, or background was imported.
- Added the two deterministic renderer checkpoints: `lantern-chest-unlock` (114) and `lantern-glass-production` (115).
- The lantern body retains six distinct closed 7 mm glass panes. Each pane is a separate 12-triangle cuboid, so the generic dielectric material accounts for exactly 72 triangles with transmission 0.94 and IOR 1.52.

## Validation evidence

- Focused fresh Debug CTests passed: `horde_rt_production_prop_asset_tests`, `horde_rt_character_render_slot_smoke`, `horde_rt_development_static_asset_tests`, and `horde_rt_static_gltf_asset_tests` (4/4). The production-prop test was rebuilt after adding the exact six-pane gate.
- Fresh Windows captures completed with RT presentation at `reports/task-7-final/windows-lantern-chest-unlock/capture-manifest.json` and `reports/task-7-final/windows-lantern-glass-production/capture-manifest.json`. They are deterministic visual/provenance checks, not a matched performance acceptance claim.
- Foundation Host run `reports/foundation-runs/run-20260828-062323/` passed Debug and Release CTests (28/28 each) and its fixed Windows captures. Its Android clean was interrupted by two locked native-build log files. After stopping only the project Gradle daemons, a fresh `gradlew clean`, `assembleDebug`, `assembleRelease`, and `lintRelease` completed successfully; `tools/test-held-item-package-contract.ps1` passed against the new unsigned Release APK.
- `adb devices -l` returned no attached device. Android evidence is therefore build/package/lint only; no installation, performance, lifecycle, visual, thermal, or owner review is claimed.

## Fix Round 1

- The previous live pane failures were not an asset-topology defect: metre-space validation now asserts six disconnected, outward-wound, two-use-edge closed components and a 7 mm nearest paired-face thickness. The existing static-GLB negative fixtures continue to reject open, non-manifold, inward, and malformed thick-volume inputs; the production gate adds the non-generic wrong-thickness guard. The generic transport bug was stochastic rough refraction inside a thick 7 mm volume, which could reach cage/neighbor geometry before its paired exit. Thick transmission now preserves its geometric refracted direction; roughness remains on reflection. Primary stack identity includes instance plus material, so an exit from a different pane cannot close the wrong volume.
- `LanternSocket`, `Hinge`, `Flame`, and `Light` are now resolved from loaded GLB sockets. Checkpoint staging supplies only the chest world placement; ring/body transforms and emitter/light positions are composed from one authoritative lantern hinge transform. The chest lid is composed from the loaded `ChestLid` rear-hinge root and a named staged -70 degree open pose.
- Fresh RT-presented Windows captures for 114/115 have zero transport overflow, shadow overflow, secondary reject, open-volume, and production-pane-attributed counters. Exact capture hashes and manifest paths are tracked in [TASK_7_FIX_ROUND_1_WINDOWS_CAPTURES_2026-08-28.json](evidence/TASK_7_FIX_ROUND_1_WINDOWS_CAPTURES_2026-08-28.json). These are capture evidence, not a matched performance acceptance.
- Sanitized exact Meshy settings/task IDs and accepted/rejected decisions are tracked in `assets/models/props/provenance/task-7/`. The prior `references/lantern_icon_reference.png` statement is corrected: no such repository/distributable file exists.
- `SM-S948B` remains unavailable through ADB. The exact APK mobile 4-interface visual/performance/lifecycle confirmation remains a Task 9 gate; it is not inferred from this Host repair.

Audio/haptic manual revalidation required: NO — asset generation, processing, rendering, and deterministic visual checkpoints add no audio cues, event transport changes, spatial source changes, or haptic behavior.
