# Engine contracts for targeted changes

Reference for the subsystem being changed, not a repository-wide reading prerequisite. The root [AGENTS.md](../AGENTS.md) supplies the product and working boundaries. These contracts retain the engineering safeguards previously mixed with release history there; dated reports remain in their original files.

## Renderer and assets

### RT ownership and phone compatibility

- Shared scene: `src/vulkan/raytracing/PresentableTinyRtScene.cpp`. Shaders: `shaders/raytracing/minimal.rgen`, `minimal.rmiss`, and `minimal.rchit`. Use `tools/compile-raygen.ps1` to regenerate embedded SPIR-V after shader changes.
- Keep `vkCmdTraceRaysKHR` as the real frame dispatch/presentation path and keep `selectedCapabilities_.rtScene.presented` / report `rtScene.presented` honest. CPU tests, successful pipeline creation, or an RT dispatch without successful presentation do not establish that a frame was presented.
- The phone-safe shading route uses `rayQueryEXT` inside raygen. A historical recursive depth-2 experiment failed phone pipeline creation; prove capability and pipeline creation on the phone before making recursion the default.
- Keep one frame in flight while the held-torch TLAS uses a host-written instance buffer. Increasing concurrency requires proper per-frame TLAS/instance-buffer ownership, not just a higher frame count.
- Preserve the presentation-format-driven `outputRedBlueSwap` push constant: RGBA RT storage is raw-copied to common BGRA swapchains. Losing the correction can turn warm fire cyan.
- Preserve strict ASTC selection on the phone and real capability diagnostics. A desktop success is not evidence that the Android driver or asset route works.

### Reusable visuals and bounded transport

- Production props use the measured static GLB/PBR route, fixed-capacity instance/material metadata, shared sockets, and immutable BLAS. Do not restore per-object geometry/material branches or fullscreen held-torch overlays.
- Held-prop geometry, emissive flame and direct-light placement must agree in world/hand space. Preserve wall-aware retraction, camera/body separation, selective body masks (including the established `0x04` culling contract), and shadow/reflection participation. The owner-selected block-arm presentation is intentional pending acceptable skinned hands/gauntlets; do not silently replace it as incidental cleanup.
- Use the reusable world-space fire, bounded dielectric glass, held-item sockets and fixed-step lantern-swing systems for extensions. Asset generation alone is not integration: preserve source/runtime separation, textures, licence evidence, budgets, and the measured runtime asset contract in `ASSET_PIPELINE.md` and root `ASSET_LICENSES.md`.
- Water refracted and High-quality reflected opaque hits use the terminal ordinary material/direct-light path, shared active-light selection, and actual visibility for interface highlights. Preserve `gl_RayFlagsNoOpaqueEXT` transparent filtering, accumulated secondary hit distance from the camera, and physical roof traversal for the moon. Do not double-count a transmitted glossy bounce and water reflection, add water-only transport floors, or use screen-position shadow masks. Water-on-water secondary hits terminate without recursion.
- The waterfall width control scales the world-Z cross-lane span; world-X is the thin transmission depth. Shader stream centres/radii must match the instance transform. Resolve the instance from current code rather than treating an old TLAS slot number as permanent.
- Two simultaneous skeletons are a validated milestone ceiling, not a permanent design limit. Do not add a third/fourth enemy in unrelated work. A larger encounter needs a scoped, measured renderer/simulation design and a phone pass; keep the lich singular until that work is authorised. Recheck current source before relying on historical BLAS, TLAS, pose-bucket or test counts.

Useful evidence when changing these paths: `PLAYER_BODY_RT_SLICE_2026-07-14.md`, `WATER_TRANSMISSION_SHADOW_VALIDATION_2026-08-24.md`, and `FIRE_PBR_REWARD_LANTERN_PLAYER_UPGRADE_VALIDATION_2026-08-30.md`. These are historical evidence, not automatic certification of newer code.

## Simulation and feedback

- `src/gameplay/simulation/GameSimulation.cpp` is the shared 60 Hz authority for movement, collision, encounters, combat, vitality, retry, finale and semantic events. It produces immutable `SimulationSnapshot` output consumed through `src/vulkan/raytracing/SimulationFrameAdapter.cpp`; preserve the existing `RtSceneFrameInputs` boundary.
- Android input crosses JNI through `src/gameplay/simulation/InputMailbox.h`. Preserve coherent two-slot publication and monotonic attack/parry/reset/retry counters. A bare atomic published index is insufficient: a writer can lap a reader and overwrite its slot. Do not restore direct JNI writes into render-thread gameplay state.
- Platform audio/haptics drain ordered `GameplayEvent` records. Preserve Android's fixed 128-entry transport, which drops only the newest event on overflow, and Windows' fixed-capacity delayed-fall queue. One-bit-per-sound polling loses repeated same-type events.
- Preserve event-time source/listener data and identities. Nonfatal accepted hits emit `PlayerDamaged`; the lethal hit emits only `PlayerKilled`. Both platforms retain 140 ms separation between positional skeleton impact and fall audio. Reset, retry and Android lifecycle transitions cancel stale delayed fall cues.
- Deterministic captures import exact authored checkpoint state and then freeze simulation. Preserve zero-delta skeleton/lich snapshot finalisation, including the historical 0.1.3 capture contracts; do not substitute ordinary live simulation advancement to make a capture pass.
- Owner audio/haptic checks are change-triggered. Use the [manual-validation boundary](AGENT_VALIDATION.md#manual-audio-and-haptic-validation) when semantics or playback can change; an unrelated renderer or documentation change is not itself a reason to require owner sign-off.

For relevant background, consult `SHARED_SIMULATION_FOUNDATION_2026-08-10.md` or the affected sections of root `PROJECT_DECISIONS.md`; do not load all historical combat and validation reports by default.

## Platform interaction

- Android entrypoint: `android/app/src/main/java/com/samfa12/hordelanternrt/MainActivity.java`; native bridge: `android/app/src/main/cpp/android_probe_bridge.cpp`. Windows diagnostic/platform path: `src/platform/windows/DiagnosticWindow.cpp`.
- Preserve left-side movement, right-side 360-degree look, clamped pitch and unbounded yaw. Keep entry/pause/settings game-facing; expose diagnostics on request or startup failure rather than replacing the normal experience with a probe UI.
- Keep the Android HUD compact/collapsible at large accessibility font scales; do not alter the user's system font setting.
- While the post-lich Android RT Lab is open, `showEndingOverlay()` stays gated by `rtLabVisible`; the repeated finale poll must not replace the lab. Closing the lab clears the flag deliberately before restoring the ending card.
- Windows RT Lab trackbars stay opaque native controls, repaint after scroll hide/show, and forward wheel input to the lab's vertical scroll owner. Preserve the existing regression contracts for these behaviours.
- Automated capture evidence does not replace touch feel, perceived audio/haptics or actual lifecycle checks. Report missing evidence precisely instead of inferring it from a different platform or older build.

## 1.6.1 integration contracts (development)

- Shipping consumes its immutable compiled Mobile/High strategy pair and performs no Diagnostic buffer IO; final Shipping SPIR-V must remain free of diagnostic atomics and binding 22. Policy selection must not become a nonphysical glass or fake-RT fallback.
- Frame evidence joins recorded CPU/resource/strategy facts with the exact successful submission and its owning fence/final successful idle. Initial signalled fences own no submission. Never read Diagnostic buffers after failed idle or label prior GPU/counter results as the current recorded frame.
- Optional telemetry identity, clock or timestamp failure must not stop otherwise valid rendering. Preserve independent real-graphics fence ownership for tokenless drains, explicit unavailable/error evidence, and monotonic identity seed floors through Android moves/restarts. No second submission counter or reused serials.
- Resource inventory records actual live allocation sizes and selected memory flags. Host-visible and device-local byte classifications can overlap on UMA; they are not additive totals. CPU command-record timings are not GPU execution timings.
- The audit authorizes dedicated native-RT PlayerWorldBody/PlayerViewmodel ownership, not switching the full body into primary rays or overlay arms. Keep the current production presentation until the dedicated modelled route passes its acceptance gates; do not describe the partial telemetry work as completed viewmodel integration.
- The unresolved Hotstrike-derived skeleton redistribution issue remains owner-controlled. Do not remove/replace the shipped skeleton, rewrite history or licensing statements, or change distribution as a workaround.
