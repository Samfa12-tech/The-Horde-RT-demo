# Horde Lantern RT - showcase alpha 0.1.1

Release date: 2026-07-17
Package version: `0.1.1-alpha.1`
Android version code: `2`

## What changed

- Extended the original chamber into the complete walkable showcase route: skeleton encounter, three-turn shadow corridor, blue skylight chamber, four coloured-light bays, open framed threshold, hero mirror, and final lich room.
- Added the deterministic lantern gutter/drop sequence, complete first-person body, procedural gait, wall-aware prop retraction, sword combat, and player/skeleton footsteps.
- Added one bay-selected local light at a time in yellow, blue, deep red, and restrained green, plus moving lantern shadows and a post-defeat sliding roof aperture.
- Added a single-bounce hero mirror whose reflected lighting follows the live scene.
- Added the CC0 Meshy placeholder lich with emissive eyes and staff, visible charge electricity, spatial attack/hurt/death sounds, recoil, three-hit defeat with a two-second hit lockout, and a death animation.
- Removed the rejected stained pane while retaining the open architectural threshold.
- Preserved real `vkCmdTraceRaysKHR` swapchain presentation and the phone-safe `rayQueryEXT` raygen path.

## Validation

- Windows Debug and Release builds and all five host CTests pass.
- The complete route, collision, reset, audio, mirror, combat, lighting transitions, and BGRA-correct presentation were walked and approved on Windows RTX.
- The complete route was also walked on Samsung `SM-S948B` with strict ASTC assets and honest RT presentation. The user reported no visual or interaction issues and described it as performant.
- At 75% RT resolution, every required warm phone zone retained a median of three consecutive 120-frame average windows below 13.7 ms at thermal status 3. The 100% pass verified full `1440x2980` RT extent and image correctness without imposing a 50 FPS requirement.

## Known alpha limits

- Only one skinned enemy is rendered, animated, and refit at once. The skeleton and lich are selected sequentially by route zone.
- The lich is a placeholder. Its robe and staff share a Meshy biped rig, so some deformation remains visibly imperfect.
- The current sword, lantern, body, flames, and electricity use compact procedural geometry chosen for the measured RT path.
- Water, simultaneous hordes, broader enemy AI, blocking/dodging, and the staged textured sword are deferred.
- Hardware RT is required. Unsupported devices receive diagnostics; there is no raster fallback.

## Credits and licences

The Windows archive includes `ASSET_LICENSES.md`, and both applications expose credits in-app. The release includes five Poly Haven CC0 material sets, the Hotstrike Studio skeleton derivative with conservative Meshy CC BY 4.0 attribution, the CC0 Meshy placeholder lich, and seventeen FilmCow sound cues under FilmCow's custom royalty-free project-use licence.
