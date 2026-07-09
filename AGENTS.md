# Horde Lantern RT - Agent Instructions

This repo is a native Vulkan hardware ray tracing game/tech-demo project. Keep work aligned with the project memory and decisions before adding features.

## Non-negotiables

- RT or nothing: do not replace the main path with raster-only rendering, baked lighting, screen-space effects, browser WebGPU, or fake RT.
- Android phone is first-class and currently the primary target.
- Windows RTX remains an equal validation target, but do not let desktop-only polish break the phone path.
- Unsupported devices should show clear diagnostics instead of silently falling back.
- Keep the repo clean. Do not paste in a giant engine or sample dump.

## Current implementation shape

- Android app entrypoint: `android/app/src/main/java/com/samfa12/hordelanternrt/MainActivity.java`.
- Android native bridge: `android/app/src/main/cpp/android_probe_bridge.cpp`.
- Windows diagnostic path: `src/platform/windows/DiagnosticWindow.cpp`.
- Shared presentable RT scene: `src/vulkan/raytracing/PresentableTinyRtScene.cpp`.
- RT shaders: `shaders/raytracing/minimal.rgen`, `minimal.rmiss`, `minimal.rchit`.
- Embedded SPIR-V arrays in `PresentableTinyRtScene.cpp` must be regenerated after shader edits.
- Current phone-safe path tracing is implemented with `rayQueryEXT` in the raygen shader, not recursive closest-hit tracing.

## Working rules

- Preserve actual `vkCmdTraceRaysKHR` presentation through the swapchain.
- Keep `selectedCapabilities_.rtScene.presented` / report `rtScene.presented` honest: only true after an RT-produced frame reaches successful swapchain presentation.
- Prefer small, shippable vertical slices over large speculative rewrites.
- When adding visual/gameplay features, keep a visible phone build runnable at each step.
- Record asset licenses in `ASSET_LICENSES.md` before shipping any imported asset.

## Visual direction

- Historical gothic action demo.
- Start in a dark torch-lit corridor or ruin.
- Prioritize lantern/fire lighting, wet stone, fog, silhouettes, shadows, and obvious RT mood.
- Gameplay comes after the visual RT proof feels like a scene, not a probe.

## Current phone scene controls

- Left-side drag walks/strafs.
- Right-side drag gives 360 camera look.
- Pitch stays clamped; yaw should remain unbounded for 360 look.
- A visible handheld medieval flame torch and its light contribution are passed to the RT shaders through push constants.
- Diagnostics are intentionally hidden behind the small HUD so the app first reads as a game scene.

## Known renderer constraint

- A recursive path-tracing experiment with pipeline recursion depth 2 failed on phone pipeline creation.
- Prefer phone-safe ray-query path tracing inside raygen while keeping `vkCmdTraceRaysKHR` as the frame dispatch/presentation path.
- If trying recursion again, prove capability and pipeline creation on the phone before making it the default.

## Next slice target

- Phase 1C should focus on collision, reflections, and gothic PBR material proof.
- Add simple corridor/room collision before expanding movement.
- Add commercial-safe open-source PBR textures for dry stone, wet stone, moss, puddles/water, and old metal.
- Record every imported texture or mesh in `ASSET_LICENSES.md`.

## Build notes

- Android debug build from `android/`: `.\gradlew.bat assembleDebug installDebug --console=plain`.
- Windows Vulkan SDK was installed at `C:\VulkanSDK\1.4.350.0` during development.
- Shader compile command uses `C:\VulkanSDK\1.4.350.0\Bin\glslangValidator.exe`.
