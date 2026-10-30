# Bounded static primitive ownership — 12 October 2026

Source parent: `a0e0cc7e5df1e170dfdc175c3a7a5bb6bea53e35`.
This removes a CPU admission obstacle to independent zone BLAS ownership.
**The production chapter still uses its existing combined BLAS and resident
buffers. No GPU loading, retirement or streaming is claimed by this milestone.**

Previously, repeated registration of a packed asset always exposed every
primitive. The [failing baseline](before-test.log) reproduces fourteen ownership
and invalid-range failures. Registration now accepts an optional explicit
asset-local primitive offset/count, restricted to the static PBR role, with
empty/out-of-bounds/overflow rejection before packing. Default zero/zero retains
the whole-asset route. Each unique asset still packs once; overlapping instances
may deliberately share immutable ranges. Geometry, materials, texture aliases,
descriptors and shader/ABI records are unchanged.

The tests cover distinct and overlapping owners, registration reordering, local
hit indices/materials/vertex spans, both integer overflow boundaries, rejected
player roles, shared-provider texture aliases, and the saturated thirteen-asset
ceiling. Range aliases add neither a fourteenth asset nor packed primitive rows.
Existing BLAS geometry construction and StaticPBR hit decoding both address
`primitiveBase + BLAS-local geometryIndex`, matching the new metadata.

The real native chapter proof loads current entry, tomb, terrain and all 64 tree
placements, then performs the actual coalescing. Its eleven rows partition into
two shared rows, three other tomb families and six terrain/forest families.
Every triangle/index/vertex byte, material row and atlas allocation is preserved;
native packed chapter vertex/index bytes are 26,252,736 / 4,606,848.
The shared masonry row deliberately includes tomb architecture. It must remain
resident unless a later safe partition separates it. The complete current scene
already uses all 32 primitive rows; this proof does not raise that limit.

## Validation and inspected compatibility

- Debug/Release applications and affected native targets rebuilt. Eight affected
  CTests pass in each: ABI, tomb/forest dressing, static GLB/collapse, scene
  preflight/resource ownership and viewmodel admission. A final fixture-only
  change guards reads after failed initialization; the two affected tests are
  rebuilt and rerun separately. Logs and native output are retained here.
- Android four-ABI Debug build, unit-test and lint tasks succeeded. Unchanged
  Java/lint analysis was UP-TO-DATE. Closed package and held-item/source-separation
  checks passed. No phone install or execution.
- Both actual RT frames below were opened and inspected: RTX 5050 Laptop,
  960×540 at 100%, Diagnostic High, HIGHER shadows, checkpoint
  `chapter-landing-north`, twelve settling frames. Comparison with the parent
  records zero changed pixels and unchanged geometry/material/texture bytes,
  live resource counts and shader identities. The overhead apparatus remains;
  these are compatibility frames, not grounded-stake or chapter acceptance.
- [Exact file/build/APK and preserved budget/backup identities](identities.json),
  [pixel comparison](image-comparison.json), and
  [parent six-job CI receipt](ci-a0e0cc7e-verified.json).
  Subsequent exact-commit CI is reported on PR27 after execution.

| Backend | Inspected frame | State / shader / resource manifest |
| --- | --- | --- |
| RayTracingPipeline | [Landing](pipeline-normal.png) | [Manifest](pipeline-normal.json) |
| RayQueryCompute | [Landing](query-normal.png) | [Manifest](query-normal.json) |

No shader, capacity, quality, deadline or assertion is weakened. The original
torch sweep and its 180-second policy remain preserved, not newly executed here.
Twelve frozen frames are not sustained performance evidence.

## Remaining chapter work

Independent immutable zone owners must still acquire, publish, retain and retire
their actual BLAS, vertex/index, metadata and texture allocations with owning
submission fences and stale-generation rejection. CPU subranges and a TLAS mask
swap do not establish those lifetimes. Keep all required secondary-ray/light
contributors until a measured retention policy proves them unnecessary.

The measured native rig cannot reach a grip at apron Y=2.076 from the suspended
rim staging point, even with the 0.72 m bounded fold. A short edge stake with a
tie around Y=2.55–2.70 and a 0.25–0.35 m lateral offset is a viable measured
candidate, requiring actual rope/apron contact, slack-tail management, rig,
equipment/capsule clearance and two-backend inspection before enabling it.
The stake/graveyard, full inspected route, sustained costs and owner/phone gates
remain incomplete. Mist issue28 and replacement-canopy work remain deferred.

Owner checks for the eventual integrated candidate: tomb/guards/Keeper/claim;
stake deployment and two rope round-trips; fresh exterior lantern raise and
Kit speech; trail/lookout/backtracking; pause/reset and equipment/light restore.
Android must physically check Current/Higher and saved Lower → Current migration,
touch/lifecycle behaviour and the same chapter route. No new playtest launch was
performed here. Fresh wider-chapter audio/haptic review remains pending; this
admission-only change does not alter playback.
