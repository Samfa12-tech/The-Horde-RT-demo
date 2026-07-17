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
- Regenerate the embedded raygen SPIR-V after shader edits with `tools/compile-raygen.ps1`.
- Current phone-safe path tracing is implemented with `rayQueryEXT` in the raygen shader, not recursive closest-hit tracing.

## Working rules

- Preserve actual `vkCmdTraceRaysKHR` presentation through the swapchain.
- Keep `selectedCapabilities_.rtScene.presented` / report `rtScene.presented` honest: only true after an RT-produced frame reaches successful swapchain presentation.
- Prefer small, shippable vertical slices over large speculative rewrites.
- When adding visual/gameplay features, keep a visible phone build runnable at each step.
- Record asset licenses in `ASSET_LICENSES.md` before shipping any imported asset.
- Keep one frame in flight while the held-torch TLAS uses a host-written instance buffer; changing this requires proper per-frame TLAS/instance-buffer ownership.
- The RT storage image is RGBA but is raw-copied to common BGRA swapchains. Preserve the presentation-format-driven `outputRedBlueSwap` push constant or warm fire will render cyan.

## Visual direction

- Historical gothic action demo.
- Start in a dark torch-lit corridor or ruin.
- Prioritize lantern/fire lighting, wet stone, fog, silhouettes, shadows, and obvious RT mood.
- Gameplay comes after the visual RT proof feels like a scene, not a probe.

## Current phone scene controls

- Left-side drag walks/strafs.
- Right-side drag gives 360 camera look.
- Pitch stays clamped; yaw should remain unbounded for 360 look.
- The held torch is a separate low-poly BLAS instance refit into the TLAS from the camera pose each frame. Its emissive flame mesh and direct-light estimate share the same hand-space placement; do not restore the old fullscreen torch overlay.
- The player uses a yaw-relative torso, four IK arm segments, procedural pelvis/legs/boots, gait, head shadow/reflection geometry, and wall-aware held-prop retraction. Body geometry remains selectively masked and the camera origin must stay outside it. See the historical arm foundation in `docs/PLAYER_BODY_RT_SLICE_2026-07-14.md` and final route evidence in `docs/HORDE_SHOWCASE_WINDOWS_VALIDATION_2026-07-16.md`.
- Branded entry/pause/settings surfaces keep diagnostics tucked away unless requested or startup fails, so the app first reads as a game scene.

## Known renderer constraint

- A recursive path-tracing experiment with pipeline recursion depth 2 failed on phone pipeline creation.
- Prefer phone-safe ray-query path tracing inside raygen while keeping `vkCmdTraceRaysKHR` as the frame dispatch/presentation path.
- If trying recursion again, prove capability and pipeline creation on the phone before making it the default.

## Current validated baseline and guardrails

- The combat/ASTC phone gate passed on `SM-S948B`: strict ASTC selection, honest RT swapchain presentation, and two 126-interval samples at 12.500 ms median / 16.667 ms p95. See `docs/COMBAT_ASTC_PHONE_VALIDATION_2026-07-14.md`.
- The articulated grip/pitch revision is phone-verified: strict ASTC selection, honest RT presentation, live grip/swing composition, and sustained warm evidence around 50-52 FPS at thermal status 2. Preserve mask `0x04` culling and the compact material route. See `docs/PLAYER_BODY_RT_SLICE_2026-07-14.md`.
- The complete showcase route is Windows- and Android-device-validated: lower body and lantern drop, bounded skylight/torch/mirror lighting, spatial audio, and sequential skeleton/lich selection. At 75% on `SM-S948B`, every required warm zone retained a median of three 120-frame average windows below 13.7 ms at thermal status 3. Preserve the one-active-skinned-enemy limit until a separate multi-enemy phone measurement. See `docs/HORDE_SHOWCASE_WINDOWS_VALIDATION_2026-07-16.md`, `docs/HORDE_SHOWCASE_ANDROID_VALIDATION_2026-07-17.md`, and `docs/COLOURED_LIGHT_ROUTE_PLAN_2026-07-15.md`.
- Keep the HUD compact or collapsible at large Android accessibility font scales; do not change the user's system font setting.
- Android Debug now has twelve named checkpoints, three-window measurement, a deterministic 13-waypoint replay, and a live-validated evidence runner. Use `tools/run-android-showcase-validation.ps1` after meaningful Android renderer or gameplay-route changes; automation does not replace hands-on touch, perceived audio, or lifecycle checks. See `docs/ANDROID_SHOWCASE_AUTOMATION_2026-07-17.md`.
- Do not run a second concurrent enemy, add broader AI, block/dodge, or unrelated gameplay. Preserve sequential skeleton/lich selection and the one-active-skinned-enemy slot unless a later phone-measured plan explicitly changes that scope.
- Keep the textured sword LOD out of the runtime until the static GLB/PBR path exists and is measured on phone.

## Build notes

- Android debug build from `android/`: `.\gradlew.bat assembleDebug installDebug --console=plain`.
- Standard Android checkpoint/replay gate from repo root: `.\tools\run-android-showcase-validation.ps1`.
- Windows Vulkan SDK was installed at `C:\VulkanSDK\1.4.350.0` during development.
- Shader generation from the repo root: `.\tools\compile-raygen.ps1`.
