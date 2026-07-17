# Android Showcase Automation

## Purpose

The Android debug build now has a repeatable validation path for the complete showcase. It replaces repeated manual walking for timing and collision regressions with deterministic checkpoints and a frame-indexed route replay, while keeping hands-on visual, touch, audio, lifecycle, and thermal approval as separate evidence.

The automation is debug-only. Release builds compile with `HORDE_RT_DEBUG_CHECKPOINTS=OFF`, reject the native request entry points, ignore debug intent commands, and retain the public game flow.

## One-command run

Connect and authorise exactly one Android device, then run from the repository root:

```powershell
.\tools\run-android-showcase-validation.ps1
```

The default run:

- builds and installs the side-by-side debug package;
- requires strict environment and lich ASTC selection;
- requires a genuinely RT-produced frame to reach swapchain presentation;
- measures `opening`, `worst-bend`, `skylight`, `green`, and `lich` at 75%;
- discards 120 warm-up frames, then records three consecutive 120-frame windows per checkpoint;
- enforces the existing 20 ms median-of-window-averages gate for those five 75% checkpoints;
- runs the deterministic 13-waypoint route through the finale;
- checks the saved native state against the expected checkpoint, zone, window count, replay completion, and honest presentation state;
- restores the screen to asleep if the script woke it, and always stops the debug app on exit.

For report-only 100% coverage and screenshots captured after timing:

```powershell
.\tools\run-android-showcase-validation.ps1 -Include100 -Capture
```

Useful focused runs:

```powershell
.\tools\run-android-showcase-validation.ps1 -Mode Benchmark -Checkpoints opening,worst-bend,skylight,yellow,blue,red,green,mirror,lich
.\tools\run-android-showcase-validation.ps1 -Mode Replay -SkipBuild -SkipInstall
```

## Named checkpoints

| Name | Expected state |
|---|---|
| `opening` | Fresh opening state |
| `skeleton` | Fresh skeleton-room encounter view |
| `worst-bend` | Fresh zig-zag corner |
| `lantern-drop` | Lantern failure triggered |
| `skylight` | Lantern settled; skylight chamber |
| `yellow`, `blue`, `red`, `green` | Lantern settled; selected coloured-light bay |
| `mirror` | Lich selected and active; mirror composition |
| `lich` | Lich selected and active; finale combat view |
| `finale-roof` | Three accepted hits, completed death, roof fully open |

Checkpoint state is constructed through the shared gameplay state machines rather than by patching renderer output. Normal movement, reset, combat, and release behaviour remain unchanged.

## Evidence bundle

Each run creates a unique ignored working directory under `reports/android-showcase-runs/run-<timestamp>/` containing:

- `validation.md` and `summary.json`;
- `timing.csv` with three 120-frame averages, median, derived FPS, thermal status, and battery temperature;
- one native checkpoint-state JSON file per measured view;
- deterministic replay state;
- scoped `logcat.txt`;
- private Vulkan capability reports;
- APK hash, package/build/device properties, and before/after raw battery and thermal dumps;
- optional fixed checkpoint screenshots.

`reports/` is Git-ignored so routine runs do not dirty the repository. Promote only a reviewed result into a dated `docs/` validation record when it represents a milestone or release gate.

Timing is CPU wall-clock time from frame start through `vkQueuePresentKHR`; it is not a Vulkan GPU timestamp. This is deliberate continuity with the renderer's existing frame telemetry and should be labelled accurately when results are quoted.

## What the replay proves

The route replay uses the same shared walkable rectangles, obstacles, collision resolver, zone query, lantern/enemy updates, and rendered frame loop as normal play. Its fixed `0.032` world-unit step is independent of ADB gesture duration and display refresh rate. It detects stalls, wrong-zone arrivals, and failure to reach all 13 waypoints.

It does not prove that touch controls feel good, that sound is audible or directional to a listener, that a screenshot looks artistically correct, or that lifecycle recovery works. Keep these short hands-on checks after meaningful input, audio, rendering, or lifecycle changes:

1. Walk and look with both touch regions; swing three accepted hits against the lich.
2. Confirm player and skeleton footsteps, lich hit cries, charge/electricity, recoil, death, and the moving roof.
3. Inspect zig-zag seams, lantern drop, skylight depth, four light colours, mirror exposure, wet floor, player shadow, and prop wall retraction.
4. Exercise pause/resume and Home/surface recreation.
5. Confirm the phone remains comfortable and responsive at the recommended 75% tier.

## Current validation status

The shared checkpoint builders and route replay pass Windows host smoke tests. Android Debug and Release APKs compile with the automation enabled only for Debug. The first full live execution and a second corrected fast execution both passed on `SM-S948B`; see `ANDROID_SHOWCASE_AUTOMATION_VALIDATION_2026-07-17.md`. The previously completed hands-on Android showcase validation remains recorded separately in `HORDE_SHOWCASE_ANDROID_VALIDATION_2026-07-17.md`.
