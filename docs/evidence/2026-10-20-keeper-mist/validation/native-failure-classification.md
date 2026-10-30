# Native baseline classification and CPU ownership note

Both existing failures reproduce before the Keeper/mist fixture changes, on clean
tracked sources at `eaffada89b02cd0ce542a622ee656f67ae989952`. The two existing
Release targets built successfully. Focused CTest returned exit 8: resource
inventory failed in 4.33 s and character-slot smoke failed in 3.52 s, total
7.88 s. This is a failing baseline receipt, not a pass or a fixture regression.
No source was edited. The Release lane was returned to the parent after these
two tests; no app/device action or process termination was performed.

Receipts: [initial clean-source identity](native-baseline-source.json),
[Release build](native-baseline-release-build.log),
[CTest summary](native-baseline-release-ctest.log),
[complete focused output](native-baseline-release-ctest-detail.log),
[failure index](native-baseline-release-failed.log), and
[asset/source classification proof](native-failure-classification-proof.json).
The latter records matching baseline Git blobs for both tests, both affected
shader includes, the Android bridge and terrain admission. The runtime GLB's raw
SHA-256 matches the baseline committed LFS object; reading its index accessors
independently gives 37,846 triangles.

## Resource inventory

The stale terrain quantity explains both broad failure messages. This corrects
the earlier CPU-preparation note's ambiguous wording about a rejected roster:
the printed primitive/material counts satisfy the frozen roster assertions.

- `tests/RtSceneResourceInventoryTests.cpp:478` pins terrain support to 34,050
  triangles. Current r23 is 37,846; the same admitted triangles generate support
  through `src/scene/ChapterTerrainAdmission.h:20`.
- The aggregate index formula at test line 1031 repeats 34,050. Actual chapter
  indices are 1,291,902 versus expected 1,280,514. Their difference, 11,388, is
  exactly `(37,846 - 34,050) * 3`.
- Observed materials/primitives/layers are 30/31/18,15,15,1 for Mobile and
  30/32/18,15,15,1 for High, matching lines 468 and 476-477. The intake output
  also matches the baseline index count, baseline height and entry height in
  lines 1032-1039. The unchanged manifest ceilings remain 21,134 vertices and
  39,618 triangles; actual r23 has 19,486 vertices and 37,846 triangles.

History attributes the stale support pin to `8b098d370`; the aggregate formula
still contained it in `1906a733b`. Terrain commits `11b768dd` and `48662cc1`
changed r21 34,050 to r22 36,746 and then r23 37,846 within unchanged ceilings.
The admitted asset SHA-256 is
`c636a67911b7f5c5381c30ec7a49e5d4ff15c70fe688b368ba52efe77b537774`.

Smallest follow-up: update the two terrain expectations to the hash-pinned
current count, preferably using one named expected quantity. Retain all capacity,
layer, instance, emissive, copied-payload and baseline-geometry checks. Do not
increase resource capacities or replace the total with an unexplained number.
Rerun the focused target afterward: the combined predicates do not individually
report every successful subcondition, so this diagnosis does not claim a pass.

## Character-slot smoke

The eight reported failures are stale source-text expectations introduced before
the current implementation in `b2d548a2`:

| Existing assertion | Current implementation and classification |
| --- | --- |
| Six exact normal branches, `CharacterRenderSlotSmoke.cpp:957-967` | `rt_hit_decode.glsl:7-17` computes axis=`normalCode/2`, sign from its low bit. Codes 0-5 still decode to +Y, -Y, +X, -X, +Z, -Z, exactly matching the CPU enum. The six absent literal branches account for six failures. |
| Unconditional water distance addition, test lines 1339-1347 | `rt_dielectric_common.glsl:169-180` adds `h.t + waterPathLength * float(!regionalWater)`. Regional query distance already includes travel to the bed; thin-film query starts after its separate exit. Adding the regional water distance twice would be incorrect. Reflection local distance and camera-path accumulation remain at lines 197-203. |
| One-line RT tuning assignment, test lines 1792-1796 | `android_probe_bridge.cpp:3940-3941` selects default tuning for an isolated survey and one `gRtLabState.Snapshot()` for an ordinary frame. The route reset remains present. The obsolete unconditional assignment is absent. |

Smallest follow-up: maintain these assertions around the current contracts,
including all six signed axes, both regional/thin-film distance cases, ordinary
snapshot coherence, isolated-survey defaults and route reset. Do not merely check
that functions exist or restore obsolete shader/bridge statements. Source/math
inspection is not a physical water or mist appearance acceptance test. No new
renderer contract defect is established by these eight string failures.

## Why skinning cannot simply precede the sole fence

The fresh phone baseline measured roughly 5.9 ms player plus 5.2 ms companion
skinning at the exterior landing (11.136 ms total), and 6.608 ms player-only in
the lower view. Landing fence wait was 137.828 ms and GPU duration 137.713 ms.
These are independent medians from the prior matched run, documented in
`docs/evidence/2026-10-20-phone-performance/cpu-preparation.md`; fence and GPU
durations overlap and must not be added or treated as an implemented speedup.

Android's successful sole-fence wait is at `android_probe_bridge.cpp:3817` and
owning GPU/Diagnostic completion at 3840. Ordinary input/simulation advancement
occurs afterward at 4337-4351; the current value frame is built at 4428.
Consequently the fresh pose needed for this frame does not yet exist before
the wait. Moving sampling/advancement changes the input-time boundary; using
the old pose changes geometry/equipment freshness; advancing twice changes
simulation/events. The wait also protects reused mapped buffers and AS owners.

The smallest reversible next slice is a CPU ownership fixture/refactor retaining
the current post-fence order, with one owned immutable preparation bundle on the
same thread. Carry exact input/animation identity and scene/surface/actor
generations alongside tick, body/view streams, sockets and held transforms.
Tick alone cannot distinguish a same-tick pose/checkpoint change. Existing
`PlayerRenderSlot.h:245-259` outputs are borrowed until another update, while
`SkinnedMeshAsset.h:258-261` has mutable scratch/rig caches; copying references
or adding worker access does not create an immutable bundle.

The focused host test should compare the bundle's bytes/sockets against the
current path, then prepare A and change the same-tick request or generation to B:
A must be rejected or rebuilt. A controllable unsignalled/failed fence must
observe zero GPU writes/recording. Abort recording or submission, retry, and
require the same prepared payload plus pending refit to survive until exactly
one successful submission (`CharacterRenderSlot.h:114-116`). Keep simulation
steps/events unchanged and publish no completion evidence for failed attempts.
Mapped writes, BLAS/TLAS recording, Diagnostic IO and resource retirement remain
after proven fence ownership. This slice establishes ownership/freshness before
any scheduling experiment; no CPU optimization or renderer rewrite is adopted.
