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
- `PARRY` dispatches on button press and cancels only a correctly timed frontal skeleton strike; verify the metal clang, distinct short haptic, attacker stagger, and immediate riposte opportunity.
- Log reports strict ASTC KTX2 for both environment and lich, all 17 SoundPool loads, and honest RT presentation.
- Use `.\tools\run-android-showcase-validation.ps1` from the repo root for the default six 75% checkpoints and 13-waypoint replay. Each checkpoint retains three 120-frame windows and reports descriptive 60/50/30 FPS bands plus thermal/governor context; crossing 20.000 ms is not an automatic failure. Report 100% separately. The runner verifies the installed `base.apk` hash against the local Debug APK and records source/shader identity. Investigate matched regressions above 15% and retain sustained route order.
- Automation is regression evidence, not a substitute for a short hands-on touch, perceived audio/directionality, visual-art, and pause/Home lifecycle pass. Audio/haptic owner checks are change-triggered rather than required by every milestone: state `Audio/haptic manual revalidation required: YES/NO` and why. This reconciliation is `YES` because listener-at-event-time routing changes. See `OWNER_RELEASE_SAFETY_CHECKLIST.md` for owner-only signing recovery checks.

Debug reports are available through `adb shell run-as com.samfa12.hordelanternrt.debug`; stable release builds are deliberately non-debuggable. Desktop and routine automation reports remain under ignored `reports/` until a reviewed milestone is promoted into `docs/validation/`.
