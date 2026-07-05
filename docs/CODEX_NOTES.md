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
- Update docs whenever build scope changes.

## Do not

- Do not claim Android support is complete without a compiling Android native build and on-device report verification.
- Do not claim RT works without hardware RT mode selection.
- Do not add gameplay, fake RT, raster-only renderer, or fallback-only paths before this diagnostic shell stage is complete.
- Do not replace native probe logic with Java/Kotlin-only diagnostics.

## Current status notes

- `horde_rt_capability_probe` exists as the first concrete native proof target.
- Android is wired as a minimal `android/` app shell in prior slices and remains the primary probe path there.
- 0C adds `horde_rt_diagnostic_window` using the same probe core and report data.

## Reference hierarchy

1. Primary base/reference: KhronosGroup/Vulkan-Samples, especially `samples/extensions/ray_tracing_basic`.
2. Main learning/reference: NVIDIA `nvpro-samples/vk_ray_tracing_tutorial_KHR`.
3. Focused reference snippets: Sascha Willems Vulkan examples.
4. Backup/reference only: Diligent Engine.
5. Deferred/not first base: The Forge and Unreal Engine.

## Next smallest task

Start Phase 1A by adding the minimal Vulkan RT shader/toolchain and pipeline skeleton for a tiny hardware RT scene, guarded by the existing RT capability result.
