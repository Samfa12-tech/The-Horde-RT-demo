# F08 — bounded coherent dynamic-buffer ownership

Parentd4e6cba plus the resource/test/CI patch in this commit. One candidate,
limited to RtGpuResources, dynamic scene buffer creation and the existing resource
fixture. No shader, asset, animation, material, audio, backend or scale change.

## Contract and change

Opt-in mapping covers the separate world-body/viewmodel vertices, two skeleton
pose buffers, lich vertices, held light/fire, TLAS instances and mutable instance/
material metadata. Map each logical range once at initialization; repeated checked
copies use the owned pointer. The existing scene moves transfer/reset the owner;
destruction unmaps before freeing and clears it. Mapping failure remains visible
and fails initialization, not a silent route change. Diagnostic/reset/readback,
texture staging and immutable geometry retain their independent mapping paths.

Only selected HOST_VISIBLE|HOST_COHERENT memory is admitted, so this does not
invent noncoherent flush ownership. Both platforms still wait for their sole
frame fence before recording/writing, and keep the host-write/AS/shader barriers.
No second frame in flight. [Vulkan mapping rules](https://docs.vulkan.org/refpages/latest/refpages/source/vkMapMemory.html)
require caller synchronization even while memory remains mapped.

DynamicUpload retains one copy operation and the exact byte count per upload;
it does not pretend operationCount is a map-call counter. Unit Vulkan doubles
separately prove one lifetime map, no per-write maps/unmaps, exact-end writes,
overflow/null/zero rejection, idempotent mapping, move transfer, single cleanup,
noncoherent rejection and map-failure/null-success cleanup. Existing temporary
map/copy/unmap and observation-abort tests remain intact.

## Current evidence

- Fresh Windows Debug/Release game and four affected fixture builds PASS.
  Debug4/4 CTests11.85s, then mapping-only1/1 after the stronger cleanup-order
  assertion; Release4/4 with that assertion5.85s. Logs retained at
  C:/Dev/tmp/horde-dynamic-mapping-{debug,release}-{build,ctest}.log.
- Exact Debugaa483a07014f4cd8761d0e4f469dbc71a6e1b37f2e27206e67c2b4cb5ff7faaf
  completes13 deterministic native RTX5050 Pipeline/High960x540 captures, exit0,
  empty stderr, complete RT-storage-image manifest. Opening, mirror and two-enemy
  images inspected for visible geometry/ownership failures. This is not image
  equivalence, live player acceptance or sustained performance. Retained:
  C:/Dev/tmp/horde-dynamic-mapping-captures-20261003.
- Exact Shipping870056a0f6dd25cd22b6c65e5086fea18cdb420cbb4ce3836ab5c68b0b37de46
  actual four-module SPIR-V extraction/validation/disassembly PASS. Module hashes
  unchanged; zero diagnostic atomics/no binding22 in both backend pairs.
  Receipt: C:/Dev/tmp/horde-dynamic-mapping-shipping-containment.json.

## Finite next step / limits

Push this reviewed candidate for fresh focused resource and Android compile CI.
The Vulkan-host lane now explicitly includes this fixture. Exact-phone functional,
lifecycle/memory and matched warm timings remain open: S26 is occupied by
Briarhold. Do not use it, repeat finished runs or claim an FPS saving. This proves
removed API calls/ownership, not a millisecond benefit. Immutable device-local
staging remains a separate F08 subtask, not resolved by this change.

Audio/haptic manual revalidation:NO; semantic inputs/playback unchanged.
No merge, signing recovery, release or publication.
