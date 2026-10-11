# Chapter terrain runtime

Exact native GLB from ../../source/chapter-terrain-r19; original editable sources stay outside runtime packages. Two ordinary opaque PBR materials: damp-sand trail and Forest Ground 01 banks. The existing production prop owner includes these two primitives; no extra BLAS or descriptor owner is added. Texture arrays append two layers at upload with exact legacy mip bytes retained.

The same imported vertices generate authoritative terrain support. This scene remains GPU resident; it is not streaming. Native admission does not establish finished chapter art, full-body collision or physical phone acceptance. See docs/evidence/2026-10-11-chapter-live-terrain/README.md for exact evidence.
