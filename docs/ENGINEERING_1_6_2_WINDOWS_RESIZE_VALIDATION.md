# Full Showcase output resize validation

The Debug-only `--validate-output-resize <absolute-output-directory>` mode measures the missing full-game A4 path. It retains the production Showcase renderer, authored opening and `StageLanternBenchmark(LanternHeldHigh)` state, accepted player presentation, High water/fire and build-selected physical glass. Each state is staged once and frozen before measurement. It makes the real 100→75→50→100 graphics transactions without entering the compact preview or retaining another GPU scene.

`DiagnosticWindow.cpp` extracts the existing normal Apply resize body into `ApplyPendingOutputResize`; both the normal loop and validation call this exact helper. Validation uses `GraphicsEditSession` → `QueueGraphicsCommand` → output resize → ordinary `RenderFrame` presentation → `FinishGraphicsFrame` acknowledgement. Confirm changes only this process's isolated in-memory tuple. User preferences are neither loaded nor written; controller input, gameplay advancement/event consumption, audio startup/playback, progress writes and externally delivered input/menu messages are suppressed. Authored workload staging occurs before each measurement, not during the measured render.

`resize_stall_ms` spans the native steady-clock timestamp immediately before queuing Apply through the first ordinary successful `vkQueuePresentKHR` return for the replacement output. It includes command handling and host/driver waits; it is not scanout/compositor latency or GPU-only execution. Existing `idle_and_resize_ms` ends before the following render/present and remains separately labelled. Readback time is separate again. Each completed first-frame packet must join the exact scene epoch, measurement generation, record attempt/record/submission serials, slot and simulation tick before capture.

The manifest records the actual executable SHA256, build/display IDs, device/driver/API/backend, actual compiled shader keys/hashes, material encoding, requested/effective settings and dimensions, owning canonical completed packets (CPU stages, available GPU/Diagnostic data and tracked allocation inventory), and before/after opaque Vulkan handles. All actual BLAS, TLAS, pipelines, SBT buffers, descriptor sets and texture images must remain equal. New output image/memory/view handles and a new output epoch must be observed. Allocation property categories may overlap and are not resident VRAM, a budget or peak-memory measurement. Eight uniquely named production RT readback PNGs retain baseline and return-to-100 separately; no native UI is composited.

Relative, duplicate or nonempty destinations and capture/checkpoint/benchmark/RT Lab combinations are rejected. Release rejects the mode. Initial settling requires twelve ordinary RT presents and permits at most eight recreation restarts; measured transitions permit no recreation. Frame-fence/acquire waits are two seconds only in isolated validation. Device-idle/readback retain production driver behavior, so an external process deadline is required.

Lead-only execution, from the task root after the current native Debug target is rebuilt and assets staged:

```powershell
./run-rt-capture.ps1 -Mode Resize `
  -Executable ./source/build/presets/windows-x64-debug/Debug/HordeLanternRT.exe `
  -OutputDirectory 'C:\absolute\fresh\resize-pipeline' -DeadlineSeconds 300
# Separate process/new directory for the admitted Compute backend:
./run-rt-capture.ps1 -Mode Resize -Compute `
  -Executable ./source/build/presets/windows-x64-debug/Debug/HordeLanternRT.exe `
  -OutputDirectory 'C:\absolute\fresh\resize-compute' -DeadlineSeconds 300
```

Source handoff checks: owned `git diff --check` and external launcher's PowerShell parse pass. The portable launch fixture covers absolute/UNC inputs, conflicts, fixed workload/scale roster, stale/tokenless completion identity and eight non-overwriting filenames. Compile, fixture execution and real GPU evidence are pending the lead's exclusive validation slot; this note does not claim them. Independent source review was requested before execution.

Remaining acceptance: native UI/accessibility/interaction, allocation failure and interrupted confirmation recovery, exact Android-device resize/lifecycle evidence, and owner visual/changed-audio acceptance. Scene-only readback and in-memory transactions do not close those gates. This agent performed no build, GPU or phone action for this slice; the lead owns the current explicitly allocated phone.

## Lead-run exact GPU evidence

On 3 October 2026 the lead completed both actual backends on NVIDIA GeForce RTX 5050 Laptop GPU with Debug executable SHA256 `35ffa35c39267bc3509b803da1c166c2f817b6579c460e55272a9c5f0aeb02e0`, build/display 1.6.2. Both manifests are complete, contain eight records (six real transitions), and have matching launch receipts with exit 0 and zero synchronization/validation error markers. Independent source review found no actionable seam defect before execution.

| Actual backend | Manifest SHA256 | Native request-to-first-ordinary-present range | Idle/output-resize range | Separate readback range |
| --- | --- | --- | --- | --- |
| RayTracingPipeline | `e2e1d68ed4cbb7d015b482dfd37ffe6566ecfa55411c4a5cf07693bdb5e0368c` | 6.1714-13.8334 ms | 1.7404-3.3708 ms | 4.3616-21.0316 ms |
| RayQueryCompute | `f7c17a15d3ff08bf8145dddc2f7db34ffa6b0ca180ca4be9c8697676d1dc4cad` | 4.1139-6.4170 ms | 1.7069-2.8355 ms | 2.7137-14.5001 ms |

Private raw evidence remains in ignored `reports/1.6.2-resize-pipeline-torch` and `reports/1.6.2-resize-compute-torch`; this note contains only sanitized artifact identities and measured ranges. Readback ranges include all eight baseline/transition captures; the other ranges include only the six transitions. These are one exact Debug candidate's bounded frozen-state measurements, not balanced baseline comparisons, sustained/thermal evidence, Shipping performance, Android latency, scanout, native UI acceptance or allocation-failure recovery. Completed canonical packets and actual preserved handle vectors are the ownership authority. Adoption requires both completed manifest and process exit 0 because teardown can fail after measurement completion. The external process deadline remains required for driver idle/readback calls.
