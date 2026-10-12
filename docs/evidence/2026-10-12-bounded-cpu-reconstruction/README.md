# Bounded CPU reconstruction for chapter preparation

Continues PR27 from `b3e3a4ef9da6ec2282139301e1d117bdfeed4316` without
changing scene presentation, shader budgets or ordinary residency. The admitted
immutable source now pins a CPU lease; full replacement reconstruction copies
complete records across turns before the existing bounded GPU uploads. A
256KiB limit produces177 CPU turns,164 upload turns and two AS stages.
Every role, index, material, transform and metadata byte is preserved.

The native full-chapter fixtures exposed a real cleanup defect: the CPU-only
Destroy early return retained the new source cache. Reset now precedes that
return. A separate failing fixture proved a late failed mesh admission could
be copied; an admission marker now rejects it. Red and cleanup-failure evidence
is preserved. Cancellation at every copy boundary, atomic take and move/cache
lifetimes are covered. Callers must prevent mutable aliases to a pinned source.

| RayTracingPipeline | RayQueryCompute |
| --- | --- |
| [Reconstructed owner](pipeline/cutover-01-selected.png) | [Reconstructed owner](query/cutover-01-selected.png) |

Both final native views were opened. Selected/restored output is byte-identical
to each admitted view: four comparisons, zero changed pixels. Real partial CPU
and GPU cancellation preserves the admitted owner. The camera, settings,
shader/build hashes, rope/light state and inventory are in the
[exact receipt](candidate-receipt.json). No foreground input was sent.

Measured cumulative CPU preparation is15.5–21.4ms, distributed over177 turns,
instead of the preceding37.9–43.3ms single packing step. Maximum individual
advance is6.7–9.6ms and cutover3.5–10.1ms across these four Debug hops. These
finite captures are not sustained performance. Allocation/reserve latency is
not time bounded; AS construction and cutover still synchronously complete.
Nonempty family selection still uses the diagnostic synchronous packer.

The source cache adds44,904,764 CPU payload bytes, distinct from allocator/OS
peak memory. Each complete GPU owner remains67,334,720 bytes; temporary owner
overlap is155,579,904 bytes including measured scratch. **Normal-play residency
saving remains zero.** No zone gate or eviction policy is claimed by this slice.

Debug4/4 and Release4/4 pass; four Android Debug ABIs, assemble/lint/unit tasks,
exact APK/ASTC and held-item package checks pass. Original torch180-second
coverage and all production limits remain unchanged. The APK is an intermediate
build, not the final chapter phone candidate, and has not been installed.

Parent CI38193691355 has five passing jobs and a cancelled MSVC lane. Its
Unicode edit test passed1.59s; remote-report viewport assertion failed and the
asset-policy test then exceeded the remaining job time. Those are being
investigated, not reported as green. Normal asynchronous AS preparation,
conservative spatial retention, two-way zone ownership, remaining prepared
wood/stump/snag, full-route gallery/sustained measurements and physical
acceptance still prevent chapter completion. Easy Tree/canopy and mist28 stay
deferred.
