# 1.6.2 compiled RT pipeline reuse ledger

7 October 2026. This records the bounded Android `RayTracingPipeline` cache
integration. It does not change Glass policy, pipeline selection, render scale,
quality, acknowledgement rules, or timeout policy. Exact ACK and the normal
post-ACK confirmation interval remain unchanged.

## Cache behavior and ownership

The cache is scoped to one Android logical device and retains at most two
compiled strategy-pair entries. Each key includes the current resolved
preflight artifact identities and exact SPIR-V bytes for both ray-generation
modules and the shared miss/closest-hit modules; backend, instrumentation and
quality; pipeline stage entry points, flags, specialization and extension
state; shader group composition; recursion/create flags and base-pipeline
state; descriptor-set-layout bindings, types, counts, stage/binding flags and
immutable-sampler identities; and pipeline-layout flags and push-constant
ranges. The device identity is part of the key. Empty extension/specialization
state and null base-pipeline values describe the current Vulkan create calls.
The key must be updated if those calls gain inputs.

A hit leases the cached pipelines, their original pipeline layout and
descriptor-set layout. Each new scene still creates its descriptor pool, set,
diagnostic buffer as required, descriptor writes and shader-binding tables.
Glass visibility and scene profile do not alter this pipeline pair. Only the
Android `RayTracingPipeline` path supplies the cache; `RayQueryCompute` and
other callers keep their existing construction path.

The cache never evicts an entry without an explicit device-idle proof. Runtime
publication does not request eviction: if both slots are occupied, key creation
fails, or owner allocation is unavailable, that scene keeps ordinary ownership
of its newly built objects. Bundle leases prevent cached handles from being
destroyed while a scene uses them. Scene teardown destroys scene-owned SBT and
descriptor resources before releasing its lease. Android device teardown
first proves device idle and presentation retirement, destroys the scene,
destroys the now-unleased cached pipelines/layouts, then continues device
teardown. A failed retirement proof retains the device resources.

`compiled_pair_reused` is true only for a bundle that borrowed an existing
cache entry; a miss that successfully publishes its objects is reported as a
miss. The keyed initialization log also records the selected artifact pair and
resident entry count. Existing per-stage and per-pipeline CPU timings remain
unchanged; a hit creates no Vulkan pipeline and therefore has no pipeline-call
timing for that attempt. These measurements are CPU initialization time, not
GPU time or displayed FPS.

## Validation and measured limits

The driver `VkPipelineCache` result remains negative evidence for expected
latency improvement. On the same SM-S948B/Android 16 configuration, the prior
null-cache Glass Off comparison was 16,694.868 ms request CPU wall and
16,174.583 ms pipeline creation. Supplying a non-null driver cache yielded
16,659.422 ms and 16,197.380 ms for Glass Off, then 16,588.701 ms and
16,176.576 ms for return On. All admitted pipelines were still created; a
non-null driver cache did not establish reuse or a measured improvement.
The full previous result and conditions remain in
[the phone timing record](ENGINEERING_1_6_2_MENU_PHONE_2026_10_07.md).

The new compiled-object cache has **no phone latency or memory-benefit result
yet**. No measured hit, exact-ACK outcome, or owner-visible effect is claimed.
The fake host tests cover cache key and object-ownership behavior; the integrated
native build covers compilation:

- `scabbard-cache-host-tests-20261007-05.log`: cache, transition, ABI, bundle
  lifetime and character targets passed (5 of 6 selected targets). The socket
  target initially failed an old atlas assertion; the corrected standalone
  `scabbard-socket-final-test-20261007.log` passed 1/1 in 14.47 s.
- `integrated-native-build-20261007-06.log`: Windows default native build passed.
- `scabbard-cache-android-native-java-20261007.log`: Android native compilation
  passed for all four configured ABIs; Java tests passed 176/176 and lint
  reported zero errors with 62 warnings.

The cache seam's committed bundle lease hooks are `463ee08c3160a4cb3856c9744d227c539787e4d4`;
all 12 aggregate CI checks passed for that committed hook state. Scene and
Android runtime integration compiled successfully in the integrated build.
On-device cache-hit, latency, memory and current-scene presentation checks
remain pending; the host tests do not establish any of those outcomes. The
earlier Scene build04 failure from an undefined
`executionBackend` was corrected to use the backend resolved in
`selectedPreflight.request.executionBackend` before the successful integrated
build.
