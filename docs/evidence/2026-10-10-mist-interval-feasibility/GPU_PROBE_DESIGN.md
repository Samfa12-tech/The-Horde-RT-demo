# Proposed exact GPU endpoint and cell probe

This is an instrumentation design only. It was not added to the existing prototype harness, compiled, or dispatched.

Use a small fixed list of frozen camera pixels already in the prototype correctness report, and force each source identity for each ray in the diagnostic pass. Emit one record per `(quality, pixel, aperture)` into a bounded readback SSBO. Keep the existing candidate and raw interval limits unchanged. The probe must use the very same cone discovery, clipping, sort, union, and cell-coverage functions as the image path; it must not recompute an alternate interval implementation in the shader.

Each record should contain:

```text
uint schemaVersion
uint qualityId
uint sourceId
uint pixelX, pixelY
uint invalidFlags          // bit 0 callback overflow; bit 1 raw overflow; bit 2 invalid cell
uint candidateCallbacks
uint rawIntervalCount
uint unionIntervalCount
float endpoints[64][2]     // sorted, disjoint [start,end); only first unionIntervalCount entries valid
uint sampleCount           // exactly the selected 2, 6, or 8 schedule
float cellCoverage[8]      // first sampleCount entries, ordered near-to-far
```

For successful records, compare every endpoint to the CPU oracle with a predeclared absolute tolerance, and compare each cell's coverage with a separate tolerance. For invalid records, require a nonzero invalid flag and ignore all endpoints and coverage. The readback consumer should reject count values above their capacity before indexing arrays. A forced-source bit is diagnostic-only, cannot affect normal presentation, and has to restore the ordinary source-selection path after probing.

The fixed ray roster should include both an aperture-zero and aperture-one weighted witness, a ray with disconnected shadow intervals, and a ray with overlapping triangle intervals. If the frozen geometry cannot provide both interval shapes for each aperture, report the uncovered combinations rather than substituting synthetic GPU data. Add separate synthetic GPU controls for exact 64/65 raw intervals and 512/513 callbacks only if they can inject candidate records into the same post-query processing path without changing production descriptors or runtime code. Those controls would test bounded shader mechanics, not hardware BVH visitation.

The output distinguishes data the previous union-length capture collapsed: endpoint positions, overlap merge, disconnected segments, and each cell's coverage vector. It still would not prove moving-rope AS updates, production ownership, driver portability, visual quality, or performance.
