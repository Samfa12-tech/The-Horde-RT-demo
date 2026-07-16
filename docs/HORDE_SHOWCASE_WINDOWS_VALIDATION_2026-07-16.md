# Horde Showcase Route - Windows-First Validation - 2026-07-16

## Status

**Windows-validated / Android device validation pending.**

This goal is deliberately not a signed release and has not been uploaded. Android validation in this document means compilation only unless explicitly stated otherwise.

## Implemented route

The native hardware-ray-traced route now plays in this order:

`skeleton -> lantern failure -> blue skylight -> coloured torches -> open threshold -> hero mirror -> floating staff-lit lich -> sliding finale skylight`

- Slice A geometry, collision, skeleton leash, reset, brackets, frames, warm BGRA output, and honest RT swapchain presentation are retained.
- The player has a low-poly leather torso, pelvis, articulated limbs, boots, neck/head silhouette, and compact deterministic gait. The head uses reflection/shadow mask `0x10`: it appears in the hero mirror and visibility rays but never blocks first-person primary rays. Held hands/props sample the shared route union and retract before their BLAS geometry crosses a wall.
- The reset-only lantern sequence gutters for 0.70 s, releases into a 0.45 s authored fall, is settled by 1.15 s, and has the left arm lowered by 1.20 s. Visible flame emission and its analytic direct-light strength are zero after release.
- The settled torch remains visibly above the floor and is clamped to a prop-safe inset of the final zig-zag leg, preventing a wall-facing trigger from burying it in masonry.
- Skeleton/lich state is plural and capacity-configurable, while the current measured limit selects, animates, refits, and renders one skinned enemy at a time. Crossing into the skylight side selects/resets the lich; returning to the shadow corridor selects/resets the skeleton.
- Four physical bay flames use yellow, blue, deep red, and restrained green. Only the current five-metre bay contributes the analytic local light.
- The skylight chamber uses a bounded desaturated-blue aperture estimate with one of two deterministic aperture samples and one visibility query.
- The rejected stained pane and its BLAS/transmission tint were removed after hands-on review; the architectural threshold remains open.
- The existing hero frame contains a one-bounce mirror that reflects the world, props, complete player, selected enemy, and dynamic roof while shading its bounce from the current active light instead of a constant studio-like ambient term.
- The finale ceiling has a physical aperture closed by a separate stone-panel BLAS on mask `0x20`. After the complete lich death clip, it slides west over 4.5 seconds; primary and visibility rays see the opening grow and a bounded cool aperture light sweeps into the room.
- Water remains deferred.

## Lich placeholder

- Source: user-supplied fixed-hash GLB, 9,188 triangles, one 24-joint biped skin, nine clips.
- Runtime mapping: living phases retain the safer continuous `Idle_02` while whole-instance hover/orbit supplies locomotion; `Dead` is non-looping. The visibly distorted `Walking` clip is no longer selected at runtime, and the absent cast clip remains honestly unmapped.
- Animation and BLAS refit run at 30 Hz. The Windows Debug `F5` selector can swap the active skinned character for clip/refit inspection; `F6` moves to the finale, `F7` moves to the lantern-transition inspection point, `F8` cycles deterministic validation views for the opening, worst bend, skylight, far torch passage, and finale, and `F9` inspects the settled lantern without resetting its sequence.
- A separate 48-byte skinned vertex ABI retains UVs without changing the skeleton's established 32-byte stream.
- Windows samples raw RGBA8 KTX2 base/emissive textures. Android packages strict ASTC 6x6 KTX2 assets and has no uncompressed lich fallback.
- Meshy's duplicated whole-surface emissive image is ignored. A deterministic derived violet mask retains staff-crystal and eye/gem candidates. Forty UV-audited emissive staff vertices drive the analytic staff-light position through their actual animated skin weights; no missing staff bone is assumed.
- Charge timing is 1.20 s from 0.55x to 2.2x torch strength, with route-obstruction/range-tested damage at the peak and 1.8 s recovery. The caster maintains a bounded 3-5 m preference with lateral hover movement through all living phases. It requires exactly three accepted hits within 2 m with a 2.0 s lockout between hits. Every accepted hit produces a 0.38 s rigid recoil/lean and an audible positional FilmCow body-hit cry; the third starts the full 2.967 s `Dead` clip, whose final pose is held while the roof opens.
- Known placeholder faults remain: the staff and robe are fused into the biped mesh, there is no rigid staff bone or cloth rig, and some clips visibly distort them.

## Spatial audio

- Shared emitters/listeners use equal-power stereo pan, inverse-distance rolloff, and route-obstacle attenuation.
- Windows uses per-voice XAudio2 output matrices with WinMM fallback. Skeleton and lich cues remain positional; UI, sword, and player steps remain centred.
- Two alternating player steps are driven by actual post-collision travel: the first contact occurs after 0.24 m and repeats every 0.72 m, so wall-hammering does not create phantom steps. Player and skeleton dirt contacts are deterministically normalized to 0.78 peak before their centred/spatial runtime gains. The lich adds bounded charge, impact, fall, and body-hit cry cues from the same approved FilmCow archive; the short cry is normalized to 0.84 peak and is not masked by a duplicate fencing impact.
- Android publishes left/right gains to SoundPool and compiles successfully, but device directionality and distance behavior are unvalidated.

## Automated validation

| Check | Result |
|---|---|
| Windows Debug build | Passed |
| Windows Debug CTest | 5/5 passed |
| Windows Release build | Passed |
| Windows Release CTest | 5/5 passed |
| Route/collision smoke | Passed continuous zones, three bends, sliding, shortcut rejection, old-wall rejection, gallery/posts |
| Gameplay smoke | Passed lantern timing/reset, lower pose, route-light mapping, plural enemy gates, lich LOS/range/charge/recovery, three-hit/2 s lockout, complete death retention, and 4.5 s finale-skylight progression |
| Route/collision smoke | Also passes held-prop full reach in open space and retraction at end/side walls |
| Spatial-audio smoke | Passed pan, distance, centring, maximum range, obstruction, timed enemy cadence, and post-collision travel cadence/reset |
| Skinned loader smoke | Passed skeleton and lich clip/UV import, fixed vertex counts, 32/48-byte ABIs, derived staff-emission UV audit |
| Android `assembleDebug` | Passed all configured ABIs; no install/device claim |

The closing code review additionally verified release-active test failures, route-union audio obstruction across the zig-zag, a frozen lantern release pose, matching held-light/prop retraction depth, a recoil-following lich staff sample, corrected dynamic-roof face normals, distinct player-head material classification, and complete acceleration-structure/shader visibility barriers. Per-frame diagnostic file writes were removed from the render/audio submission paths after acceptance.

## Shader artifact

Generated with `tools/compile-raygen.ps1` after the final shader edit:

- SPIR-V bytes: 71,908
- SPIR-V words: 17,977
- SHA-256: `235755255B8DB0736DD83FDFCD9EEC7B95BBA3B777C0CE9E92A0629B245ACCAC`
- `minimal.rgen` SHA-256: `8A499E5C3AC4B871223EF9EAC70FFDFADD9F11D84255CE28094C9F96330361B2`
- embedded `MinimalRayGenShader.inc` SHA-256: `96A22D786E6998043C7A2EC07A89078DB3F718A628D9322702E3D74A23DE2D79`

The runtime still dispatches `vkCmdTraceRaysKHR`, performs the phone-safe work with `rayQueryEXT`, keeps pipeline recursion depth 1 and one frame in flight, preserves presentation-format BGRA correction, and only marks `rtScene.presented` after successful swapchain presentation.

## Windows live evidence

The RTX 5050 Laptop GPU launched the Debug implementation through the native RT pipeline. The opening remained warm, the finale showed the textured floating lich under a violet visibility-tested staff light, and the hero mirror reflected the finale/player composition. The laptop's internal approximately 165 FPS limit is treated as a cap, not a renderer ceiling.

The build records every 120-frame window to `reports/windows_showcase_timing.csv` with render scale, route zone, median, p95, average, median-derived FPS, and a 165-cap flag. The unlocked rerun captured at least six windows in every requested view at both scales. Representative final consecutive triples are below; every 75% median is far below the required 20 ms. The roughly 5.7-6.0 ms medians are explicitly treated as laptop-cap-bound rather than proof of an uncapped renderer ceiling.

| Scale | View / reported zone | Consecutive median ms | Consecutive p95 ms | 165-cap flag |
|---:|---|---|---|---|
| 75% | Existing chamber / opening | 5.952 / 5.973 / 5.970 | 6.079 / 6.181 / 6.148 | yes / yes / yes |
| 75% | Worst bend / shadow corridor | 5.897 / 5.947 / 5.955 | 6.197 / 6.077 / 6.086 | yes / yes / yes |
| 75% | Lantern-off blue skylight | 5.982 / 5.994 / 5.979 | 6.046 / 6.040 / 6.048 | yes / yes / yes |
| 75% | Far passage / green bay | 6.002 / 5.991 / 5.976 | 6.056 / 6.049 / 6.055 | yes / yes / yes |
| 75% | Final room / active lich | 5.995 / 5.972 / 5.975 | 7.323 / 7.325 / 7.269 | yes / yes / yes |
| 100% | Existing chamber / opening | 5.726 / 5.694 / 5.862 | 6.373 / 6.254 / 6.310 | yes / yes / yes |
| 100% | Worst bend / shadow corridor | 5.927 / 5.979 / 5.941 | 6.197 / 6.204 / 6.146 | yes / yes / yes |
| 100% | Skylight chamber | 5.990 / 6.007 / 6.005 | 6.061 / 6.060 / 6.056 | yes / yes / yes |
| 100% | Far passage / green bay | 5.974 / 5.979 / 5.981 | 6.056 / 6.066 / 6.066 | yes / yes / yes |
| 100% | Final room / active lich | 5.930 / 5.968 / 5.954 | 7.250 / 7.109 / 7.698 | yes / yes / yes |

The image pass confirmed warm BGRA presentation; the opening skeleton and route geometry; bend occlusion; lantern-off desaturated-blue skylight without the former unconditional warm haze; four physical bay flames and selected yellow/blue/deep-red/restrained-green illumination; the open threshold; the light-aware hero mirror reflecting complete player/lich geometry; and the textured, floating, violet staff-lit lich finale. Hands-on review also accepted the lantern drop, wet floor, mirror composition, corrected corridor geometry, removed pane, post-drop sword swing, wall-aware prop retraction, visible staff/eyes/electricity, reflection-only head fix with the chest visible when looking down, and the moving finale roof/light sweep.

The final 100% user-driven run then produced one continuous report sequence through opening, skeleton room, shadow corridor, skylight chamber, all four individually named torch bays, open threshold, and finale. Its latest-process window counts were opening 27, skeleton room 3, shadow corridor 86, skylight chamber 13, yellow/blue/red bays 5/4/4, green bay 14, transmission threshold 6, and finale 25. This proves a natural route traversal rather than only Debug teleport coverage. The raw evidence is retained at `docs/validation/horde-showcase-windows-2026-07-16/windows_showcase_timing.csv`.

Final hands-on acceptance confirmed audible player and skeleton footsteps, visible and audible feedback on every accepted lich hit, exactly three hits with the two-second lockout, lich death playback, wall-aware prop retraction, the sliding post-death roof and incoming light, and the light-aware mirror. F10 independently proved the centred step asset/output path while temporary event/voice diagnostics verified movement and spatial submissions through XAudio2; those per-event file writes were removed during the closing review.

## Deferred phone gate

When `SM-S948B` access returns:

1. Install the debug APK and confirm strict lich/environment ASTC selection plus honest RT swapchain presentation.
2. Walk and wall-hammer every segment, all corners, both enemy gates, reset, pause/resume, lantern transition, open threshold, mirror, prop retraction, three-hit fight, and moving finale roof.
3. Review lower-body crop/gait, lantern fall, head shadows, derived lich emissive mask, staff/robe deformation, normalized player/skeleton steps, audio panning/distance/obstruction, and hit feedback.
4. Capture the planned 75% sustained per-zone thermal windows and report 100% separately without imposing a 50 FPS requirement.
5. Raise the active skinned-enemy count above one only after a separate multi-enemy phone measurement.

Before any public distribution of the placeholder lich, retain the original Meshy CC0 source page or licence screenshot in the project record.

The compile-only Android artifact is `android/app/build/outputs/apk/debug/app-debug.apk`, 58,887,407 bytes, SHA-256 `44D49F9F30BEED08A1B579546D32845B1BF4E8916816A85B58FA39A59D536273`. Its APK contains the lich GLB, normalized/new FilmCow cues, and only the strict ASTC lich derivatives; the raw Windows KTX2 files are not packaged into Android.
