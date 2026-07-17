# RTXPT 1.8.1 Reference and Regression Gates

## Purpose

This is a reference note for future Horde Lantern RT work inspired by NVIDIA RTXPT v1.8.1. It is guidance, not a request to import RTXPT or replace the current renderer. Any change justified by this note must improve at least one of:

- measured performance;
- visible image quality or stability; or
- build, test, profiling, capture, or packaging workflow.

No change is accepted if it causes a graphical regression, weakens the real-RT proof, hides an unsupported-device failure, or breaks the Android phone path.

RTXPT reports a 25-30%+ performance improvement over its previous release, shader-specialized performance/quality presets, improved denoising stability, reduced memory use, and automated parameterized screenshot support. Those results are desktop sample results, not Horde targets. The transferable lesson is the optimization method: reduce shader cost, specialize deliberately, measure continuously, and preserve visual evidence.

Reference: <https://github.com/NVIDIA-RTX/RTXPT/releases/tag/v.1.8.1>

## Applicable renderer lessons

### 1. Reduce register pressure and divergence

RTXPT attributes a significant part of its gain to reducing register spills and divergent execution. For Horde, inspect `shaders/raytracing/minimal.rgen` before adding more features:

- keep ray-query state and material data compact;
- shorten the lifetime of temporary values;
- avoid carrying optional lighting/effect data through every path;
- make bounded loops and feature branches easy for the compiler to eliminate;
- profile Windows shader changes, then gate them on the Android device.

The phone-safe `rayQueryEXT` path remains the default. Do not trade it for recursive closest-hit tracing without a new phone capability and pipeline-creation proof.

### 2. Prefer measured shader variants over a universal expensive path

RTXPT uses settings that can be baked into shaders so unused loops and features disappear. Horde may use a small number of explicitly measured variants for:

- render scale;
- primary/visibility ray budget;
- bounded bounce or transmission work;
- fog/atmosphere quality;
- optional mirror or secondary-light quality.

Variants must have stable names, recorded compile inputs, and a clear runtime diagnostic. Avoid a large permutation matrix. The default variant must remain the current visually accepted 75% phone route.

### 3. Improve light selection before adding more lights

RTXPT invests in next-event estimation, adaptive importance sampling, and light-sampling caches. The Horde equivalent is bounded and authored: prioritize the held/dropped lantern, current torch bay, skylight aperture, mirror-relevant light, and active lich staff light. Do not broaden to many dynamic lights until a phone measurement shows the current route has margin.

Any sampling change must be checked for:

- warm fire remaining warm rather than cyan on BGRA swapchains;
- coloured bays retaining their intended hue separation;
- lantern-off zones remaining lantern-off;
- mirror and visibility behaviour remaining physically consistent;
- no new flicker, fireflies, leaks, or unstable temporal noise.

### 4. Treat material complexity as an explicit budget

RTXPT supports multiple material permutations. Horde should continue using a compact material route, adding a new class only when it produces visible value or removes measurable cost. New material paths must include an asset/texture test and must not silently fall back to an untextured or raster-only substitute.

### 5. Preserve temporal stability when improving quality

RTXPT's denoising work highlights motion-vector and history stability for reflections and refractions. If Horde adds temporal accumulation, filtering, or history-dependent lighting, test camera motion, lantern motion, enemy refit, mirror motion, and the moving finale roof. A sharper still frame is not an improvement if it produces ghosting, trails, flicker, or unstable glossy stone during play.

## Non-applicable RTXPT features

- Do not import the RTXPT/Donut engine or its desktop sample architecture.
- Do not make DirectX SER or Opacity Micromaps a phone requirement. RTXPT documents Vulkan SER/OMM as work in progress and its release feature is tied to modern DXR/Agility support.
- Do not use RTXPT's desktop VRAM or 4K results as Android budgets.
- Do not add DLSS, DLSS-RR, or other vendor-specific quality dependencies to the Android acceptance path.

## No-graphical-regression acceptance gate

Every meaningful renderer, shader, asset-format, or lighting change must pass this order:

1. **Build integrity**: regenerate embedded raygen SPIR-V after shader edits; run the relevant Windows Debug/Release build and CTests; build Android Debug; reject stale shader or missing-asset states.
2. **RT honesty**: confirm the selected RT capability, actual `vkCmdTraceRaysKHR` dispatch, successful RT-produced swapchain presentation, and correct `outputRedBlueSwap` handling.
3. **Phone performance**: run the Android showcase validation at the persisted 75% scale, using the existing named checkpoints and three-window measurements. Report 100% separately; do not replace the 75% gate with a desktop result.
4. **Visual checkpoint comparison**: review fixed evidence for opening, worst bend, lantern-off skylight, yellow/blue/red/green bays, open threshold, mirror, active lich, and finale roof. Compare against the accepted baseline images and route video where available.
5. **Hands-on quality pass**: walk, turn, wall-retract the held prop, trigger lantern failure, inspect the mirror, complete the three-hit lich encounter, pause/resume, background/resume, and listen for the relevant audio cues.
6. **Decision**: accept only when performance is improved or unchanged within measurement noise and every required visual/behavioural checkpoint is equal or better. If a visual difference is intentional, record the before/after evidence and reason in a validation document before merging.

## Performance evidence rules

- Keep the current Android 75% sustained route as the primary gate.
- Retain three consecutive 120-frame window values, the median-of-window-averages, derived FPS, thermal status, device, render scale, shader hash, and build identity.
- Treat the current phone route and one-active-skinned-enemy limit as the baseline. Do not spend its margin on broader AI, a second active skinned enemy, or unrelated effects.
- Treat Windows timing as useful for diagnosis, not proof of phone readiness. Note frame caps explicitly.
- Prefer a small isolated change with before/after evidence over a broad renderer rewrite.

## Workflow improvements worth pursuing

The safest workflow improvements are those that make regressions harder to ship:

- one integrated clean-build/package/stale-shader/licence gate for Windows and Android;
- a developer-only overlay or state dump for route zone, render scale, RT mode, presentation status, selected enemy, lantern state, shader/build identity, and frame timing;
- fixed screenshot capture at the existing route checkpoints;
- parameterized replay/validation commands with timestamped evidence bundles;
- automatic comparison of required evidence presence and shader/artifact hashes;
- a short release checklist that confirms assets, licenses, package layout, and the preserved stable signing identity.

Developer visibility must stay hidden from branded entry, pause, settings, and normal player-facing surfaces unless diagnostics are requested or startup fails.

## Suggested next work slice

The next safe technical slice is a raygen cost audit, not a feature expansion:

1. record the current shader/build hashes and Android 75% baseline;
2. inspect register pressure, divergence, and branch/loop cost on Windows;
3. make one small shader simplification or compile-time specialization;
4. regenerate SPIR-V and run the integrated build/stale-shader checks;
5. rerun the Android showcase gate and fixed visual checkpoints;
6. keep the change only if the image is equal or better and the measured phone result is equal or better.
