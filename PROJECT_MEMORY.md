# Horde Lantern RT - Project Memory

Last updated: 2026-07-16

## Identity and release state

- Working/public title: **Horde Lantern RT**.
- Purpose: native Vulkan hardware-ray-tracing game/technology demo.
- Principle: **RT or nothing**; unsupported devices receive honest diagnostics, never a fake fallback.
- Primary target: Android phone. Equal validation target: Windows RTX.
- Current release: **Initial Showing Alpha 0.1.0**, package version `0.1.0-alpha.1`.
- Canonical downloads: https://samfa12.itch.io/the-horde. Samfa12.com links to itch rather than hosting a second copy; the live `/games/` card, itch link, GitHub link, thumbnail, and released status were rendered and verified on 2026-07-15.
- Source: https://github.com/Samfa12-tech/The-Horde-RT-demo.
- Windows itch channel: upload `#18339908`, build `#1798649`, `windows-x64`.
- Android itch channel: upload `#18341739`, build `#1798652`, `android`.
- Signed Android APK SHA-256: `13bced0aa40e4a102e25aa1c57083f7feb4b12ca7a8d492c46cc7c6cfdda932a`.
- Windows ZIP SHA-256: `1bae34e6d323bbd201ff4dd113f3c78518788f90a298a7babce1a446da2721cc`.
- Signing certificate SHA-256: `8245277a11bca5576f116724507f799d6f4c178ce5fbb7e3981415c9e6b3c245`.
- The release JKS lives outside Git. It and both passwords need an independent owner backup.

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
- Thirteen FilmCow WAVs cover UI, sword, impacts/fall, quiet alternating player/skeleton footsteps, and skeleton attack.
- Android uses SoundPool per-cue gain; Windows uses asynchronous WinMM playback with non-interrupting movement cues.

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
- Collision/combat: `src/gameplay/CorridorCollision.h`, `src/gameplay/SwordCombat.h`
- Licences: `ASSET_LICENSES.md`
- Release readiness: `docs/ALPHA_RELEASE_READINESS_2026-07-15.md`
- Packaging: `tools/package-alpha.ps1`, `tools/package-signed-alpha.ps1`, `tools/push-alpha-to-itch.ps1`

## Next-step sequence

1. Preserve the published alpha and its stable signing identity.
2. Back up the JKS and both passwords independently.
3. Build the coloured-light route in bounded slices: lower body/lantern drop, zig-zag shadow corridor, blue skylight, bay-selected coloured torches, bounded coloured transmission, one hero mirror, optional measured shallow water, and an emissive reskin/replacement in the existing enemy slot.
4. Gate each meaningful renderer change on the phone at the recommended quality tier; report 100% separately.
5. Keep real RT and honest diagnostics. Reduce bounded effect area/ray cost before expanding gameplay or substituting fake effects.

## Alpha refresh validation - 2026-07-16

- Android debug uses `com.samfa12.hordelanternrt.debug` beside the stable public package; release keeps `com.samfa12.hordelanternrt`.
- Android native packaging is 16 KiB-page compatible through a static C++ runtime, `0x4000` ELF `LOAD` alignment, and verified 16 KiB APK alignment.
- Android publishes a first 30-frame timing sample so resolution, FPS, and frame time appear promptly; later updates retain the 120-frame window.
- Windows carries a Per-Monitor V2 manifest, DPI-scaled layout/fonts, minimum sizing, and in-app credits. The freshly extracted package passed the full live sweep at the machine's active 125% scale.
- The live itch page now directly credits FilmCow, Poly Haven, Hotstrike Studio, Meshy, CC BY 4.0, and the full licence manifest; Code and Graphics are selected in the AI disclosure.
- Hotstrike public-source permission was requested at `https://itch.io/post/16578566`. Finished-game packaging remains permitted, but the public raw-GLB/history question is still pending their reply.
