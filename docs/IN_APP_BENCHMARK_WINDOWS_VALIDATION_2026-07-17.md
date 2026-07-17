# In-app benchmark - Windows validation - 2026-07-17

## Implemented player flow

Both platform menus now expose `RUN BENCHMARK`. The player-facing benchmark is separate from the Debug checkpoint harness and is compiled into Debug and Release.

- Pass 1 visibly traverses the shared deterministic 13-waypoint showcase route as a warm-up.
- The simulation resets cleanly, then pass 2 repeats the same course and records the official sample.
- Manual movement, look, attack, settings and diagnostic input are blocked during the run. `Esc` / Android Back cancels instead of admitting paused frames.
- Completion requires two complete laps, 26 reached waypoints, a non-empty measured pass and an RT-produced successful swapchain presentation for every measured frame.
- The report does not invent an FPS quality threshold. `COMPLETE` is an integrity result; frame-time figures remain descriptive.
- The timing surrounds the native render/present call and reports the actual swapchain present mode because results can be display-limited.
- Route simulation uses a fixed `1/60` gameplay step and identical movement on both laps; wall-clock render/present time remains the measured value.
- `1% low` is the reciprocal of the average frame time of the slowest one percent of measured frames.

The result contains build and embedded-raygen identity, GPU/API, RT and present modes, material path, render scale and extents, course integrity, overall average/median/P95/1%-low values, and per-zone timing.

## Export behavior

Windows finishes in a scrollable read-only report panel. The text is selectable and the panel provides `COPY REPORT`, `SAVE AS...`, and `BACK TO MENU`. Each successful run also writes timestamped text and JSON files under the executable-relative `reports/` directory.

Android finishes in a selectable dynamic report panel with `COPY REPORT` and `SAVE REPORT`. Save uses the system `ACTION_CREATE_DOCUMENT` picker, producing a user-chosen text document rather than exposing the app-private evidence directory. Android lifecycle/touch/export behavior remains a connected-phone validation item.

## Windows Release evidence

The Release app was run end-to-end at 100% on the validated NVIDIA GeForce RTX 5050 Laptop GPU:

- build `0.1.1-alpha.1`, embedded shader `2efd80c12f09`;
- Vulkan 1.4.341, `RayTracingPipeline`, `MAILBOX` present mode;
- internal/presentation extent `1232x803`;
- laps `2/2`, waypoints `26/26`, measured frames `1838`;
- honest RT presentation on every measured frame;
- 7.827 ms average, 6.216 ms median, 16.508 ms P95, 20.896 FPS 1% low;
- selectable result panel visible with Copy, Save As and Back controls;
- the report panel hid the gameplay HUD, while keeping Copy, Save As and Back visible;
- `COPY REPORT` placed the complete 1,769-character report, including integrity and saved paths, on the Windows clipboard;
- the timestamped JSON parsed successfully with `result: complete`.

These figures are representative local evidence, not a new release promise or cross-device score. The JSON and text artifacts remain ignored build evidence under `build/Release/reports/`.

## Build and automated validation

- Windows Debug and Release build.
- All seven CTests pass, including `horde_rt_showcase_benchmark_smoke`.
- The focused smoke covers two frame-symmetric deterministic laps, 26 waypoints, final-lap sampling, slowest-1% aggregation, presentation-integrity failure and deterministic text/JSON generation.
- Android Debug and Release assemble for all four ABIs and Release lint passes.

## Pending phone gate

When the phone is attached, validate portrait and landscape at the normal and 1.7 accessibility font scales, course visibility, Back cancellation, thermal behavior, Home/surface interruption, report scrolling/selection, clipboard copy, document-provider export, and return from the picker. Record 75% as the sustained phone tier and 100% separately.
