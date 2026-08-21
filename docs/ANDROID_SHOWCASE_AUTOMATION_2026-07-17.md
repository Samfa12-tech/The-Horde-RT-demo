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
- measures `opening`, `two-enemy-combat`, `worst-bend`, `skylight`, `green`, and `lich` at 75%;
- discards 120 warm-up frames, then records three consecutive 120-frame windows per checkpoint;
- enforces the strict below-20.000 ms median-of-window-averages gate and thermal status 0-3 ceiling for those six 75% checkpoints; 18.500 ms inclusive to below 20.000 ms is `PASS - LOW HEADROOM`, while 20.000 ms or above is `FAIL`;
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

GPU timestamp queries remain enabled by default. For a matched instrumentation A/B, cool the phone back to the same thermal status and comparable reported temperatures before each run, use the same freshly built APK, checkpoint order, scale, and background state, and run separate cold app sessions:

```powershell
.\tools\run-android-showcase-validation.ps1 -Mode Benchmark -Scale 75 -Checkpoints lich -GpuTiming Enabled -SkipBuild
.\tools\run-android-showcase-validation.ps1 -Mode Benchmark -Scale 75 -Checkpoints lich -GpuTiming Disabled -SkipBuild -SkipInstall
.\tools\compare-android-gpu-timing-ab.ps1 -FirstRunDirectory <enabled-run> -SecondRunDirectory <disabled-run>
```

`Disabled` removes the per-frame query reset, timestamp writes, and result readback only. Native `vkCmdTraceRaysKHR`, ray-query shading, swapchain presentation, render scale, and the 20 ms CPU gate are unchanged. Each run hashes the local and installed APK and refuses an artifact mismatch; schema-5 summaries also record commit/dirty state, embedded raygen hash, device serial, OS build fingerprint, and Vulkan GPU/driver identity. The comparator requires one enabled and one disabled run with matching provenance and honest timing-row metadata, AP/SKIN/BAT starting temperatures within 2 C by default, the 20 ms gate, and no enabled-versus-disabled regression above the 15% investigation threshold. Historical schema-4 evidence remains readable but is explicitly labelled as lacking serial/fingerprint provenance.

## Named checkpoints

| Name | Expected state |
|---|---|
| `opening` | Fresh opening state |
| `skeleton` | Fresh skeleton-room encounter view |
| `two-enemy-combat` | Fresh two-skeleton encounter at its deterministic measurement pose |
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
- `timing.csv` with three 120-frame averages, median, derived FPS, thermal status, battery temperature, and GPU instrumentation mode;
- one native checkpoint-state JSON file per measured view;
- deterministic replay state;
- scoped `logcat.txt`;
- private Vulkan capability reports;
- matching local/installed APK hashes, source commit/dirty state, embedded raygen hash, package/build/device properties, and before/after raw battery and thermal dumps;
- optional fixed checkpoint screenshots.

`reports/` is Git-ignored so routine runs do not dirty the repository. Promote only a reviewed result into a dated `docs/` validation record when it represents a milestone or release gate.

Timing is CPU wall-clock time from frame start through `vkQueuePresentKHR`; it is not a Vulkan GPU timestamp. This is deliberate continuity with the renderer's existing frame telemetry and should be labelled accurately when results are quoted.

## What the replay proves

The route replay uses the same shared walkable rectangles, obstacles, collision resolver, zone query, lantern/enemy updates, and rendered frame loop as normal play. Its fixed `0.032` world-unit step is independent of ADB gesture duration and display refresh rate. It detects stalls, wrong-zone arrivals, and failure to reach all 13 waypoints.

It does not prove that touch controls feel good, that sound is audible or directional to a listener, or that a screenshot looks artistically correct. Its automated Home/resume check proves only the recorded lifecycle contract, not subjective comfort. Keep these short hands-on checks after meaningful input, visual, or lifecycle changes. Owner audio/haptic checks are separately change-triggered: every milestone must state `Audio/haptic manual revalidation required: YES/NO` with a reason, defaulting to `NO`; require `YES` only for material changes to spatialisation, source/listener event data, cue/backend/event/haptic routing, feedback timing, or damage/death semantics. The current reconciliation is `YES` because listener-at-event-time routing changes:

1. Walk and look with both touch regions; fight both opening skeletons, then swing three accepted hits against the lich.
2. Confirm the two skeletons remain readable, have distinguishable positional audio/haptic impacts, preserve corpse/survivor separation, and still retain the lich hit cries, charge/electricity, recoil, death, and moving roof.
3. Inspect zig-zag seams, lantern drop, skylight depth, four light colours, mirror exposure, wet floor, player shadow, and prop wall retraction.
4. Exercise pause/resume and Home/surface recreation.
5. Confirm the phone remains comfortable and responsive at the recommended 75% tier.

## Current validation status

The original shared checkpoint builders and route replay pass Windows host smoke tests. Android Debug and Release APKs compile with the automation enabled only for Debug. Historical exact two-enemy commit `b3428a7` passed its six-checkpoint 75% gate, matched GPU-timing A/B, replay/captures, and Home/resume. Exact `daa5892` later passed functionality but not its complete warm performance gate. Current exact runtime/authority candidate `4a4d360` passes strict ASTC, honest presentation, a cooled six-checkpoint 75% gate, matched GPU-timing A/B, replay, 13 captures, Home/resume, and the complete clean Host foundation. The owner reports that audio sounds good on the exact installed candidate; exact-candidate haptic confirmation remains pending. See `ANDROID_SHOWCASE_AUTOMATION_VALIDATION_2026-07-17.md`, `TWO_SKELETON_COMBAT_ANDROID_VALIDATION_2026-08-12.md`, and `CURRENT_DEVELOPMENT_BASELINE_VALIDATION_2026-08-21.md`. Owner-only release safety is in `OWNER_RELEASE_SAFETY_CHECKLIST.md`.
