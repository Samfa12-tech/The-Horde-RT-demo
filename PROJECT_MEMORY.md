# Horde Lantern RT - Project Memory

Last updated: 2026-07-18

## Identity and release state

- Working/public title: **Horde Lantern RT**.
- Purpose: native Vulkan hardware-ray-tracing game/technology demo.
- Principle: **RT or nothing**; unsupported devices receive honest diagnostics, never a fake fallback.
- Primary target: Android phone. Equal validation target: Windows RTX.
- Current release: **Showcase Alpha 0.1.1**, package version `0.1.1-alpha.1`.
- Canonical downloads: https://samfa12.itch.io/the-horde. Samfa12.com links to itch rather than hosting a second copy; the live `/games/` card, itch link, GitHub link, thumbnail, and released status were rendered and verified on 2026-07-15.
- Source: https://github.com/Samfa12-tech/The-Horde-RT-demo.
- Windows itch channel: upload `#18339908`, build `#1801016`, `windows-x64`.
- Android itch channel: upload `#18341739`, build `#1801017`, `android`.
- Signed Android APK SHA-256: `ae73afec2c75b317187aeb61d81a592ec8bb4d8b5e89ef9b474fb2a60ae1354a`.
- Windows ZIP SHA-256: `8a254c9d14b35bf868f1cb96619dc572f3505a9564b668aa55241b33bfeaec2e`.
- Signing certificate SHA-256: `8245277a11bca5576f116724507f799d6f4c178ce5fbb7e3981415c9e6b3c245`.
- The release JKS lives outside Git. It and both passwords need an independent owner backup.
- Release proof: `docs/SHOWCASE_ALPHA_RELEASE_VALIDATION_2026-07-17.md`.

## Locked creative direction

- Historical gothic action demo: dark ruin, wet stone, fog, torch/lantern light, silhouettes, shadows, and obvious RT mood.
- Lighting and atmosphere come before broader combat.
- Keep the one-enemy loop bounded; do not add a second concurrent enemy, block/dodge, or broad AI without a later phone-measured plan.
- The authored coloured-light route is complete and published in Showcase Alpha 0.1.1; its design history is `docs/COLOURED_LIGHT_ROUTE_PLAN_2026-07-15.md` and its final evidence is in the Windows/Android/release validation records.

## Current renderer

- Android and Windows build BLAS/TLAS, RT pipeline/SBT, dispatch `vkCmdTraceRaysKHR`, write an RT storage image, and present it through the swapchain.
- `rtScene.presented` becomes true only after an RT-produced frame reaches successful presentation.
- The phone-safe path uses `rayQueryEXT` in raygen for primary, visibility, and bounded bounce/transmission work with pipeline recursion depth 1.
- A recursive depth-2 closest-hit experiment compiled but failed during phone pipeline creation. Do not restore it without proving capability and pipeline creation on the phone.
- The RT storage image is RGBA. Common BGRA swapchains require the presentation-format-driven `outputRedBlueSwap` path on raw copy; scaled modes use a format-aware blit.
- One frame remains in flight while held-prop TLAS transforms use host-written instance data.
- Android uses strict ASTC KTX2 arrays: ASTC 6x6 diffuse/ARM and ASTC 4x4 normals. Windows uses executable-relative raw RGBA8 arrays.

## Current alpha scene and controls

- The complete route is skeleton encounter -> three-turn moving-shadow corridor -> lantern failure/drop -> blue skylight -> yellow/blue/red/green bays -> open framed threshold -> light-aware hero mirror -> floating staff-lit lich -> post-death sliding roof.
- The rejected stained pane is not present. The threshold remains open; shallow water is deferred while the wet-floor proof remains live.
- The held and dropped lantern are RT geometry. Visible flame and direct-light contribution both reach zero after the authored fall; the old fullscreen overlay must not return.
- The complete low-poly player uses torso, articulated IK arms, pelvis/legs/boots, procedural gait, a head shadow/reflection instance, and wall-aware held-prop retraction.
- The procedural sword swings independently of the torch and the lich requires three accepted hits with a two-second lockout.
- The skeleton uses Hotstrike Studio's base asset processed with Meshy. The CC0 Meshy lich uses restrained `Idle_02`/`Dead` skinning plus whole-instance hover/orbit; its visibly distorted walking clip is deliberately not presented.
- One skinned enemy is animated/refit/rendered at a time: skeleton in the opening route, lich after the skylight gate.
- Android: left drag movement/strafe, right drag 360 look, Swing button, Android Back pause/resume.
- Windows: WASD, left-drag look, right mouse/Space swing, Esc pause/resume, R restart, F1 controls, F2 diagnostics, Debug-only F3 live developer overlay, Alt+Enter fullscreen.

## UI, settings, diagnostics, and audio

- Both platforms have branded entry, pause, controls, settings, RT diagnostics, restart, and quit flows.
- Both platform menus expose a release-safe two-pass benchmark: pass 1 warms the deterministic 13-waypoint route, pass 2 measures it, and completion produces a selectable/copyable/exportable text report plus automatically archived JSON evidence. Windows is live-validated; Android device validation waits for the phone.
- Both platform menus include `More by Samfa12`, which opens https://samfa12.com/ in the system browser.
- Technical output stays tucked away unless requested or startup fails.
- Both platforms persist a 50-100% RT render-resolution slider with 100% default.
- Android also persists SFX enable/volume, look sensitivity, and compact HUD.
- Windows persists SFX, sensitivity, display mode, and render scale beside the executable.
- Android diagnostics report internal resolution, FPS, frame time, dispatch resolution, and honest RT presentation.
- The shared Debug developer overlay reports build/shader identity, GPU/API, RT mode/presentation, render scale/extents/timing, route/lantern/enemy state, BLAS/TLAS/instance counts, active skinned count, and material route. Windows F3 is live-validated and non-pausing; Android Debug/Release compile, but the Android overlay still needs device touch/layout/performance/lifecycle validation.
- Seventeen FilmCow WAVs cover UI, sword, normalized alternating player/skeleton footsteps, skeleton attack, and lich charge/impact/fall/hurt reactions.
- Android uses SoundPool per-cue gain; Windows uses XAudio2 per-voice matrices with WinMM fallback.

## Validated Android release state

- Device: Samsung `SM-S948B`, Adreno 840, Vulkan 1.4.295, Android 16.
- The exact stable-key-signed 0.1.1 APK installed as an update and reported `versionCode 2` / `versionName 0.1.1-alpha.1`.
- Strict environment plus lich ASTC and `RayTracingPipeline` selected; honest RT swapchain presentation reconfirmed.
- All seventeen SoundPool clips loaded; no native renderer or Android runtime failure occurred.
- The full route, touch controls, combat, reset, pause/Home lifecycle, and accessibility-scale UI passed hands-on validation.
- At 75%, every required warm route zone remained below 13.7 ms median-of-three-window averages at thermal status 3. The later deterministic cool/status-0 default checkpoint set measured 7.667-16.123 ms and completed all 13 replay waypoints.
- At 100%, full `1440x2980` image/extent presentation passed without a 50 FPS requirement. The latest automated opening was 25.191 ms.
- Evidence: `docs/HORDE_SHOWCASE_ANDROID_VALIDATION_2026-07-17.md`, `docs/ANDROID_SHOWCASE_AUTOMATION_VALIDATION_2026-07-17.md`, and `docs/SHOWCASE_ALPHA_RELEASE_VALIDATION_2026-07-17.md`.

## Validated Windows release state

- GPU: NVIDIA GeForce RTX 5050 Laptop GPU.
- Release builds as `HordeLanternRT.exe` with GUI subsystem, icon/version resource, static MSVC runtime, and executable-relative assets.
- A clean 0.1.1 candidate extraction launched without the source tree, selected `RayTracingPipeline`, and honestly presented the full route.
- Windows Debug/Release and all seven CTests pass; hands-on route, collision, mirror, combat, lighting, reset, and spatial-audio validation passed.
- The Windows Release in-app benchmark is live-validated at 100%: 2/2 frame-symmetric laps, 26/26 waypoints, 1,838 measured frames, honest RT presentation throughout, selectable/copyable UI, and parseable timestamped text/JSON. See `docs/IN_APP_BENCHMARK_WINDOWS_VALIDATION_2026-07-17.md`.
- The package includes both enemy GLBs, raw environment and lich textures, seventeen FilmCow WAVs, release notes, controls, and `ASSET_LICENSES.md`.

## Asset and licence state

- Five Poly Haven environment sets are retained under CC0 and packed into current platform formats.
- The skeleton derivative ships under Hotstrike's finished-project/modification permission plus the conservative Meshy Free-plan CC BY 4.0 attribution route.
- FilmCow SFX use FilmCow's custom royalty-free project-use terms; the complete source archive is not redistributed.
- Meshy sword source/LOD and torch study are staged only. Neither is loaded or distributed until a measured static GLB/PBR path and appropriate attribution route exist.
- Preserve public Hotstrike/Meshy credit and keep `ASSET_LICENSES.md` with Windows packages and linked from the download page.

## Important files

- Android UI: `android/app/src/main/java/com/samfa12/hordelanternrt/MainActivity.java`
- Android native bridge: `android/app/src/main/cpp/android_probe_bridge.cpp`
- Windows presentation/UI: `src/platform/windows/DiagnosticWindow.cpp`
- Shared RT scene: `src/vulkan/raytracing/PresentableTinyRtScene.cpp`
- Raygen source: `shaders/raytracing/minimal.rgen`
- Embedded raygen: `src/vulkan/raytracing/MinimalRayGenShader.inc`
- Shared showcase route: `src/gameplay/ShowcaseRoute.h`
- Collision/combat: `src/gameplay/CorridorCollision.h`, `src/gameplay/SwordCombat.h`
- Licences: `ASSET_LICENSES.md`
- Release readiness: `docs/ALPHA_RELEASE_READINESS_2026-07-15.md`
- Packaging: `tools/package-alpha.ps1`, `tools/package-signed-alpha.ps1`, `tools/push-alpha-to-itch.ps1`
- Build/test/demo cycle plan: `docs/BUILD_TEST_DEMO_CYCLE_PLAN_2026-07-17.md`
- RTXPT-derived performance/quality/workflow reference and no-regression gates: `docs/RTXPT_1_8_1_REFERENCE_AND_REGRESSION_GATES_2026-07-17.md`
- Developer overlay Windows validation and Android build boundary: `docs/DEVELOPER_OVERLAY_WINDOWS_VALIDATION_2026-07-17.md`

## Next-step sequence

1. Preserve the published alpha and its stable signing identity.
2. Back up the JKS and both passwords independently.
3. Treat the complete 0.1.1 route as the preserved playable baseline; the stained pane was rejected, water remains deferred, and no second concurrent enemy is authorised without a measured plan.
4. The deterministic checkpoint, three-window benchmark, native route replay, and bounded Android evidence runner foundation is complete and live-validated.
5. Developer overlay/state visibility is complete and live-validated on Windows; Android build plumbing is complete but device validation waits for the phone. Continue with that phone gate, then the integrated cross-platform clean-build/package/stale-shader/licence gate and fixed video/presentation capture.
6. Gate each meaningful renderer/gameplay-route change on the phone at 75%; report 100% separately and retain the short hands-on touch/audio/lifecycle pass.
7. Keep real RT and honest diagnostics. Reduce bounded effect area/ray cost before expanding gameplay or substituting fake effects.
8. Treat `docs/BUILD_TEST_DEMO_CYCLE_PLAN_2026-07-17.md` as the detailed backlog and `docs/DOCUMENTATION_CHECKPOINT_2026-07-17.md` as the documentation authority map.

## Showcase route blockout - 2026-07-16 historical milestone

- The original room, material gallery, skeleton encounter, controls, reset, and shader path remain intact.
- A shared route definition now owns walkable rectangles, preserved obstacles, spawn/room coordinates, showcase zones, and the pure position-to-zone query for both platforms.
- The route extends through a three-turn shadow corridor, physical-aperture skylight chamber, four unlit torch bays, empty transmission frame, and dry empty finale with a mirror surround.
- Collision resolves from previous to proposed position with swept union checks and X/Z wall sliding; `ShowcaseRouteSmoke` covers every zone, corners, shortcuts, the old far wall, and preserved obstacles.
- Windows Debug/Release tests and the Android debug build/install passed. Strict ASTC and honest RT swapchain presentation were observed on `SM-S948B`; see `docs/SHOWCASE_ROUTE_BLOCKOUT_VALIDATION_2026-07-16.md` for evidence and the remaining manual performance sweep.
- Hands-on feedback added a raised masonry skylight shaft plus a skeleton arena leash and collision-aware movement. Directional attenuation, distance rolloff, and wall-aware audio remain an explicit follow-up rather than part of the geometry slice.

## Alpha refresh validation - 2026-07-16

- Android debug uses `com.samfa12.hordelanternrt.debug` beside the stable public package; release keeps `com.samfa12.hordelanternrt`.
- Android native packaging is 16 KiB-page compatible through a static C++ runtime, `0x4000` ELF `LOAD` alignment, and verified 16 KiB APK alignment.
- Android publishes a first 30-frame timing sample so resolution, FPS, and frame time appear promptly; later updates retain the 120-frame window.
- Windows carries a Per-Monitor V2 manifest, DPI-scaled layout/fonts, minimum sizing, and in-app credits. The freshly extracted package passed the full live sweep at the machine's active 125% scale.
- The live itch page now directly credits FilmCow, Poly Haven, Hotstrike Studio, Meshy, CC BY 4.0, and the full licence manifest; Code and Graphics are selected in the AI disclosure.
- Hotstrike public-source permission was requested at `https://itch.io/post/16578566`. Finished-game packaging remains permitted, but the public raw-GLB/history question is still pending their reply.

## Windows-first complete showcase route - 2026-07-16

- The playable route now sequences skeleton -> deterministic lantern gutter/drop -> bounded blue skylight -> yellow/blue/deep-red/restrained-green torch bays -> open framed threshold -> light-aware one-bounce hero mirror -> floating staff-lit lich -> post-death sliding skylight.
- Player presence is complete with a reusable-BLAS leather pelvis, articulated thighs/shins/boots, procedural gait, exact retained hand grips, and reset-only lantern failure/lowered left arm.
- `ShowcaseGameplay.h` owns deterministic lantern, lower-body, lighting, plural roster/director, and lich charge/recovery state. Only one skinned enemy is selected/rendered/refit at once; the capacity remains configurable for later Horde measurements.
- The lich uses continuous living `Idle_02` and non-looping `Dead` clips; whole-instance hover/orbit replaces the visibly distorted walking clip. Its separate 48-byte UV stream, raw Windows KTX2, strict Android ASTC 6x6, derived violet emissive map, and forty skin-weighted staff vertices drive the visible staff light/electricity. It takes three hits with a two-second lockout; each accepted hit produces recoil plus a positional cry, and death opens the finale roof over 4.5 seconds.
- Player travel and skeleton cadence now produce accepted audible footsteps. Skeleton and lich spatial cues share equal-power pan, distance rolloff, and route-obstruction attenuation through XAudio2 on Windows and published left/right SoundPool gains on Android. Android playback passed hands-on testing, while perceived stereo directionality and distance remain explicitly uncertified.
- Windows Debug/Release and all five CTests pass, and the final hands-on Windows route/audio/combat verdict passed. The complete route is also device-validated on `SM-S948B`: strict environment/lich ASTC, honest RT presentation, full hands-on traversal, lifecycle recovery, and controlled warm 75% measurements all pass. Every required zone's median of three 120-frame average windows was below 13.7 ms at thermal status 3; see `docs/HORDE_SHOWCASE_ANDROID_VALIDATION_2026-07-17.md`. Label this state **Windows-validated / Android-device-validated**.
- Preserve the latest raygen artifact recorded in `docs/HORDE_SHOWCASE_WINDOWS_VALIDATION_2026-07-16.md`. The lich's Meshy CC0 evidence is retained at `assets/models/enemies/meshy/lich_placeholder_source_licence.png`. Preserve the current one-active-skinned-enemy limit until a separate multi-enemy phone measurement.
