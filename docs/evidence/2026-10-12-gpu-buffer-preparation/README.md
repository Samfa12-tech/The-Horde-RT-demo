# Failure-atomic GPU buffer preparation

The existing chapter now prepares the six static and two dynamic geometry and
metadata uploads as an isolated candidate. It adopts their handles only after
all allocation, address, mapping and exact-byte upload checks succeed. Invalid
inputs or a failed row leave the destination unchanged and clean up candidate
allocations, including retained mappings. Existing live resources cannot be
overwritten. Material metadata is prepared before adoption too.

`RtGpuResources::PrepareBufferUploads` is the reusable preparation seam. It
does not submit GPU work, alter descriptors or acceleration structures, validate
zone generations, or retire an admitted scene. It is now used by ordinary scene
initialization, including the six-row compact preview. The next zone transaction
still needs candidate BLAS/TLAS, completed-work and command-buffer ownership,
atomic descriptor/CPU-state cutover, and safe reverse loading. Everything in the
current chapter remains resident; this is not streaming or chapter completion.

The implementation started from `d6f6dcd87b70ddee2441b2cca6c3a05b1e9cb341`.
[The receipt](validation-receipt.json) binds the final implementation files and
actual Debug/Release/APK bytes. That parent passed all six CI jobs in
[run 38165532001](https://github.com/Samfa12-tech/The-Horde-RT-demo/actions/runs/38165532001):
Vulkan 38/38 including all seven traversal tests and native forest admission;
MSVC 132/132 including Unicode note editing in 0.50 seconds; GCC/Clang 121/121
each; Android and the sanitizer lane passed. New-commit CI is a separate result.

## Validation and limits

- The behavioral test failed before implementation; [red evidence](gpu-buffer-batch-red-test.log)
  is retained. The passing test covers exact eight/six-row mixed uploads,
  48 injected Vulkan failures across all rows, complete preflight, a preserved
  admitted sentinel, move ownership and exactly-once unmap/destroy/free.
  These are mocked Vulkan lifecycle tests, not physical driver fault injection.
- Debug application build plus four resource tests and three bundle-containment
  tests passed. Release application build and the same seven tests passed.
- Android built all four Debug ABIs and passed lint. Unchanged local Java tests
  were `UP-TO-DATE`; this is not a fresh local Java execution. Closed runtime
  package-byte/licence/source-separation and held-item package checks passed.
- Fresh compilation of all eight raygen variants matched the frozen catalog and
  unchanged budgets. The first catalog invocation lacked required absolute
  paths; its [captured preflight reproduction](gpu-buffer-batch-shader-catalog-missing-paths.log) and
  [corrected successful check](gpu-buffer-batch-shader-catalog.log) are retained.
- No shader, geometry, texture family, mask, quality, three-actor/18-layer limit,
  original torch coverage/deadline, released artifact or protected source was
  changed. The current untracked Blender backup hash remains identical; this
  does not recover its previously overwritten historical bytes.
- A separate read-only ownership review found no consequential defect and
  confirmed the eight/six-row profiles and later initialization-failure cleanup.
  It performed no builds and did not certify a live zone transaction.

## Inspected actual RT gallery

Ten frames below were opened. Both hardware backends ran hidden, input-free
native development captures on the RTX 5050 Laptop GPU at 960×540 and 100% scale,
Diagnostic High. Exact shaders, cameras, settings, completed submissions and
resource inventories are in the six adjacent manifests. All processes exited.

| Scene | RayTracingPipeline | RayQueryCompute |
| --- | --- | --- |
| Planted pull-up | [Frame](pullup-pipeline.png) | [Frame](pullup-query.png) |
| Second woodland bend | [Frame](bend-pipeline.png) | [Frame](bend-query.png) |
| Compact graphics preview | [Frame](preview-pipeline-overview.png) | [Frame](preview-query-overview.png) |
| Preview Current shadows | [Frame](preview-pipeline-shadows-current.png) | [Frame](preview-query-shadows-current.png) |
| Preview Higher shadows | [Frame](preview-pipeline-shadows-higher.png) | [Frame](preview-query-shadows-higher.png) |

[Pixel comparisons](gpu-buffer-batch-frame-comparisons.json) found **zero changed
pixels in all 38 matched comparisons**: four chapter frames against the inspected
parent and 34 preview poses against retained issue23 evidence, each matched by
camera and effective settings. This preserves each backend's existing output;
it does not claim the two backends are mutually identical. Preview Current and
Higher are separate established fixture poses, not a new same-view mist study.
Mist issue28 remains deferred.

Frame resource counts and allocation sizes are unchanged. The chapter still has
32 primitive rows, shared materials/textures, 52 buffers and 63 allocations in
these views. Twelve settling frames and concurrent development work do not
establish sustained performance or a measured loading hitch. The planted hand
view remains dark/tight and the existing sparse canopy is deferred owner work;
neither is represented as newly accepted. This gallery is a regression check,
not the required full chapter gallery.

## Remaining chapter gates

Real two-way zone commit/retirement, graveyard/lid composition, remaining admitted
woodland dressing, full-route inspected views, sustained resource-overlap costs,
owner audio/haptic/traversal acceptance and fresh phone validation remain open.
Phone Current/Higher and saved Lower→Current migration are still physically
pending. PR27 base conflicts remain; no merge or release occurred.
