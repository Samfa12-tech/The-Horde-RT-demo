# Codex Notes

This task is the first executable Vulkan hardware RT proof and is gated by real capability reporting only.

## Do

- Keep the repository clean and native-first.
- Query real Vulkan extensions/features before any RT rendering claims.
- Keep unsupported devices honest with explicit missing extension/feature diagnostics.
- Keep the same core shared between Windows CLI and Android-native probe execution.
- Android native shell is now a minimal Java + JNI UI activity under `android/`.
- Emit both text and JSON reports in `reports/`.
- Update notes whenever build/test scope changes.

## Do not

- Do not claim Android support is complete without a compiling Android native build and on-device report verification.
- Do not claim RT works without hardware RT mode selection.
- Do not add gameplay, fake RT, raster-only renderer, or fallback-only paths before this proof is wired.

## Current status notes

- `horde_rt_capability_probe` now exists as the first concrete native proof target.
- Android is now wired as a minimal `android/` app shell in this phase.

## Reference hierarchy

1. Primary base/reference: KhronosGroup/Vulkan-Samples, especially `samples/extensions/ray_tracing_basic`.
2. Main learning/reference: NVIDIA `nvpro-samples/vk_raytracing_tutorial_KHR`.
3. Focused reference snippets: Sascha Willems Vulkan examples.
4. Backup/reference only: Diligent Engine.
5. Deferred/not first base: The Forge and Unreal Engine.

## Next smallest task

Run the Android probe on the Galaxy S26 Ultra, verify `files/reports` output with `adb run-as`, then add native Vulkan surface rendering for overlay text on Android and Windows without fake RT paths.

Latest verification status:

- Device: `SM-S948B` (`samsung`)
- RT result: `RayTracingPipeline`
- Report files in app private `files/reports` were read successfully from `com.samfa12.hordelanternrt` on-device.
