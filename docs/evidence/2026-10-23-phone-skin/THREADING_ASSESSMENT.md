# Android thread ownership and bounded parallelism assessment

Read-only assessment of source `bceed0c16936cb40400536e1cad58b0040f131f6`
after the phone was released. No phone reacquisition, thread implementation,
affinity, priority, simulation or Vulkan synchronization change is made.

**Horde is multithreaded, but its expensive per-frame CPU preparation is mostly
serial.** Background threads protect UI, lifecycle, audio and save responsiveness;
they do not constitute an animation/skinning job pool. We have not measured
per-core utilization or established that all available CPU capacity is used well.

## Actual application owners

| Thread/owner | Current work and source pointers |
| --- | --- |
| Android main/UI thread | Activity lifecycle, touch/controller publication and HUD. JNI publishes input/commands; it does not create a parallel gameplay authority. `MainActivity.java`, `InputMailbox` and `android_probe_bridge.cpp:4101,4190`. |
| Native surface owner | One process-lifetime thread serializes surface start/stop and joins the previous render generation. `android_probe_bridge.cpp:5895-5957`. It is a lifecycle owner, not a per-frame job worker. |
| Native swapchain/render thread | `SwapchainRenderLoop` at `android_probe_bridge.cpp:5193`, created at5838. Owns shared simulation advance, scene pose preparation, mapped uploads, command recording, submit and presentation. |
| Music PCM worker | Java `HordeMusicPCM` in `HordeMusicPlayback.java:34,135`. Owns native bank/session/mix, PCM writes and playback-head accounting. Controls are synchronized; audio-focus delivery is on main. It is separate from frame work. |
| Ambience PCM worker | `HordeAmbiencePCM`, `HordeAmbiencePlayback.java:28,87`. Owns its Core session/output and waits when inactive. Existing platform speech/SFX/framework threads do not imply a game job pool. |
| Campaign save worker | One condition-variable worker in `CampaignSaveWorker.cpp:4-5,17-22`. Consumes bounded immutable requests for storage work; simulation/commit acknowledgements remain on the owner. |
| Occasional helpers | MainActivity's single-worker report executor at176, update executor at265, environment-survey export/preparation at802/835 and report-export helpers. Report deadline/cleanup executors are separate. These are on-demand I/O/tool tasks, not animation parallelism. |

Source search finds no application CPU skin/job pool, parallel vertex loop,
`std::async`/parallel-execution path or app-set affinity/priority policy for these
frame owners. Java/driver/runtime/framework threads may also exist; their
presence or CPU/core placement was not captured in this window. Android's
scheduler chooses placement for the current threads; actual core topology,
migrations, frequencies and running/runnable time remain unmeasured.

## Current critical path and synchronization

Android explicitly uses **one frame in flight** (`android_probe_bridge.cpp:119`).
The render owner enters `RenderFrame` at3820, waits that frame's fence at3851,
then acquires the swapchain image. The input mutex is released at4190 before
normal simulation/GPU work; it is not held throughout all skinning. The existing
60 Hz `GameSimulation.AdvanceFrame` calls are on this render thread
(4277/4282 for benchmark variants,4366/4384 for normal/paused paths).

Scene preparation then runs synchronously:

1. Solve player pose/IK, skin world vertices, repack and upload.
2. Skin viewmodel vertices from the same solved pose, repack and upload.
3. Evaluate Kit's clip/palette, skin vertices, ground/bound/repack and upload.
4. Prepare/upload any other active character slots.
5. Record host-write visibility, required BLAS operations, the AS dependency,
   TLAS update/build and trace-read visibility; record RT dispatch and output copy.
6. Submit at4695 and present at4808 through the same graphics queue. The next
   frame waits the same single in-flight ownership chain.

Relevant implementation: `PlayerRenderSlot.cpp:786`,
`PresentableTinyRtScene.cpp:6796,6851,6948,6971`,
`CharacterRenderSlot.cpp:248`; barriers/AS/trace at
`PresentableTinyRtScene.cpp:7631,7663,7899,7925,8334`.
GPU BLAS/TLAS/trace execution follows those dependencies asynchronously after
submission; CPU recording time does not measure its GPU cost. Multiple driver
threads or hardware RT parallelism do not make these CPU pose calls parallel.

One frame in flight also limits CPU/GPU overlap. The large wait occurs before
current-frame simulation and pose preparation. A pose worker launched only
after that wait can shorten the CPU preparation segment; it cannot remove the
whole fence wait. Do not simply change the constant to two: current mutable
buffers, refit resources, input/feedback/presentation ownership, memory and latency
would need a separate audit. That is a different, larger experiment.

## What the latest phone counters establish

The landscape33% pure control has median player world vertices4.5489ms,
viewmodel vertices1.5818ms, Kit vertices4.6984ms and Kit copies0.6689ms. Player
pose is0.0311ms and Kit clip/palette0.0203ms. Four BLAS command-record calls
total0.0304ms and TLAS recording0.0077ms. These identify vertex/copy work as a
more credible CPU target than parallelizing tiny clip evaluation or AS recording.

Its GPU median is39.6475ms, fence wait39.9757ms and whole-frame cycle56.0719ms.
These are elapsed, overlapping/owned intervals, not measured CPU utilization;
medians cannot be summed into a frame equation. A CPU worker does not lower the
GPU duration. At20%, GPU17.8562ms and frame cycle33.0509ms make reducing the CPU
preparation gap worth investigating, but this is a different thermal/resolution
sample, not an accepted gain forecast. Existing frame cap/pacing also remains.

The normal-cache candidate has **no admitted phone samples**. Today's three
landscape profiles are controls; candidate starts failed temperature matching,
then status1. The broader CPU-utilization question therefore remains open even
though the architecture and elapsed costs are now more specific.

## Highest-value bounded parallelism experiment

After a scheduler trace, prototype **one persistent generic-character pose
worker**, initially exercised with Kit's existing `SkinnedPbrActor::Pose`, while
the render owner prepares the player world/viewmodel. The independent actor
storage (`SkinnedPbrActor.h:27`) is a concrete starting seam. This should remain
a generic actor job, not a special Kit rendering branch or a new engine-wide job
framework. No new worker per frame, simulation thread or parallel Vulkan submit.

Copy the immutable pose request and tick/generation identity; give the worker
exclusive ownership of that actor's mutable mesh/vectors. Finish/join before the
owner reads Kit bounds, uploads vertices, records BLAS or publishes pose/refit
metadata. Do not concurrently pose a shared `SkinnedMeshAsset`. Return worker-local
metrics instead of racing the existing `RtStageAccumulator`. Cancel/retire/join
before scene or lifecycle resources are destroyed. Preserve cadence, normals,
contacts, deterministic per-vertex outputs and failed-submit ownership.

This could overlap approximately5ms of observed Kit work with player work, but
that is a hypothesis about a component, **not5ms promised frame saving**. Queue,
wakeup/join, contention and changed CPU placement may consume or reverse it.
Splitting a single vertex range is another possible seam, but should not be
added at the same time; it needs new range APIs and deterministic bounds/copy
ownership. First test the smaller independent-actor seam.

Run actual-asset byte/geometry/contact parity and single-owner failure/lifecycle
checks. Measure total critical-path wall time, per-thread CPU time, job wake/join
overhead, extra buffers/copies and sustained thermal cost. Keep matched control,
candidate and closing control at one resolution before expanding the test matrix.
Retain/reject the candidate if no worthwhile robust gain is shown.

Android devices can have different performance/efficiency core classes; this
phone's actual topology and placement were not captured. Scheduling/thermal
policy matters. A worker on a slower core can lengthen the join; saturating additional cores can
raise power/thermal load. Vertex data, output copies, GPU and audio also compete
for cache/memory bandwidth. No bus counter was collected, so representation byte
counts do not establish bandwidth saturation. Do not pin all work to fast cores
or use the number of cores as the worker count. Android's own guidance describes
the thermal/portability costs and generally recommends avoiding manual affinity.
[Android scheduling guidance](https://developer.android.com/agi/sys-trace/threads-scheduling).

## Trace needed before an implementation claim

At the next explicitly owner-offered phone window, capture a short local
Perfetto/Android system trace alongside an exact same-scale hold, including:

- Process/thread names and lifetimes, CPU scheduling switches/wakeups,
  running versus runnable/blocked intervals, core migration and frequency/idle.
- Native render TID, music/ambience/save TIDs and any experimental worker TID;
  stage/owning-record ATrace slices if the existing CSV clocks cannot be aligned
  unambiguously. Trace markers would be a small separately reviewed optional
  instrumentation change, not already present evidence.
- Vulkan wait/acquire/submit/present and GPU queue slices/counters when the
  actual device/tool exposes them, plus unchanged submitted settings, display
  intervals, thermal start/end and a no-trace control to bound profiling overhead.

This distinguishes actual computation, scheduler delay, lock/join waits,
GPU/presentation blocking and frequency throttling. CPU scheduling events and
wakeup latency are documented by [Perfetto](https://perfetto.dev/docs/data-sources/cpu-scheduling);
Android's [system profiler documentation](https://developer.android.com/agi/sys-trace/system-profiler)
describes per-core scheduling/frequency and Vulkan/GPU sources. Availability on
this exact phone must be verified; no GPU counter or privileged capability is
assumed. Raw traces remain private/local unless separately consented for sharing.

No such trace was taken today. A denied `/proc/uptime` read was not bypassed,
and an old-interval `dumpsys cpuinfo` snapshot cannot substitute. The phone stays
released to Reef; this assessment uses source and saved evidence only.
