# Build and Validation Checklist

## Shared prerequisites

- Vulkan SDK available (`C:\VulkanSDK\1.4.350.0` on the current Windows machine).
- Raygen edits regenerated with `.\tools\compile-raygen.ps1`.
- Imported assets recorded in `ASSET_LICENSES.md` before runtime use.

## Windows RTX

```powershell
cmake -S . -B build
cmake --build build --config Debug --target horde_rt_capability_probe horde_rt_diagnostic_window
.\build\Debug\horde_rt_capability_probe.exe
.\build\Debug\HordeLanternRT.exe
```

Verify:

- The selected device reports `RayTracingPipeline` with ray-query support.
- The interactive scene presents an RT-produced frame through the swapchain.
- `WASD`, mouse-drag look, swing, Esc pause/resume, restart, settings, diagnostics, and fullscreen work.
- Torch colour remains warm on the BGRA presentation path.

## Android phone

```powershell
cd android
.\gradlew.bat assembleDebug installDebug --console=plain
adb shell am start -n com.samfa12.hordelanternrt.debug/com.samfa12.hordelanternrt.MainActivity
adb logcat -d -s HordeRtProbeBridge HordeLanternAudio AndroidRuntime
```

Verify:

- The log includes `RT frame reached Android swapchain presentation.`
- Unsupported devices show explicit diagnostics instead of a fallback renderer.
- Touch movement/look, collision, lantern drop, coloured bays, mirror, sequential skeleton/lich selection, three-hit finale, and sliding roof remain functional.
- `SWING` triggers an independent sword arc; verify accepted-hit recoil/cry, two-second lockout, lich death, and player/skeleton footsteps.
- Log reports strict ASTC KTX2 for both environment and lich, all 17 SoundPool loads, and honest RT presentation.
- Use `.\tools\run-android-showcase-validation.ps1` from the repo root for the default five 75% checkpoints and 13-waypoint replay. Each enforced checkpoint must retain three 120-frame windows at or below 20 ms; 100% is report-only.
- Automation is regression evidence, not a substitute for a short hands-on touch, perceived audio/directionality, visual-art, and pause/Home lifecycle pass.

Debug reports are available through `adb shell run-as com.samfa12.hordelanternrt.debug`; stable release builds are deliberately non-debuggable. Desktop and routine automation reports remain under ignored `reports/` until a reviewed milestone is promoted into `docs/validation/`.
