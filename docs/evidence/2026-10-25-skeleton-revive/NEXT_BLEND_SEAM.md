# Next bounded blend proposal, not implemented

Actual reverse sampling works, but Dead0 is a different crouched pose from
Idle0. A direct join visibly snaps. The generic loader already supports local-
joint blending; the expanded skeleton path does not expose it.

1. In `SkinnedMeshAsset.{h,cpp}`, add a default-null trailing
   `const SkinnedClipBlend*` to expanded `Skin`, forwarding to its existing
   `SkinUniqueGeometry`. Preserve old callers and exact endpoints. Blend local
   joint translation/scale and shortest-path quaternion rotation before a single
   skin pass, not expanded vertices, normals or world transforms.
2. In `CharacterRenderSlot.{h,cpp}`, carry the optional request in the skeleton
   frame plan. Include target clip/time/weight in cache refresh and pose-bucket
   equivalence; weight-only changes must refresh, identical two-actor requests
   should share a bucket, destruction/recreation clears identity. Preserve null
   behavior and visibility.
3. Pass a default-empty CPU-only request through `RtSceneFrameInputs` and its two
   existing `PrepareFrame` call sites in `PresentableTinyRtScene.cpp`. Only the
   explicit hidden fixture sets it. No snapshot/gameplay binding, shader/ABI,
   descriptor, material or capacity change is needed for this authoring proof.

Evaluate one candidate: unchanged reverse timing and roots, then a provisional
0.5-second local-joint Dead0 -> Idle0 blend, exact endpoint hold and ordinary idle
clock. Keep health zero and no application/events advancing. A blend can still
skate or cross feet; it is not accepted merely for removing the one-frame jump.

Use focused old-output/endpoint, finite request, bucket/cache/weight and return-
to-null tests, then the same actual RT oblique comparison. Record added pose/
copy/refit work without claiming performance. No broad animation framework or
new asset is warranted. This is more than the current one-selector fixture and
must be reviewed before implementation; it is not a real resurrection system.
