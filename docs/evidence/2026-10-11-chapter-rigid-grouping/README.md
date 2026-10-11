# Chapter rigid-primitive grouping

The development rescue chapter now coalesces the chest base's adjacent iron/latch primitives and the lantern ring's ring/hinge primitives. The production native importer already bakes those nodes into vertex positions; required latch/grip/hinge sockets are independent records and remain intact. The articulated lantern body remains a separate owner. Original GLBs, textures, materials, quality profiles and shaders are unchanged. Legacy/preview routes are unchanged.

This recovers two primitive metadata slots: **High 32 -> 30, Mobile 31 -> 29**, within the unchanged capacity of 32. Materials remain 30, texture uploads remain 15/12/12 and asset-owner capacity remains full at 13. This is capacity recovery for subsequent terrain admission, not streaming or a frame-rate improvement.

## Native and visual evidence

The new integration assertions failed before implementation (31/32 primitives); the retained red run is under ignored `reports/chapter-rigid-grouping-red-test.log`. The corrected real-asset test checks every world-baked vertex byte, ordered triangle/material tuple and required socket against the original native imports. This supplements the existing idempotence and invalid-index grouping tests.

Matched `lantern-glass-production` (115) captures use Diagnostic High, 960x540 at 100%, HIGHER shadows, High water/fire, Standard dust, mist On and twelve settling frames. Native Pipeline and QueryCompute both presented successfully and completed their owning submissions. Both images were opened and inspected. **Each backend's image has zero changed pixels against its prior exact-byte-texture-reuse baseline.** Camera, scene state, shader identities, quality and lights also match.

| Backend | Baseline | Candidate |
| --- | --- | --- |
| RayTracingPipeline | [Prior capture](../2026-10-11-chapter-texture-reuse/pipeline-after.png) | [Grouped capture](pipeline-after.png) |
| RayQueryCompute | [Prior capture](../2026-10-11-chapter-texture-reuse/query-after.png) | [Grouped capture](query-after.png) |

Primitive metadata is 512 -> 480 bytes. Measured device-local allocation is 299,956,848 -> 299,956,816 bytes on each tested backend. Vertex/index counts, texture allocation and production-prop BLAS bytes are unchanged. No timing improvement is claimed from these short settling captures.

[Exact receipt](receipt.json) records executable/shader identities, owning frame, camera, lights, settings and resource counts. Debug EXE: `f2f5b27703a7c1b26aa36457b72ac9869e9f57b32f2951c1dd1dc33d633090bd`. Release EXE: `38933a313c11c33d40b37f67f682157a6958a2348fecaa300c9bb0c54db361ab`.

Windows Debug/Release builds and real native resource tests passed. Six affected Debug CTests passed (81.51 seconds): resource inventory, production props, held-item sockets, held-item vertical rig, rescue attachment and rescue adversarial regression. The same six Release CTests passed (10.47 seconds). Exact-commit CI is recorded separately when complete. Frozen shader bytes are the same as the preceding validated capture; no shader regeneration or budget change occurred.

A first hidden capture process was started without retaining its parent; it produced no receipt and is not counted as evidence. The subsequent bounded capture workflow retained process ownership through completion. No owner input was sent, no interactive playtest or Android installation occurred.

Remaining: material/zone-owner admission, actual terrain integration and support publication, Kit ownership, the complete chapter/gallery and physical phone acceptance. Previously retained held-lantern checkpoint failures are unaffected and remain unresolved. Audio/haptic manual revalidation required: NO for rigid grouping; existing chapter audio gates remain pending.
