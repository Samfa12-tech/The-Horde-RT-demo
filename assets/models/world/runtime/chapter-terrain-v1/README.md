# Chapter terrain runtime

Exact native GLB from ../../source/chapter-terrain-r21; original editable sources stay outside runtime packages. It incorporates the pinned connected-region macro relief and preserves 15,472 near-route triangles exactly. Two ordinary opaque PBR materials: damp-sand trail and Forest Ground 01 banks. The existing production prop owner includes these two primitives; no extra BLAS or descriptor owner is added. Texture arrays append two layers at upload with exact legacy mip bytes retained. Actual geometry is 17,588 exported vertices / 34,050 triangles, below the unchanged manifest ceilings of 21,134 / 39,618.

The same imported vertices generate authoritative terrain support. This scene remains GPU resident; it is not streaming. Native admission does not establish finished chapter art, full-body collision or physical phone acceptance. See docs/evidence/2026-10-11-regional-terrain/README.md for current evidence and the earlier chapter-live-terrain receipt for preserved history.

The material-only source `../../source/chapter-terrain-material-r01` now authors
the named Forest Ground linear base-colour factor at 0.5. The trail, exact r21
GLB/Blender source and encoded maps are unchanged. This reversible appearance
default uses the existing PBR path and needs owner/phone visual acceptance.
