# Initial alpha Android phone validation - 2026-07-15

## Exact build and device

- Candidate installed: `Horde-Lantern-RT-Alpha-0.1.0-alpha.1-Android-preview-debug-signed.apk`.
- Candidate SHA-256: `6bfe0001e881a727bb8f019b6ac2b742bcd1a43fa5bcae000e52b806415fbafd`.
- Device: Samsung `SM-S948B` (`R5GL219SZGK`), Adreno 840, Vulkan 1.4.295.
- Physical display: `1440x3120`; app RT presentation extent at 100%: `1440x2980`.
- The user's accessibility font scale remained unchanged at `1.7`.
- Display rotation remained `0` in portrait throughout the pass.

The full interaction and render-scale pass used the debug-certificate preview above. A final release sanity pass then installed the exact stable-key-signed candidate `Horde-Lantern-RT-Alpha-0.1.0-alpha.1-Android.apk` (SHA-256 `590cc2cbecbf598dfb7a7c67c6e2e3b39d46380dff87974356698a10da12d72e`).

`apksigner` verified one non-debug v2 signer, `CN=Samfa12, OU=Games, O=Samfa12, C=AU`, certificate SHA-256 `8245277a11bca5576f116724507f799d6f4c178ce5fbb7e3981415c9e6b3c245`.

## Runtime and RT honesty

- Strict ASTC runtime assets were present: diffuse/ARM ASTC 6x6 and normal ASTC 4x4 KTX2 arrays. No raw Windows texture arrays were staged into the app-private runtime directory.
- The capability report selected `RayTracingPipeline` and recorded `RT scene presented: yes` only after successful swapchain presentation.
- Diagnostics visibly reported internal resolution, live FPS, live frame time, dispatch resolution, and honest presentation state.
- No Android runtime crash, SoundPool load failure, or native RT record/presentation failure was observed during the pass.

## UI, lifecycle, controls, and scene

- The app remained portrait at font scale `1.7`; the complete start/pause menu and settings panel fit without clipping.
- Android Back opened the pause menu instead of exiting.
- Resume, restart route, settings, diagnostics, movement, right-side 360 look, and Swing were exercised with real input events.
- Home -> resume retained the same process (`23344` before and after), restored the RT surface without black output, and did not create a duplicate activity/render process.
- Restart route placed the player inside the sealed room facing the deep arch, with the skeleton behind the arch and the clear skylight visible above it.
- A sustained movement input into the rear wall remained inside the room; no skybox escape or wall traversal occurred.
- The packaged lantern is visibly a shaft/cage with layered emissive flame volumes rather than the former pair of loose triangles.

## Audio

- All thirteen packaged FilmCow clips completed `SoundPool` loading on the phone (`SFX loaded` IDs 1 through 13, with no load rejection/error).
- Android registered the app's player as `SoundPool`, `USAGE_GAME`, `CONTENT_TYPE_SONIFICATION`, and recorded the Horde UID actively using audio after UI/combat input.
- Runtime routing for menu cues, two sword swings, two impacts, enemy fall, alternating player footsteps, alternating skeleton footsteps, and skeleton attack is present in this exact build.

## Render-scale validation

All three requested positions recreated the RT target, re-presented honestly, retained warm rather than cyan output, and reported the matching dispatch size:

| Setting | Internal RT extent | Observed phone evidence |
|---|---:|---|
| 100% (default) | `1440x2980` | Route/view-dependent live reports ranged roughly from 34 to 60 FPS during a long thermal session. The default remains 100% as requested, but it does not retain the 50+ FPS target in every view when hot. |
| 75% | `1080x2235` | At thermal status 3, 21 consecutive 120-frame telemetry windows measured 10.933 ms median, 15.050 ms p95, and 12.200 ms mean (about 91.5 FPS at the median). The contemporaneous visible report recorded 81.31 FPS / 12.30 ms. |
| 50% | `720x1490` | The live report recorded 163.12 FPS / 6.13 ms after honest RT re-presentation. |

The legacy `SurfaceFlinger --latency` history returned at most 120 timestamps on this Android build, so no 126-interval result is claimed. The 75% result uses the renderer's existing 120-frame telemetry and is the honest sustained release recommendation for this phone.

## Evidence

- `docs/validation/android-final-2026-07-15/02-menu.png`
- `docs/validation/android-final-2026-07-15/03-settings.png`
- `docs/validation/android-final-2026-07-15/04-diagnostics-75.png`
- `docs/validation/android-final-2026-07-15/05-settings-50.png`
- `docs/validation/android-final-2026-07-15/06-restart-100.png`
- `docs/validation/android-final-2026-07-15/07-rear-wall.png`
- `docs/validation/android-final-2026-07-15/08-rear-wall-collision.png`
- `docs/validation/android-final-2026-07-15/09-home-resume.png`
- `docs/validation/android-final-2026-07-15/10-signed-release.png`

## Signed release result

The release-signed APK retained portrait rotation `0`, font scale `1.7`, the default 100% `1440x2980` RT dispatch, strict ASTC materials, and honest RT swapchain presentation. All thirteen SoundPool clips loaded again with no app/native Vulkan failure. It is published to `samfa12/the-horde:android` as itch upload `#18341739`, build `#1797750`, version `0.1.0-alpha.1`.
