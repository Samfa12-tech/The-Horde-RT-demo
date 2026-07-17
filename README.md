# Horde Lantern RT

Horde Lantern RT is a native Vulkan hardware-ray-tracing technology demo for Android and Windows. Its historical-gothic route moves from a lantern-lit skeleton encounter through coloured-light and mirror studies to a staff-lit lich finale.

- Public alpha: https://samfa12.itch.io/the-horde
- Source repository: https://github.com/Samfa12-tech/The-Horde-RT-demo
- Current package version: `0.1.1-alpha.1`
- Current itch builds: Windows `#1801016`; Android `#1801017`
- Published SHA-256: Windows `8a254c9d14b35bf868f1cb96619dc572f3505a9564b668aa55241b33bfeaec2e`; Android `ae73afec2c75b317187aeb61d81a592ec8bb4d8b5e89ef9b474fb2a60ae1354a`
- Primary validated phone: Samsung `SM-S948B` / Adreno 840
- Validated Windows GPU: NVIDIA GeForce RTX 5050 Laptop GPU

Development note: the in-app benchmark, developer overlay, and Samfa12 menu link are source changes made after `0.1.1-alpha.1`; they are not included in the itch builds listed above yet.

The APK declares Android 7 / API 24 as its packaging minimum, but hardware support is intentionally much narrower: the device and driver must expose Vulkan acceleration structures, ray-tracing pipeline, ray query, buffer device address, deferred host operations, and the required ASTC formats. Only `SM-S948B` on Android 16 is currently device-certified.

## RT or nothing

The demo uses Vulkan acceleration structures, an RT pipeline and shader binding table, `vkCmdTraceRaysKHR`, an RT storage image, and swapchain presentation. The phone-safe shading path uses `rayQueryEXT` inside raygen with pipeline recursion depth 1. Unsupported devices show explicit diagnostics; there is no browser, raster, baked, screen-space, or fake-RT fallback.

`rtScene.presented` becomes true only after an RT-produced frame reaches successful swapchain presentation.

## Showcase alpha contents

- Portrait-first Android presentation and a native Windows desktop build.
- Branded entry, pause, controls, settings, diagnostics, restart, and quit flows.
- A player-facing two-pass `Run benchmark` course ends in a selectable, copyable, exportable text report and automatically archives JSON evidence.
- A `More by Samfa12` menu button opens https://samfa12.com/ in the system browser.
- Persisted 50-100% RT render-resolution scale, defaulting to 100%.
- Android SFX volume and compact-HUD settings; Windows SFX, sensitivity, display-mode, and render-scale settings.
- Collision-safe starting chamber and material gallery, a leashed skeleton encounter, and a three-turn moving-shadow corridor.
- Lantern failure and drop, blue skylight chamber, yellow/blue/deep-red/restrained-green torch bays, open threshold, wet stone, fog, and a single-bounce hero mirror.
- A low-poly held torch with wooden shaft, iron cage, and layered emissive flame volumes.
- A complete camera-relative RT player body with articulated two-bone-IK arms, procedural gait, held torch/sword, and wall-aware prop retraction.
- A Hotstrike Studio skeleton derivative followed sequentially by a CC0 Meshy placeholder lich with emissive staff/eyes, charge electricity, spatial audio, three-hit combat, death animation, and a sliding-roof finale.
- Seventeen FilmCow UI, sword, movement, skeleton, and lich reaction/attack WAV cues.

The signed Android APK contains both enemy GLBs, strict ASTC KTX2 environment and lich textures, seventeen FilmCow WAVs, four ABI libraries, and launcher assets. The Windows ZIP contains `HordeLanternRT.exe`, an executable-relative `assets/` tree, release notes, controls, and `ASSET_LICENSES.md`.

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
- `F3`: live non-pausing developer overlay (Debug builds only)
- `Alt+Enter`: fullscreen/windowed

## Current validation

The complete showcase route is Windows-validated and Android-device-validated on `SM-S948B`. Android selected strict environment and lich ASTC textures, honestly presented RT frames, survived pause/resume and Home/surface recreation, and completed the full hands-on route without a reported issue.

Render scaling was verified at:

| Scale | Android internal RT extent | Result |
|---:|---:|---|
| 100% | `1440x2980` | Full-extent/image check passed; latest cool automated opening 25.191 ms median of three 120-frame averages, report-only |
| 75% | `1080x2235` | Sustained recommendation; warm hands-on route zones below 13.7 ms at thermal status 3; cool automated default set 7.667-16.123 ms at status 0 |
| 50% | `720x1490` | Initial-alpha opening diagnostic recorded 163.12 FPS / 6.13 ms; retained as historical evidence, not a complete-route baseline |

Windows Release was launched from a clean extraction using only its packaged assets. It reported `RayTracingPipeline`, `RT scene presented: yes`, and live resolution/FPS/frame-time diagnostics. The 100% and 75% render targets were verified at `982x628` and `737x471` respectively.

The in-app Windows Release benchmark is live-validated: its warm-up plus measured course completed 26/26 waypoints with honest RT presentation on every measured frame, displayed a selectable result, copied the complete report to the clipboard, and wrote parseable timestamped text/JSON. Android builds and lints; its touch/lifecycle/export gate waits for the connected phone.

See:

- `docs/DOCUMENTATION_CHECKPOINT_2026-07-17.md`
- `docs/HORDE_SHOWCASE_WINDOWS_VALIDATION_2026-07-16.md`
- `docs/HORDE_SHOWCASE_ANDROID_VALIDATION_2026-07-17.md`
- `docs/ANDROID_SHOWCASE_AUTOMATION_2026-07-17.md`
- `docs/ANDROID_SHOWCASE_AUTOMATION_VALIDATION_2026-07-17.md`
- `docs/IN_APP_BENCHMARK_WINDOWS_VALIDATION_2026-07-17.md`
- `docs/SHOWCASE_ALPHA_RELEASE_NOTES_2026-07-17.md`
- Historical 0.1.0 readiness and validation records remain under `docs/`.

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
adb shell am start -n com.samfa12.hordelanternrt.debug/com.samfa12.hordelanternrt.MainActivity
adb logcat -d -s HordeRtProbeBridge HordeLanternAudio AndroidRuntime
```

Look for:

- `PBR material encoding: ASTC 6x6 diffuse/ARM + ASTC 4x4 normal (KTX2)`
- `RT frame reached Android swapchain presentation.`
- `SFX loaded` IDs 1 through 17

Debug builds expose reports with `adb shell run-as`; release builds deliberately do not set `android:debuggable`.

For repeatable Android checkpoint timing and deterministic route-collision replay, connect one authorised device and run:

```powershell
.\tools\run-android-showcase-validation.ps1
```

The debug-only runner collects a timestamped evidence bundle without changing the public release path. See `docs/ANDROID_SHOWCASE_AUTOMATION_2026-07-17.md`.

## Package and publish

Create a release key once, outside Git:

```powershell
.\tools\create-android-release-key.ps1
```

For a signed rebuild:

```powershell
.\tools\package-signed-alpha.ps1 -KeyStorePath '<outside-repo path>'
.\tools\push-alpha-to-itch.ps1 -Version 0.1.1-alpha.1 -Channels Both
```

The packaging and push scripts securely prompt for signing secrets, reject debug/unsigned Android candidates, verify hashes, and keep Windows and Android on separate itch channels. Add `-ConfirmPush` only after the preflight passes.

The current packaging defaults describe the already-published immutable `0.1.1-alpha.1` release. Do not rebuild or republish changed source under that version. A future public build must first bump CMake/Windows/Android/package metadata, increment Android `versionCode` above 2, provide matching release notes, and update the guarded candidate hashes.

Android native code is linked for 16 KiB page compatibility. The release uses a static C++ runtime, 16 KiB ELF `LOAD` alignment, and AGP 8.7.2 APK alignment; `package-alpha.ps1` rejects candidates that fail either APK or ELF verification or reintroduce `libc++_shared.so` from the r26 NDK.

Never commit a keystore, signing properties, credentials, APK, or generated candidate directory. Losing the release JKS or its passwords prevents compatible Android updates.

## Asset and licence policy

All shipped third-party assets are recorded in `ASSET_LICENSES.md`. The current release includes:

- Five Poly Haven environment sets under CC0.
- Free Stylized Skeleton by Hotstrike Studio, modified through Meshy; the release uses the conservative Meshy Free-plan CC BY 4.0 attribution route.
- The active placeholder lich created and animated with Meshy under CC0; the retained licence screenshot and hash are recorded in `ASSET_LICENSES.md`.
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

## Showcase route status

The complete route is Windows- and Android-device-validated: lower body and lantern drop, zig-zag shadows, blue skylight, bay-selected coloured torches, an open framed threshold, one hero mirror, and a sequential staff-lit lich finale with a post-death sliding skylight. See `docs/HORDE_SHOWCASE_WINDOWS_VALIDATION_2026-07-16.md` and `docs/HORDE_SHOWCASE_ANDROID_VALIDATION_2026-07-17.md`.

Only one skinned enemy is animated, refit, and rendered at a time; the plural roster remains configurable for a future separately measured Horde slice.

The 75% setting is the sustained phone recommendation. Preserve real RT at the documented quality tier; reduce bounded effect area or ray cost before considering any broader feature expansion.
