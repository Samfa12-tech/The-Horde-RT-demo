# Horde Lantern RT

Horde Lantern RT is a native Vulkan hardware-ray-tracing technology demo for Android and Windows. It is an early historical-gothic showing built around a torch-lit ruin, wet stone, a bounded material gallery, and one animated skeleton encounter.

- Public alpha: https://samfa12.itch.io/the-horde
- Source repository: https://github.com/Samfa12-tech/The-Horde-RT-demo
- Current package version: `0.1.0-alpha.1`
- Primary validated phone: Samsung `SM-S948B` / Adreno 840
- Validated Windows GPU: NVIDIA GeForce RTX 5050 Laptop GPU

## RT or nothing

The demo uses Vulkan acceleration structures, an RT pipeline and shader binding table, `vkCmdTraceRaysKHR`, an RT storage image, and swapchain presentation. The phone-safe shading path uses `rayQueryEXT` inside raygen with pipeline recursion depth 1. Unsupported devices show explicit diagnostics; there is no browser, raster, baked, screen-space, or fake-RT fallback.

`rtScene.presented` becomes true only after an RT-produced frame reaches successful swapchain presentation.

## Initial alpha contents

- Portrait-first Android presentation and a native Windows desktop build.
- Branded entry, pause, controls, settings, diagnostics, restart, and quit flows.
- Persisted 50-100% RT render-resolution scale, defaulting to 100%.
- Android SFX volume and compact-HUD settings; Windows SFX, sensitivity, display-mode, and render-scale settings.
- Closed starting chamber with a collision-safe spawn, deep room-two arch, and the skeleton staged behind it.
- Thin clear skylight transmission, material-ID gallery surfaces, wet stone, fog, moonlight, and bounded reflection work.
- A low-poly held torch with wooden shaft, iron cage, and layered emissive flame volumes.
- A camera-relative RT torso and articulated two-bone-IK arms holding a procedural torch and sword.
- One Hotstrike Studio skeleton derivative, textured/rigged/animated with Meshy, using a narrow walk/attack/death/respawn loop.
- Thirteen FilmCow UI, sword, impact, fall, player-footstep, skeleton-footstep, and skeleton-attack WAV cues.

The signed Android APK contains the skeleton GLB, strict ASTC KTX2 material arrays, thirteen FilmCow WAVs, four ABI libraries, and launcher assets. The Windows ZIP contains `HordeLanternRT.exe`, an executable-relative `assets/` tree, release notes, controls, and `ASSET_LICENSES.md`.

The staged Meshy sword LOD and torch study are not used by the runtime or included in either download. Their metadata is retained for later measured GLB/PBR work.

## Controls

### Android

- Left-side drag: walk and strafe
- Right-side drag: 360-degree look
- `SWING`: sword attack
- Android Back: pause/resume

### Windows

- `WASD`: walk and strafe
- Left mouse drag: look
- Right mouse or `Space`: sword attack
- `Esc`: pause/resume
- `R`: restart route
- `F1`: controls
- `F2`: RT diagnostics
- `Alt+Enter`: fullscreen/windowed

## Current validation

The stable-key-signed Android candidate was installed on `SM-S948B` and reconfirmed in portrait at accessibility font scale `1.7`. It selected the strict ASTC path, dispatched at `1440x2980` at the default 100% scale, loaded all thirteen SoundPool clips, and honestly presented RT frames.

Render scaling was verified at:

| Scale | Android internal RT extent | Result |
|---:|---:|---|
| 100% | `1440x2980` | Default; hot performance varies with route/view and may fall below 50 FPS |
| 75% | `1080x2235` | Sustained recommendation; 10.933 ms median / 15.050 ms p95 across 21 renderer telemetry windows at thermal status 3 |
| 50% | `720x1490` | 163.12 FPS / 6.13 ms observed in live diagnostics |

Windows Release was launched from a clean extraction using only its packaged assets. It reported `RayTracingPipeline`, `RT scene presented: yes`, and live resolution/FPS/frame-time diagnostics. The 100% and 75% render targets were verified at `982x628` and `737x471` respectively.

See:

- `docs/ALPHA_ANDROID_PHONE_VALIDATION_2026-07-15.md`
- `docs/ALPHA_WINDOWS_VALIDATION_2026-07-15.md`
- `docs/ALPHA_RELEASE_READINESS_2026-07-15.md`
- `docs/ALPHA_RELEASE_NOTES_2026-07-15.md`

## Build and run

### Windows

```powershell
$cmake = "C:\Program Files (x86)\Microsoft Visual Studio\2022\BuildTools\Common7\IDE\CommonExtensions\Microsoft\CMake\CMake\bin\cmake.exe"
& $cmake -S . -B build
& $cmake --build build --config Debug --target horde_rt_diagnostic_window
.\build\Debug\HordeLanternRT.exe
```

The standalone capability probe remains available as target `horde_rt_capability_probe`.

### Android development build

```powershell
cd android
.\gradlew.bat assembleDebug installDebug --console=plain
adb shell am start -n com.samfa12.hordelanternrt/.MainActivity
adb logcat -d -s HordeRtProbeBridge HordeLanternAudio AndroidRuntime
```

Look for:

- `PBR material encoding: ASTC 6x6 diffuse/ARM + ASTC 4x4 normal (KTX2)`
- `RT frame reached Android swapchain presentation.`
- `SFX loaded` IDs 1 through 13

Debug builds expose reports with `adb shell run-as`; release builds deliberately do not set `android:debuggable`.

## Package and publish

Create a release key once, outside Git:

```powershell
.\tools\create-android-release-key.ps1
```

For a signed rebuild:

```powershell
.\tools\package-signed-alpha.ps1 -KeyStorePath '<outside-repo path>'
.\tools\push-alpha-to-itch.ps1 -Version 0.1.0-alpha.1 -Channels Both
```

The packaging and push scripts securely prompt for signing secrets, reject debug/unsigned Android candidates, verify hashes, and keep Windows and Android on separate itch channels. Add `-ConfirmPush` only after the preflight passes.

Never commit a keystore, signing properties, credentials, APK, or generated candidate directory. Losing the release JKS or its passwords prevents compatible Android updates.

## Asset and licence policy

All shipped third-party assets are recorded in `ASSET_LICENSES.md`. The current release includes:

- Five Poly Haven environment sets under CC0.
- Free Stylized Skeleton by Hotstrike Studio, modified through Meshy; the release uses the conservative Meshy Free-plan CC BY 4.0 attribution route.
- A bounded FilmCow Recorded SFX subset under FilmCow's custom royalty-free project-use terms.

Do not redistribute source assets as standalone asset packs. Preserve the public Hotstrike/Meshy credit and the full licence manifest with releases.

## Architecture and invariants

- Android entrypoint: `android/app/src/main/java/com/samfa12/hordelanternrt/MainActivity.java`
- Android native bridge: `android/app/src/main/cpp/android_probe_bridge.cpp`
- Windows presentation: `src/platform/windows/DiagnosticWindow.cpp`
- Shared scene: `src/vulkan/raytracing/PresentableTinyRtScene.cpp`
- Raygen source: `shaders/raytracing/minimal.rgen`
- Embedded raygen SPIR-V: `src/vulkan/raytracing/MinimalRayGenShader.inc`

After raygen edits, run `tools/compile-raygen.ps1`. Keep one frame in flight while the held-prop TLAS uses host-written instance data. Preserve presentation-format-driven red/blue swapping on the 100% raw-copy path so warm fire does not render cyan.

## Next route

The initial showing alpha is packaged and published. The next bounded visual route is described in `docs/COLOURED_LIGHT_ROUTE_PLAN_2026-07-15.md`: lower-body/lantern-drop staging, a zig-zag shadow corridor, blue skylight, bay-selected coloured torches, bounded coloured transmission, one hero mirror, and an emissive-model replacement in the existing one-enemy slot.

The 75% setting is the sustained phone recommendation. Preserve real RT at the documented quality tier; reduce bounded effect area or ray cost before considering any broader feature expansion.
