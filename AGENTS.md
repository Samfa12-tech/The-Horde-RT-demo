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
- Shared gameplay authority: `src/gameplay/simulation/GameSimulation.cpp` with immutable `SimulationSnapshot` output.
- Shared renderer adapter: `src/vulkan/raytracing/SimulationFrameAdapter.cpp`.
- Android continuous and edge input crosses JNI through `src/gameplay/simulation/InputMailbox.h`; do not restore direct JNI mutation of render-thread gameplay state.
- RT shaders: `shaders/raytracing/minimal.rgen`, `minimal.rmiss`, `minimal.rchit`.
- Regenerate the embedded raygen SPIR-V after shader edits with `tools/compile-raygen.ps1`.
- Current phone-safe path tracing is implemented with `rayQueryEXT` in the raygen shader, not recursive closest-hit tracing.

## Working rules

- Android device evidence maintenance: whenever new Android device evidence appears (local validation, capability report, logcat, screenshot, or user report), update `docs/ANDROID_RT_DEVICE_COMPATIBILITY_RECORD.md` in the same task. Preserve the exact model code and classify the evidence type. Do not mark devices as working from SoC/vendor claims alone, and do not overwrite prior evidence.

- Preserve actual `vkCmdTraceRaysKHR` presentation through the swapchain.
- Keep `selectedCapabilities_.rtScene.presented` / report `rtScene.presented` honest: only true after an RT-produced frame reaches successful swapchain presentation.
- Prefer small, shippable vertical slices over large speculative rewrites.
- When adding visual/gameplay features, keep a visible phone build runnable at each step.
- Record asset licenses in `ASSET_LICENSES.md` before shipping any imported asset.
- Keep one frame in flight while the held-torch TLAS uses a host-written instance buffer; changing this requires proper per-frame TLAS/instance-buffer ownership.
- The RT storage image is RGBA but is raw-copied to common BGRA swapchains. Preserve the presentation-format-driven `outputRedBlueSwap` push constant or warm fire will render cyan.
- Keep gameplay on the existing owning application/render thread. Movement, collision, encounters, combat, vitality, retry, finale, and semantic events belong to the shared 60 Hz `GameSimulation`, not platform loops.
- Android input publications use a coherent two-slot mailbox with monotonic attack/reset/retry counters. A bare atomic published index is insufficient because a writer may lap a reader and overwrite its slot.
- Platform audio and haptics drain ordered `GameplayEvent` records. Do not restore one-bit-per-sound polling that collapses repeated same-type events.
- Preserve feedback semantics: nonfatal accepted hits emit `PlayerDamaged`, the lethal hit emits only `PlayerKilled`, and Android keeps the 140 ms separation between skeleton impact and fall audio.
- Deterministic captures import exact authored checkpoint state and then freeze simulation. Preserve the zero-delta skeleton/lich snapshot finalization required by the 0.1.3 hashes.

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
- The published complete showcase route remains Windows- and Android-device-validated at one active skinned enemy. At 75% on `SM-S948B`, every required warm zone retained a median of three 120-frame average windows below 13.7 ms at thermal status 3. The development candidate now supports exactly two skeletons, but it is not the new phone-performance baseline until its separate six-checkpoint device gate passes. See `docs/HORDE_SHOWCASE_WINDOWS_VALIDATION_2026-07-16.md`, `docs/HORDE_SHOWCASE_ANDROID_VALIDATION_2026-07-17.md`, and `docs/TWO_SKELETON_COMBAT_SLICE_2026-08-11.md`.
- Keep the HUD compact or collapsible at large Android accessibility font scales; do not change the user's system font setting.
- Android Debug now has thirteen named checkpoints, three-window measurement, a deterministic 13-waypoint replay, and an evidence runner whose default 75% gate includes `two-enemy-combat`. Use `tools/run-android-showcase-validation.ps1` after meaningful Android renderer or gameplay-route changes; automation does not replace hands-on touch, perceived audio, or lifecycle checks. See `docs/ANDROID_SHOWCASE_AUTOMATION_2026-07-17.md`.
- After the simulation/renderer foundation merged, the owner reported a hands-on pass for basic controls, audible audio, and perceived haptics on an installed development build on `SM-S948B`; no exact APK or merged-build provenance was captured. The separate exact automated development candidate still failed the 20 ms lich gate at 23.604 ms, so do not describe current development `main` as a full phone-performance pass.
- Two simultaneous skeletons are the hard development ceiling. Do not add a third/fourth enemy, broader AI, block/dodge, or unrelated gameplay. Keep the lich singular and do not promote the two-skeleton candidate to the playable phone baseline without its matched `SM-S948B` gate.
- Keep the textured sword LOD out of the runtime until the static GLB/PBR path exists and is measured on phone.

## Build notes

- Android debug build from `android/`: `.\gradlew.bat assembleDebug installDebug --console=plain`.
- Standard Android checkpoint/replay gate from repo root: `.\tools\run-android-showcase-validation.ps1`.
- Windows Vulkan SDK was installed at `C:\VulkanSDK\1.4.350.0` during development.
- Shader generation from the repo root: `.\tools\compile-raygen.ps1`.
- Additive Windows configure/build/test presets are in `CMakePresets.json`.
- `.github/workflows/shared-simulation-host.yml` exercises non-hardware shared gameplay tests only; it does not prove Vulkan RT presentation or phone behavior.
