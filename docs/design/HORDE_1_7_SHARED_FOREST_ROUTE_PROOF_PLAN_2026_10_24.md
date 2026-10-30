# Shared forest: smallest full-size route proof

Execution status: the [actual Windows existing64 proof](../evidence/2026-10-24-shared-forest-route/README.md)
now passes resource/representation/recreation checks. The following preserves
the reviewed implementation sequence; normal adoption, timing and visible
forest depth remain unadmitted. Continue with the separately budgeted
[F01 background sector](HORDE_1_7_F01_BACKGROUND_SECTOR_2026_10_24.md).

Source review at `3898bcee`. This follows the [actual room proof](../evidence/2026-10-24-shared-blas-pair/README.md); it is an implementation plan, not normal adoption or a forest-depth pass. It preserves the existing admitted pine/alder and all64 collision placements. Replacement-canopy/Easy Tree remains deferred.

## Exact capacity boundary

| Resource | Existing chapter | Straight shared-existing64 proof |
| --- | ---: | ---: |
| Packed primitives, Mobile |31 |35 |
| Packed primitives, High with glass |32 |36 |
| Materials during packing, before aliases |32 |32 |
| Packed materials after aliases |30 |30 |
| Final materials including world palette |32 |32 |
| Unique static assets |13 |13 |
| Physical TLAS definitions |28 |92 |
| Instance metadata storage |26 |28, with new rows26/27 |

These are engine storage/ABI capacities, not hardware ceilings or measured cost. `PresentableTinyRtScene::kTlasInstanceCount` currently aliases capacity and asserts it equals the final Keeper role plus one. Separate ordinary active28 from proposed storage92; increasing storage must not make ordinary launches submit92 definitions.

The common owner currently has11 primitives. Bark also contains deadwood (`ForestDeadwood.cpp`), and Alder contains understory (`ForestUnderstory.cpp`). Keep these non-tree groups: common becomes9, then two masters contribute3 each. Removing all four forest groups would lose scenery. High's extra pane must be counted, even though the Mobile intake test starts at31.

Material packing is already full **before** alias compaction. Reuse the existing four canonical material indices; do not append six master materials and rely on later aliasing to recover headroom.

## Bounded patch sequence

1. In `ForestDressing.h/.cpp`, add one helper that appends each normalized master once, remapping its three primitive material indices to the existing canonical families. Preserve the compatibility bake.
2. In the diagnostic shared path in `PresentableTinyRtScene.cpp`, prepare the same64 root-fitted transforms, seed the existing four families, append unchanged understory/deadwood/other dressing and consolidate the common world. **Append masters after consolidation** so same-material consolidation cannot merge them back into world geometry.
3. In `PresentableTinyRtSceneStaticGeometry.cpp::PackLoadedStaticAssets`, register common9, pine3 and alder3 as disjoint contiguous ranges of the same `&collapseStaticAsset_`. Existing selected-range packing owns that asset once. Keep common world bounds separate from master-local bounds.
4. Reuse the generic registered-static builder for two immutable master BLAS; append64 unit-scale transforms referencing rows26/27. Retain all existing28 physical roles, masks, UVs/tangent handedness and one-frame-in-flight ownership. No tree shading branch is needed: the current generic hit decoder already applies instance transforms.
5. Review `RtSceneAbi.def`, generated outputs and every fixed-array/count assumption for36 primitive rows,28 metadata rows and92 physical storage entries. Retain13 assets,32 materials and18 texture layers. Rebuild the **current** shader bank through the existing source workflow; seal the same bank for control/candidate, never substitute an old private bank.
6. Add a narrow Debug selection, default OFF, for baked-resident versus shared-existing64. Both proof variants conservatively retain all contributors. Begin through the existing hidden Showcase path with F01 north, F03 forward and F04 centre; compare full-size roots, silhouette, bark normals, shadows/reflections, allocations, startup, timings and recreation/reset. Only expand to the retained directional sheet if those views pass.

## Visibility/provenance is a separate substantive gate

`AdmitOrdinaryStaticDetail()` currently certifies exactly84219 **world-space** triangle IDs in the common owner. Master-local triangle IDs cannot inherit it. The proof must explicitly gate omission/cutover in **both** variants and label itself fully resident; do not weaken or bypass the normal production certificate.

Normal adoption must identify omission by master + placement + primitive + triangle, evaluate transformed geometry and reprove source/light/receiver bounds. Conservative whole-instance retention at boundaries may be simpler than partial-tree removal, but still needs measured acceptance. Existing static cutover also needs matching metadata/definition/certificate generation and retirement proof before reusing immutable BLAS.

Collision remains in `ForestTreePlacementContract.h` and the prepared terrain/root checks. No transform, path, support surface, save state, terrain boundary or town content changes in the existing64 proof.

## Visible forest-depth milestone after equivalence

Equivalent shared ownership of the same64 near-route trees still leaves the actual sparse F01/F04 views. The next content slice is one north/west F01 background sector using admitted masters, respecting terrain, route enclosure, lookout landmarks and the dry Bellwether approach. State its additional instance budget separately:92 fits the existing64 only. Measure real allocation/RT cost and region/lifecycle correctness; terrain occlusion alone does not establish residency or shadow/reflection savings. No tree-count target, background billboard or playable1.8 town is admitted by this plan.
