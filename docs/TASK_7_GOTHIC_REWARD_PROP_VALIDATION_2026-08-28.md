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

## Fix Round 2

### Root cause and reusable transport repair

- The first empty Round 2 capture was an asset-contract bug: `ChestLid` is a
  mesh node, not a socket. The runtime now requires the authored
  `ChestLidHinge` socket from the chest-base GLB and composes it with the lid's
  exact identity node transform. Tests compare all 16 elements of every hinge,
  flame, light, and lid matrix and evaluate both checkpoint cameras against the
  exact 960x540 raygen projection.
- The lantern's opaque tracery previously penetrated the closed pane slabs by
  1.0 mm. The reusable asset processor moved the six pane centres from a
  0.194 m to 0.190 m apothem without reducing the authored 185x345x7 mm clear
  aperture. The production fixture now proves 2.99993 mm minimum radial
  cage/glass clearance, six outward closed components, exact thickness, exact
  component bounds, and negative open/duplicated/wrong-thickness/inward/
  single-flipped-triangle fixtures.
- Shadow attribution proved that the large former mismatch population was not
  front-face reconstruction error: surface and emitter shadow segments really
  begin inside one or more closed panes. The bounded shadow stack now accepts
  consecutive leading exits as implicit origin-containing media, matches later
  exits by exact instance plus material, and attenuates a finite endpoint that
  remains inside a volume. This retains Mobile 4-interface/2-volume and High
  8-interface/4-volume bounds.
- Thick rough transmission follows the geometric Snell direction to the paired
  exit, then applies its deterministic rough lobe. Roughness therefore remains
  active in reflected and transmitted energy without letting a microfacet
  entry direction jump a 7 mm slab into its cage. Reflection/TIR/exit lobes are
  constrained to the physical ideal interface hemisphere. Repeated TIR still
  inside a volume at the fixed interface bound is conservatively absorbed and
  counted; a non-TIR exhaustion remains a transport failure.
- A valid dielectric or water hit reached by the one permitted reflected ray is
  now an explicitly attributed terminal approximation, not an opaque
  reclassification, rejection, or recursive dielectric bounce.
- Three checkpoint-114 rays enter pane instance 8 exactly at a shared slab
  edge, after which finite-precision triangle traversal sees ordinary world
  instance 0 / mossy-stone material before the analytically required paired
  exit. The closed/outward topology, 2.99993 mm cage clearance, zero same-
  instance/different-material events, zero after-TIR events, and exact
  instance/material masks isolate this from overlap, corrupt topology, TIR,
  or a second dielectric. The bounded integrator conservatively absorbs only
  this unresolved still-interior energy. It increments
  `primaryClosedVolumeAbsorptionCount` and retains terminal/volume/material
  attribution; it does not leak a fallback, shade a dielectric as opaque,
  reset a stack, increment an unclosed-volume failure, or use a lantern-
  specific shader branch. The same rule is reusable for validated closed
  dielectric meshes, while exact generic instance/material mismatches remain
  failures.

Reference/behavior tests cover smooth and rough thick/thin transmission,
reflection/transmission energy partition, paired entry/exit, same/different
instance and material, nested media, critical-angle refraction, TIR and bounded
TIR termination, water termination, millimetre-scale origin epsilon/self-hit
classification, finite and origin-inside shadow transmittance, and explicit
Mobile/High interface and volume limits.

### Exact final Windows evidence

Both final images were inspected directly. Checkpoint 114 visibly contains the
open Gothic chest, lantern cage, transparent panes, and engine flame;
checkpoint 115 visibly contains the ring, full lantern body, transparent panes,
and engine flame. Both manifests report honest RT swapchain presentation.

- Checkpoint 114:
  `reports/task-7-fix-round-2s-closed-volume-absorption-114/capture-manifest.json`;
  PNG SHA-256
  `3dce6ec6d42be04b946a876980ad437c62c0f0b40b4c499ac146b4efeca8f564`.
  Transport overflow, shadow overflow, secondary reject, unclosed volume,
  primary/shadow unclosed, pane-stack failure, instance/material mismatch, and
  interface/volume budget failures are all zero. Intentional attribution is
  closure absorption 3, ordinary opaque-after-open observation 3, TIR 409,
  bounded TIR absorption 58, implicit-origin shadow exits 287, and secondary
  dielectric terminals 380.
- Checkpoint 115:
  `reports/task-7-fix-round-2s-closed-volume-absorption-115/capture-manifest.json`;
  PNG SHA-256
  `58cc8edc76b6349f722200ac477123f5e1299ebd5b75d9d6d2b818a065aa1971`.
  Every failure counter above is zero and closure absorption is zero.
  Intentional attribution is TIR 1044, bounded TIR absorption 129,
  implicit-origin shadow exits 513, and secondary dielectric terminals 539.

The tracked machine-readable record is
`docs/evidence/TASK_7_FIX_ROUND_2_WINDOWS_CAPTURES_2026-08-28.json`.
The final generic embedded raygen SHA-256 is
`24ef76391936a8189acaf4bd35cf6e7d040fb4693b241cb7b931bc4a381f6b0e`;
compiled SPIR-V SHA-256 is
`25acc660bdc5c7cdcf23064a6e9a2d8c35cbfe435b717f433841b24eeb5d621d`.

### Exact final gates and evidence boundary

- Fresh Windows Debug and Release builds each passed 28/28 CTests.
- Generated ABI freshness passed at
  `821418c6c28f1827e5064f8a1551e3aa04c393d9e9df2ff37bdcaf0f6e3dfe54`.
  Generic and legacy raygen compile/freshness checks passed with zero function
  calls and 29 ray-query sites.
- Android `assembleDebug`, unsigned `assembleRelease`, and `lintRelease` passed
  in one 97-task build. Debug APK SHA-256 is
  `8a52e32f0a806c045f913a8bcdcb8501cd2080767caf11fe3848d22223fd970b`;
  unsigned Release APK SHA-256 is
  `6b785a412f7fb396b31124b7c848db2743c75a0bd11f332ab620c03ff6aca47c`.
  The held-item/player package and exact CC BY 4.0 attribution contract passed.
- `adb devices -l` exposed no device. No Android installation, presentation,
  capture, performance, lifecycle, thermal, or owner verdict is claimed.
- The absent external icon's historical locator SHA-256 is recorded explicitly
  as unverified, never as a repository/distributable artifact hash.

Audio/haptic manual revalidation required: NO — this round changes only static
prop processing, bounded dielectric RT math/diagnostics, visual checkpoints,
tests, and documentation. It does not change event identity/timing/transport,
listener or source routing, spatialisation, playback, cue assets, haptics, or
damage/death feedback.
