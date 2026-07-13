# Codebase Audit and Simplification - 2026-07-13

## Outcome

- Removed more than 2,300 tracked lines of disabled, abandoned, duplicated, or misleading Phase-0 code and documentation.
- Reduced `PresentableTinyRtScene.cpp` by roughly 950 lines by deleting the disabled legacy raygen SPIR-V blob; the active generated include remains unchanged.
- Removed 188,817,040 bytes of ignored and reproducible local build/reference data while retaining the current Android build output.
- Preserved the phone-safe `rayQueryEXT` shading path, `vkCmdTraceRaysKHR` dispatch, one-frame-in-flight invariant, primitive/material ordering, held-prop TLAS ownership, and RGBA/BGRA presentation correction.

## Removed

- The obsolete `TinyRtScene` empty-command-buffer bootstrap and its one-use acceleration-structure, pipeline, and SBT validation helpers. It created a second logical device but did not build or dispatch a real RT scene.
- The abandoned root-CMake Android NativeActivity prototype. The supported phone path remains `android/app` with `MainActivity` and the JNI bridge.
- The unreferenced non-rendering `HordeLanternApp` scaffold library.
- Dead Windows command-buffer pre-recording that was always reset before first submission.
- An unused raygen hash helper and stale Phase-0 notes that contradicted the live renderer.

## Consolidated and corrected

- `cmake/HordeRtSources.cmake` is now the single shared native source manifest for root Windows targets and the Android Gradle/CMake build.
- `src/gameplay/CorridorCollision.h` holds the previously identical Android/Windows corridor bounds and arch-post collision logic.
- Android synchronization arrays are sized by the locked `kMaxFramesInFlight == 1` value and use loop-based cleanup instead of inactive second slots.
- The selected `RayTracingPipeline` mode now requires `VK_KHR_ray_query` and its feature because the active raygen shader depends on them.
- The standalone capability probe no longer reports a synthetic RT bootstrap. `rtScene.status` remains `Not attempted` until the real platform renderer reaches successful swapchain presentation.
- `tools/compile-raygen.ps1` deterministically compiles and embeds the active raygen shader.
- Authoritative source, Android, asset, install, phase-plan, and agent guidance now describe the implemented renderer rather than Phase 0.

## Validation

- Raygen regeneration succeeded and retained SPIR-V SHA-256 `97774EAEEAEA4850D10980F2FDA9BB8776757D941E9C6A24C5DF3864A574D520`.
- Fresh Windows CMake configuration succeeded.
- `horde_rt_capability_probe` and `horde_rt_diagnostic_window` built successfully with MSVC.
- The capability probe selected the RTX 5050 as `RayTracingPipeline`, required ray-query support, and reported `RT scene status: Not attempted` / `presented: no` before presentation.
- The interactive Windows RT window remained running during a six-second presentation smoke test.
- Android `assembleDebug` succeeded for `arm64-v8a`, `armeabi-v7a`, `x86`, and `x86_64`.
- No phone was connected, so install, cold launch, RT-present log, and the 126-interval performance gate remain pending.

## Deliberately deferred

- A raygen-only RT pipeline/SBT could remove the currently unused miss and closest-hit runtime groups, but this is driver-facing and requires live Windows plus phone validation before landing.
- Full Android/Windows swapchain-presenter consolidation is deferred; lifecycle, input, timing, and surface ownership differ enough that a large abstraction would add risk.
- Primitive/material numeric ranges and held-flame pose duplication should be replaced incrementally with named shared data, preserving exact geometry order and push-constant compatibility.
- Android control writes and render-thread reads need a separately designed snapshot/atomic fix before larger platform refactoring.
