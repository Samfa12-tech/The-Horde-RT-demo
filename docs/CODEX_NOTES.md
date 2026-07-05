# Codex Notes

Codex should treat `PROJECT_DECISIONS.md` and the Phase 0 docs as the current source of truth.

## Do

- Keep the repo clean.
- Implement native Vulkan work directly.
- Prioritise Android and Windows capability probing.
- Query real Vulkan extensions and feature structs before claiming RT support.
- Keep unsupported-device diagnostics clear.
- Update docs when decisions change.
- End each task with files changed, what was verified, known limitations, and the exact next smallest task.

## Do not

- Do not build gameplay before RT capability proof.
- Do not add browser, WebGPU, Three.js, Babylon.js, Godot renderer, Unreal-first, raster-only, SSR, baked-lighting, or compute-only path-tracing substitutes.
- Do not claim RT is working until real Vulkan RT extension/feature detection exists.
- Do not import untextured Meshy assets.
- Do not dump a giant third-party engine into this repo.
- Do not let stale prompts override `RT or nothing`.

## Reference hierarchy

1. Primary base/reference: KhronosGroup/Vulkan-Samples, especially `samples/extensions/ray_tracing_basic`.
2. Main learning/reference: NVIDIA `nvpro-samples/vk_raytracing_tutorial_KHR`.
3. Focused reference snippets: Sascha Willems Vulkan examples.
4. Backup/reference only: Diligent Engine.
5. Deferred/not first base: The Forge and Unreal Engine.

## Next smallest task

Implement the real Vulkan device capability probe that detects and reports Vulkan RT support on Windows and Android, writes a JSON/text capability report, and shows an unsupported diagnostic screen when hardware RT is unavailable.
