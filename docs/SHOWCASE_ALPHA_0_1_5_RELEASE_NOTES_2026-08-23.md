# Horde Lantern RT - Showcase Alpha 0.1.5

Package version: `0.1.5-alpha.1`
Android version code: `6`

## What changed

- Replaced the opaque waterfall prototype with three real, separated world-BLAS water streams, a rounded shallow catchment, a connected floor runnel, and a recessed barred drain.
- Added deterministic, light-aware RT water optics: IOR 1.333 refraction, Schlick Fresnel reflection, Beer-Lambert absorption, animated geometry-bound flow normals, impact highlights, and bounded non-recursive secondary shading.
- Added switchable Android water quality: Mobile by default, High for the bounded reflection query, and Off for primary water intersections without changing the native RT presentation path.
- Reframed the existing automatic lantern-drop beat as a roof-water drench while preserving its checkpoint, 0.70-second gutter, 1.15-second settle, traversal, and collision behavior.
- Added low, deterministic blue-grey ritual ground mist around the lich without obscuring the sword, silhouette, or roof/dawn finale.
- Added a licensed positional waterfall loop by DRAGON-STUDIO via Pixabay, with world-space attenuation and stereo pan on Windows and Android.
- Added shared collision-safe directional dodge behavior and complete Windows controller support for the tested Backbone One: both sticks, RT attack, LT parry, B/Circle dodge, D-pad menu navigation, A select, Menu/Start pause, focus outlines, and controller render-scale adjustment.
- Corrected Windows mouse capture and continuous controller-look ownership across render frames.

## Rendering and compatibility

- Native `vkCmdTraceRaysKHR` swapchain presentation, phone-safe `rayQueryEXT` shading, recursion depth one, strict Android ASTC, one frame in flight, nine BLAS, and nineteen TLAS slots remain intact.
- Water is real geometry and RT shading, not a raster fallback, screen-space effect, particle curtain, or fake overlay.
- Android remains intended for explicitly validated Vulkan hardware-RT phones; unsupported devices receive diagnostics instead of a fallback renderer.
- Windows requires a Vulkan hardware-ray-tracing GPU and current driver.
- On the exact `SM-S948B` Debug candidate at 75% RT scale, median CPU-present-loop measurements were 12.541 ms at `lantern-drop`, 11.363 ms at `skylight`, and 20.457 ms at `lich`. These are device-specific observations, not universal frame-rate promises.

## Evidence boundary

- The owner accepted the exact Windows water/mist/controller candidate in hands-on play.
- The exact Android Debug candidate passed strict ASTC, honest RT presentation, focused measurements, the 13-waypoint replay, all 13 captures, and Home/resume. The public signed APK was subsequently installed and pulled back byte-for-byte on `SM-S948B`, with strict ASTC and honest presentation across Home/resume.
- `Audio/haptic manual revalidation required: YES - PASSED` because this release adds a positional loop and dodge changes listener position. The owner approved the exact Windows candidate, then confirmed waterfall audio, haptics, and pause/resume on the exact installed Android release; no haptic cue or routing changed.

Published itch builds are Windows `#1908330` and Android `#1908331`. See `SHOWCASE_ALPHA_0_1_5_RELEASE_VALIDATION_2026-08-23.md` for exact hashes and evidence, and `ASSET_LICENSES.md` for asset provenance and licence terms.
