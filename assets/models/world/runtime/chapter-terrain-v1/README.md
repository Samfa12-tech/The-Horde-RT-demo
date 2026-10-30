# Chapter terrain runtime

Exact native GLB from ../../source/chapter-terrain-r23; original editable sources stay outside runtime packages. It incorporates the pinned connected-region macro relief, bounded bed/bank source subdivisions and the explicitly bounded new-crest authoring policy documented in that source folder. It preserves 15,472 near-route triangles exactly. Two ordinary opaque PBR materials: damp-sand trail and Forest Ground 01 banks. The existing production prop owner includes these two primitives; no extra BLAS or descriptor owner is added. Texture arrays append two layers at upload with exact legacy mip bytes retained. Actual geometry is 19,486 exported vertices / 37,846 triangles, below the unchanged manifest ceilings of 21,134 / 39,618. The 18,441,928-byte GLB has SHA-256 `c636a67911b7f5c5381c30ec7a49e5d4ff15c70fe688b368ba52efe77b537774`.

The same imported vertices generate authoritative terrain support. This scene remains GPU resident; it is not streaming. Native admission does not establish finished chapter art, full-body collision or physical phone acceptance. See ../../source/chapter-terrain-r23/README.md and its compact receipts for current conformance/native admission, docs/evidence/2026-10-11-regional-terrain/README.md for earlier regional evidence and the chapter-live-terrain receipt for preserved history.

The material-only source `../../source/chapter-terrain-material-r01` records the
named Forest Ground linear base-colour factor at 0.5. The unchanged runtime
manifest carries this default with the r23 geometry; r21's preserved sources and
all encoded maps remain unchanged. This reversible appearance default uses the
existing PBR path and needs owner/phone visual acceptance.
