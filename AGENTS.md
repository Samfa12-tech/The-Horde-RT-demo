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
- Android input publications use a coherent two-slot mailbox with monotonic attack/parry/reset/retry counters. A bare atomic published index is insufficient because a writer may lap a reader and overwrite its slot.
- Platform audio and haptics drain ordered `GameplayEvent` records. Android uses a fixed 128-entry transport that drops only the newest event on overflow; Windows retains delayed fall events in a fixed-capacity queue. Do not restore one-bit-per-sound polling that collapses repeated same-type events.
- Preserve feedback semantics: nonfatal accepted hits emit `PlayerDamaged`, the lethal hit emits only `PlayerKilled`, and both platforms keep the 140 ms separation between positional skeleton impact and fall audio. Reset, retry, and Android lifecycle transitions cancel stale delayed fall cues.
- Deterministic captures import exact authored checkpoint state and then freeze simulation. Preserve the zero-delta skeleton/lich snapshot finalization required by the 0.1.3 hashes.
- Manual owner audio/haptic validation is change-triggered, not milestone-triggered. Every milestone must explicitly state `Audio/haptic manual revalidation required: YES/NO` and why; default **NO**. Require **YES** only for changes that can affect listener/source event-time data or identity, spatialisation/attenuation/pan/obstruction, playback backend/gain/cues/assets, event transport/timing, haptic routing/cues/patterns/intensity, or player damage/death feedback. Unrelated RT, visual, UI, asset, build, packaging, telemetry, documentation, unrelated AI, or unrelated animation changes do not trigger an owner check when automated contracts pass and semantic inputs are unchanged. Current reconciliation was **YES** because listener-at-event-time routing and platform feedback transport/timing changed; the exact-candidate owner check passed, so future work returns to the normal change-trigger rule. See `docs/OWNER_RELEASE_SAFETY_CHECKLIST.md` for separate owner-only signing recovery work.

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
- Showcase Alpha 0.1.5 publishes the accepted RT waterfall/catchment/drain, lich mist, positional water loop, shared directional dodge, and Windows controller path on top of the bounded two-skeleton/parry runtime. Source `172bb0a` passed a fresh 13/13 Debug and Release Host gate plus the recovered connected-phone replay/capture/Home-resume gate. Windows itch build `#1908330` launched from exact ZIP SHA-256 `631b9f01a4d348e18733c989ebacc9c32ce9005ac9498e49e8757a0a36411166`; Android build `#1908331` uses the established certificate and exact APK SHA-256 `1e81238a6e1b0e934c50eb15e80fc8efd39c06f16ca8960c428b22e5f5d5a7f2`, byte-matched after installation on `SM-S948B` with strict ASTC and honest Home/resume presentation. See `docs/RT_WATERFALL_LICH_MIST_VALIDATION_2026-08-23.md` and `docs/SHOWCASE_ALPHA_0_1_5_RELEASE_VALIDATION_2026-08-23.md`.
- Current development water uses the terminal ordinary opaque material/direct-light path for refracted and High-quality reflected hits, with shared active-light selection and real visibility for interface highlights. Transparent filtering uses `gl_RayFlagsNoOpaqueEXT`; secondary hit distance is accumulated from the camera; the ordinary moon traverses physical roof geometry; and no transmitted glossy bounce may double-count the water reflection. Fixed water-only transport floors and screen-position shadow masks are prohibited. Water-on-water secondary hits terminate without recursion. Current deterministic Windows and exact `SM-S948B` evidence are in `docs/WATER_TRANSMISSION_SHADOW_VALIDATION_2026-08-24.md`; preserve the documented bounded RT cost rather than lowering quality or resolution to hide it.
- When the post-lich Android RT Lab is open, `showEndingOverlay()` must remain gated by `rtLabVisible`. The completed-finale poll runs repeatedly and otherwise replaces the lab immediately. Closing the lab deliberately clears that flag before restoring the ending card. On Windows, RT Lab trackbars must remain opaque native controls, repaint after scroll hide/show, and forward wheel input to the lab's vertical scroll owner. Waterfall width scales the world-Z cross-lane span of TLAS instance 19; world-X remains the thin transmission depth, and shader stream centres/radii must match that transform. Preserve the RED/GREEN contracts. Host run `run-20260825-070928` passes and the owner accepted Windows scroll/repaint and width behavior; exact fixed-APK phone retest remains pending.
- Keep the HUD compact or collapsible at large Android accessibility font scales; do not change the user's system font setting.
- Android Debug now has thirteen named checkpoints, three-window measurement, a deterministic 13-waypoint replay, and an evidence runner whose standard 75% route includes `two-enemy-combat`. Use `tools/run-android-showcase-validation.ps1` after meaningful Android renderer or gameplay-route changes; automation does not replace hands-on touch, perceived audio, or lifecycle checks. See `docs/ANDROID_SHOWCASE_AUTOMATION_2026-07-17.md`.
- Performance is evidence, not a single hard frame-time gate. Report median-derived bands at the 16.667 ms (60 FPS), 20.000 ms (50 FPS), and 33.333 ms (30 FPS) reference lines, plus checkpoint order, temperature, Android thermal status, and GPU thermal power level when available. Crossing 20 ms does not by itself fail a candidate. Investigate matched regressions above 15%, growing memory/resource use, or unexplained workload changes before accepting them; never weaken RT or silently lower resolution to manufacture a pass.
- Sustained exact-APK diagnostics on `SM-S948B` held graphics allocation, native heap, PSS/RSS, and thread counts essentially flat while the GPU fell from 1300 MHz to 578-646 MHz at Samsung GPU thermal power level 7. Treat fresh-process speedups and cooled runs as useful context, but use sustained warm behavior as the primary player-facing report.
- Two simultaneous skeletons are the current validated milestone ceiling, not a permanent game-design limit. Do not add a third/fourth enemy inside unrelated work; a future four/five-enemy slice requires an explicit measured renderer/simulation design and phone pass. Keep the lich singular until that work is authorized.
- Current Vulkan-enabled host configurations have thirteen CTests. Latest RT Lab usability/width Host run `run-20260825-070928` passed fresh Debug/Release 13/13 CTests, 13 Windows captures, Android Debug/unsigned Release/lint, packaging/licence, shader freshness, and evidence hashing. Exact current-water `SM-S948B` runs `run-20260825-041641` and `run-20260825-042147` passed artifact parity, focused timing, replay/captures/Home-resume, and the repeated performance investigation. The later RT Lab overlay-ownership and cross-platform waterfall-width fixes are host/Android-build verified but still need an exact fixed-APK phone retest. The portable CI lane has nine Vulkan-disabled tests and does not prove hardware RT.
- Keep the textured sword LOD out of the runtime until the static GLB/PBR path exists and is measured on phone.

## Build notes

- Android debug build from `android/`: `.\gradlew.bat assembleDebug installDebug --console=plain`.
- Standard Android checkpoint/replay gate from repo root: `.\tools\run-android-showcase-validation.ps1`.
- Windows Vulkan SDK was installed at `C:\VulkanSDK\1.4.350.0` during development.
- Shader generation from the repo root: `.\tools\compile-raygen.ps1`.
- Additive Windows configure/build/test presets are in `CMakePresets.json`.
- `.github/workflows/shared-simulation-host.yml` exercises non-hardware shared gameplay tests only; it does not prove Vulkan RT presentation or phone behavior.
