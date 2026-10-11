# Disconnected zone families in one existing RT owner

Implementation parent: `6b0e02d3b31bf042818eb91468ff78b0ccb4a25b`.

The current chapter saturates the 32-primitive table. The native common+forest
selection needs source families 0, 1 and 5–10, separated by the tomb dressing
families. Allocating more BLAS owners or primitive rows is not an admitted way to
work around that limit.

## Implemented and checked

`StaticRtAssetRegistration` now accepts a borrowed, strictly increasing source
primitive list during `InitializeSelectedGeometry`. Its retained union packs
once, in source order. One owner's result must be contiguous in the packed hit
ABI: interleaving unrelated owners is rejected. The legacy packer rejects this
explicit route before mutating admitted data. Contiguous/default registrations
continue to use the existing behavior.

The behavioral test first failed 12 assertions with the previous implementation.
It now checks exact retained geometry/material identity, duplicate/reversed/
out-of-range/overflow selections, conflicting ranges, invalid roles, legacy
rejection, failure preservation and identical overlapping owners. The native
fixture additionally imports the actual entry, tomb sources, regional terrain
and tree pair, checking every selected position, normal, tangent, UV, local
index, geometry transform and material record.

| One existing owner | Primitive rows | CPU vertex payload | CPU index payload |
| --- | ---: | ---: | ---: |
| Complete native chapter asset | 11 | 26,252,736 B | 4,606,848 B |
| Common + tomb | 5 | 2,119,872 B | 319,080 B |
| Common + forest | 8 | 25,590,336 B | 4,444,656 B |

These are upload payload counts, not GPU allocation savings. Common masonry
still contains tomb surfaces. Shared material rows and texture identities remain
intact. No capacity, shader, geometry, mask, quality or deadline was changed.

Final focused tests pass 2/2 in Debug (12.56 s) and 2/2 in Release (3.20 s),
covering native forest dressing and static GLB admission. Their raw logs and
the red run are retained beside the build logs.

## Native renderer regression inspection

Both Windows applications build. The actual Debug forest-bend captures were
opened and inspected, on RTX 5050 Laptop GPU at 960×540 / 100%, Diagnostic High.
[Pipeline](bend-pipeline.png) and [QueryCompute](bend-query.png) preserve their
respective parent output: [full-frame comparisons](frame-comparisons.json) show
zero changed pixels, with matched camera, rope/equipment state and shader hashes.
Manifests and exact executable/source identities are adjacent.

The scene still uses its complete registration in normal play; explicit lists
are an opt-in preparation capability. These captures prove unchanged normal
rendering, not a rendered subset or GPU zone handoff. The inspected sparse
canopy remains the owner's deferred tree task. This is not the full chapter
visual acceptance gallery or a sustained performance measurement.

## CI recurrence repaired in the parent

All six jobs passed on exact parent `6b0e02d3`:
[run 38170448572](https://github.com/Samfa12-tech/The-Horde-RT-demo/actions/runs/38170448572).
MSVC ran 132/132 tests; the Unicode note completed in 328 ms under the unchanged
2,000 ms deadline, and the native UI test passed in 0.63 s. Vulkan-host ran
39/39, including the seven requested traversal CTests. GCC/Clang each ran
121/121. Android Debug ABIs, Java contracts, lint and package checks, and the
selected sanitizer job passed. This is current-parent evidence, distinct from
this selection checkpoint's subsequent CI.

The earlier `37310bd5` failure and injected deadline failure remain retained in
[the font-policy record](../2026-10-12-report-ui-font-policy/README.md). The trace
supports a font-measurement slow path; this passing cold-host run does not prove
which particular Windows font service caused it or guarantee all future hosts.

## Remaining implementation boundary

Real zone loading/retirement is still required. Prepare an isolated GPU arena and
changed BLAS/TLAS while the old scene remains valid; commit only after current
generation and completed-work checks, invalidating all recorded references.
Rebind dynamic actor/droplet uploads and retain required shaft, ground, rope,
lid, secondary-ray and light contributors. Failure must preserve the safe side
and destroy only the candidate. Measure actual allocation overlap/retirement.
CPU selection alone must not be labelled streaming.

The chapter is incomplete: graveyard/lid composition, remaining prepared props,
full gallery, sustained measurements and owner/phone acceptance remain open.
Phone Current/Higher shadows and saved Lower→Current migration remain physically
pending. Mist #28 stays deferred. No interactive game launch, phone install,
merge or release occurred. Audio/haptic manual revalidation: NO for this slice.
