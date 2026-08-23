# Unlockable RT Lab Implementation Plan

> **For agentic workers:** REQUIRED SUB-SKILL: Use superpowers:subagent-driven-development to implement this plan task-by-task. Every production change follows red-green-refactor.

**Goal:** Add a permanent post-lich RT Lab on Windows and Android whose route-local controls modify real Vulkan RT geometry, lighting, atmosphere, and bounded secondary-ray work in real time.

**Architecture:** Shared renderer tuning types carry clamped values through `SimulationFrameAdapter` without changing gameplay authority. Falling waterfall streams become one independently transformed BLAS/TLAS instance; the finale roof reuses its existing instance. Platform layers persist only the unlock and own non-persistent UI tuning.

**Tech Stack:** C++20, Vulkan ray-tracing pipeline and `rayQueryEXT`, GLSL/SPIR-V, Win32 controls, Android Java/JNI, CMake/CTest, Gradle.

## Global Constraints

- Preserve native `vkCmdTraceRaysKHR`, phone-safe `rayQueryEXT`, strict ASTC selection, honest `rtScene.presented`, one frame in flight, and presentation-driven red/blue swap.
- Shared 60 Hz `GameSimulation` remains the sole gameplay authority; tuning never changes collision, encounters, combat, events, or immutable simulation snapshots.
- Authored defaults reproduce the existing behavior: waterfall 100%, no roof/dawn override, fog 100%, hue 0 degrees, light intensity 100%, workload Authored.
- Clamp waterfall width to 25-200%, roof/dawn to 0-100%, fog and light intensity to 0-200%, and hue shift to -180 through +180 degrees.
- Unlock persists only after genuine finale completion; checkpoints, benchmark/capture automation, and debug injection never save it.
- Unlock survives route restart, Begin Again, and ordinary settings reset. Tuning survives closing the panel but resets on route restart and process launch.
- Opening RT Lab pauses simulation while RT frames and telemetry continue.
- Do not change audio, haptic, event timing, listener/source semantics, versions, packaging, publication, or deployment.

---

### Task 1: Shared RT tuning contract

**Files:**
- Create `src/vulkan/raytracing/RtSceneTuning.h` for enums, defaults, clamping, override resolution, and light groups.
- Modify `src/vulkan/raytracing/SimulationFrameAdapter.h/.cpp` and `PresentableTinyRtScene.h/.cpp`.
- Modify `tests/CharacterRenderSlotSmoke.cpp` for behavior-level tuning and adapter contracts.

**Produces:** `RtWorkloadPreset { Lean, Authored, Max }`, `RtLightGroup { Torch, Skylight, Passage, Staff }`, `RtLightTuning`, `RtSceneTuning`, `ClampRtSceneTuning`, and an adapter overload accepting tuning.

- [ ] Add focused tests that fail because the tuning API does not exist; cover authored defaults, every lower/upper clamp, independent light groups, and optional roof/dawn resolution without mutating the source snapshot.
- [ ] Implement the smallest header and adapter changes that satisfy those tests.
- [ ] Append eleven tuning floats after `waterQuality` in CPU and GLSL push constants, preserving every released offset and setting the packed ABI to 120 bytes.
- [ ] Update ABI tests and verify focused tests pass before running the full Debug CTest suite.
- [ ] Commit the shared contract and tests.

### Task 2: Truthful waterfall, lighting, atmosphere, and workload

**Files:**
- Modify `src/vulkan/raytracing/PresentableTinyRtScene.h/.cpp`.
- Modify `shaders/raytracing/minimal.rgen` and regenerate `MinimalRayGenShader.inc`.
- Modify `tests/CharacterRenderSlotSmoke.cpp` for observable renderer/shader contracts.

**Produces:** A tenth BLAS and twentieth TLAS instance for local-space falling streams; matching shader water classification/profile; centralized light tuning; fog scaling; deterministic Lean/Authored/Max branches.

- [ ] Add failing tests for 10/20 resource reporting, the dedicated waterfall instance and transparent-water path, 120-byte append-only ABI, and deterministic 2/6/8 workload behavior.
- [ ] Move only the three falling streams to a local-space waterfall BLAS. Keep catchment, runnel, drain, roof slot, and collision in the static world. Scale only the lateral waterfall axis around the authored centre with a non-degenerate 0.25-2.0 transform.
- [ ] Treat the new instance as water in primary, ignore-water, visibility, normal/profile, reflection, and refraction paths; make shader profile centres/radii track the same width scale.
- [ ] Resolve the roof transform and shader aperture from the same effective tuned value.
- [ ] Centralize zero-preserving hue rotation and intensity scaling for Torch, Skylight/Dawn, Passage, and Staff/Mist, plus 0-2 fog density scaling.
- [ ] Implement explicit unrolled workload branches: Lean has no general bounce and two mist samples; Authored keeps current bounce/visibility and six samples; Max evaluates both local and skylight visibility targets, keeps the bounce, and uses eight samples. Water quality remains independent.
- [ ] Compile raygen, regenerate the embedded include, run `tools/compile-raygen.ps1 -Check`, then focused and full Debug tests.
- [ ] Commit the renderer/shader slice.

### Task 3: Android permanent unlock and live RT Lab

**Files:**
- Modify `android/app/src/main/java/com/samfa12/hordelanternrt/MainActivity.java`, `ProbeBridge.java`, Android strings, and `android_probe_bridge.cpp`.
- Modify host smoke coverage where native tuning publication can be exercised without Android framework mocks.

**Produces:** Persistent `rt_lab_unlocked`, mutex-protected native tuning publication, typed JNI scene/light/workload/reset methods, ending and pause entry points, live panel, and non-persistent Debug injection.

- [ ] Add failing native tests for coherent tuning publication/reset and genuine-completion-only persistence decisions where platform-independent behavior can be extracted cleanly.
- [ ] Persist the unlock only when the real finale reaches Complete outside debug automation, benchmark, and capture paths.
- [ ] Preserve the unlock when Reset Defaults clears ordinary settings; clear only tuning on route reset/relaunch.
- [ ] Add the ending unlock card and later pause-menu entry. RT Lab hides gameplay controls, pauses simulation, keeps rendering, and restores the previous menu/game state on Back.
- [ ] Add scrollable 48dp controls for four scene sliders, light group plus hue/intensity, three workload presets, Restore Authored, Back, and a roughly 250 ms GPU/render-scale/water-quality readout.
- [ ] Add typed JNI methods backed by one mutex-protected tuning snapshot. Debug intent values may set non-persistent tuning/unlock for validation but must never write the progress preference.
- [ ] Build Debug/Release and lint, exercise the focused tests, and commit Android integration.

### Task 4: Windows permanent unlock and live RT Lab

**Files:**
- Modify `src/platform/windows/DiagnosticWindow.cpp` and focused desktop-controller tests.

**Produces:** INI progress persistence, ending/pause integration, Win32 live controls, controller/keyboard navigation, telemetry, and non-persistent Debug injection.

- [ ] Add failing tests for slider stepping/focus behavior and extract any small pure unlock/settings decision needed for behavior-level coverage.
- [ ] Load/save `[progress] rtLabUnlocked=1` independently of ordinary settings; save only after real non-capture finale completion.
- [ ] Add the ending unlock actions and pause-menu entry, pausing simulation while the renderer continues.
- [ ] Add scene, light, workload, telemetry, Restore Authored, and Back controls with tab order, arrow keys, D-pad adjustment, and the existing gold focus outline.
- [ ] Reset tuning on route restart/process launch while retaining the unlock; add non-persistent Debug launch injection for repeatable captures.
- [ ] Run focused and full Debug tests, launch the exact Debug executable for interactive UI/renderer smoke, and commit Windows integration.

### Task 5: Cross-platform verification and evidence

**Files:**
- Modify automation only where required to inject non-persistent Authored/Max profiles.
- Update `README.md`, platform/gameplay documentation, `docs/ANDROID_RT_DEVICE_COMPATIBILITY_RECORD.md`, and add `docs/RT_LAB_VALIDATION_2026-08-23.md`.

**Produces:** Fresh host, Windows visual, Android build, and exact-phone evidence for the final candidate; no package or deployment.

- [ ] Run shader staleness check, clean Windows Debug/Release builds and CTests, deterministic authored captures, Android Debug/Release builds and lint, and `tools/run-foundation-validation.ps1 -Mode Host`.
- [ ] Verify fresh-profile lock, genuine finale unlock, relaunch persistence, Begin Again persistence, settings-reset preservation, route tuning reset, paused-simulation/live-render behavior, width extremes, roof extremes, isolated light groups, and all workload presets.
- [ ] Confirm exactly one authorized `SM-S948B`, then run `tools/run-android-showcase-validation.ps1` with authored standard evidence plus matched Authored/Max waterfall, skylight, and finale measurements. Preserve resolution/RT settings and report thermal context and reference bands.
- [ ] Inspect Windows and Android captures for water/refraction continuity, roof occlusion/light response, colour isolation, panel readability, and honest RT presentation.
- [ ] Record exact commands, hashes, evidence boundaries, and device details. Mark `Audio/haptic manual revalidation required: NO` with the unchanged-semantics reason.
- [ ] Run final whole-branch review and commit documentation/evidence. Do not version, package, publish, or deploy.
