# Horde Lantern RT full repository audit and game plan

Date: 2026-08-01
Baseline: `5db3730` / Showcase Alpha `0.1.3-alpha.1`

## Decision

The showcase demo is complete enough to freeze as the visual and release baseline. The next milestone is not "add a horde" directly. It is a short pre-production foundation that removes cross-platform simulation drift, makes enemy instances real data, and preserves the phone-measured RT path.

The engine should remain small and purpose-built. Do not replace it with a general ECS, imported engine, raster fallback, recursive phone path tracer, general fluid simulation, or per-particle BLAS system.

## Audit outcome

The project is unusually well validated for a native RT demo: the published route, deterministic replay, capture set, packaging checks, honest presentation, strict Android ASTC path, and physical-phone evidence form a strong regression baseline. The main risk is architectural concentration rather than missing proof.

The three largest product source files now contain most platform and renderer behavior:

- `src/platform/windows/DiagnosticWindow.cpp`: about 3,900 lines.
- `src/vulkan/raytracing/PresentableTinyRtScene.cpp`: about 3,200 lines.
- `android/app/src/main/cpp/android_probe_bridge.cpp`: about 2,300 lines.

Windows and Android independently implement Vulkan presentation and gameplay orchestration. `PresentableTinyRtScene` independently owns allocation, uploads, procedural world construction, animation, BLAS/TLAS, player IK, shader ABI, dispatch, copy, and capture. These boundaries were appropriate for proving the demo, but they are the wrong seams for a full game.

## Findings

### P0 - resolve before wider source distribution

1. **Raw skeleton redistribution permission remains unresolved.** `ASSET_LICENSES.md:22` and `assets/models/enemies/meshy/skeleton_biped_merged_animations_v01.METADATA.md:39-42` distinguish permitted finished-game use from prohibited standalone redistribution. The raw derivative is nevertheless present in public Git/LFS history while the permission request remains pending. Resolve permission, or replace the asset and plan public-history remediation before wider collaboration. The current licence gate at `tools/run-foundation-validation.ps1:444` verifies manifest presence; it does not fail this unresolved legal state.

### P1 - fix during simulation foundation

1. **Android input has a C++ data race.** JNI writes camera and movement state under `gSwapchainMutex` at `android/app/src/main/cpp/android_probe_bridge.cpp:2316-2321`, while the render thread reads and mutates the same `gSwapchainContext` outside that mutex around `:1974-2006`. Replace direct context mutation with an atomic or double-buffered input mailbox consumed by the simulation thread.

2. **Android movement is frame-rate dependent.** Android advances by `0.032` units per rendered frame at `android_probe_bridge.cpp:1665-1669`; Windows advances by `1.9 * deltaSeconds` at `DiagnosticWindow.cpp:1624-1629`. Both also allow faster diagonal movement. Normalize input and move both platforms through one fixed simulation tick.

3. **The apparent multi-enemy capacity is metadata, not implementation.** `EnemyDirector::Select` always emits one selected/rendered enemy at `src/gameplay/ShowcaseGameplay.h:443-460`. The renderer has one enemy TLAS instance at `PresentableTinyRtScene.cpp:2741-2755`, and the shader couples `instance == 2` to one global enemy kind at `shaders/raytracing/minimal.rgen:233-246`. Do not raise or advertise capacity until stable entity IDs, character render slots, and instance metadata exist.

4. **Gameplay orchestration is duplicated.** Encounter selection, attack handling, damage, death, retry, audio cues, and finale state are separately sequenced at `DiagnosticWindow.cpp:2273-2426` and `android_probe_bridge.cpp:1684-1799`. Extract `GameSimulation::Tick(input, dt) -> snapshot + events`; platform code should own only input collection, UI, audio playback, surfaces, and presentation.

5. **Combat semantics are still one-demo-enemy semantics.** `src/gameplay/SwordCombat.h:31-175` combines the player sword, skeleton movement, attack timing, hit testing, death, and respawn. The lich accepts a distance-only hit on attack request at `ShowcaseGameplay.h:631-655`, rather than at a sword animation event with facing. Split player actions, entity health, enemy actions, navigation, and animation-event hit windows before adding combat clips.

6. **Enemy rendering scales poorly in its current form.** CPU skinning expands the skeleton pose to triangle vertices at `src/scene/SkeletonBipedModel.cpp:639-713`, followed by upload and BLAS refit at roughly 30 Hz in `PresentableTinyRtScene.cpp:2674-2720`. Multiple unique poses would repeat that work per entity. Start with two entities sharing a pose/BLAS, then add bounded pose buckets only when measured.

7. **Android runtime assets will not scale linearly with enemy count.** The current Release APK is about 53.5 MiB, with about 48.4 MiB of compressed assets. The skeleton GLB contributes about 29.7 MiB and the lich about 13.8 MiB. The skeleton contains an unused embedded 4096-square texture and both models retain unused clips. Produce stripped runtime GLBs and enforce per-asset size, triangle, joint, texture, and clip budgets before adding characters.

8. **Git LFS policy and current objects disagreed.** `.gitattributes:1-5` routes GLB, PNG, KTX2, RGBA, and MP4 files through LFS, but the audit found six current Poly Haven array files stored as ordinary Git objects. This pause-point commit normalizes those current files into LFS; rewriting older public history remains a separate decision.

9. **The existing Android haptics path was not perceptible on the primary device.** The prior implementation used only `View.performHapticFeedback` with subtle constants during a 180 ms UI poll. The owner corrected an earlier validation report and reported no perceived haptics on `SM-S948B`. The audit revision adds direct permission-backed vibration for swing, damage, and fatal damage, an enabled-by-default settings toggle with preview, and view haptics as fallback. The exact installed Debug APK passed settings-preview, Swing, real-damage, and fatal-cue checks on `SM-S948B`, and the owner physically confirmed the result; intensity, comfort, and cue distinction remain tuning questions.

### P2 - simplify as adjacent bounded slices

1. **Renderer upload failures were not consistently diagnostic.** Several startup `vkMapMemory` calls previously copied without checking the result. The audit pass replaces them with one checked `WriteBuffer` helper, preserving the unsupported/failure diagnostic policy.

2. **Renderer frame input was an expanding positional API.** `RecordTraceAndCopy` previously accepted a long ordered list of camera, lighting, gameplay, and encounter arguments. The audit pass introduces `RtSceneFrameInputs` as the named platform-to-renderer snapshot. This is the extension seam for character slots and bounded effect records.

3. **Shader/CPU ABI has too many manual mirrors.** Push constants, material IDs, descriptor bindings, instance indices, and masks are repeated between `PresentableTinyRtScene.cpp` and `minimal.rgen`. Generate or validate one shader ABI definition and add reflection/invariant tests before adding effect buffers.

4. **The RT pipeline carries unused shader stages.** Raygen performs traversal exclusively with `rayQueryEXT`, while embedded miss/closest-hit stages and three SBT groups remain. A separately validated raygen-only pipeline can remove them while preserving `vkCmdTraceRaysKHR`, recursion depth 1, and empty unused SBT regions.

5. **Release and asset manifests have multiple authorities.** Version identity is repeated across CMake, Gradle, Windows resources, strings, packaging, and release scripts. Runtime asset lists are repeated across Gradle, packaging, and validation. Introduce one checked-in release manifest and one runtime-asset manifest consumed by every tool.

6. **Windows Release can fall back to the source checkout.** `CMakeLists.txt:91` embeds the source path and `DiagnosticWindow.cpp:433` can use it when packaged assets are missing. Make this fallback Debug-only so an incomplete local package cannot appear healthy.

7. **Automated test coverage stops before the important seams.** The seven host smoke targets cover deterministic logic well, but there is no test target for platform orchestration, Vulkan resource state, shader ABI, Android JNI/Java lifecycle, or manifest generation. There is also no checked-in CI workflow. Shared simulation and resource helpers should be extracted with tests rather than after feature growth.

8. **Active documentation had drift.** `PROJECT_DECISIONS.md:20` described already completed Android overlay and integrated validation work as remaining; this audit corrects that active authority. Dated evidence should stay immutable, while current guidance should remain limited to the README, project memory/decisions, compatibility record, release process, and this roadmap.

9. **Build reproducibility can improve.** The install checklist omits several exact prerequisites, the Gradle wrapper lacks `distributionSha256Sum`, and tool scripts retain machine-specific default paths. Add presets, central tool discovery, a prerequisite check, and a pinned Gradle distribution checksum.

## Simplification sequence

Each step must preserve the published route before the next begins.

1. **Shared simulation seam**
   - Add `InputSnapshot`, `GameSimulation`, `SimulationSnapshot`, and a bounded `GameplayEvent` queue.
   - Use a fixed 60 Hz accumulator with capped catch-up.
   - Move movement, attacks, vitality, encounters, finale, and event emission out of both platform render loops.
   - Keep flat arrays and stable IDs; do not add a general ECS.

2. **Renderer internals, without a renderer rewrite**
   - Extract checked allocation/upload, generic BLAS construction, static-world construction, dynamic-character slots, and frame recording.
   - Persistently map one-frame dynamic buffers; stage immutable world data to device-local memory only after measurement proves benefit.
   - Null borrowed handles/function pointers on destroy and document the existing device-idle lifetime precondition.

3. **Single content authority**
   - Move route bounds, lights, spawn groups, checkpoints, retry locations, and effect volumes into one compile-time content description.
   - Generate/validate CPU and shader identifiers from a shared ABI source.
   - External level tooling remains deferred.

4. **Repository/release simplification**
   - Add release/runtime-asset manifests, LFS integrity checks, runtime asset budgets, prerequisite checks, and a lightweight CI host lane.
   - Keep hardware RT presentation and physical-phone performance outside CI as explicit device gates.

## Full-game roadmap

### Milestone 1 - simulation foundation

Deliver the shared fixed-step simulation, input mailbox, event queue, normalized movement, persistent encounter state, and deterministic integration tests. Preserve every current capture and route outcome.

### Milestone 2 - measured two-enemy combat proof

- Add stable enemy IDs and a fixed-capacity entity array.
- Render two skeleton instances sharing one immutable model and initially one pose/BLAS.
- Add deterministic circle separation, spawn validation, and one attack token so enemies do not strike simultaneously.
- Keep the published default at one active skinned enemy until the dedicated `SM-S948B` gate passes.
- Attempt four only after two passes; do not jump directly to a crowd.

### Milestone 3 - combat animation architecture

- Define clip metadata and action states: locomotion, wind-up, active hit window, recovery, hurt, and death.
- Emit hit events exactly once from animation time; rendering never chooses damage or a winner.
- Give each enemy independent health/action state and persistence.
- Route swing, hit, hurt, parry, heavy impact, and death haptics through the same bounded gameplay-event queue as audio, with platform-specific playback and intensity settings.
- Add bounded pose buckets for visually varied enemies before considering unique per-entity CPU skin/refit.
- Keep block, dodge, broad AI, and the staged textured sword out of this milestone.

### Milestone 4 - fire system

- Add a small CPU-culled emitter buffer to `RtSceneFrameInputs`/the renderer ABI.
- Preserve emissive flame meshes as real TLAS geometry for visibility and reflections.
- Evaluate bounded analytic flame/glow and embers in raygen; do not create/refit a BLAS per particle.
- Importance-select the strongest one or two shadowed direct lights per pixel; secondary fires provide cheaper emissive contribution.
- Add a deterministic `fire-room` checkpoint and capture.

### Milestone 5 - shallow water and steam

- Author actual shallow water geometry/volumes instead of extending the current coordinate-only puddle mask at `minimal.rgen:101-105`.
- Add animated normals, absorption, and bounded reflection/transmission ray queries. No SSR or raster fallback.
- Represent steam as a small set of bounded density volumes integrated in raygen and clipped by primary-hit depth. No fluid simulation.
- Add quality/cost bounds and deterministic `steaming-water` checkpoint evidence.

### Milestone 6 - full-game content layer

Build encounter definitions, spawn groups, progression, save/checkpoint data, additional licensed runtime-stripped enemies, combat content, and level sections on the measured systems. Only at this point should the title stop being treated internally as a diagnostic/showcase shell.

## Acceptance gates

### Every foundation change

- Windows Debug and Release build cleanly; all host CTests pass in both configurations.
- Android Debug and unsigned Release build; `lintRelease` passes.
- `tools/compile-raygen.ps1 -Check` passes after any shader/ABI change.
- The deterministic 12-capture Windows set remains bit-exact unless an intentional visual change is reviewed and re-baselined.
- RT presentation remains honest; `vkCmdTraceRaysKHR`, ray-query traversal, one frame in flight, strict Android ASTC, and raw-copy BGRA compensation remain intact.

### Simulation

- Equivalent movement/combat outcomes under 30, 60, and 120 Hz render delivery and a bounded 100 ms hitch.
- No diagonal speed gain.
- Two identical events in one UI poll interval retain distinct entity IDs and positions.
- Leaving and re-entering a zone cannot heal or reset a living encounter unless authored explicitly.

### Multiple enemies and effects

- Two/four enemies retain independent IDs, health, action, death, and persistence without overlap.
- Animation hit events fire once; off-angle and out-of-window swings miss.
- Android settings expose an enabled-by-default haptics toggle whose activation provides an immediate preview; swing, damage, and fatal cues are distinguishable by hand on the physical device.
- Add `two-enemy-combat`, `fire-room`, and `steaming-water` checkpoints to the Android evidence runner.
- Historical 2026-08-01 proposal: on `SM-S948B` at 75%, discard warm-up, retain three 120-frame windows, require honest presentation every frame, and hard-gate the median below 20 ms. The 2026-08-21 owner decision supersedes that last criterion: 20 ms is now a descriptive 50 FPS reference, sustained thermal/governor context is retained, and matched regressions above 15% are investigated.
- Report 100% separately. Automation does not replace touch, animation readability, spatial audio, lifecycle, or thermal-comfort checks.
- Update `docs/ANDROID_RT_DEVICE_COMPATIBILITY_RECORD.md` whenever new device evidence is collected.

## Validation performed during this audit

- Windows Debug build: pass.
- Windows Debug CTest: 7/7 pass.
- Windows Release build: pass.
- Windows Release CTest: 7/7 pass.
- Deterministic Windows RT capture set: 12/12 generated successfully and all PNG SHA-256 values are bit-exact against the 2026-07-31 pause-point baseline.
- Android Debug build across the configured four ABIs: pass.
- Android unsigned Release build across the configured four ABIs: pass.
- Android `lintRelease`: pass.
- Canonical Host foundation gate, including shader staleness, negative safety gates, packaging, licence checks, and evidence hashes: pass (`run-20260801-175222`).
- Revised-haptics Debug APK installed byte-for-byte on `SM-S948B`, with the vibration permission granted. After unlock, the settings preview, Swing, real skeleton damage, and fatal production paths all produced completed Android vibrator-service effects; `YOU FELL` appeared and the owner confirmed the revised haptic was physically felt. Basic operation passes, while intensity/comfort/cue distinction remain tuning questions.
- `git diff --check`: pass apart from existing line-ending conversion notices.

No runtime visual baseline or shader was changed, so this audit makes no new renderer-visual or performance claim. The compatibility record does add the exact-device haptics evidence described above.
