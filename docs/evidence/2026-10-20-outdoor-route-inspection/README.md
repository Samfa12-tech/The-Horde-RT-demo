# Outdoor route inspection and capture repair - 20 October 2026

The three canonical exterior inspection checkpoints can again reach the first
bend, second bend and lookout through the actual rope and prepared trail root.
This is a diagnostic-fixture repair, not a production collision or placement
change. Work continues in the existing WP2 worktree/branch and draft PR 27.

## Finding and bounded correction

The first current-source hidden capture failed before producing a shot. Its
player stopped at `(-23.8234,-3.55062)`, just before the authored root centred at
`(-23.5,-3.4)`. The checkpoint imported historical showcase movement rules, which
treat STEP triangles as blockers. The current bounded step motor was disabled.
Terrain was ready: the printed `terrain=1,2,2,2,2` means terrain presence and four
readiness enums, not stale terrain generations. This was not a demonstrated
live-player path obstruction or floating-root defect.

Only checkpoints **201/202/203** now use the existing 240-tick accepted-hit/death
seed, already used by the reunion. It enables current rules before the same
ordinary ascent and fixed-step walk. Historical showcase imports retain their
separate policy. No teleport, pose injection, collision exemption, removed root,
new readiness grant or weaker capture validator was added.

The new regression uses the fully assembled production terrain, graveyard,
trees, ferns, rocks and deadwood/contact. It compares legacy rejection and
bounded motor acceptance on the identical root, then observes ascent/pull-up,
actual raised Wood support, before/after root traversal, bounded movement and
the exact authored final camera/native support for all three checkpoints.

The [red run](red-test.log) reproduced all three failures at the same point,
plus their absent-current-seed assertions: six failed checks, retained unchanged.
The [final selected lane](host-tests.log) passes **6/6 CTest registrations in
26.89 seconds**, including prepared dressing/deadwood, step movement, development
asset/staging policy and rescue/terrain journey. The same-root collision check
and original route assertions remain; no assertion was weakened.

## Actual RT image review

The corrected Debug build produces four complete native RT shots:

| Shot | Backend | Inspection |
| --- | --- | --- |
| First bend | RayTracingPipeline | The sand trail remains visible through the banks/trees; no demonstrated floating base or blocked route in this framing. |
| Second bend | RayTracingPipeline | Trail continuation and distant village roof remain legible; rock/deadwood projections need no justified relocation from this view. |
| Lookout | RayTracingPipeline | Village/church silhouettes remain unobstructed beyond the trail endpoint. |
| Same lookout | RayQueryCompute | Same scene/camera/settings and native BLAS/TLAS traversal; landmark/readability finding agrees. No cross-backend pixel identity is claimed. |

Each shot follows twelve settling frames and has its own matching completed,
successfully presented RT frame with final-idle ownership. The manifests retain
shader identity, real dispatch/copy, camera, equipment/visibility and image hash.
All four are 960x540 at 100%, Diagnostic/High, High water/fire and Higher shadows,
with production lighting configuration and Windows Mist On. No lighting or user
graphics setting is edited. Mobile Mist Off remains separate and unchanged.

Original images/OBJ files remain only under ignored
`reports/outdoor-composition-20261020-r2/{first-bend,second-bend,lookout,lookout-query}`.
Their hashes and exact executable are in [receipt.json](receipt.json); raw owning
manifests are retained here. They are diagnostic accepted-hit/death-seeded route
inspections, not naturally earned victory/saves, full-route owner acceptance or
a matched lighting comparison against historical images.

Before correction, historical route contact sheets, the previous loaded-lookout
shot and older water/woodland shots were inspected. Their hashes/status are
recorded separately. The old `woodland64-lookout-settled` manifest is incomplete
because primary arms were absent; its image does not become accepted evidence.
The retained historical views are context, not the corrected candidate's proof.

No new placement/intersection defect was established in these selected views.
The existing grounding test reports 4,992 imported root samples, zero exposed,
with maximum gap about -0.020 m. This is strong root-footprint evidence, not an
artistic certification of every dressing object or a full 360-degree survey.
Understory currently uses centre/slope grounding; its whole-footprint coverage
is a remaining audit limit, not an invented visible defect.

## Wind/LOD gap and selected small next experiment

Living trees still admit only static LOD1 pine/alder; trees and ferns bake into
the existing static owner. Wind has no bounded deformation/refit owner. Current
shots use **20 BLAS, one TLAS and 28/28 TLAS instances**. A quick wind toggle that
refits the entire forest or raises the instance cap is not justified.

The [read-only asset audit](lod-audit.json) reads actual GLB accessors:

| Existing mesh | Vertices | Triangles | Native vertex/index payload |
| --- | ---: | ---: | ---: |
| One pine LOD1 | 5,602 | 5,000 | 418,528 bytes |
| One alder LOD1 | 5,632 | 5,102 | 421,672 bytes |
| 32 of each | 359,488 | 323,264 | 26,886,400 bytes |

Payload uses the existing 64-byte native vertex and 32-bit indices; it excludes
AS/allocator overhead, textures and other dressing. It is not a GPU-time claim.

**Selected next experiment:** one original pine LOD2 derivative in a Debug-only
static-bake A/B, retaining exact bark/root/contact, existing PBR families and
instance ownership. Measure candidate file/vertex/index bytes and actual native
allocation/GPU deltas at fixed near/mid/far views, including silhouettes,
connectivity and supported shadow/reflected/transmitted visibility. Reject it if
shape or secondary views regress. Keep production defaults, collision and all
existing caps fixed. No derivative was generated, admitted or adopted in this
checkpoint; no saving or completed runtime LOD transition is claimed. This is
the smaller reversible investigation selected before a wind ownership rewrite.

## Exact scope and remaining work

[Source freeze](source-freeze.json) pins the three changed build/test/registry
files before final checks, Debug build and captures. Windows Debug and four-ABI
Android Debug Shipping/Mobile builds pass; no Java source changed or Java suite
was rerun for this fixture-only change. Release host tests compile the corrected
registry; a new Windows Release application was not needed for Debug-only capture.
Raw failure/build/test/process evidence is retained. The final read-only review
found no actionable defect and confirmed the narrow scope.

The game has no `.game-dev/adapter.json`; the Game Visual Debugging skill's
bounded-evidence approach was applied through its existing native hidden capture
workflow. No adapter was installed. Every owned process exited; the native window
stays unshown. Outer foreground samples were null throughout, so they do not
identify any active external window. The Windows owner INI stayed byte-exact.

Geometry/assets, route boundaries, landmark placement, collision, saves, lighting,
shader/quality/capacity policy and canonical owner licence/memory remain intact.
No phone, public push, release, town expansion, participating mist/shafts, purchase,
new worktree, reset/stash, process termination or Reef operation occurred.

The [six recovery choices](../../design/HORDE_1_7_RECOVERY_DECISIONS_FOR_MORNING_2026_10_20.md)
remain unchanged and pending for Sam. They do not block independent work. The
[remaining-scope inventory](../../design/HORDE_1_7_REMAINING_SCOPE_2026_10_20.md)
retains unfinished outdoor effects/content and physical acceptance separately.
Audio/haptic manual revalidation required: **NO for this diagnostic-fixture-only
slice**; the earlier audible checkpoints retain their own listening requirements.
