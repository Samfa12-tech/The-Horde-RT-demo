# Existing-master shared forest boundary - 24 October 2026

This is the next implementation boundary, not an accepted forest-density or performance result. The existing pine/alder artwork remains selected. The specific owner deferral of replacement-canopy/Easy Tree generation remains in force.

## What production currently does

`ForestDressing.cpp` loads two LOD1 masters (5,000 and 5,102 triangles), fits one rigid burial offset for each of 64 placements, and copies those vertices into four world-baked PBR material groups. Roots are not deformed per vertex. The common owner contains 323,264 tree triangles. Material batching reduces owners; it does not share the repeated tree geometry or its BLAS.

The new `PreparedForestDressing` separates two immutable asset-local payloads, canonical material mappings and 64 rigid transforms from that compatibility bake. The GLB loader has already applied source-node transforms to the vertices. Those transforms must not be applied a second time. A prepared handle is a snapshot of its support surface, not a cross-generation terrain cache.

Normal gameplay continues through the same four-group bake. The ribbon and native-terrain semantic receipts match the sealed control. This is CPU ownership/preparation evidence only; no VRAM reduction, RT speedup, surrounding forest completion or streaming acceptance follows from it.

## Bounded RT proof now implemented

The [actual pair checkpoint](../evidence/2026-10-24-shared-blas-pair/README.md) now demonstrates shared resources, direct-view equivalence, frozen/reset consistency and same-owner recreation. It establishes no speed gain. Room props/foreground trees prevent a clear isolated reflection witness, so full secondary appearance, unit-scale route use and Android remain open. The explicit proof below remains separate from production forest adoption.

Use a named diagnostic-only generic static-mesh pair in the existing preview room. Retain all seven ordinary roles and keep ordinary preview behavior unchanged. One pine master can be held through an aliasing immutable prepared-owner handle. Use identical canonical materials, texture subsets and two nonidentity transforms in both variants:

| Variant | Extra mesh payload / BLAS | Extra physical TLAS definitions |
| --- | --- | --- |
| Baked control | Two transformed copies grouped by the same three primitives / one BLAS | One identity definition |
| Shared candidate | One original three-primitive master / one BLAS | Two transforms referring to that BLAS and one metadata row |

Totals are honestly eight versus nine physical TLAS definitions and five unique static assets, within the existing 28-definition / 13-asset capacities. Do not pad the control with a masked dummy to hide that overhead. Sharing a custom metadata index means `LastInstanceMasks()` alone cannot prove two physical transforms; record actual definitions, transforms and shared BLAS address equality.

The 8.69 m pine does not fit the ordinary preview ceiling. A common, explicitly diagnostic uniform scale may fit both variants into that room; it proves payload/material/normal/shadow/reflection equivalence, not full-size roots or a real forest view. A full-size proof requires a separately reviewed open fixture. The current generic hidden-preview guard remains; a narrowly named Debug-only hidden pair is necessary for non-foreground validation. No foreground activation is authorized.

The current generic hit decoder already transports geometry normals/tangents through committed object-to-world transforms. No tree-specific shading branch or old private shader substitution is justified. The pine atlas uses Bark 14, Pine 15 and Moss 16; only Bark has normal/ORM 14. Preserve deferred asset texture groups and identical subsets in both variants.

Measure actual allocation bytes, immutable BLAS ownership, TLAS overhead, completed-frame timing and startup/build work. Inspect real RT geometry, materials, edge differences, normals, shadows and reflections, plus pause/reset and successful-idle teardown/recreate. Triangle counts alone cannot admit a performance improvement. CPU-baked versus GPU-transformed floating-point intersections may differ at edges; do not promise byte-identical pixels without observing them.

## Boundary before full production adoption

Production currently uses all 13 unique static-asset owner slots, 31 packed primitive rows and 30 packed materials; Showcase appends two world-material records, filling the final 32-material budget. Full 64-placement TLAS use also exceeds the current 28-definition storage capacity. ForestBark includes deadwood and ForestAlder includes understory, so replacing trees cannot simply remove all four groups. Retaining those two non-tree groups and appending six master primitive mappings yields 35 rows (31 minus two plus six), above the current 32-row capacity. Reusing the existing four canonical material rows can avoid material growth.

These are explicit engine storage/ABI capacities, not evidence of a hardware limit or acceptable frame cost. Existing per-owner primitive ranges can reuse one aggregate asset pointer without adding unique asset owners. Reuse packed payloads and shared metadata where appropriate; do not add 64 material/metadata rows unnecessarily. No production capacity change is admitted by the room proof.

Before changing those capacities or the normal forest route, account for primitive/material headroom, immutable BLAS lifetime, per-frame transform ownership, selected-region publication and recreation failure. Reprove ordinary-static omission provenance: its existing baked-forest certificate must never be bypassed. A hidden tree can still contribute RT shadows and reflections; visual terrain occlusion alone does not remove CPU/GPU/residency work.

Only after this boundary is proven should one bounded F01 north/west background sector be added and judged against the original topography, route enclosure and F04 lookout vista. Preserve the dry northwest-to-southeast Bellwether approach. Town interiors/playable streets remain 1.8, and a count of 64 near-route placements is not a convincing vast forest.
