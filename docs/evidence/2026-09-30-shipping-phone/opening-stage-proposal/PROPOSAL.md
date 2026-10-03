# Opt-in Android Shipping GPU-stage probe (proposal only)

Status: design only; no repository files changed, no APK built, and no device work performed.

Repository inspected: `C:/Users/sam_s/Documents/the Horde RT Demo/.worktrees/horde-1.6.1-engineering-pass`, HEAD `eafbf8262442a82e0835edf5cf5306d718e64633`. Existing `.superpowers/...` untracked files were left untouched.

## Proposed bounded change

Add an investigation-only compile-time gate (default OFF, enabled only for a local probe build) and an explicit runtime opt-in. In `SwapchainContext`, own two independent `horde::vulkan::GpuFrameTimer`s and one fixed per-slot pending record. Do not repurpose, share state with, or alter `gpuFrameTimer`; its full-command-buffer timing and standard Shipping benchmark remain unchanged. Do not add a generic telemetry subsystem.

Pass optional geometry and trace timer pointers plus the existing `currentFrame` into `PresentableTinyRtScene::RecordTraceAndCopy` (null/default on ordinary builds). The scene records these two pairs only when the probe is enabled:

1. Geometry/AS interval: begin immediately before `UpdateDynamicInstances(...)` at `PresentableTinyRtScene.cpp:5459`; end immediately after it returns successfully. This covers GPU commands encoded by dynamic instance/BLAS/TLAS updates and their dependencies, not the CPU time spent preparing them.
2. Trace interval: begin immediately before the dispatch inside `ExecuteObservedTraceCopyCommands` (`PresentableTinyRtScene.cpp:5537–5555`); end immediately after the actual `vkCmdTraceRaysKHR_` (or compute-dispatch fallback) call. Keep image transitions, transfer/blit, and presentation outside this interval.

The timers use the existing `GpuFrameTimer` pair implementation and timestamps; they are independent query pools, each indexed by the same `currentFrame` slot. Android currently has `kMaxFramesInFlight = 1` (`android_probe_bridge.cpp:69`), so the proposal uses that constant rather than assuming two slots. Timer initialization is best-effort and must never make RT startup fail. Disabled, unsupported queue, initialization/query error, record failure, canceled submission, missing identity, and valid sample are distinct output states.

## Submission identity and collection

The owning render thread is the only owner, matching `GpuFrameTimer`'s no-lock contract. After a successful `vkQueueSubmit`, use the already-prevalidated `RtEvidenceSubmitTransaction.identity` (`RtFrameEvidenceCoordinator.h:56–61`) and its `submissionSerial` as the sequence passed to each successfully recorded stage timer's `MarkSubmitted`. Do not allocate a second counter. Copy that exact identity plus record-time labels into the same frame-slot pending record only after queue submission succeeds. If the transaction is invalid, leave identity unavailable and do not mark a stage sample as joinable.

On the next successful wait of `inFlightFences[currentFrame]`, at the existing `CompleteFence` site (`android_probe_bridge.cpp:2398–2425`), collect each submitted stage timer independently. Join only when the collection is consumed and its `submissionSequence` equals the saved `identity.submissionSerial`; otherwise emit an explicit mismatch/error status, not a duration. Include the exact submitted identity fields (`sceneEpoch`, `measurementGeneration`, `recordAttemptSerial`, `recordSerial`, `simulationTick`, `frameSlot`, `submissionSerial`), stage/zone, sample status and milliseconds. Capture zone from the exact rendered simulation snapshot used to build `frameInputs`; capture `playerRenderRoute` and active strategy at record time (the evidence path already exports `activeStrategy` at HEAD `eafbf82`). Do not read mutable global state at fence completion to reconstruct labels. Treat queue submission timing as valid for the submitted frame even if presentation later fails; record presentation outcome separately if useful.

Every cancellation path must cancel both stage recordings independently: failed scene record, `vkEndCommandBuffer`, fence reset, queue submit, and any invalidated/identity-less evidence transaction. A failure in one probe timer must not fail a render or discard the other stage. Respect the existing evidence lifecycle: `BeginFrame`/`BeginRecord`, `PrevalidateSubmit`, successful submit, then `CommitGraphicsSubmit`; do not introduce another submission lifecycle.

## Lifecycle and opt-in boundaries

- Initialize the two timers in `InitialiseRtSceneForSwapchain` after the device/scene exist, only under the compile-time gate plus runtime opt-in. Use the same physical device, device, graphics queue family, and `kMaxFramesInFlight` as the full timer. Preserve each timer's `Telemetry()` status/diagnostic.
- Collect only after the matching successful fence, or through the existing successful device-idle drain (`CompleteRtEvidenceAfterDeviceIdle`, lines 2082–2122). Never read queries following a failed fence or failed `vkDeviceWaitIdle`.
- In swapchain release/recreation, after successful idle, drain any owned submissions, then `ResetAfterDeviceIdle()` both timers and clear pending labels before the scene/query epoch is reused. On failed idle, do not claim completion or read/reset in-flight query state; mark pending probe records canceled/unknown and follow the established device-lost/teardown path. Destroy both query pools before `vkDestroyDevice` in `DestroySwapchainContext`.
- Probe enablement must not depend on `gpuFrameTimingEnabled`: the standard full timer can remain enabled/disabled exactly as today. Probe output must not be added to `RefreshGpuTimingTelemetry`, `RtPerformanceEvidenceSnapshot`, benchmark windows, or standard release reports. The normal Shipping build has the compile-time gate off; no permanent Shipping overhead or expanded normal instrumentation.

## Interpretation limits / experiment design

This is feasible as a bounded probe by reusing `GpuFrameTimer`; no permanent framework is needed. It yields GPU timestamp intervals for dynamic geometry/AS work and the whole RT dispatch, not a decomposition of the dispatch. It cannot independently time lighting, shadow rays, OpaqueFast material work, or player-geometry traversal inside the trace shader. Therefore it cannot by itself separate GPU lighting/shadow cost from player geometry. Do not infer that split from geometry-stage time or aggregate trace time.

For the requested opening-room investigation, retain and report the record-time `activeStrategy` and require `OpaqueFast` for the intended sample set; discard/segregate any other strategy. Keep opening-room pose, simulation tick/replay, render scale, scene, route, and capture window fixed. The active strategy evidence verifies route selection, not shader-internal cost. A later player-geometry-versus-lighting/shadow comparison needs a separately authorized, controlled workload comparison (or suitable existing measurement instrumentation) and must report its changed workload; do not alter this probe's shaders, budgets, geometry, animation, or rendering to manufacture the split. Probe runs are separate from the frozen full-timing APK evidence because query commands and query-pool traffic can perturb timings.

## Exact source hooks reviewed

- `src/vulkan/GpuFrameTimer.h:68–104` and `src/vulkan/GpuFrameTimer.cpp:217–335`: supported/error status, per-slot begin/end/cancel/submit/collect API; collection must follow owning fence.
- `src/vulkan/raytracing/RtFrameEvidenceCoordinator.h:56–123` and `.cpp:340–418, 759+`: immutable submit candidate/identity and fence/idle completion lifecycle.
- `src/vulkan/raytracing/PresentableTinyRtScene.cpp:4319` (`UpdateDynamicInstances`), `5420–5462` (call), `5479–5484` (selected strategy), `5537–5555` (actual RT/compute dispatch).
- `android/app/src/main/cpp/android_probe_bridge.cpp:69, 141–230` (frame slots/context), `1951–1968` (swapchain release), `2082–2122` (idle completion), `2220–2254` (RT/timer init), `2277–2305` (destroy), `2398–2425` (fence collection), `2822–2859` (record), `2896–2961` (submit identity/commit).

## Review risks for implementation owner

1. Reusing `RecordBegin/RecordEnd` means the helper uses its existing TOP_OF_PIPE/BOTTOM_OF_PIPE timestamp semantics; these are interval timings, not isolated hardware-engine counters. Validate supported timestamps/status and report that limitation.
2. Probe query reset/write commands add work. Keep the probe off by default, short-window and opt-in; never compare its times as if they were the immutable full-timer run.
3. Do not let optional query failures alter scene recording, submission, evidence, or presentation. A stage may be `unavailable` while the frame succeeds.
4. Handle failed idle/device loss conservatively: no fence-less query read. Resource destruction remains ordered before device destruction.
5. `RtFrameEvidenceCoordinator` currently accepts only the full timer adapter. Stage timers should remain a small Android-owned sidecar keyed by the exact transaction identity, not broaden that coordinator's generic API unless implementation proves that unavoidable.

