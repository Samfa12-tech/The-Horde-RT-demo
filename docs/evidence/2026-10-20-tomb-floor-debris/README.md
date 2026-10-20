# Tomb floor masonry chips - 20 October 2026

Six small original fractured stones now sit beneath the existing broken right
roof shoulder, in front of the collapsed masonry. They reuse the MedievalWall02
material and its existing grouped primitive. There are no new lights, textures,
asset slots, collision objects, simulation fields or save bytes. The default
chapter geometry includes them on both platforms.

This is a sparse first decay pocket, not whole-opening artistic acceptance.
Kicking is **not implemented**. All thirteen static asset owners are occupied;
moving chips inside the immutable merged world BLAS would introduce unsuitable
world updates. A future bounded moving-stone owner needs explicit transient
simulation/RT instance ownership. No physics subsystem was added.

The [receipt](receipt.json) pins source, binaries, raw manifests, all 48 original
PNG hashes, completed-frame checks, settings custody and measured costs.

## Geometry, placement and disabled detail

- Six closed fractured prisms add **72 triangles, 216 vertices and 216 indices**:
  13,824 vertex bytes plus 864 index bytes = **14,688 payload bytes**.
- Exact native bounds are `[1.636636, -0.951000, 3.146048]` through
  `[1.790576, -0.891000, 3.360385]`; the bases sit 1 mm into the existing floor.
  Maximum authored height is 6 cm. Deterministic construction checks cover
  closed edges, outward normals, finite orthonormal PBR bases and invalid input.
- On/off owner comparisons preserve existing geometry, material/texture
  identity and provenance. The added triangles have TombDressing provenance.
  Disabling debris omits it entirely and preserves the prior non-debris geometry.
  Legacy architecture-disabled fixtures also retain their previous contents.
- Tests preserve clear swept central-route, bier/grate passage, combat,
  chest/pickup and checkpoint approaches. Collision/support code is unchanged;
  these cosmetic stones are not invisible blockers. This is not a certificate
  for every possible perimeter foot position or physical contact.
- The [first placement audit](placement-audit.json) rejected a pocket behind the
  opaque wall at Z=3.4: all nine coarse samples were occluded from both origins.
  That trial was never shipped or used for an RT gain claim.
- The [final exact-recipe probes](final-placement-probes.json) retain 36 top
  vertices/centroids clear from both origins and 30 floor/low-clearance probes.
  The ordinary spawn faces away; only the labelled diagnostic camera includes
  them in-frame. One lower corner overlaps another chip's side near Y=-0.936257;
  no sampled top-view occlusion results. Float32 emulation and omitted meshes
  remain explicit limitations, with native RT inspection performed separately.

Geometry is original code-authored work with the existing CC0 masonry maps;
see [asset provenance](../../../ASSET_LICENSES.md). No download, paid generation
or new asset licence was needed.

## Builds and tests

Windows Debug/Release application builds and Android Debug with all four packaged
ABIs pass. Android uses Shipping instrumentation / Mobile dielectric quality;
this is build-only evidence. Exact APK SHA-256:
`1586fcc82d841b697c0239c2e56be66c90f4068e56a5ce79aca5af4f9503f5a5`.
No device was queried, installed, launched or changed.

Fifteen affected CTest registrations pass across the retained fourteen-pass run
and corrected launch-test follow-up: tomb dressing/recess, forest merge, collapse,
terrain exclusion, route, campaign checkpoint/play-session/codec, rescue/chapter
journey, static cutover, corridor certificate, graphics defaults and survey launch.
The two journey registrations share an executable. Android's 20 graphics
preference and six performance-profile tests also pass.

Failures remain recorded. The first two-test run found a pre-existing river-camera
assertion, reproduced from exact HEAD test/header bytes in a tiny ignored local
fixture. The first correction fixed coordinates but used the wrong yaw sign;
the affected run therefore retained fourteen passes and that failure. The final
test expectation is `(134.3, 131.2, yaw 3.041924)` for the unchanged E56/N-5 camera
toward E58/N-25. Only that test was rebuilt/rerun, and passed. Production river
geometry/camera and both application executables were unchanged by this correction.
This is targeted coverage, not a claim that every repository test passes.

## Native hardware RT comparison

Four bounded unshown render-only surveys each completed **120/120** matching
presented, fence-completed RT frames. One off/on pair uses `vkCmdTraceRaysKHR`;
the other uses the shared BLAS/TLAS-backed RayQueryCompute backend. Simulation
tick and all published snapshot bytes remain unchanged. Each run captures six
ordinary opening images then six images from the legal shared-floor inspection
camera, with fixed 1/60 render time. No earned progress or combat fixture is used.

RTX 5050 Laptop, driver 610.47, Diagnostic/High, **960x540 at 100%**, High water/fire,
Current shadows, Low dust and **PC Mist On** are matched. Tomb fog is unchanged;
the existing exterior survey fog override is restricted to its river profile.
These short readback runs are not sustained FPS or phone/thermal evidence.

| Whole-RT GPU time, inspection view | Debris off median | Debris on median | Observed delta |
| --- | --- | --- | --- |
| Trace-rays | 2.213408 ms | 2.162448 ms | -0.050960 ms |
| Hardware ray-query compute | 2.151792 ms | 2.149840 ms | -0.001952 ms |

Each median retains frames 70-119 after ten settling frames in that view. Tail,
opening-view timing and exact samples are in the receipt. Negative deltas are
short-run observations, **not an optimisation gain**. No measured regression
appeared in these pairs, so the tiny static pocket remains enabled. A player-facing
debris toggle is **not implemented**; the pre-initialization Debug omission is an
A/B diagnostic only. If mobile cost is unacceptable, omit this detail or add an
actual player option before accepting it on mobile.

All resource counts remain stable during each run. Within either backend pair,
the feature adds zero buffers, allocations, BLAS/TLAS objects, instances,
pipelines, descriptor sets, materials or primitive owners. Existing merged prop
BLAS storage grows by 6,400 bytes. Live inventory changes are **+14,848 host-visible
bytes** and **+21,248 device-local bytes**. These classifications can overlap;
do not add them. Both backends retain 20 BLAS, one TLAS and 28 instances.

Six original frames were visually inspected: both opening off/on images and all
four backend inspection off/on images. Small grounded chips appear beneath the
break without a new obstruction or missing/collapsed geometry. At inspection
frame 119 each pair changes 410 RGB pixels within `[468,398]-[492,429]`. Opening
frame 59 differs by one pixel. This is bounded visual evidence, not owner art,
live movement, shadow/reflection-composition or phone readability acceptance.

Settings INI hashes match before/after every process. All four owned processes
exit 0; no process was killed or left pending. Foreground APIs returned null
throughout, and the game window remained unshown/unfocused. That cannot certify
continuity of a particular external foreground HWND. Complete original manifests
and PNGs remain in ignored `reports/tomb-debris-20261020-r1/`; compact canonical
first/transition/final frame extracts and hashes are committed here, not binaries
or images. The [hidden runner](hidden-rt-runner.py) is an exact-run recipe whose
existing r1 directory must not be reused. Use a fresh named reports directory for
an explicitly authorized reproduction. The offline probe/summary scripts use the
worktree root as cwd and retain their output paths in ignored reports.

## Remaining acceptance and next source slice

Sam's mobile **Mist Off** direction is retained. Shared/Android startup and Reset
already select Off; local graphics tests verify fresh/default and explicit saved
choices. No mobile setting was read or changed, and PC quality was not reduced.
Natural earned saves/death recovery, full tomb-to-lookout/return play, touch/
controller feel, listening, whole-opening art and warm Keeper/exterior performance
remain open. No phone/combat retry, push, merge, release, paid acquisition, new
worktree, canonical-owner reset/stash or Reef action occurred. Canonical dirty
licence/memory file hashes remain unchanged.

Audio/haptic manual revalidation required: **NO**. This static geometry changes
no event transport/timing, source/listener inputs, corridor audio obstruction,
playback, damage or haptic ownership.

The [updated milestone map](../../design/HORDE_1_7_MILESTONE_MAP_2026_10_20.md)
selects dedicated presentation for the four existing dodge directions. Simulation
already moves/protects the player, but the rig currently receives only Idle/Walking
locomotion. Thread transient accepted dodge state through immutable animation
snapshots and add a bounded pose family while preserving all trajectories,
collision, command/timing/damage rules, aim and grips. No new dodge mechanics,
combos, save schema, heavy outside content or healing policy is implied.
