# Build / Test / Demo Cycle Plan

## Purpose

Make every meaningful renderer or gameplay change easy to build, measure, reproduce, and present on both Android and Windows without weakening the RT-or-nothing proof.

This is a supporting production/tooling plan for the current **Windows-validated / Android-device-validated** showcase. It does not expand the active-enemy limit or replace the bounded route with a broader game.

## Priority order

## Implemented foundation - 2026-07-17

- Priorities 1 and 2 now have shared named checkpoints, deterministic encounter presets, fixed three-window measurement, strict ASTC/honest-presentation gates, and native state evidence.
- Priority 3 now has a deterministic 13-waypoint Android route replay for movement, collision, zone, lantern, and enemy-update regression coverage. Full raw touch/look/swing/pause input recording remains deferred because ADB gesture timing is refresh-rate dependent; hands-on input validation remains required.
- The first bounded part of Priority 5 is available as `tools/run-android-showcase-validation.ps1`, producing a timestamped Android evidence bundle. Cross-platform packaging, shader-staleness, licence, and clean-package gates remain future work.
- Priority 4 now has a shared compact formatter and Debug-only live overlay. Windows F3 is live-validated without pausing or intercepting input; Android Debug/Release and Release lint pass, but the Android device gate remains pending.
- The engineering checkpoint surface remains Debug-only. A separate release-safe two-pass in-app benchmark now reuses the shared route and produces player-exportable reports without exposing checkpoint controls.

### 1. Deterministic showcase mode — complete

- Twelve named checkpoints now cover Opening, Skeleton, Lantern Drop, Skylight, each coloured-light bay, Mirror, Lich, and Finale Roof.
- Lighting, enemy, lantern, roof, and encounter states are reproducibly selectable in Android Debug and host-tested shared code.
- Keep normal player traversal intact; checkpoints are for development, validation, and presentation.

### 2. Repeatable measurement — engineering harness and player-facing Windows flow complete

- A fixed 13-waypoint route and default five-checkpoint benchmark cover the important showcase cost centres.
- Evidence records render scale, internal resolution, frame time, derived FPS, thermal/battery state, RT mode, active skinned-enemy count, and presentation status.
- Preserve separate 100% and recommended 75% Android results.
- The in-app benchmark runs one visible warm-up lap and one measured lap, aggregates overall and per-zone timing, requires honest RT presentation, and emits selectable text plus JSON. Windows Release is live-validated; Android device lifecycle/export validation is pending.

### 3. Reproducible input — partial

- Native deterministic movement/collision replay is complete. Raw Android touch and Windows keyboard/mouse/look/swing/pause recording remains deferred because wall-clock input injection is refresh-rate dependent.
- Mark replay results as automated evidence; retain hands-on verification where the device cannot expose a state directly.

### 4. Developer visibility — Windows complete; Android device validation pending

- The shared formatter reports build/shader identity, GPU/API, RT mode, route zone, render scale/extents/timing, BLAS/TLAS/instance counts, material encoding, lantern/enemy state, active skinned count, and honest RT presentation.
- Windows Debug toggles it with F3 without pausing; F2 remains the full paused diagnostic surface. The compact control is disabled for input and hides during entry, pause, settings, and diagnostics.
- Android Debug has a hidden non-interactive view, `horde.debug.overlay` intent, long-press status toggle, and a native 4 Hz snapshot publisher. Both APK build types and Release lint pass; touch pass-through, font scale 1.7, thermal timing, audio/lifecycle interaction, and visual approval remain a connected-phone gate.
- Windows Release omits the developer UI and Android Release returns no developer overlay text. Keep technical output hidden from branded player-facing surfaces unless requested or startup fails.

### 5. One-command validation and packaging — Android evidence runner complete; integrated package gate pending

- Provide a command path that builds Windows and Android, runs CTests/smoke tests, regenerates embedded raygen SPIR-V when needed, packages assets, and collects a timestamped report bundle.
- Add clean-package smoke checks for executable-relative Windows assets, strict Android ASTC contents, signing state, hashes, and required licence files.
- Fail clearly on stale shaders, missing assets, unsupported capabilities, or invalid package layout.

### 6. Capture and presentation — fixed screenshots partial; video/orbit pending

- Capture fixed screenshots and short videos for the opening, shadow corridor, skylight, coloured lights, mirror, combat, and finale.
- Add a presentation camera/orbit option for showing the player body, held props, reflections, and RT lighting without changing the first-person game path.
- Add optional render-scale comparison views using the real renderer and clearly labelled diagnostics.

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
