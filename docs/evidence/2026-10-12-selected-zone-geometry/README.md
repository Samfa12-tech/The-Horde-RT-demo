# Selected zone geometry preparation

This is a prerequisite for the active chapter's GPU zone gate. It is **not
streaming, resource retirement, or chapter completion**. The full chapter now
uses failure-atomic static candidate admission with its existing full source
registrations. Its actual resident geometry, materials, textures, BLAS/TLAS
layout and both shading backends remain unchanged.

Parent source is `3c15c53410b80ac6b09c743312aed24cbb2da129`. That exact
checkpoint's six CI jobs passed in [run 38162791741](https://github.com/Samfa12-tech/The-Horde-RT-demo/actions/runs/38162791741).
All seven requested traversal CTests executed; the MSVC Unicode note UI fixture
passed in 0.44 seconds. This is parent evidence, distinct from this commit's CI.

## Change and tests

`RtStaticMeshSlot::InitializeSelectedGeometry` packs the union of registered
immutable primitive ranges once per native source. It preserves source-local
primitive order and complete indexed vertex spans, removes absent geometry
from the candidate payload, and remaps each owner's primitive base. Full
selection remains byte-identical to the legacy packer. Shared material rows,
texture-family allocation, node/socket source metadata and full dynamic
geometry remain intact. Source assets are never edited. Full selections borrow
the source during preparation; partial selections do not first copy the retired
geometry into a temporary asset just to discard it.

Empty candidates, malformed/overflowing ranges, bad vertex/index spans, absent
materials/transforms, duplicate owners and illegal dynamic subranges fail
without mutating the admitted slot. Tests check preserved instance/primitive
metadata, vertices, indices, transforms, material records and all texture counts.
They cover reordered/overlapping owners and retained texture-provider aliases.
The first behavioral run failed 14 assertions against legacy packing; its log
is retained. Negative tests continue to execute rather than weakening admission.

The native test loads the actual current entry, tomb props/niches, terrain and
tree pair. It verifies every retained vertex/normal/tangent/UV/index/material
byte for both selections. These are **CPU upload payload bytes**, not GPU
allocation sizes, memory savings, or measured streaming performance:

| Native combined-owner selection | Primitive rows | Vertex bytes | Index bytes | Omitted vertex/index bytes |
| --- | ---: | ---: | ---: | ---: |
| Complete | 11 | 26,252,736 | 4,606,848 | 0 |
| Common + tomb | 5 | 2,119,872 | 319,080 | 28,420,632 |
| Common + forest | 8 | 25,590,336 | 4,444,656 | 824,592 |

The two common rows include a coalesced collapse/tomb masonry range. The second
selection therefore does **not** prove removal of all tomb geometry. Textures
and material families remain shared. The full runtime is still at 32 static
primitive rows; an additional split cannot assume extra capacity. The Vulkan
host CI selection now also builds and executes `horde_rt_forest_dressing_tests`
using the already-required native fixture payloads.

## Actual RT inspection

Final Windows Debug captures use RTX 5050 Laptop GPU, 960x540 at 100%,
Diagnostic High optical profile/HIGHER shadows. Pipeline and QueryCompute use
their respective unchanged compiled strategy pairs. Exact camera, rope state,
shader hashes, masks and successful RT output evidence are in the adjacent
manifests; executable/APK/source/log identities are in `validation-receipt.json`.
The legacy application label still says 1.6.2; these are development chapter
artifacts, not replaced released 1.6.2 packages.

| View | Pipeline | QueryCompute |
| --- | --- | --- |
| Grounded pull-up | [Frame](pullup-pipeline.png), [manifest](pullup-pipeline.json) | [Frame](pullup-query.png), [manifest](pullup-query.json) |
| Second forest bend | [Frame](bend-pipeline.png), [manifest](bend-pipeline.json) | [Frame](bend-query.png), [manifest](bend-query.json) |

The frames are opened and inspected. Retain the previous milestone's candid
limitation: the grip view is dark/tightly framed, and owner grip/feel acceptance
is still open. Tree-canopy replacement remains deferred by the owner. This small
gallery is a packing regression check, not the complete chapter acceptance
gallery. Twelve settled frozen samples are not sustained travel measurements.
The [full-resolution comparison](frame-comparisons.json) finds zero changed
pixels against the parent corrected pull-up on each backend: floor/rim shadows
are preserved. Cross-backend differences are 2,560 pull-up pixels (maximum
channel difference 60) and 3,468 bend pixels (maximum 46); this is not a claim
that the two hardware strategies are bit-identical.

## Validation and next implementation boundary

Windows Debug/Release applications build. Nine affected CTests pass in each:
native GLB admission/intake negatives, native forest and collapse, ABI, texture
subset, and exact atlas alias/append payloads. Android all four Debug ABIs and
lint pass; unchanged Java unit tests are explicitly UP-TO-DATE. Closed APK
runtime-byte/licence/source-separation and held-item contracts pass. No device
install, gameplay input, release, shader edit, capacity increase or deadline
change occurred. The preceding milestone's full original torch test coverage
and explicit 180-second policy are preserved; this slice changes neither its
gameplay nor timeout.

Next, prepare a separate GPU static arena and changed zone BLAS/TLAS while the
old arena stays valid. Retained common BLAS can survive if local geometry order
and decoded bytes remain exact; dynamic refits must use adopted buffers. Swap
only completed candidate descriptors/handles after current-generation validation
and draining/invalidation of all referencing command buffers. Failure must
destroy only the candidate. Measure actual before/peak/after allocations and
retired bytes; CPU packing alone cannot satisfy this gate. Procedural world
geometry and still-relevant shaft/secondary-ray/light contributors need explicit
ownership too. Static textures remain shared until a separately verified exact
layer/remap transaction can retire eligible families.

The chapter remains incomplete: GPU handoff/retirement, graveyard/lid and prepared
woodland dressing, complete inspected route, sustained overlap costs, owner
audio/haptic/traversal and physical Android acceptance remain open. Phone
CURRENT/HIGHER and saved Lower-to-Current migration are still pending. No
merge/release; PR base conflicts are retained.
