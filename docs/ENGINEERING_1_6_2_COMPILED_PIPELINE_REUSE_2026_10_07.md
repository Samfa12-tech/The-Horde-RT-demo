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

The compiled-object cache now has an exact-package phone hit/latency result,
recorded below. The fake host tests separately cover cache key and
object-ownership behavior; the integrated native build covers compilation:

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
The host tests do not establish on-device outcomes. The
earlier Scene build04 failure from an undefined
`executionBackend` was corrected to use the backend resolved in
`selectedPreflight.request.executionBackend` before the successful integrated
build.

## Exact d08 phone result

Allocated SM-S948B / Android 16, Pipeline backend 1, output 1440×2980,
newly traced 720×1490 at saved/custom 50%, Mobile water/fire, Glass On,
Current shadows, cap30, Mist On. Exact source is
`d08d3c4827ef5d1ce95b74c21e29663aa33e9e7d`; Debug APK SHA-256
`87a3ed413d09de098af45d74802da42e534ff09de3d467a8ff937d2b1d5a6ff7`
(138,462,724 bytes). Installed base pullback matches. Private evidence is
`task-4/integrated-162-d08d3c48-20261007/`.

| Operation | Native request CPU wall ms | Scene init ms | UI observation ms | Result |
| --- | ---: | ---: | ---: | --- |
| Cold Entry | initialization only | 13961.468 | not sampled | Miss; one resident pair |
| Open preview, serial 2 | 305.519 | 188.911 | 546.738 | Hit; current RT preview presents |
| Glass Off, serial 3 | 366.806 | 110.118 | 562.491 | Hit; current RT preview presents |
| Return On, serial 4 | 326.240 | 120.301 | 514.857 | Hit; current RT preview presents |
| Use, serial 5 | 282.888 | no rebuild | 405.029 | Exact native ACK; Keep available |
| Restore, serial 6 | 279.163 | no rebuild | 528.057 | Exact native ACK; no Keep/save |

Open-preview initialization still logs the prior request context serial 1;
chronology and target profile identify it separately from serial 2's receipt.
Comparison requests correctly report `exact_ack=0`; they are not trials.
Use and Restore both report `exact_ack=1`. Back subsequently returns to Entry
profile 2 with a hit (84.035 ms initialization) and exact restore ACK. All
preference entries remain identical through install, trial, Restore and Back.
Owned PID 26335 is stopped after recovery; original rotation lock 0 was retained.

This bounded sequence establishes avoidance of repeated pipeline compilation:
Off/return On requests fall from the preserved driver-cache observations of
16.659/16.589 seconds to 0.367/0.326 seconds. It is not a statistical latency
distribution, cold-start improvement or sustained-FPS result. Off/return On
PSS is 431187/428792 kB, RSS 548292/525744 kB and swap PSS 52/22205 kB;
these process observations do not isolate cache residency or prove memory
savings. USB-powered conditions do not establish reliable battery power.

The subsequent Java-only scrollbar package retains byte-identical native
payloads. Its landscape rotation exposes a repeated recreation/waiting-for-RT
failure, recorded in the phone ledger; portrait cache hits do not close that
surface recovery gate. Exact ACK and confirmation timeouts remain unchanged.
