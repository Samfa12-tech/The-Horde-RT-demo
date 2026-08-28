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
- `RewardLanternHingeSocket`, `Hinge`, `Flame`, and `Light` are now resolved from loaded GLB sockets. Checkpoint staging supplies only the chest world placement; ring/body transforms and emitter/light positions are composed from one authoritative lantern hinge transform. The chest lid is composed from the loaded `ChestLid` rear-hinge root and a named staged -70 degree open pose.
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

## Fix Round 3

Round 3 capture evidence is retained as historical only. It corrected the
chest/lantern relative composition but incorrectly treated asset-local `Y=0`
as the corridor floor; Fix Round 4 supplies the final world-space evidence.

Round 3 supersedes the Round 2 conclusion that any ordinary terminal reached
with an open validated volume could be treated as a shared-edge miss. The old
three-count absorption evidence remains above as historical review evidence,
but that blanket policy is rejected and is no longer present in the shader.

### Historical asset-local reward composition

- The former checkpoint stage translated the complete chest by `+0.37 m`, so
  its authored Y bounds became `+0.37..+0.75 m`. At the same time the ambiguous
  `LanternSocket` target was only `+0.18 m` above the chest origin while the
  lantern body extends downward from its hinge. This inverted the composition:
  the chest visibly floated over and penetrated the lantern.
- Checkpoint staging in this historical round left the chest on its asset-local
  base plane (`Y 0.000..0.380 m`), not the route floor. The reusable chest asset exposes the semantic
  `RewardLanternHingeSocket` at exact local translation
  `(0.000, +1.280, +0.300) m`, intended for the Task 8 reveal/claim contract.
- At the exact shared `0.90` reveal/inspection scale, the body AABB in chest
  space is `X -0.2016..+0.2016`, `Y +0.4025..+1.2665`,
  `Z +0.0696..+0.5304 m`. It clears the chest top by `22.5 mm`. The opened lid
  ends at `Z -0.107700456 m`, leaving more than `177 mm` of AABB separation.
  Ring/body also retain at least the tested base/lid clearance.
- Tests transform every corner of the chest, opened lid, ring, and body AABBs,
  prove floor contact and nonintersection, and prove the full bounds—not only
  socket origins—fit the exact 960x540 raygen frustum. Both checkpoints use the
  same Task 8 transform and camera framing; glass-only mode changes visibility,
  not scale or composition.

### Strict open-stack behavior

- An opaque, miss, or water terminal while any thick dielectric volume remains
  open is again an explicit global/primary unclosed-volume failure. The shader
  retains terminal/volume/material attribution and shows the conspicuous
  bounded fallback; it neither shades through the terminal nor absorbs it.
- Behavioral negatives cover an opaque intersection, a non-edge missing exit,
  and ordinary/water terminals inside nested media. Generic dielectric hits
  still proceed to exact instance/material stack validation. Existing rough
  thick/thin, nested media, inside-origin shadow, finite-endpoint Beer-Lambert,
  water, TIR, epsilon, and Mobile/High contracts remain green.
- The capture correction removed every former open-stack event, so no
  primitive/component/barycentric absorption exception was necessary. The
  retained `primaryClosedVolumeAbsorptionCount` ABI field is zero in both
  captures and has no shader increment site.
- `productionPaneSecondarySameMediumCount` now increments only when both the
  reflection origin and terminal are production-pane instance 8 with the same
  material. Generic same-instance/material reflection terminals no longer
  contaminate this production-specific counter.

### Historical Round 3 evidence and gates

Both final PNGs were inspected directly and both manifests report an honestly
presented RT swapchain frame. The tracked machine-readable record is
`docs/evidence/TASK_7_FIX_ROUND_3_WINDOWS_CAPTURES_2026-08-28.json`.

- Checkpoint 114 visibly reads as an opened floor-contact Gothic chest revealing
  the complete lantern above and forward of the lid/base. PNG SHA-256:
  `9d88984ae26db1446343fd887dd60674f2d81681d3c02719fef1b75b43fdcf09`.
- Checkpoint 115 visibly shows the isolated complete cage, finial, transparent
  panes, and engine flame. PNG SHA-256:
  `52b6530d18f83f8b351dad4fd7e4eb8298ee72516a1fca08606639c38df405f4`.
- Both captures have zero transport/shadow overflow, reject, global/primary/
  shadow unclosed volume, pane-stack failure, open miss/opaque, mismatch,
  interface/volume budget failure, and closed-volume absorption. Both report
  production-pane origin `221`, terminal `211`, guarded same-medium `211`,
  different-medium `10`, primary TIR `168`, bounded TIR termination `15`, and
  secondary dielectric terminal `221`. Intentional inside-origin shadow exits
  are `565` for checkpoint 114 and `429` for checkpoint 115.
- Generic embedded raygen SHA-256 is
  `a148ef9dc3f7137a459d21e480c12ad13c61e14c2ef376ebe37f3caf6d85b3eb`;
  SPIR-V SHA-256 is
  `46128e287c8bf86363cddb529906955fa25fabca29c294122a77cce54716a400`
  (`896640` bytes, `224160` words, `49884` instructions, `7524` branch
  operations, `131` loops, `3080` selection merges, one function, zero calls,
  `29` ray-query sites). Legacy compile/freshness and ABI SHA-256
  `821418c6c28f1827e5064f8a1551e3aa04c393d9e9df2ff37bdcaf0f6e3dfe54`
  also pass.
- Fresh Windows Debug and Release builds each pass 28/28 CTests. Android
  `assembleDebug`, unsigned `assembleRelease`, and `lintRelease` pass in one
  97-task build; the held-item/player package and exact CC BY 4.0 attribution
  contract passes. Debug APK SHA-256 is
  `f8efc8500461b153f25f9c6643fc7b22722b80b90ca54c7efe3199b5f43e2b52`;
  unsigned Release APK SHA-256 is
  `50cb0c6e699296c5860ba15ef189ce4790c7f1ff84a12702549820aec354c6dd`.
- `adb devices -l` exposed no device. No Android installation, presentation,
  capture, performance, lifecycle, thermal, or owner verdict is claimed.

Audio/haptic manual revalidation required: NO — Round 3 changes only static
reward composition, dielectric failure classification/diagnostics, visual
checkpoint framing, tests, assets, and documentation. Gameplay event identity,
timing and transport, listener/source routing, spatialisation, playback, cue
assets, haptics, and damage/death feedback are unchanged.

## Fix Round 4 — final coordinate authority and evidence

Round 4 supersedes every earlier 114/115 capture as the Task 7 final visual
evidence. The glass/transport conclusions and strict failure behavior from
Round 3 remain current; only the route-floor authority, prop world Y, and
camera pitch changed.

### One route-floor and reward-stage authority

- `kRouteFloorWorldY` is the single C++ authority for the authored corridor
  floor at world `Y=-0.95 m`. Initial corridor floor/wall geometry, extended
  route floors/walls, floor-bound corridor props, and the deterministic reward
  stage consume it rather than duplicating `-0.95` literals.
- `kProductionRewardChestStageWorldFromBase` is derived from that floor
  constant and is consumed by both the renderer and production-prop behavior
  test. The chest full world AABB is exactly `Y -0.95..-0.57 m`; the authored
  reward hinge is `Y +0.33 m` and the opened lid origin is `Y -0.61 m`.
- At the unchanged `0.90` lantern scale, flame and light origins are exactly
  `(-12.2401924, -0.1605, -15.27)` and
  `(-12.2401924, -0.1335, -15.27)`. The full transformed body remains
  `22.5 mm` above the chest, and its chest-local full AABB remains
  `177.899 mm` clear of the opened lid. The shared checkpoint pitch is now
  `-0.35`, which contains every chest/lid/ring/body bound in the exact 960x540
  projection. Checkpoint 115 remains the exact same transform at `0.90` scale;
  glass-only mode changes visibility only.

### Final visible evidence

Both PNGs were inspected directly and both manifests report complete, honest
RT swapchain presentation. The tracked machine-readable record is
`docs/evidence/TASK_7_FIX_ROUND_4_WINDOWS_CAPTURES_2026-08-28.json`.

- Checkpoint 114 visibly shows the chest contacting the cobble floor, its lid
  open, and the complete reward lantern revealed above/forward with transparent
  panes and the engine flame. PNG SHA-256:
  `4fb924c25e35052116a46d7e1102c907acb19f8e863ecf0d657fbc17e168890b`.
- Checkpoint 115 visibly shows the isolated complete Gothic cage, ring, finial,
  transparent panes, and engine flame at the grounded reveal transform. PNG
  SHA-256:
  `05e8ecdbf03d2c0573db3f939c27a931d3e5eef2b3db38c9e0521e26c2c19477`.
- Both captures have zero transport/shadow overflow, secondary reject,
  near-self-hit, global/primary/shadow unclosed volume, production-pane stack
  failure, open miss/opaque, mismatch, interface/volume budget failure,
  terminal/volume/material mask, and closed-volume absorption counters.
- Both report production-pane origin/terminal/guarded same-medium `225/225/225`,
  different-medium `0`, primary TIR `869`, bounded TIR termination `134`, one
  intentional finite-endpoint shadow volume, and secondary dielectric terminal
  `225`. Intentional implicit-origin shadow exits are `323` for checkpoint 114
  and `291` for checkpoint 115.

### Final gates and evidence boundary

- Focused Debug and Release tests pass 4/4. Fresh full Windows Debug and Release
  builds each pass 28/28 CTests.
- Generic and legacy shader compile/freshness pass unchanged. Generic embedded
  SHA-256 is
  `a148ef9dc3f7137a459d21e480c12ad13c61e14c2ef376ebe37f3caf6d85b3eb`;
  generic SPIR-V SHA-256 is
  `46128e287c8bf86363cddb529906955fa25fabca29c294122a77cce54716a400`.
  ABI freshness passes at
  `821418c6c28f1827e5064f8a1551e3aa04c393d9e9df2ff37bdcaf0f6e3dfe54`.
- Android `assembleDebug`, unsigned `assembleRelease`, and `lintRelease` pass in
  one 97-task build; the held-item/player runtime-package and exact CC BY 4.0
  attribution contract passes. Debug APK SHA-256 is
  `094ef6c34c68ffebd44c818926a58e13e6138d947adaeb5b16eb81f6db9072a8`;
  unsigned Release APK SHA-256 is
  `7b7ee35f7326af5b9feda6e725ae9c99b52b3f83656b002e64014c73967c1cde`.
- `adb devices -l` exposed no device. No Android installation, presentation,
  capture, performance, lifecycle, thermal, or owner verdict is claimed.

Audio/haptic manual revalidation required: NO — Round 4 changes only static
world-space prop placement, deterministic visual framing, tests, and evidence.
Gameplay event identity/timing/transport, listener/source routing,
spatialisation, playback, cue assets, haptics, and damage/death feedback are
unchanged.
