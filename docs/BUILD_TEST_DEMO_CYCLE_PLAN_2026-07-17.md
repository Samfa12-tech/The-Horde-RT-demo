# Build / Test / Demo Cycle Plan

## Purpose

Make every meaningful renderer or gameplay change easy to build, measure, reproduce, and present on both Android and Windows without weakening the RT-or-nothing proof.

This is a supporting production/tooling plan for the current **Windows-validated / Android-device-validated** showcase. It does not expand the active-enemy limit or replace the bounded route with a broader game.

## Priority order

## Implemented foundation - updated 2026-07-22

- Priorities 1 and 2 now have shared named checkpoints, deterministic encounter presets, fixed three-window measurement, strict ASTC/honest-presentation gates, and native state evidence.
- Priority 3 now has a deterministic 13-waypoint Android route replay for movement, collision, zone, lantern, and enemy-update regression coverage. Full raw touch/look/swing/pause input recording remains deferred because ADB gesture timing is refresh-rate dependent; hands-on input validation remains required.
- Priority 5 is complete as `tools/run-foundation-validation.ps1 -Mode Host|Full`: fresh dual-configuration Windows builds/tests, clean Android builds/lint, shader-staleness, package/layout, asset/licence, release-identity, hashes, Windows capture, and the connected-phone evidence gate share one timestamped report contract.
- Priority 4 has a shared compact formatter and Debug-only live overlay. Windows F3 and the Android long-press surface are device-validated; Release omits/rejects engineering automation paths.
- The engineering checkpoint surface remains Debug-only. A separate release-safe two-pass in-app benchmark now reuses the shared route and produces player-exportable reports without exposing checkpoint controls.
- Priority 6's deterministic still-capture portion is complete for all 12 shared checkpoints on Windows and Android. Video, presentation orbit camera, and comparison-view UI remain deferred.

### 1. Deterministic showcase mode — complete

- Twelve named checkpoints now cover Opening, Skeleton, Lantern Drop, Skylight, each coloured-light bay, Mirror, Lich, and Finale Roof.
- Lighting, enemy, lantern, roof, and encounter states are reproducibly selectable in Android Debug and host-tested shared code.
- Keep normal player traversal intact; checkpoints are for development, validation, and presentation.

### 2. Repeatable measurement — complete on Windows and Android

- A fixed 13-waypoint route and default five-checkpoint benchmark cover the important showcase cost centres.
- Evidence records render scale, internal resolution, frame time, derived FPS, thermal/battery state, RT mode, active skinned-enemy count, and presentation status.
- Preserve separate 100% and recommended 75% Android results.
- The in-app benchmark runs one visible warm-up lap and one measured lap, aggregates overall and per-zone timing, requires honest RT presentation, and emits selectable text plus JSON. Windows Release and Android device lifecycle/export are live-validated.

### 3. Reproducible input — partial

- Native deterministic movement/collision replay is complete. Raw Android touch and Windows keyboard/mouse/look/swing/pause recording remains deferred because wall-clock input injection is refresh-rate dependent.
- Mark replay results as automated evidence; retain hands-on verification where the device cannot expose a state directly.

### 4. Developer visibility — complete

- The shared formatter reports build/shader identity, GPU/API, RT mode, route zone, render scale/extents/timing, BLAS/TLAS/instance counts, material encoding, lantern/enemy state, active skinned count, and honest RT presentation.
- Windows Debug toggles it with F3 without pausing; F2 remains the full paused diagnostic surface. The compact control is disabled for input and hides during entry, pause, settings, and diagnostics.
- Android Debug has a hidden non-interactive view, `horde.debug.overlay` intent, long-press status toggle, and a native 4 Hz snapshot publisher. Both APK build types, Release lint, touch pass-through, font scale 1.7, thermal timing, audio/lifecycle interaction, and visual approval passed on `SM-S948B`.
- Windows Release omits the developer UI and Android Release returns no developer overlay text. Keep technical output hidden from branded player-facing surfaces unless requested or startup fails.

### 5. One-command validation and packaging — complete

- `tools/run-foundation-validation.ps1` defaults to the daily `Host` gate; `-Mode Full` adds the required `SM-S948B` checkpoint/replay/lifecycle/capture gate and reports 100% separately.
- The gate produces timestamped logs, source/build/asset/APK/ZIP/licence hashes, summaries, manifests, PNGs, and validation-only packages beneath ignored `reports/foundation-runs/`.
- Validation artifacts are explicitly unpublishable, never enter `releases/candidates/`, and use an unsigned validation-only Android build without reading or requiring release secrets.
- Negative fixtures prove rejection of stale embedded SPIR-V, missing licences/assets, invalid ZIP root layout, immutable 0.1.1/0.1.2 identities, old Android version codes, and another 0.1.2 upload before Butler is contacted.
- Future candidate packaging requires explicit `Version` and `VersionCode`, rejects the immutable published 0.1.1/0.1.2 lines, and requires `versionCode > 3`.

### 6. Capture and presentation — deterministic screenshots complete; video/orbit pending

- Windows Debug `--capture-showcase <directory>` and Android Debug capture intents produce all 12 shared scene-only checkpoints with fixed animation time, settling frames, honest presentation, camera/state/build/shader/GPU/extent/colour-route metadata, and PNG SHA-256. Release rejects both automation paths.
- Windows captures read back the RT storage image and encode PNG through WIC, applying red/blue normalization only for the raw-copy BGRA route. Android hides menu, touch, HUD, diagnostics, and developer overlays before stable-frame ADB capture.
- Short video, a presentation/orbit camera, and optional render-scale comparison views remain explicitly deferred.

## Game-facing support after the tooling foundation

- Sword impact, lich recoil/cry, three-hit lockout, death, and encounter feedback are implemented; a distinct hit-pause treatment remains optional polish.
- Named checkpoint presets now reset lantern, enemy, lighting bay, roof, player position, and encounter state for development; player-facing independent scenario controls are not planned for this bounded alpha.
- Preserve large Android accessibility font-scale support and compact HUD behaviour.

## Guardrails

- Keep real `vkCmdTraceRaysKHR` swapchain presentation and honest `rtScene.presented` reporting.
- Keep the phone-safe `rayQueryEXT` path unless a stronger RT path is proven on-device.
- Keep one active skinned enemy until a separate multi-enemy phone measurement passes.
- Measure each meaningful renderer change on the phone at the recommended tier before expanding effects or gameplay.
- Do not add fake raster, screen-space, baked, browser, or overlay substitutes for RT proof.

## Completion signal

The cycle is working when a change can be built from a clean checkout, exercised through the same named route or replay, measured on both targets, captured at fixed presentation points, and handed off with a compact report and package-verification result.
