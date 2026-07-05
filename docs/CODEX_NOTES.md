# Codex Notes

This task is the first executable Vulkan hardware RT proof and is gated by real capability reporting only.

## Do

- Keep the repository clean and native-first.
- Query real Vulkan extensions/features before any RT rendering claims.
- Keep unsupported devices honest with explicit missing extension/feature diagnostics.
- Prefer Windows CLI proof, then Android-native shell next.
- Emit both text and JSON reports in `reports/`.
- Update notes whenever build/test scope changes.

## Do not

- Do not claim Android support is complete without a compiling Android native build.
- Do not claim RT works without hardware RT mode selection.
- Do not add gameplay, fake RT, raster-only renderer, or fallback-only paths before this proof is wired.

## Current status notes

- `horde_rt_capability_probe` now exists as the first concrete native proof target.
- Android is documented for next-phase integration; it is not yet complete.

## Reference hierarchy

1. Primary base/reference: KhronosGroup/Vulkan-Samples, especially `samples/extensions/ray_tracing_basic`.
2. Main learning/reference: NVIDIA `nvpro-samples/vk_raytracing_tutorial_KHR`.
3. Focused reference snippets: Sascha Willems Vulkan examples.
4. Backup/reference only: Diligent Engine.
5. Deferred/not first base: The Forge and Unreal Engine.

## Next smallest task

Wire the same Vulkan capability-probe core into an Android native app shell on the Galaxy S26 Ultra, display the report on screen, and write the report to device storage.
