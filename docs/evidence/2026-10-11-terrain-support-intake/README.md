# Terrain support and native-intake milestone

This is a tested CPU preparation foundation for the chapter terrain, not live terrain admission, a playable chapter candidate, full capsule collision, GPU streaming, or visual acceptance. Production gameplay, rendering, shaders and all existing limits are unchanged.

## Changes

- Build immutable support from native-imported world-space triangle positions and admitted ground/stone material identities. The native importer has already baked node transforms; the adapter does not transform them again.
- A bounded BVH selects the highest upward face and its actual plane height/normal. Steep faces remain explicitly nonwalkable; downward shell faces cannot become floors.
- Movement probes require continuous projected triangle coverage, including triangle-boundary height discontinuities. The first implementation failed a five-millimetre crack placed between 10 cm samples. That failed run is retained; the correction checks intervals rather than merely increasing sample frequency.
- Nine probes remain a footprint approximation, not a swept capsule certificate. Authored obstacles, headroom, arbitrary holes between probes, generation publication and renderer readiness still require integration. Intersected steep upward faces conservatively reject a sweep, even beneath another floor. Interval capacity overflow fails closed.
- The BVH reserves its exact recursively determined node count rather than twice the triangle count. On the same 39,618-triangle candidate, retained CPU array allocation (excluding object/control-block and allocator overhead) falls from 5,388,048 to 2,873,928 bytes (2,514,120 bytes saved), with the same query results and 57,095 tested triangle candidates. This is CPU allocation evidence, not a GPU or frame-rate claim.
- Malformed native intake commands now fail instead of silently running the ordinary asset suite. Separate registered negative tests cover missing and unknown arguments.

## Current local validation

Windows Debug and Release: 4/4 affected registered CTests passed in each configuration. These include the native GLB suite, its two intake-command negatives, and the new support suite. The support cases include shared edges, reverse travel, outer edges, thin cracks, short raised slivers, allowed small steps, stationary support, slope limits, overlapping heights, invalid coordinates/geometry, exact interval capacity and overflow.

The actual local r13 source candidate imported through `StaticMeshAsset::Load`: 39,618 triangles, 21,134 native vertices, two primitives and two materials. GLB SHA-256: `fbfae78a332c2d8b5af9ae511906a9675f8c0eff6acfa9d6ccd386e95a5f0a0a`. The source candidate remains under refinement and is not a runtime package in this checkpoint.

Native support checked 309 route stations, 2,781 centre/ring queries and 616 forward/reverse sweeps, all passing. The Release observation was 13.0003 ms support preparation and 15.4643 ms for the whole route check. These are one local host observation; no sustained performance or mobile conclusion follows. The test exposes a read-only `--inspect-terrain-support <glb> <manifest> <stations>` route, where each station row is engine X, Z, proposed Y.

| Test artifact | SHA-256 |
| --- | --- |
| Debug static GLB suite | `6dc2631620fbb2afc461bad9dc75e6fced1d9760e6ab1319af0791ebefeeb328` |
| Release static GLB suite | `911205fe24b4a2c0e710eca01c7b687f7ae3c48900fc2266155c9d451566b339` |
| Debug triangle support suite | `93fd9f600bce1e8ea6ed237739803f0ba2a86866679096ec07ccf9f485de25bf` |
| Release triangle support suite | `165a6abf4f08279bfd0e0980df1aca93e99320d01e42490553410be17f34ebfd` |

The Vulkan-host CI roster now explicitly builds the support target and selects the three new CTests, preserving all existing route/rescue selections. Exact pushed-commit CI is recorded separately when complete.

## Remaining chapter gates

No live switch to this support until matching terrain geometry and destination collision/render readiness are authoritative. The r13 entry mound still needs visual refinement. Native RT captures, full capsule/headroom and terrain-edge checks, actual two-way GPU lifetimes, visible Kit and the complete dressed route remain outstanding. Phone and owner acceptance remain pending. No game/device was launched for this CPU milestone. Audio/haptic manual revalidation required: NO for these preparation/test-only changes; earlier chapter cue changes retain their existing pending gates.
