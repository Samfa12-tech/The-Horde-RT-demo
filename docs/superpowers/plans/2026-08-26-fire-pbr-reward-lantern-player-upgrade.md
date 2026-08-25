# Fire, PBR Props, Reward Lantern, Glass, Physical Carry, and Player Upgrade Implementation Plan

> **For agentic workers:** REQUIRED SUB-SKILL: Use `superpowers:subagent-driven-development` (recommended) or `superpowers:executing-plans` to implement this plan task-by-task. Every production change follows red-green-refactor, ends in a runnable Windows and Android state, and stops at its owner approval gate.

**Goal:** Deliver reusable static GLB/PBR, held-item/socket, fire-emitter, skinned-player, dielectric, interaction/reward, and deterministic secondary-motion systems, proven by a production sword, torch, Gothic reward lantern, chest, and first-person-compatible player.

**Architecture:** `GameSimulation` remains the only authority for commands, held-light state, finale/reward progression, player action state, fire phase, and lantern motion. A pinned narrow glTF reader and checked sidecar/runtime manifest feed bounded static/skinned asset records; a generated CPU/GLSL ABI maps existing and new TLAS instances to generic materials and effects without object-named shader branches. The renderer keeps one frame in flight, real `vkCmdTraceRaysKHR` presentation, ray-query traversal, strict Android ASTC, and the current 20-instance TLAS capacity by reusing retired procedural-player slots.

**Tech Stack:** C++20, Vulkan 1.2+ hardware ray tracing, `VK_KHR_ray_tracing_pipeline`, `VK_KHR_ray_query`, GLSL/SPIR-V, pinned `cgltf`, CMake/CTest, PowerShell asset tools, KTX2/ASTC, Android Java/JNI/Gradle, Win32, Meshy 7/Smart Topology, Blender/FFmpeg/shared game tooling where relevant.

**Spec:** Owner request dated 2026-08-26 plus `C:\Users\sam_s\Downloads\Horde_Lantern_RT_Next_Update_Package.zip`. The ZIP is supporting design input; current repository authority and this fresh reconciliation govern execution.

## Global Constraints

- Base implementation work from clean `main` commit `b45a1b71aa9b74178df72c8060f44b46b04a46b4` unless the owner explicitly authorizes rebasing to a later commit.
- Preserve production `vkCmdTraceRaysKHR`, raygen `rayQueryEXT`, recursion depth 1, strict Android ASTC, honest `rtScene.presented`, one frame in flight, and presentation-format red/blue compensation.
- Keep Android first-class and Windows an equal renderer/interaction validation target.
- Keep gameplay and physical-motion authority in the shared fixed 60 Hz `GameSimulation`; platform layers publish coherent input and consume immutable snapshots/events only.
- Preserve the coherent two-slot Android mailbox, monotonic edge commands, bounded ordered gameplay events, 140 ms impact/fall separation, and current damage/death feedback semantics.
- Preserve the two-skeleton/two-pose-bucket ceiling and singular lich. Do not add enemies, a general ECS, a general physics engine, raster fallback, screen-space fire/refraction, or per-particle BLAS.
- Keep the current 120-byte append-only push-constant ABI stable until an explicit ABI task changes it; append descriptor bindings and validate CPU/GLSL layout rather than repurposing existing bindings.
- Keep `kTlasInstanceCount == 20` in the first implementation. Reuse procedural-player slots only after their old path is disabled for that frame and retire their shader branches before assigning new content semantics.
- Source/high assets remain in Git LFS. Only validated, measured runtime GLBs, manifests, KTX2 textures, and licences enter Windows/Android packages.
- No Meshy key, bearer token, signed URL, signing secret, or private credential may enter output, logs, reports, Git, or chat.
- Meshy use and credits are owner-approved for this project. Model selection is autonomous, but task IDs, exact prompts/settings/costs, hashes, account/licence evidence, and accept/reject decisions are recorded.
- Do not version, sign, upload, publish, deploy, rewrite Git history, alter Android identity, or resolve GitHub issue #13 without a separate explicit owner action.
- Every milestone records exact commands/results, shader/resource counts, performance evidence boundaries, known limitations, and the exact audio/haptic classification string.

---

## Fresh Audit Record

### Authority and baseline

- `git pull --ff-only origin main` reported already up to date.
- Base commit: `b45a1b71aa9b74178df72c8060f44b46b04a46b4` (`docs: reconcile Showcase Alpha 1.5.2 authority`).
- The worktree was clean before the baseline and remained clean after it.
- Fresh unchanged Host gate: `reports/foundation-runs/run-20260826-060353/`.
- Host result: PASS; Debug 13/13 CTests, Release 13/13 CTests, 13 deterministic Windows RT captures, Android Debug, unsigned Release, `lintRelease`, shader freshness/negative gates, package/licence checks, and evidence hashes all passed.
- Raygen baseline: source SHA-256 `99c09cb56cbd7411594138104bc042a628e727ee543effc80c20b09776012e11`; compiled SPIR-V SHA-256 `157c82d8123adae6d14f80866dd2f3e66d4f2527753a04ae94810e06837af192`; 405,728 bytes, 101,432 words, 22,983 instructions, 3,520 branch operations, 24 loops, 1,456 selection merges.
- Register pressure is not numerically available because NVIDIA Nsight is not installed. Shader structure, exact size/hash, Windows timing, and exact-phone matched timing remain the evidence route.
- Existing unresolved evidence boundary: the signed 1.5.2 APK and the later RT Lab overlay/waterfall-width fixes still lack an exact fixed-APK `SM-S948B` retest.

### Current architecture

- `LanternSequence` actually owns the original torch drench/drop: Held -> Guttering -> Falling -> Settled. The renderer already calls the object a torch.
- Lich defeat currently owns the entire finale clock: approximately 2.967 s death, 4.50 s roof opening, and 1.75 s dawn reveal; `FinaleCompleted` is emitted once at the end.
- Shared commands are attack, parry, dodge, route reset, and retry. There is no Interact or held-light pose command.
- Checkpoints import exact authored state, zero-delta-finalize lich/skeleton snapshots, and freeze without advancing tick zero.
- Only RT Lab unlock and platform settings persist. There is no gameplay/profile inventory save.
- Renderer resources are 10 BLAS, 1 TLAS, and 20 physical instances. Instance custom indices 0-19 currently encode object identity and shader special cases.
- Descriptor bindings 0-10 are occupied. The push constant is exactly 30 floats/120 bytes.
- Torch, sword, procedural body/limbs/head, roof, characters, and water are decoded by object/instance-specific branches in `minimal.rgen`.
- Clear glass is a thin parallel-transmission proof, not closed-volume dielectric transport. Water has bounded real entry/exit, terminal opaque shading, reflection, and visibility but remains geometry-specific.
- `SkeletonBipedModel` contains a bespoke JSON/GLB reader restricted to one mesh, one primitive, one skin, narrow accessor types, and four semantic clips. `SkinnedCharacterModel` is currently only an alias.
- The staged sword LOD1 is 12,358 triangles with four embedded 2K maps; it is suitable as the first loader proof but cannot ship until account-plan/provenance acceptance is resolved.
- The staged torch LOD1 is 1,538 triangles but lacks a complete PBR material; it is a silhouette/reference candidate, not a production asset.
- GitHub issue #13 tracks unresolved public-source redistribution of the Hotstrike skeleton derivative. It remains out of scope; there are no open pull requests.

### Reference-image interpretation

- The lantern target is the compact central hanging object only: blackened Gothic metal, pointed canopy, deliberate ogee/quatrefoil tracery, compact ring/hinge, warm glass/fire, and pointed lower finial.
- The surrounding architectural arch, floor/background, and projected amber beams are not lantern geometry or texture content.
- The supplied Android image is a historical problem statement: oversized HUD, simple held-prop presentation, and limited character/hand fidelity. Current source/build output, not that screenshot, is the regression authority.

### Current external-contract corrections

- Meshy currently exposes `meshy-7`/`latest`, optional Ultra preview for 5 additional credits, and `model_type: smart-topology` with `ai_model: meshy-t2`; Smart Topology is triangle-only with a 100-15,000 face target. The old low-poly mode is deprecated.
- Meshy 7 PBR refine does not produce an emission map, and its `remove_lighting` parameter is accepted but ignored. Emissive masks/cores and no-baked-lighting acceptance must therefore be created/verified locally rather than assumed from API flags.
- `KHR_materials_volume` requires closed/manifold geometry for non-zero thickness. A ray tracer should use the actual traced entry/exit distance rather than a baked thickness texture.
- `KHR_materials_ior` defaults to 1.5; ordinary window glass is approximately 1.52. Transmission is the fraction of non-specularly-reflected light that passes through the surface.
- `VK_KHR_ray_query` is explicitly complementary to `VK_KHR_ray_tracing_pipeline`, matching the current phone-safe design.
- Recommended parser adjustment: pin MIT `cgltf` at commit `85cd62382dfea638278962690cf515023f33ed00`. It is a single-file, dependency-free glTF 2.0 reader that already exposes the required core data and material extensions. This is smaller and safer than expanding the current home-grown JSON parser into a second incomplete glTF implementation.

## Owner Defaults Proposed for Approval

1. Reward state is route-local; no permanent profile lantern unlock in this programme.
2. Lich defeat unlocks the chest, but roof/dawn/finale completion waits until the lantern is claimed and its high-pose reveal completes.
3. Chest open and lantern claim are two deliberate Interact presses: first open, then claim after the lid reaches Open.
4. Controls: Windows `E` / controller `A` / contextual Android `INTERACT`; Windows `F` / controller `Y` / contextual Android `RAISE` or `LOWER` for held-light pose. Menu meanings remain unchanged because gameplay and menu contexts are disjoint.
5. Use one full-body first-person-compatible skinned player with primary-ray head hiding/masking and real shadow/reflection participation. Do not create separate viewmodel and shadow-body rigs.
6. Use the staged sword for loader proof. Retain it for production only if licence/account evidence, silhouette, PBR, grip, and phone budget pass; otherwise regenerate under the current documented Meshy account.
7. Use the staged torch only as a silhouette/reference. Default to Meshy 7 source plus reviewed Smart Topology/remesh/retexture because the staged runtime mesh lacks complete PBR.
8. Initial mobile static-prop profile: 1K mipmapped ASTC (6x6 base/ORM/emissive, 4x4 normal), 16 layers per texture class, 32 materials, 32 primitives, 8 static assets, 4 fire emitters with at most 2 active per frame. Source textures may be 2K/4K; increasing the mobile profile requires a measured phone/art review.
9. Do not add new chest/lantern sounds or haptics in the core implementation. If the owner later requests those cues, implement them as a separate feedback commit with manual revalidation required.

---

## Milestone 0: No-Behaviour Semantic and Shader-Maintainability Baseline

**Scope:** Disambiguate the original torch failure from the new reward lantern and create maintainable shader include boundaries without changing ABI, captures, gameplay, or assets.

**Invariants:** Numeric material IDs, custom indices, descriptor bindings, 120-byte push constants, checkpoint IDs/names, fixed timings, event semantics, resource counts, and every pixel remain unchanged.

**Files:**

- Modify `src/gameplay/ShowcaseGameplay.h`.
- Modify `src/gameplay/simulation/GameSimulation.h/.cpp`, `InputSnapshot.h`, and `SimulationSnapshot.h` only for names.
- Modify `src/vulkan/raytracing/SimulationFrameAdapter.cpp`, `PresentableTinyRtScene.h/.cpp`, platform diagnostics, tests, and current docs for terminology.
- Create `shaders/raytracing/include/rt_scene_abi.glsl`, `rt_hit_decode.glsl`, `rt_lighting.glsl`, `rt_dielectric_common.glsl`, and `rt_atmosphere.glsl`; `minimal.rgen` remains the single entry point.
- Modify `tools/compile-raygen.ps1` to resolve and hash explicit include dependencies for staleness checks.

**Tests and steps:**

- [ ] Rename `LanternPhase/Snapshot/Sequence` to `TorchFailurePhase/Snapshot/Sequence`, `lanternStrength` to `torchLightStrength`, and ambiguous members to explicit torch names without changing layout or values.
- [ ] Add source-contract tests that forbid the old ambiguous type names while preserving checkpoint `lantern-drop` as a historical external identifier.
- [ ] Split shader source text into includes with no algorithm change; make the compile script track included files so a stale include fails the existing negative gate.
- [ ] Run focused gameplay/adapter tests, `tools/compile-raygen.ps1`, `tools/compile-raygen.ps1 -Check`, the full Host gate, and `tools/compare-foundation-captures.ps1` against `run-20260826-060353`.
- [ ] Require byte-identical 13 PNG hashes and either byte-identical SPIR-V or a documented nonvisual compiler-line-map difference plus byte-identical captures and identical instruction/branch/loop counts.

**Commits:**

1. `refactor: disambiguate torch failure from reward lantern state`
2. `refactor: split raygen into tracked shader includes`

`Audio/haptic manual revalidation required: NO — names and shader source organization change no listener/source data, cues, event timing, transport, haptic routing, or gameplay feedback.`

**Exit:** Clean Host PASS, bit-exact captures, no asset generation, and no visual/runtime behavior change.

## Milestone 1: Static GLB/PBR Contract and Generic RT Metadata

**Scope:** Add a narrow validated asset reader/compiler and a fixed-capacity generic static RT route, then render the staged sword in a development-only inspection checkpoint.

**Invariants:** Existing world/character paths, custom-index meanings, captures, and resource ownership remain intact until the generic sword proof is explicitly enabled. No staged source asset enters a release package.

**Files:**

- Add `third_party/cgltf/cgltf.h` and its MIT `LICENSE`, pinned to `85cd62382dfea638278962690cf515023f33ed00`.
- Create `src/scene/assets/GltfDocument.h/.cpp`, `AssetManifest.h/.cpp`, `AssetValidation.h/.cpp`, and `StaticMeshAsset.h/.cpp`.
- Create `src/vulkan/raytracing/RtSceneAbi.def`, generated `RtSceneAbi.generated.h`, generated `shaders/raytracing/include/rt_scene_abi.generated.glsl`, `RtStaticMeshSlot.h/.cpp`, and `RtTextureArrays.h/.cpp`.
- Create `tools/prepare-static-rt-asset.ps1` and `tools/generate-rt-scene-abi.ps1`.
- Create `tests/fixtures/static-gltf/` with small valid and negative GLBs/manifests.
- Create `tests/StaticGltfAssetTests.cpp` and `tests/RtSceneAbiTests.cpp`; register `horde_rt_static_gltf_asset_tests` and `horde_rt_scene_abi_tests` in `CMakeLists.txt`.

**Interfaces:**

- `AssetManifest::Load(path) -> validated schema 1 manifest` with metre scale, +Y up/+Z forward, asset/LOD budgets, named sockets, runtime texture profile, and material overrides.
- `StaticMeshAsset::Load(runtimeGlb, manifest) -> vertices, indices, primitive records, node transforms, materials, sockets, bounds, and deterministic diagnostic`.
- `StaticRtVertex`: 64 bytes (`vec4 position`, `vec4 normal`, `vec4 tangent`, `vec4 uv0`).
- `RtInstanceMetadata`: 32 bytes; primitive base/count, stable object ID, flags, emitter index, and asset index.
- `RtPrimitiveMetadata`: 16 bytes; vertex/index offsets, index count, material index.
- `RtMaterialGpu`: aligned vec4/uvec4 fields for base colour, emissive factor/strength, metallic/roughness/occlusion/transmission, IOR/thickness/attenuation, four texture layers, and material flags.
- Initial hard capacities: 20 metadata entries indexed by `instanceCustomIndex`, 8 static assets, 32 primitives, 32 materials, and 16 layers in each base/normal/ORM/emissive array. Overflow is a named initialization failure.

**Tests and steps:**

- [ ] Add RED fixtures for bad GLB magic/version/chunks, out-of-range accessors/indices, unsupported primitive mode, missing normals/UV/tangents, non-finite transforms, negative scale, missing sockets, material/texture/capacity overflow, and unsupported required extensions.
- [ ] Accept core GLB 2.0 triangle primitives, 16/32-bit indices, node TRS/matrix transforms, multiple meshes/primitives, positions/normals/UV0/tangents, material factors/textures, and the four required KHR material extensions. Reject sparse/morph/Draco/meshopt data in the first static path with exact diagnostics.
- [ ] Generate tangents offline only; runtime assets using a normal texture must contain validated tangents.
- [ ] Run the current Khronos glTF Validator in the offline preparation tool, retain its JSON report beside the runtime budget report, and treat unreviewed errors/warnings as asset rejection.
- [ ] Produce stripped runtime GLBs with no source-resolution image payloads and mipmapped KTX2 texture arrays: Android ASTC and Windows RGBA8 KTX2.
- [ ] Generate and test CPU/GLSL size/offset/enum agreement. Append new descriptor bindings after 10; do not repurpose bindings 0-10 or push constants.
- [ ] Build multi-geometry BLAS records so `geometryIndex` selects a primitive/material; do not add triangle-range object branches.
- [ ] Reuse physical sword TLAS slot/custom index 3 for the generic proof. `RtInstanceMetadata[3]` identifies the generic asset; shader branching is by material/object flags, never by `sword`.
- [ ] Add a development-only `pbr-sword-closeup` checkpoint and render the staged LOD through the generic path with source-tree fallback disabled in packaged Release.
- [ ] Record BLAS bytes/build time, vertex/index/material/texture memory, descriptor usage, shader size, Windows capture, Android package exclusion, and a focused `SM-S948B` Debug measurement before promoting the route.

**Commits:**

1. `build: pin cgltf for audited glb parsing`
2. `test: add static glb and asset manifest contract fixtures`
3. `feat: add bounded static glb pbr asset reader`
4. `feat: add generated rt instance and material abi`
5. `feat: render staged sword through generic static rt slot`

`Audio/haptic manual revalidation required: NO — this milestone adds asset parsing, GPU metadata, and a development visual proof without changing gameplay events, listener/source state, audio playback, or haptics.`

**Exit:** Both new CTests pass in Debug/Release, Host gate passes, the staged sword is generic-path-only, source assets remain unshipped, and exact-phone resource/performance evidence is recorded.

## Milestone 2: Shared Held-Item Sockets and Production Sword/Torch

**Scope:** Create one item transform path for sword, original torch, and future reward lantern; promote accepted PBR sword/torch assets and remove their procedural geometry after parity.

**Invariants:** Torch failure timing/trajectory/flame strength, sword combat hit timing, wall clearance, camera limits, one-frame ownership, and platform gameplay authority stay unchanged.

**Files:**

- Create `src/gameplay/items/HeldItemState.h/.cpp`, `HeldLightState.h/.cpp`, and `HeldItemKinematics.h/.cpp`.
- Create `src/vulkan/raytracing/HeldItemRenderSlot.h/.cpp`.
- Create `tests/HeldItemSocketTests.cpp`; register `horde_rt_held_item_socket_tests`.
- Add checked manifests/runtime assets under `assets/models/weapons/runtime/` and `assets/models/props/runtime/` only after acceptance.
- Update `ASSET_LICENSES.md`, `docs/ASSET_PIPELINE.md`, Android/Windows runtime asset allowlists, and packaging guards.

**Interfaces:**

- Named character sockets: `LeftHand`, `RightHand`.
- Prop sockets: `Grip`; torch additionally requires `Flame` and `Light`; reward lantern later requires `GripRing`, `Hinge`, `Flame`, and `Light`.
- Composition: `worldFromItem = worldFromHandSocket * inverse(itemFromGrip)`. Wall retraction moves the shared hand/IK target; it never offsets the prop independently from the hand.
- Parent modes: `HandSocket`, `AuthoredWorldTrajectory`, and `WorldObject`. Torch failure switches once from hand to authored world trajectory at the existing release tick.

**Tests and steps:**

- [ ] Add RED tests for socket-node lookup, transform order, scale rejection, left/right handedness, wall-retraction composition, torch detach continuity, and reset/checkpoint import.
- [ ] Move the current hand target, held depth, grip, torch release, and sword action composition behind the shared state/kinematics/render-slot interfaces before replacing geometry.
- [ ] Validate the staged sword in engine. If licence/account evidence remains unresolved or art/runtime acceptance fails, generate a documented Meshy 7 source candidate and a reviewed Smart Topology/remesh runtime candidate, then texture and revalidate.
- [ ] Treat the staged torch as reference. Generate/retexture a production PBR torch with no flame/glow geometry, process it through Blender and the static asset tool, and preserve exact `Flame`/`Light` socket transforms.
- [ ] Use 1K mobile runtime maps initially; keep 2K/4K source maps out of packages. Record the first Android texture/triangle/BLAS budget before any requested increase.
- [ ] Replace procedural sword/torch geometry only after A/B checkpoints prove grip, wall retraction, shadows/reflections, torch failure, and performance.
- [ ] Add/update deterministic `pbr-torch-fire` with the existing flame representation temporarily; fire quality changes belong to Milestone 3.

**Commits:**

1. `test: add held item socket transform contracts`
2. `feat: centralize held item and wall retraction composition`
3. `assets: add validated pbr sword runtime asset`
4. `assets: add validated pbr torch runtime asset`
5. `refactor: retire procedural sword and torch geometry`

`Audio/haptic manual revalidation required: NO — prop geometry, materials, sockets, and rendering change while attack/impact/drop events, audio source positions, cue timing, transport, and haptic semantics remain unchanged.`

**Exit:** Sword and torch use the same generic asset/socket path, hands/grips stay coherent, the roof-water sequence is unchanged, package/licence gates pass, and exact-phone visual/performance evidence is accepted.

## Milestone 3: Reusable World-Space Fire Emitter

**Scope:** Replace the temporary layered flame proof with a bounded deterministic emitter used by the opening torch and later by the reward lantern.

**Invariants:** The emitter transform drives visible fire, light, reflection contribution, and shadows together. No fullscreen/camera-space effect, primary billboard, frame-random light, or spark BLAS is permitted.

**Files:**

- Create `src/gameplay/effects/FireEmitterState.h/.cpp`.
- Create `src/vulkan/raytracing/FireEmitterBuffer.h/.cpp`.
- Create `shaders/raytracing/include/rt_fire.glsl`.
- Extend generated RT ABI and `RtSceneFrameInputs` append-only.
- Create `tests/FireEmitterTests.cpp`; register `horde_rt_fire_emitter_tests`.
- Extend RT Lab tuning and focused renderer tests.

**Interfaces and budgets:**

- `FireEmitterState`: stable ID, seed, strength/fuel, fixed-step phase, colour temperature/art tint, radius/height/core/smoke parameters, and parent object/socket.
- `FireEmitterGpu`: `worldFromLocal`, colour/intensity, shape, animation, smoke/ember fields, and flags.
- Hard capacity 4 emitters; CPU/zone selection exposes at most 2 active emitters in stable ID order. Opening torch and reward lantern normally make only one active held emitter.
- Same deterministic phase/noise at all quality levels. Mobile/High vary only bounded integration/reflection sample counts.

**Tests and steps:**

- [ ] Add RED tests for fixed-tick phase, reset/import, stable camera/zone selection, tie-breaking by emitter ID, strength-to-zero on torch extinguish, and 30/60/120 render-delivery equivalence.
- [ ] Keep a small static emissive core primitive inside the generic prop BLAS. Instance metadata supplies emitter index; zero strength makes it dark without rebuilding/refitting geometry.
- [ ] Evaluate a tapered oriented world-space volume before the nearest physical hit, with deterministic low-frequency domain noise, bounded absorption/emission, and optional shallow smoke above the flame.
- [ ] Use the same low-pass emitter noise for restrained light flicker. Light position follows the `Light` socket; flame/core position follows `Flame`.
- [ ] Evaluate reflected fire only on the approved bounded reflection routes. Analytic embers are depth-clipped and capped; no particle acceleration structures.
- [ ] Add RT Lab flame strength/turbulence/smoke controls behind authored defaults and record shader size/instruction/branch changes after every fire commit.
- [ ] Inspect `pbr-torch-fire` for parallax, depth occlusion, wet-stone/mirror reflection, correlated light/shadow, deterministic captures, and no cyan BGRA regression.
- [ ] Run matched 75% `SM-S948B` opening/torch-fire evidence and investigate any >15% regression without reducing render scale, water, or effect identity.

**Commits:**

1. `test: add deterministic fire emitter contracts`
2. `feat: add bounded fire emitter gpu records`
3. `feat: render world space rt flame volumes`
4. `feat: tune coherent torch fire and light`

`Audio/haptic manual revalidation required: NO — fire visuals and direct-light evaluation change, but no sound assets, playback, source/listener event data, event timing, transport, or haptic path changes.`

**Exit:** The opening torch is the production fire proof; it remains deterministic, reflected, depth-occluded, and phone-measured through a reusable emitter path.

## Milestone 4: Skinned Player, Animation Layers, IK, and Real Bone Sockets

**Scope:** Replace the procedural player with one original full-body first-person-compatible skinned character while preserving combat authority and the held-item contract.

**Invariants:** Gameplay hit/parry windows remain owned by `SwordCombat`/`GameSimulation`; animation follows authoritative actions. No platform animation logic and no player-driven increase in enemy pose buckets.

**Files:**

- Extract/commonize `src/scene/assets/SkinnedMeshAsset.h/.cpp` from the current `SkeletonBipedModel` seam without changing enemy output.
- Create `src/gameplay/animation/PlayerAnimationState.h/.cpp` and `PlayerIkTargets.h/.cpp`.
- Create `src/vulkan/raytracing/PlayerRenderSlot.h/.cpp`.
- Create `tests/PlayerAnimationTests.cpp`; register `horde_rt_player_animation_tests`.
- Add validated player source/runtime metadata, GLB, textures, clip manifest, and licence record.

**Interfaces:**

- Authoritative `PlayerAnimationSnapshot`: locomotion blend, combat action/time, reaction, lantern pose blend, left/right IK targets, and visibility/mask flags.
- One skinned full body with actual `LeftHand`/`RightHand` bone sockets; primary-ray head/near-face masking is material/primitive metadata, while shadow/reflection participation remains enabled.
- Shared fixed-step `HeldItemKinematics` owns the intended hand/pivot target. Renderer IK solves the rig to that target; held props use final bone/socket transforms and are checked against the authoritative pivot tolerance.

**Tests and steps:**

- [ ] Add RED tests for clip mapping, locomotion/combat layer precedence, attack/parry timing continuity, high/low target continuity, two-bone IK reach/clamp, socket stability, reset/import, and 30/60/120 delivery equivalence.
- [ ] Generate an original Meshy 7 A-pose source under the approved Gothic traveller brief. Review silhouette/topology before texturing; produce a PBR runtime candidate, then rig, validate joints/weights, and retain only required idle/walk clips.
- [ ] Do not rely on a library attack clip for damage timing. Drive upper-body sword/parry and left-arm high/low poses from existing simulation action times plus IK.
- [ ] Run 30 Hz and 60 Hz CPU skin/refit A/B on Windows and `SM-S948B`. Choose 60 Hz if first-person grip quality requires it and the measured cost is accepted; otherwise retain 30 Hz only if motion review finds no visible judder.
- [ ] Keep compute skinning out of this milestone. If neither measured CPU cadence passes the visual/performance gate, stop and propose a separately reviewed compute-skinning backend rather than silently expanding this plan.
- [ ] Use TLAS slot 4 for the new player during A/B. The developer toggle masks old slots 5-16 when the skinned player is active and masks the new route when the procedural player is active; both paths are never simultaneously visible.
- [ ] Validate downward body view, camera/head clipping, torso/legs/boots, shadow/reflection, walk, pitch extremes, attack, parry, wall retraction, torch drop, and hand socket error.
- [ ] Add deterministic `player-body-grips`. Remove procedural body/limb/head geometry, their two BLAS, and instance branches 4-16 only after exact phone and owner motion/art acceptance.

**Commits:**

1. `refactor: generalize skinned character asset interface`
2. `test: add player animation ik and socket contracts`
3. `assets: add validated gothic player runtime asset`
4. `feat: add skinned player render slot and animation layers`
5. `feat: bind held items to player bone sockets`
6. `refactor: retire procedural player geometry`

`Audio/haptic manual revalidation required: NO — player rendering, animation, IK, and sockets change while combat event timing, listener pose/yaw, source coordinates, playback, transport, and haptic feedback remain unchanged.`

**Exit:** A single skinned player owns visible body/hands/sockets, procedural player instances are retired, the TLAS remains capped at 20, and exact-phone motion/performance/lifecycle evidence is accepted.

## Milestone 5: Reusable Bounded Dielectric Transport

**Scope:** Generalize shared Fresnel/terminal-hit/visibility plumbing without destabilizing water, then add closed-volume glass transport for generic materials.

**Invariants:** Current water appearance/cost model remains bounded and separately validated. No copied water coordinates, alpha-only glass, screen refraction, pipeline recursion, or unbounded glass-on-glass loops.

**Files:**

- Create `src/vulkan/raytracing/DielectricMath.h` for host-testable Fresnel, Snell, Beer-Lambert, and interface-stack math.
- Expand `shaders/raytracing/include/rt_dielectric_common.glsl` and create `rt_dielectric_transport.glsl`.
- Extend `RtMaterialGpu`/asset parsing for transmission, IOR, thickness/volume, attenuation, roughness, emissive strength, and thin-wall flags.
- Create `tests/DielectricMathTests.cpp`; register `horde_rt_dielectric_math_tests`.
- Add a small closed/manifold dielectric fixture and RT Lab glass controls.

**Budgets:**

- Mobile: 2 closed volumes / 4 interfaces on primary transmission; one terminal reflected opaque/emissive query; straight bounded shadow transmittance through 4 interfaces.
- High: 4 volumes / 8 interfaces; one terminal reflected query; shadow transmittance through 8 interfaces.
- Reflection does not recursively evaluate another reflective dielectric. Water-on-water and glass-on-glass over budget terminate with a deterministic Debug diagnostic counter and bounded fallback colour.

**Tests and steps:**

- [ ] Add RED host tests for air/glass/air entry-exit, total internal reflection, normal orientation, Schlick endpoints, Snell direction, Beer-Lambert attenuation, thin-wall mode, stack overflow, and finite output under degenerate inputs.
- [ ] Extract common Fresnel and terminal opaque/emissive shading helpers while keeping the existing water solver and captures unchanged.
- [ ] Parse/map `KHR_materials_transmission`, `KHR_materials_volume`, `KHR_materials_ior`, and `KHR_materials_emissive_strength`; reject non-manifold thick-volume assets in the offline validator.
- [ ] Implement iterative ray-query entry/exit transport using actual intersection distance for volume attenuation. Roughness changes bounded reflection/transmission response without switching to screen-space sampling.
- [ ] Add transparent shadow transmittance that respects metal/opaque blockers and finite layer budgets.
- [ ] Validate the same material under no-fire, fire-on, bright skylight, wet-stone reflection, edge Fresnel, and direct through-view conditions.
- [ ] Compile/check raygen, record size/structure, run bit-exact existing water captures where no new glass is visible, then run matched Mobile/High glass checkpoints on phone.

**Commits:**

1. `test: add bounded dielectric transport invariants`
2. `refactor: share rt dielectric and terminal shading helpers`
3. `feat: import gltf dielectric material properties`
4. `feat: add iterative ray query glass transport`
5. `feat: add bounded transparent shadow transmittance`

`Audio/haptic manual revalidation required: NO — this milestone changes material transport and shader visibility only; audio/haptic state, event timing, playback, spatialisation, and feedback semantics are unchanged.`

**Exit:** Water remains accepted, the generic dielectric fixture reads as real glass on both targets, and the phone budget is documented before lantern integration.

## Milestone 6: Gothic Chest and Icon-Faithful Reward Lantern Assets

**Scope:** Produce validated chest and lantern assets through the same static GLB/PBR/material path, with clean rigid parts, sockets, and closed glass.

**Invariants:** No surrounding arch, light beams, baked glow/fire, opaque orange glass, hand, floor, or background enters geometry/textures. Chest is rigid-node/TLAS animation, not skinned.

**Files:**

- Add source/provenance records under `assets/models/props/meshy/`.
- Add accepted runtime GLBs/manifests/KTX2 outputs under `assets/models/props/runtime/` and `assets/textures/props/runtime/`.
- Update `ASSET_LICENSES.md`, asset reports, runtime allowlists, package guards, and deterministic checkpoints.

**Asset organization:**

- Chest: `ChestBase`, `ChestLid` with rear hinge pivot, `Latch`, and `LanternSocket`.
- Lantern: `GripRing`, `Hinge`, `LanternBody`, separate closed/manifold `LanternGlass`, `Flame`, `Light`, and optional simple emissive `FlameCore`.
- Initial target budgets: chest 4k-8k triangles; lantern metal+glass 6k-10k with hard review above 12k; 2-3 materials each; 1K mobile texture profile.

**Tests and steps:**

- [ ] Generate geometry-first Meshy 7/Smart Topology candidates from the approved briefs and reject modern camping silhouettes, arch contamination, fused opaque panes, mushy tracery, and unusable hinge/ring topology.
- [ ] Use headless Blender processing to split/clean rigid parts, establish metre scale/+Y up/+Z forward, set hinge/origin, add or repair simple manifold glass panes, create sockets, validate normals/tangents/UVs, and remove hidden/interior waste.
- [ ] Texture metal/wood under neutral light. Because Meshy 7 does not emit an emission map and ignores `remove_lighting`, inspect base colour directly for baked light and author engine-controlled emissive/core data locally.
- [ ] Require black-silhouette and lit-turntable reviews against the icon: pointed canopy/finial, ring/hinge, and deliberate Gothic tracery must read at game FOV.
- [ ] Render chest base/lid and lantern ring/body as generic scene objects. Reuse freed procedural-player TLAS slots 5-8; retain roof 17, second skeleton 18, and waterfall 19.
- [ ] Add `lantern-chest-unlock` and a development glass/transmission inspection using the production lantern asset.

**Commits:**

1. `assets: add validated gothic chest source and runtime asset`
2. `assets: add validated reward lantern source and runtime asset`
3. `feat: render chest and lantern through generic scene objects`
4. `test: add lantern silhouette glass and socket asset gates`

`Audio/haptic manual revalidation required: NO — asset generation, processing, rendering, and deterministic visual checkpoints add no audio cues, event transport changes, spatial source changes, or haptic behavior.`

**Exit:** Chest and lantern pass the same asset contract as sword/torch, glass is physically separate/closed, silhouette matches the reference family, licences are complete, and both packages contain runtime assets only.

## Milestone 7: Reward Interaction, High/Low Carry, Finale Extraction, and Pendulum

**Scope:** Add shared commands and route-local chest/lantern progression, extract finale sequencing from lich combat, drive the player IK, and simulate responsive lantern motion at fixed 60 Hz.

**Invariants:** Lich combat and death feedback remain unchanged. Rendering never advances reward or motion state. Pause/reset/retry/checkpoint/Home-resume cannot duplicate commands/events or inject velocity.

**Files:**

- Create `src/gameplay/interactions/InteractionState.h/.cpp`, `ChestRewardSequence.h/.cpp`, and `FinaleSequence.h/.cpp`.
- Create `src/gameplay/items/LanternPendulum.h/.cpp`.
- Extend `InputSnapshot`, `InputMailbox`, `SimulationSnapshot`, `GameplayEvent`, `GameSimulation`, checkpoint definitions, adapter/frame inputs, and diagnostics.
- Add Android Java/JNI contextual controls and Windows keyboard/controller bindings.
- Create `tests/LanternRewardTests.cpp` and `tests/LanternPendulumTests.cpp`; register `horde_rt_lantern_reward_tests` and `horde_rt_lantern_pendulum_tests`.

**State and timing:**

- Append `EntityId::RewardChest` and `EntityId::RewardLantern`; append `ChestUnlocked`, `ChestOpened`, and `LanternClaimed` event types after existing values.
- Append monotonic `interact` and `toggleHeldLightPose` command sequences and consumed-sequence diagnostics.
- `HeldLightKind { None, Torch, RewardLantern }` and `HeldLightPose { High, TransitioningToLow, Low, TransitioningToHigh }` live in shared simulation.
- Chest phases: Locked -> ClosedUnlocked -> Opening -> LanternAvailable -> LanternClaimed. Lid opening target is 1.20 s.
- First valid Interact begins opening; a second valid Interact after Open claims once. Pickup transitions to High over 0.65 s and holds the reveal for 1.25 s before starting the existing 4.50 s roof and 1.75 s dawn sequence.
- Interaction uses a stable chest position, 1.35 m maximum range, and a 55-degree facing cone; unavailable edges are consumed without buffering.

**Pendulum interface:**

- `LanternPendulumState`: two hand-local angular displacements, two angular velocities, previous pivot position/velocity, COM length, initialized flag, and deterministic reset/import state.
- Inputs: authoritative left-hand/hinge pivot from `HeldItemKinematics`, hand basis, high/low transition, and fixed `dt`.
- Semi-implicit Euler with gravity restoring torque, angular damping, pivot-acceleration forcing, soft 45-degree cone limits, hard 55-degree safety clamp, bounded dodge/teleport acceleration, and mild front-face torsional alignment.
- Grip ring is hand-rigid; body/frame/glass/flame/light share one pendulum body transform below `Hinge`.

**Tests and steps:**

- [ ] Add RED command tests for mailbox coherence, monotonic sequence deltas, one-edge consumption, unavailable-edge consumption, and direct-vs-mailbox parity.
- [ ] Add RED reward tests for one `LichDefeated`, pre-unlock rejection, single open, single claim, delayed finale, route reset/retry/import, pause, repeated platform ending polls, and RT Lab ownership.
- [ ] Add RED pendulum tests for rest convergence, forward-start backward lag, stop overshoot, strafe/turn response, bounded dodge, damping, soft/hard clamp, teleport reset, pause/Home-resume reset, no NaNs, and equivalent fixed ticks under 30/60/120 Hz render delivery.
- [ ] Move roof/dawn/finale completion clocks into `FinaleSequence`; `LichEncounter` emits defeat state once but no longer owns post-defeat reward timing.
- [ ] Publish `E`/A/contextual `INTERACT` and `F`/Y/contextual `RAISE`/`LOWER` without overloading attack/parry/dodge or menu semantics. Test Android enlarged-font compact layout.
- [ ] Drive left-arm high/low IK from shared pose state. The authoritative pivot moves the hand target; the final bone socket and ring must remain within the accepted grip-error tolerance.
- [ ] Feed the reward lantern fire emitter from the pendulum body transform so frame, glass, flame, light, shadows, and reflections move coherently.
- [ ] Add `lantern-held-high`, `lantern-held-low`, `lantern-glass-transmission`, and `lantern-motion-extreme` checkpoints with explicit imported/frozen pendulum state.
- [ ] Run lifecycle, retry/reset, repeated ending polling, capture/replay, and exact-phone interaction/motion review. No new cue/haptic is added in this core slice.

**Commits:**

1. `test: add reward interaction and command sequence contracts`
2. `feat: add shared interact and held light pose commands`
3. `refactor: extract finale sequence from lich encounter`
4. `feat: add chest unlock open and lantern claim flow`
5. `test: add deterministic lantern pendulum contracts`
6. `feat: add fixed step physical lantern carry motion`
7. `feat: add cross platform lantern interaction controls`
8. `test: extend reward glass and motion checkpoints`

`Audio/haptic manual revalidation required: YES — delaying FinaleCompleted and appending chest/claim semantic events changes gameplay-event timing/traffic even though no new cue or haptic is planned; the exact candidate therefore needs an owner feedback pass. Adding chest/claim/toggle cues remains a separate explicitly reviewed feedback commit.`

**Exit:** Reward sequence, high/low carry, and pendulum are deterministic and platform-shared; ending overlays wait correctly; exact-phone hands-on interaction/motion/lifecycle review passes.

## Milestone 8: Consolidation, Full Evidence, and Owner Candidate

**Scope:** Remove temporary paths, reconcile authority, run the full matrix, and present an exact unpublishable candidate for owner review. Release work remains separately gated.

**Files:**

- Remove A/B toggles and obsolete procedural branches/assets not intentionally retained.
- Update `AGENTS.md`, `PROJECT_DECISIONS.md`, `PROJECT_MEMORY.md`, `README.md`, controls, `docs/ASSET_PIPELINE.md`, validation documentation, and `ASSET_LICENSES.md` only where authority actually changed.
- Add `docs/FIRE_PBR_REWARD_LANTERN_PLAYER_UPGRADE_VALIDATION_2026-08-26.md` with exact source/artifact/evidence identities.
- Update `docs/ANDROID_RT_DEVICE_COMPATIBILITY_RECORD.md` for every new exact-device evidence item.

**Full evidence:**

- [ ] Run shader generation/check/negative gates and record SPIR-V bytes/words/instructions/branches/loops/hash.
- [ ] Run fresh Windows Debug and Release builds and all 21 intended CTests in both configurations.
- [ ] Generate and inspect the original 13 plus 8 new deterministic Windows captures: `pbr-sword-closeup`, `pbr-torch-fire`, `player-body-grips`, `lantern-chest-unlock`, `lantern-held-high`, `lantern-held-low`, `lantern-glass-transmission`, and `lantern-motion-extreme`.
- [ ] Run Android clean Debug, unsigned Release, `lintRelease`, runtime-asset/package/licence/LFS/16 KiB guards, and an isolated packaged Windows launch with no source-tree fallback.
- [ ] Run the portable CI-compatible host subset and update its documented count without presenting it as Vulkan RT or device evidence.
- [ ] Require exactly one authorised ADB target before device mutation; record raw model `SM-S948B`, install exact Debug candidate, pull back `base.apk`, and require byte-for-byte parity.
- [ ] Prove strict ASTC selection, honest RT presentation, the full route/replay/captures, torch extinguish/drop, lich defeat, chest/open/claim, high/low, glass, pendulum, RT Lab ownership, Back/pause, Home/resume/surface recreation, retry, and route reset.
- [ ] Measure the matched existing 75% route plus every new effect checkpoint; report 100% separately. Record median/P95/mean, CPU and GPU RT timing where available, warm-up/window counts, thermal status, GPU thermal power/frequency, battery/AP/SKIN temperature, PSS/RSS/native/graphics memory, threads/resources, BLAS/TLAS/material/texture/emitter counts, and shader identity.
- [ ] Investigate >15% matched regressions, shader/occupancy cliffs, unexplained spikes, memory/resource growth, and workload-identity changes. Preserve quality and resolution while investigating.
- [ ] Conduct hands-on Windows and exact-phone review for art, motion, grip, touch/controller controls, camera comfort, glass readability, and sustained behavior. State evidence boundaries; automation does not prove feel.
- [ ] Run final review, `git diff --check`, secret/large-blob/runtime-source audit, and focused commits. Do not bump version, sign, package a publishable release, upload, or deploy.

**Commits:**

1. `refactor: remove temporary lantern upgrade paths`
2. `test: complete fire pbr lantern and player evidence gates`
3. `docs: reconcile fire lantern player upgrade authority`

`Audio/haptic manual revalidation required: NO — consolidation/evidence work changes no feedback semantics; Milestone 7's required YES owner pass must already cover its changed finale/event timing, and any later feedback-affecting implementation reopens that gate.`

**Exit:** All reusable systems and assets pass; exact Windows/Android evidence and limitations are documented; the owner receives hashes, reports, captures, performance tables, and a final review checklist. Publication remains unperformed.

---

## Planned Test Inventory After Implementation

The current 13 Vulkan-enabled tests remain. Add exactly these eight targets, bringing the intended Debug/Release total to 21:

1. `horde_rt_static_gltf_asset_tests`
2. `horde_rt_scene_abi_tests`
3. `horde_rt_held_item_socket_tests`
4. `horde_rt_fire_emitter_tests`
5. `horde_rt_player_animation_tests`
6. `horde_rt_dielectric_math_tests`
7. `horde_rt_lantern_reward_tests`
8. `horde_rt_lantern_pendulum_tests`

Existing tests are extended where they own integration behavior: simulation/checkpoint/event semantics, mailbox parity, controller mappings, RT Lab/ending ownership, character slots, shader source contracts, package/asset guards, and deterministic captures.

## Approval and Execution Discipline

- Approval is for the sequence and defaults above, not one giant implementation commit.
- Execute one milestone at a time, show its evidence and exact audio/haptic classification, and wait when a milestone exit requires owner art/feel acceptance.
- Meshy generation may begin in Milestone 2 after the generic static asset contract is proven; no separate credit confirmation is required.
- If current `main` advances before implementation, re-run the authority audit and baseline, then update this plan where the merge changes an interface or invariant.
