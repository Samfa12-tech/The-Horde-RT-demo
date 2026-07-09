# Codex Notes

This task is the first executable Vulkan hardware RT proof and is gated by real capability reporting only.

## Do

- Keep the repository clean and native-first.
- Query real Vulkan extensions/features before any RT rendering claims.
- Keep unsupported devices honest with explicit missing extension/feature diagnostics.
- Keep the same core shared between Windows CLI and Android-native probe execution.
- Android native shell is now a minimal `android/` app shell and should continue using JNI/native C++ probe logic.
- Add a native Win32 diagnostic window shell (`horde_rt_diagnostic_window`) and share output formatting with UI helpers.
- Emit both text and JSON reports in `reports/` and `files/reports` on Android.
- Add Android and Windows native Vulkan swapchain/surface diagnostic presentation while keeping all capability text and report generation shared.
- Update docs whenever build scope changes.

## Do not

- Do not claim Android support is complete without a compiling Android native build and on-device report verification.
- Do not claim RT works without hardware RT mode selection.
- Do not add gameplay, fake RT, raster-only renderer, or fallback-only paths before this diagnostic shell stage is complete.
- Do not replace native probe logic with Java/Kotlin-only diagnostics.

## Current status notes

- `horde_rt_capability_probe` exists as the first concrete native proof target.
- Android is wired as a native `android/` app shell with shared probe text overlay plus a native diagnostic surface and remains the primary probe path there.
- 0C adds `horde_rt_diagnostic_window` using the same probe core and report data.
- `src/vulkan/raytracing/TinyRtScene.cpp` now performs a native tiny RT device/function skeleton setup before reporting RT scene status.

## Reference hierarchy

1. Primary base/reference: KhronosGroup/Vulkan-Samples, especially `samples/extensions/ray_tracing_basic`.
2. Main learning/reference: NVIDIA `nvpro-samples/vk_ray_tracing_tutorial_KHR`.
3. Focused reference snippets: Sascha Willems Vulkan examples.
4. Backup/reference only: Diligent Engine.
5. Deferred/not first base: The Forge and Unreal Engine.

## Next smallest task

Next smallest task:

- Extend the tiny RT path into a presentable native dispatch path in Phase 1B:
  - create the minimum Vulkan pipeline/surface flow needed for native RT draw dispatch using `RayTracingPipeline`/`RayQuery` support only.

Latest verification status:

- Device: `SM-S948B` (`samsung`)
- RT result: `RayTracingPipeline`
- Report files in app private `files/reports` were read successfully from `com.samfa12.hordelanternrt` on-device.
