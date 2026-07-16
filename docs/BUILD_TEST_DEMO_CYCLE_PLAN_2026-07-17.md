# Build / Test / Demo Cycle Plan

## Purpose

Make every meaningful renderer or gameplay change easy to build, measure, reproduce, and present on both Android and Windows without weakening the RT-or-nothing proof.

This is a supporting production/tooling plan for the current **Windows-validated / Android-device-validated** showcase. It does not expand the active-enemy limit or replace the bounded route with a broader game.

## Priority order

### 1. Deterministic showcase mode

- Add named checkpoints for Opening, Skeleton, Lantern Drop, Skylight, each coloured-light bay, Mirror, Lich, and Finale Roof.
- Make lighting, enemy, lantern, roof, and encounter states reproducibly selectable.
- Keep normal player traversal intact; checkpoints are for development, validation, and presentation.

### 2. Repeatable measurement

- Add a fixed benchmark route covering the named showcase zones.
- Record render scale, internal resolution, frame time, FPS, thermal state where available, RT mode, active skinned-enemy count, and presentation status.
- Preserve separate 100% and recommended 75% Android results.

### 3. Reproducible input

- Record and replay Android touch, Windows keyboard/mouse, movement, look, swing, pause, and reset inputs.
- Mark replay results as automated evidence; retain hands-on verification where the device cannot expose a state directly.

### 4. Developer visibility

- Add a toggleable in-game overlay for GPU/API, RT mode, route zone, render scale, frame timing, BLAS/TLAS counts, ray/material diagnostics, enemy state, and honest RT presentation.
- Keep technical output hidden from the branded player-facing surfaces unless requested or startup fails.

### 5. One-command validation and packaging

- Provide a command path that builds Windows and Android, runs CTests/smoke tests, regenerates embedded raygen SPIR-V when needed, packages assets, and collects a timestamped report bundle.
- Add clean-package smoke checks for executable-relative Windows assets, strict Android ASTC contents, signing state, hashes, and required licence files.
- Fail clearly on stale shaders, missing assets, unsupported capabilities, or invalid package layout.

### 6. Capture and presentation

- Capture fixed screenshots and short videos for the opening, shadow corridor, skylight, coloured lights, mirror, combat, and finale.
- Add a presentation camera/orbit option for showing the player body, held props, reflections, and RT lighting without changing the first-person game path.
- Add optional render-scale comparison views using the real renderer and clearly labelled diagnostics.

## Game-facing support after the tooling foundation

- Improve combat readability with clear sword impact, recoil, hit pause, and encounter-state feedback.
- Add independent scenario reset controls for lantern, enemy, lighting bay, roof, player position, and encounter state.
- Preserve large Android accessibility font-scale support and compact HUD behaviour.

## Guardrails

- Keep real `vkCmdTraceRaysKHR` swapchain presentation and honest `rtScene.presented` reporting.
- Keep the phone-safe `rayQueryEXT` path unless a stronger RT path is proven on-device.
- Keep one active skinned enemy until a separate multi-enemy phone measurement passes.
- Measure each meaningful renderer change on the phone at the recommended tier before expanding effects or gameplay.
- Do not add fake raster, screen-space, baked, browser, or overlay substitutes for RT proof.

## Completion signal

The cycle is working when a change can be built from a clean checkout, exercised through the same named route or replay, measured on both targets, captured at fixed presentation points, and handed off with a compact report and package-verification result.
