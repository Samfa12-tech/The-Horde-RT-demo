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
.\build\Debug\horde_rt_diagnostic_window.exe
```

Verify:

- The selected device reports `RayTracingPipeline` with ray-query support.
- The interactive scene presents an RT-produced frame through the swapchain.
- `WASD`, mouse-drag look, and `Esc` work.
- Torch colour remains warm on the BGRA presentation path.

## Android phone

```powershell
cd android
.\gradlew.bat assembleDebug installDebug --console=plain
adb shell am start -n com.samfa12.hordelanternrt/.MainActivity
adb logcat -d -s HordeRtProbeBridge AndroidRuntime
```

Verify:

- The log includes `RT frame reached Android swapchain presentation.`
- Unsupported devices show explicit diagnostics instead of a fallback renderer.
- Touch movement/look, collision, held props, skeleton animation, PBR materials, and roof-breach moonlight remain functional.
- Run the 126-interval cold performance gate after renderer, animation, or asset-path changes; preserve a 50+ FPS median target.

Reports are written under app-private `files/reports/` and desktop `reports/`.
