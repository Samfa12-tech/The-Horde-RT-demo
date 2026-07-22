# Horde Lantern RT - showcase alpha 0.1.2

Release date: 2026-07-22
Package version: `0.1.2-alpha.1`
Android version code: `3`

## What changed

- Added the release-safe two-pass benchmark on Windows and Android. It runs the deterministic 13-waypoint route, keeps honest RT-presentation evidence, and produces selectable/exportable reports.
- Added `More by Samfa12` to the player menu and kept the detailed developer telemetry surfaces Debug-only.
- Added deterministic scene-only capture of all 12 shared showcase checkpoints on Windows and Android for repeatable visual regression evidence.
- Added one integrated host/device foundation gate covering fresh builds, tests, shader staleness, package layout, strict Android ASTC, licences, hashes, lifecycle evidence, and validation-only artifacts.
- Removed an unreachable stained-glass material branch while preserving the live clear-glass route and open architectural threshold. SPIR-V size/control flow improved, all 12 Windows A/B captures were bit-exact, and comparable-temperature phone timing did not regress.
- Expanded the Android compatibility record so confirmed local evidence, user reports, and hardware-only predictions remain clearly separated.

## Validation

- Fresh Windows Debug and Release builds pass all seven host CTests.
- The packaged Windows executable uses the real Vulkan RT pipeline, `vkCmdTraceRaysKHR`, executable-relative assets, Per-Monitor V2 DPI, and the established BGRA colour route.
- Android Debug/Release builds and Release lint pass. The release retains strict ASTC materials, four native ABIs, 16 KiB APK/ELF alignment, a static C++ runtime, and the established update-signing certificate.
- The complete deterministic 75% checkpoint/replay/capture/lifecycle gate passed on Samsung `SM-S948B`; 100% remains a separately reported image/extent result rather than the sustained recommendation.
- The current one-active-skinned-enemy limit and honest `rtScene.presented` semantics are unchanged.

## Known alpha limits

- Only one skinned enemy is rendered, animated, and refit at once. The skeleton and lich remain sequential route encounters.
- The lich is a placeholder whose shared robe/staff biped rig retains visible deformation limits.
- The current sword, lantern, body, flames, and electricity use compact procedural geometry selected for the measured RT path.
- Water, simultaneous hordes, broader enemy AI, blocking/dodging, and the staged textured sword remain deferred.
- Hardware RT is required. Unsupported devices receive diagnostics; there is no raster, baked, screen-space, browser, or fake-RT fallback.

## Credits and licences

The Windows archive includes `ASSET_LICENSES.md`, and both applications expose credits in-app. The release includes five Poly Haven CC0 material sets, the Hotstrike Studio skeleton derivative with conservative Meshy CC BY 4.0 attribution, the CC0 Meshy placeholder lich, and seventeen FilmCow sound cues under FilmCow's custom royalty-free project-use licence.
