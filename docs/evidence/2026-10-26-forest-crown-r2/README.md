# Forest crown r2: rejected, production unchanged

The trial adds visible bough volume, but **does not deliver the surrounding
forest**. Actual F01 views retain a bare middle-distance horizon. Close sprays
read as large flat fans; branch shadows become much larger without solving the
landscape. The candidate also fails the unchanged automatic static-detail
unload/restoration gate. Keep both original runtime trees. No new quality setting,
shared64 default, asset/renderer limit, terrain, placement or collision change.

[Compare the inspected views](index.html). They are WebP viewing derivatives of
actual RT readbacks; [summary.json](summary.json) pins original PNG hashes,
successful submission identities, camera/dimensions and exact asset/executable
hashes. Raw lossless images remain under ignored
`reports/forest-crown-r2-20261026/`. Studio previews are Blender renders, **not game
captures**. Neither set is generated gameplay or a performance video.

## Controlled scope and outcome

Existing worktree/PR27, source head `7659c6f429014163e5788c4f906ca7d2055407e9`.
Both isolated apps use executable SHA256
`8c11e055a65e90cab9199dc1d536a57cd7753a8506b9b938a7359c3473c8d3eb`.
Windows RTX5050 Laptop GPU; Vulkan RayTracingPipeline, Diagnostic High,
960×540 actual internal/output, DRS off, normal baked64 path. Shared64 stays OFF.
The original64 transforms, terrain, deadwood/understory, lighting and original
master files stay unchanged. No phone, foreground activation or controller input.

| Check | Control | Candidate |
| --- | ---: | ---: |
| Native process exit |0 |**1 — failed final retention gate** |
| Recorded route frames |1,524 |1,483 |
| Completed RT capture witnesses |33 |33 |
| Automatic static-detail unload / restore |1 /1 |**0 /0** |
| Maximum omitted triangles |84,219 |0 |
| Initial buffers /allocations /BLAS /TLAS instances |51 /60 /20 /28 |Same |
| Initial device-local /host-visible allocation bytes |372,464,608 /48,298,944 |Same |
| Original settings, campaign and foreground |Preserved |Preserved |

The route uses the existing four-ticks-per-render60Hz probe, from a **staged
post-Keeper reunion**, through outbound travel, lookout camera sweeps, return,
descent, tomb backtrack, repeat ascent and reset.32/33 camera tuples match exactly;
the walking-Kit observer is not an exact matched pose. The four retained paired
view cameras match. These are moving-route/camera witnesses, not a real-speed
continuous recording or a natural Keeper-to-ending human playthrough.

Both variants preserve original wood/moss, packed image buffers, indices,
materials and UVs byte-for-byte. Positive affine transforms preserve winding;
all transformed bases are finite, no candidate foliage triangle is degenerate,
and each original foliage base vertex remains fixed. Pine stays5,000 triangles/
5,602 vertices; alder5,102/5,632. Manifest and current capacities are unchanged.
See [geometry-receipt.json](geometry-receipt.json) and the
[reproducible owned-source recipe](../../../assets/models/world/source/forest-crown-study-r2/README.md).

### Preserved failures, not repaired receipts

- Initial runner requested the optional GPU-stage observer against the restored
  normal build where that observer is OFF. It exited2 before the comparison.
  [Original failure](initial-probe-mismatch.log) retained. Removed only that
  observer argument, kept the route gate and executable unchanged, used fresh
  output names; no instrumentation build or old shader substitution.
- Candidate reaches all33 completed readbacks including reset, then exits1:
  [original failure](candidate-failure.log). Its gallery is consequently **not
  complete JSON**. The summary extracts only individually complete capture
  records; it does not close/rewrite the gallery or mark the route complete.
- `AdmitOrdinaryStaticDetail` selects fully-beyond-Z24 forest-family triangles,
  then requires exactly84,219 plus receiver/water/light safety. Offline original
  tree-subset counts change80,285→80,235 (−50) after crown editing. Other unchanged
  contributors are not enumerated by this subset check; native full-owner count
  is not instrumented. With those contributors unchanged, the source predicate
  predicts84,169, which cannot satisfy the current exact guard. This explains
  why a geometry change needs a new reviewed proof; do not replace the constant
  blindly. The native observation is zero unloads/restores, not a measured new
  full-owner count. Initial analysis incorrectly expected trees alone to equal
  the full owner; its [failed assertion](initial-subset-assumption-failure.log)
  remains preserved and is not a production test failure.
- First staging junction writes were denied by Windows. Sam explicitly approved
  one bounded retry through normal escalation; it succeeded. No permissions,
  security or denied Blender-export temporary path were changed. Ordinary Python
  also lacked numpy; the unchanged installed Blender/numpy runtime ran the recipe.

The omission difference means the two route workloads diverge. No causal timing
or FPS comparison follows, even though raw frame-cycle/GPU fields are retained.
There was no closing control, quiet-window agreement, Shipping measurement or
phone trace. Allocation categories can overlap and are not summed. No gain is
claimed. Sustained phone performance is **ongoing optimisation, not a release
blocker or a reason to reduce approved1.7 scope**.

## Custody, checkpoint and next work

All151 parked owner files match the saved custody hashes; original executable
settings still match `ed609b868fd2951698c46bb17b98ffbad647bf30ca7cfacfa5d64f5342fd57c3`.
Both capture processes exited; no phone activity, installer or transfer remains.
Ignored isolated staging is preserved, not recursively cleaned. The prior
docs-head [CI40275252035](https://github.com/Samfa12-tech/The-Horde-RT-demo/actions/runs/40275252035)
finished with **all six jobs successful**. This checkpoint changes a rejected
source recipe/evidence and decision text, not runtime code; it needs diff/link/
provenance review rather than another unrelated full local suite.

Next: author actual rounded/clustered upper-crown structures using the owned
pair, then compose one F01 background sector with overlapping near/middle/far
mass. Preserve roots, protected path and landmark gaps; assess moving parallax,
shadows/reflections and current retention proof. Repeating leaf scaling or
quota-based scatter will not meet the target. This is independently executable
content work, not waiting for a phone target frame rate. Android import/
presentation/lifecycle and owner visual review will be needed for any **retained**
candidate; no device slot is requested for this rejected version. Fireflies and
other dated deferrals remain explicit unfinished decisions.

Manual audio/haptic revalidation: NO for this isolated geometry study. Existing
audio acceptance gaps remain unchanged. No merge, release or publication.
