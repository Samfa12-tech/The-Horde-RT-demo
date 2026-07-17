# Phase 1C Phone Validation - 2026-07-11

> Historical renderer-foundation record. Its “current” and “next” language applies to the 2026-07-11 milestone; use `DOCUMENTATION_CHECKPOINT_2026-07-17.md` for the current release and backlog.

## Outcome

Phase 1C passed its primary-target RT closeout on the Samsung `SM-S948B` from baseline commit `93e881856bfeec05efd9c35bddc7c7753146a94e`.

The current source built cleanly, installed, cold-launched, selected `RayTracingPipeline`, dispatched the RT scene at `1440x2812`, and reported a successful RT-produced swapchain presentation. No Android runtime or Vulkan renderer error was observed during the control, collision, reflection, and stability exercises.

## Build and runtime evidence

- APK: `android/app/build/outputs/apk/debug/app-debug.apk` (`6,762,681` bytes).
- Install: streamed `adb install -r` completed with `Success`.
- Cold launch: `Status: ok`, `LaunchState: COLD`, `TotalTime: 407 ms`.
- Device: Samsung `SM-S948B`, physical display `1440x3120`, density override `560`.
- Fresh log proof:

  ```text
  07-11 07:42:54.054 I/HordeRtProbeBridge: Started Android diagnostic surface rendering loop.
  07-11 07:42:54.057 I/HordeRtProbeBridge: RT frame reached Android swapchain presentation.
  ```

- Capability report: `RT mode: RayTracingPipeline`, `RT scene status: Presented via swapchain`, `RT scene presented: yes`.
- Internal RT render and dispatch resolution: `1440x2812`.

## Phone visual proof

- [Initial corridor and held torch](validation/phase1c_phone_corridor_2026-07-11.png): the held flame is warm orange rather than cyan, is visibly attached to the left-hand torch mesh, and produces warm floor and wall illumination. No fullscreen torch overlay is present.
- [Torch reflection proof](validation/phase1c_phone_reflection_2026-07-11.png): the reflective insert contains a dim reflected flame silhouette sourced from the RT torch instance.
- [Corridor traversal view](validation/phase1c_phone_traversal_2026-07-11.png): forward traversal reached the deeper corridor/second-room side of the authored scene while retaining fog, wet-floor response, shadowed panels, and warm lighting.

The procedural stone, wet surface, aged-metal inserts, fog, silhouettes, and second-room lighting remain deliberately early art. This pass verifies their phone rendering and material separation; it does not promote them to final PBR art.

## Controls, collision, and stability

- Exercised repeated left-side forward/back and strafe drags with real Android input events.
- Pressed into both corridor side limits, traversed the corridor depth, and alternated strafe/forward input around the arch area. The viewpoint remained bounded; no wall tunnelling, trapping, crash, or renderer error was observed.
- Exercised more than one full yaw revolution using repeated right-side drags. Yaw remained unbounded.
- Exercised both pitch extremes and returned toward neutral. The native `-0.32` / `0.28` clamp remained effective.
- The final continuous movement/stability process remained alive and responsive for at least `03:14`; the earlier control and visual-proof runs also completed without an error.

This scripted device pass proves the collision boundaries operate under sustained input. Subjective thumb comfort should continue to be checked during ordinary playtests, but it is not a Phase 1C blocker.

## Performance sample

The in-app capability report still labels FPS/frame time as `N/A`; that reporting field was not falsified or populated from an unrelated clock. A separate SurfaceFlinger latency sample of the live Vulkan `SurfaceView` produced:

- 126 frame intervals.
- Median frame time: `17.284 ms` (`57.9 FPS`).
- Average frame time: `17.372 ms` (`57.6 FPS`).
- 95th-percentile frame time: `17.908 ms`.

These figures are a short presentation-level phone baseline, not an in-engine GPU timing benchmark.

## Closeout decision

No shared renderer or shader change was required. Preserve `vkCmdTraceRaysKHR`, ray-pipeline recursion depth 1, ray-query primary/shadow/first-bounce traversal, one frame in flight, the presentation-format red/blue swap, and honest `rtScene.presented` reporting.

The next implementation slice is the narrow GLB/skinning/PBR import path for exactly one animated skeleton. The sword remains excluded until a phone-safe remesh is approved.
