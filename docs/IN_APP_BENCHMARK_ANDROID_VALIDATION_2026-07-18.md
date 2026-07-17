# In-app benchmark and developer overlay - Android validation - 2026-07-18

## Verdict

**PASS on the connected Samsung `SM-S948B` Debug build.**

The standard Android RT checkpoint/replay gate, player-facing two-pass benchmark, report UI, document-picker handoff, Back/Home cancellation, 1.7 accessibility-font layout, supported portrait recreation, and Debug developer overlay all passed. The owner then completed the requested hands-on movement/look/Swing, audio-cue, and visual-legibility pass without reporting an issue.

## Device and build

- Device: Samsung `SM-S948B`, Adreno 840, Android 16 / API 36.
- Vulkan: 1.4.295, `RayTracingPipeline`, strict environment and lich ASTC route.
- Debug APK SHA-256: `8daef7b9b2e343f73b90b33f6a686f8ea44187caee4210a550120dc3d93f642e`.
- Current system font scale: `1.7`; it was read but not changed.
- Automated evidence: `reports/android-showcase-runs/run-20260718-073732/` (ignored working evidence).

## Standard 75% checkpoint and replay gate

`tools/run-android-showcase-validation.ps1` rebuilt and installed the side-by-side Debug APK, then passed all five three-window checkpoints and the deterministic 13-waypoint route replay. Every checkpoint reported honest RT swapchain presentation and thermal status 0.

| Checkpoint | Median of three 120-frame averages | Battery |
|---|---:|---:|
| opening | 11.870 ms | 28.2 C |
| worst bend | 7.909 ms | 28.2 C |
| skylight | 8.321 ms | 29.5 C |
| green bay | 9.495 ms | 29.5 C |
| lich finale | 13.383 ms | 31.3 C |

## Player-facing benchmark

Both runs completed two frame-symmetric laps, reached 26/26 waypoints, retained 1,838 measured frames, and reported `presentedEveryFrame: true`.

| Scale | Internal / presentation extent | Average | Median | P95 | 1% low |
|---:|---:|---:|---:|---:|---:|
| 100% | `1440x2980` / `1440x2980` | 18.675 ms | 17.725 ms | 26.949 ms | 27.057 FPS |
| 75% | `1080x2235` / `1440x2980` | 13.143 ms | 12.330 ms | 19.844 ms | 34.049 FPS |

These are local device observations, not a release promise. The 75% tier remains the sustained recommendation.

## Report, lifecycle, and link behavior

- The complete report remained selectable and scrollable at the existing 1.7 font scale; `COPY REPORT`, `SAVE REPORT`, and `BACK` were reachable after scrolling.
- `COPY REPORT` executed without a crash. ADB deliberately did not read the system clipboard because it could expose unrelated private clipboard data.
- `SAVE REPORT` opened Android DocumentsUI with `HordeLanternRT-benchmark.txt`; the picker was cancelled without writing into personal storage, and the complete report returned intact.
- Android Back cancelled an active benchmark and restored the menu without a false completion report.
- Home/surface interruption cancelled cleanly and returned to a responsive menu.
- The manifest intentionally uses `sensorPortrait`; the complete report survived recreation in the supported reverse-portrait setting, and the phone's original rotation settings were restored exactly. Landscape is not an app-supported gate.
- `MORE BY SAMFA12` launched Chrome; the app source target remains exactly `https://samfa12.com/`.
- The final app-scoped log scan found no fatal Android runtime or native render-loop failure. Final thermal status was 0 at approximately 35 C battery temperature.

## Debug developer overlay

- At 75% and font scale 1.7 the seven-line panel occupied `[56,364]-[1316,827]` and exposed the correct build/shader, Adreno/Vulkan, RT/presentation, `1080x2235 -> 1440x2980` extents, route/encounter, AS counts, and ASTC material route.
- Long-pressing `NATIVE VULKAN RT ACTIVE` hid and restored the overlay.
- Home/resume retained the requested overlay state.
- The overlay view remained non-clickable. The subsequent hands-on movement/look/Swing pass completed without a reported issue.
- An overlay-active opening checkpoint produced 11.061, 17.473, and 27.250 ms window averages (17.473 ms median), stayed below the existing 20 ms validation budget, and honestly presented every sample.

## Hands-on completion

The owner confirmed `PASS` after entering the ruin, exercising walk/look/Swing, listening to the cues, and judging visual legibility. No automated, renderer, lifecycle, export, or hands-on blocker remains for this phone gate.
