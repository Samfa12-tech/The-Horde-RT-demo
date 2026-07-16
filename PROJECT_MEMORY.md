# Horde Lantern RT - Project Memory

Last updated: 2026-07-17

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
- The next authored route is defined in `docs/COLOURED_LIGHT_ROUTE_PLAN_2026-07-15.md`.

## Current renderer

- Android and Windows build BLAS/TLAS, RT pipeline/SBT, dispatch `vkCmdTraceRaysKHR`, write an RT storage image, and present it through the swapchain.
- `rtScene.presented` becomes true only after an RT-produced frame reaches successful presentation.
- The phone-safe path uses `rayQueryEXT` in raygen for primary, visibility, and bounded bounce/transmission work with pipeline recursion depth 1.
- A recursive depth-2 closest-hit experiment compiled but failed during phone pipeline creation. Do not restore it without proving capability and pipeline creation on the phone.
- The RT storage image is RGBA. Common BGRA swapchains require the presentation-format-driven `outputRedBlueSwap` path on raw copy; scaled modes use a format-aware blit.
- One frame remains in flight while held-prop TLAS transforms use host-written instance data.
- Android uses strict ASTC KTX2 arrays: ASTC 6x6 diffuse/ARM and ASTC 4x4 normals. Windows uses executable-relative raw RGBA8 arrays.

## Current alpha scene and controls

- Closed start chamber; spawn/reset is inside with room to turn and collision prevents rear-wall escape.
- A physically deep arch leads to room two; the single skeleton begins behind the arch.
- One bounded clear skylight pane uses thin RT transmission and does not incorrectly block sky/moon visibility.
- The held torch is RT geometry: wooden shaft, collars/cage, and two nested emissive flame volumes. The old fullscreen triangle overlay must not return.
- Player presence uses one yaw-relative torso plus four reusable-limb TLAS instances on mask `0x04`; two-bone IK locks hands to torch/sword grips while the props/arms follow pitch.
- The procedural sword has its own RT instance and swings independently of the torch.
- The skeleton uses Hotstrike Studio's base asset, later textured/rigged/animated with Meshy. Walking clip time is 90% to reduce ground slide.
- One narrow approach/attack/death/respawn loop is implemented.
- Android: left drag movement/strafe, right drag 360 look, Swing button, Android Back pause/resume.
- Windows: WASD, left-drag look, right mouse/Space swing, Esc pause/resume, R restart, F1 controls, F2 diagnostics, Alt+Enter fullscreen.

## UI, settings, diagnostics, and audio

- Both platforms have branded entry, pause, controls, settings, RT diagnostics, restart, and quit flows.
- Technical output stays tucked away unless requested or startup fails.
- Both platforms persist a 50-100% RT render-resolution slider with 100% default.
- Android also persists SFX enable/volume, look sensitivity, and compact HUD.
- Windows persists SFX, sensitivity, display mode, and render scale beside the executable.
- Android diagnostics report internal resolution, FPS, frame time, dispatch resolution, and honest RT presentation.
- Seventeen FilmCow WAVs cover UI, sword, normalized alternating player/skeleton footsteps, skeleton attack, and lich charge/impact/fall/hurt reactions.
- Android uses SoundPool per-cue gain; Windows uses XAudio2 per-voice matrices with WinMM fallback.

## Validated Android release state

- Device: Samsung `SM-S948B`, Adreno 840, Vulkan 1.4.295.
- Exact stable-key-signed APK installed and launched after replacing the debug-signature package.
- Portrait rotation `0`; physical display `1440x3120`; user font scale `1.7` remained unchanged.
- Strict ASTC and `RayTracingPipeline` selected; honest RT swapchain presentation reconfirmed.
- All thirteen SoundPool clips loaded; no native renderer or Android runtime failure occurred.
- Menu/settings fit at font scale 1.7; Back pause, movement, look, swing, restart, diagnostics, Home/resume, rear-wall collision, arch/skeleton/skylight staging, and scene warmth were exercised.
- Render scale verification:
  - 100%: `1440x2980`, default; hot performance is route/view dependent and can fall below 50 FPS.
  - 75%: `1080x2235`; 21 renderer telemetry windows at thermal status 3 measured 10.933 ms median / 15.050 ms p95. This is the sustained recommendation.
  - 50%: `720x1490`; visible diagnostics recorded 163.12 FPS / 6.13 ms.
- Evidence: `docs/ALPHA_ANDROID_PHONE_VALIDATION_2026-07-15.md`.

## Validated Windows release state

- GPU: NVIDIA GeForce RTX 5050 Laptop GPU.
- Release builds as `HordeLanternRT.exe` with GUI subsystem, icon/version resource, static MSVC runtime, and executable-relative assets.
- A clean candidate extraction launched without the source tree, selected `RayTracingPipeline`, and honestly presented at `982x628`.
- Packaged diagnostics recorded `163.24 FPS / 6.13 ms`; 75% recreated the RT target at `737x471` and retained warm colour.
- Windows package includes release notes, `README.txt`, `ASSET_LICENSES.md`, skeleton, raw textures, and thirteen FilmCow WAVs.

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

## Next-step sequence

1. Preserve the published alpha and its stable signing identity.
2. Back up the JKS and both passwords independently.
3. Slice A route blockout is complete. Continue in bounded slices with lower body/lantern drop, zig-zag shadows and blue skylight, bay-selected coloured torches, bounded coloured transmission, one hero mirror, optional measured shallow water, and an emissive reskin/replacement in the existing enemy slot.
4. Gate each meaningful renderer change on the phone at the recommended quality tier; report 100% separately.
5. Keep real RT and honest diagnostics. Reduce bounded effect area/ray cost before expanding gameplay or substituting fake effects.
6. Build the supporting cycle in this order: deterministic showcase checkpoints; repeatable benchmark route; input recording/replay; developer overlay; one-command build/test/package validation; fixed screenshot/video capture; then bounded combat-readability polish.
7. Treat `docs/BUILD_TEST_DEMO_CYCLE_PLAN_2026-07-17.md` as the detailed backlog. Keep its guardrails: real RT presentation, phone measurement, strict asset/package validation, compact player-facing UI, and one active skinned enemy until separately measured.

## Showcase route blockout - 2026-07-16

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
- Player travel and skeleton cadence now produce accepted audible footsteps. Skeleton and lich spatial cues share equal-power pan, distance rolloff, and route-obstruction attenuation through XAudio2 on Windows; compile-only SoundPool L/R integration remains on Android.
- Windows Debug/Release and all five CTests pass, and the final hands-on Windows route/audio/combat verdict passed. The complete route is also device-validated on `SM-S948B`: strict environment/lich ASTC, honest RT presentation, full hands-on traversal, lifecycle recovery, and controlled warm 75% measurements all pass. Every required zone's median of three 120-frame average windows was below 13.7 ms at thermal status 3; see `docs/HORDE_SHOWCASE_ANDROID_VALIDATION_2026-07-17.md`. Label this state **Windows-validated / Android-device-validated**.
- Preserve the latest raygen artifact recorded in `docs/HORDE_SHOWCASE_WINDOWS_VALIDATION_2026-07-16.md`. The lich's Meshy CC0 evidence is retained at `assets/models/enemies/meshy/lich_placeholder_source_licence.png`. Preserve the current one-active-skinned-enemy limit until a separate multi-enemy phone measurement.
