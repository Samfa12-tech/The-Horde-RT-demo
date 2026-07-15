# Horde Lantern RT - initial showing alpha 0.1.0

## Release identity

- Public title: **Horde Lantern RT**.
- Release label: **Initial Showing Alpha 0.1.0** (`0.1.0-alpha.1` package version).
- Distribution: Android APK and portable Windows x64 zip are hosted on itch at `https://samfa12.itch.io/the-horde` and linked from the rendered Samfa12.com `/games/` catalogue.
- Scope: honest torch-corridor and material-gallery alpha, not the completed 60-90 second mirror/glass/final-crypt route.

## Player-facing features

- Branded entry and pause flow on Android and Windows.
- Resume, restart route, controls, settings, RT diagnostics, and quit actions.
- Honest `Native Vulkan hardware RT active` status only after an RT frame reaches successful swapchain presentation.
- Unsupported/error diagnostics with no fallback renderer.
- Persistent look sensitivity, 50-100% RT render-resolution slider (100% default), sound-effect state/level where supported, compact-HUD control on Android, and windowed/fullscreen control on Windows.
- FilmCow UI, sword, impact/fall, quiet player/skeleton footsteps, and skeleton attack sound effects.
- Android two-zone touch movement/look plus a large `SWING` action.
- Android native libraries and APK packaging are verified for 16 KiB page alignment; the r26 shared C++ runtime is not packaged.
- Android diagnostics publish internal resolution, FPS, and frame time after the first 30-frame sample, then use steadier 120-frame updates.
- Windows WASD, left-drag look, right-click/Space attack, Esc pause, R reset, F1 controls, F2 diagnostics, and Alt+Enter fullscreen.

## Technical proof in this alpha

- Native Vulkan hardware RT on both targets.
- BLAS/TLAS, ray-tracing pipeline, SBT, `vkCmdTraceRaysKHR`, RT storage output, and swapchain presentation.
- Phone-safe ray-query work inside raygen with recursion depth 1.
- ASTC material arrays on Android and raw RGBA8 arrays on Windows.
- One animated/refitted skeleton plus camera-relative player body, torch, and sword RT instances.
- A closed starting chamber, a physically deep room-two arch, the skeleton staged behind it, and a bounded thin-transmission glass skylight.
- A phone-safe held torch with solid shaft, iron cage, and two layered emissive flame volumes.

## Known limitations

- Hardware support is intentionally narrow and diagnosed explicitly.
- The alpha contains one bounded thin glass skylight and mirror-routing proof, not a general thick-dielectric/material system or the completed showcase route.
- The single-skeleton combat loop is a presentation beat, not a broader game.
- The stable-key-signed Android APK passed final `SM-S948B` portrait, RT-presentation, diagnostics, render-scale, and SoundPool validation and is published on the itch `android` channel.
- The Windows zip is published on the itch `windows-x64` channel; the clean-extraction package passed RT presentation, portable-asset, diagnostics, and 100%/75% render-scale checks.
- The enemy is a modified Hotstrike Studio Free Stylized Skeleton. The supplied derivative was textured, rigged, and animated with Meshy; full source, licence, and attribution details are in `ASSET_LICENSES.md`.
- The release keystore and both passwords must be backed up independently; losing them prevents compatible Android updates.
