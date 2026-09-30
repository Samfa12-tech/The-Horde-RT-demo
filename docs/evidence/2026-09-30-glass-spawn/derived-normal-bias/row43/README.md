# Remaining RTX mismatch — row43 / tick646

Investigation-only evidence; production source25a379c/d63e29c. No accepted player,
geometry/material, traversal-budget or diagnostic-policy change. Native ordinary
pipeline/compute replay stages the existing gameplay-owned600warm+44steps and
freezes tick646, not the authored finale checkpoint. Filenames/checkpoint labels
are inherited; manifest `investigationOnly/liveSimulationTick` identifies reality.

Both ordinary owned native processes exit0. All41 counters match their respective
live row43 (`*-counter-comparison.json`). Pipeline mismatch1; compute failure/
recovery0. Output-only marker locates pixel(524,552) without counter changes.
Lossless83-field output witnesses on both backends also match all41 ordinary
counters (`probe-counter-comparison.json`); native exits0/0. Their manifests bind
actual compiled shader hashes and PNG bytes. Large referenced OBJ dumps and
executable/SPIR-V binaries stay local; absence here is not a changed asset.

Pipeline's first actual native hit is triangle63/component4/instance8/material115,
exiting with no open medium. The camera object origin lies **outside**, up to
1.309609m from an outward plane; this is not a legitimate camera-inside exit.
Compute returns enteringtriangle9 at0.808291972m instead. Same pixel, exact
world/object origin and matrix; world directions differ5.96e-8 and native object
directions2.384e-7, so the actual rays are not bit-identical.

Independent GLB float32-upload reproduction verifies SHA
`34a2522f2027d3fb04b77480cc929d36c5a19c6e0f33bf3fa0f0ae0959c99ec4`,72triangles,
and bit-exact returned triangle vertices. Double intersections on the pipeline's
**captured float object ray** put tri63 at0.629852548967m, baryv−1.0472e-7;
entering toptri65 at0.629852621345m (0.07238µm later), barysum1.00002161. Both are
just outside strict triangle bounds. Native pipeline baryv+3.4869e-9 is on the
shared edge. Compute's captured ray also excludes these corner candidates; its
next valid GLB hit istri9, matching the actual native query within0.0801µm.

This supports an outside-origin near-coincident entry/exit corner event and
backend-sensitive edge/arithmetic handling. It does **not** prove hardware
traversal internals, certify a universal triangle tolerance, justify ignoring
exits/loosening image gates, or explain the phone's seven mismatches without
their own witnesses. No renderer fix is claimed here. Preserve meaningful failure.

Reproduce with bundled Python/Pillow:

```powershell
python decode-native-path.py rays/11-finale-roof.png 83
python decode-native-path.py compute-rays/11-finale-roof.png 83
python analysis/compare_row43.py REPOSITORY/assets/models/props/runtime/reward-lantern-body/reward-lantern-body-lod0.runtime.glb
```

Decoder output equals retained native JSON; portable numerical analysis reproduces
the retained result byte-for-byte. It is an analytical investigation, not an RT
replacement or performance measurement. Output rows alter only witness pixels.

An additional unconditional compute-firsthit probe compiled but Windows Smart App
Control blocked execution. Empty stdout/stderr are not a successful capture;
no result manifest is claimed. `compute-firsthit-source.patch` is that **unrun**
superset probe, not exact source for either earlier native witness. Security was
not changed. Probe artifacts are now removed from production; source/catalog/
module diffs are empty against25a379c. Restoration regenerates all8compute
variants identically; native Debug build and focused2/2 tests pass.

Current whole-glass/backend-parity gates remain open. Audio/haptic manual
revalidation:NO: investigation/output only, semantic inputs and cues unchanged.
