# Horde Lantern RT - Project Memory

Last updated: 2026-07-10

## Project identity

- Working title: Horde Lantern RT.
- Repo purpose: native Vulkan hardware ray tracing game/tech demo.
- Core principle: RT or nothing.
- Primary target: Android phone.
- Secondary/equal target: Windows RTX laptop.

## Locked creative direction

- Historical gothic action scene.
- Dark corridor into ruined courtyard/colosseum direction.
- Wet stone, puddles, fog, torches, lantern light, silhouettes, and horde shapes.
- The lighting and atmosphere matter before deep combat.
- First playable direction should feel like holding a lantern in a dangerous ruin.

## Current technical state

- Android app builds, installs, and launches on connected `SM-S948B`.
- The Android path creates a Vulkan swapchain surface and presents frames from the native renderer.
- The current render path builds BLAS/TLAS, creates an RT pipeline and SBT, dispatches `vkCmdTraceRaysKHR`, renders into a storage image, and copies to the swapchain image.
- Reports set `rtScene.presented = true` only after an RT-produced frame reaches successful swapchain presentation.
- The current phone scene is a small first-person gothic corridor/ruin scene with authored triangle geometry, torch panels, reflective objects, horde silhouettes, fog, wet-floor response, a visible handheld medieval flame torch, and second-room sunlight.
- Controls are mobile FPS style: left side drag walks/strafs, right side drag gives 360 camera look. Pitch remains clamped to avoid flipping.
- The path-tracing experiment now uses `rayQueryEXT` in the raygen shader for primary hits, shadow rays, and one first-bounce sample against the same TLAS/BVH-style acceleration structure.
- A recursive closest-hit path-tracing attempt with `maxPipelineRayRecursionDepth = 2` compiled but failed on phone at pipeline creation, so the phone-safe path is ray-query path tracing inside raygen with recursion depth 1.
- Diagnostics are hidden behind the HUD tap so the app opens as a scene instead of a probe screen.
- The current Phase 1C source moves the visible torch and its light to the left side of the player view, adds basic Android corridor/arch collision, and strengthens the procedural dry/wet/mossy stone, puddle, aged-metal, and flame material table. This source slice still needs an on-device re-test.
- A textured Meshy sword is staged at `assets/models/weapons/meshy/gothic_arming_sword_rh_v01.glb`. It has embedded and sidecar 2K PBR maps, but it is not loaded by the renderer, and its 49,439 triangles exceed the Android held-prop RT budget.
- The Windows RT scene now has a verified interactive desktop path on the RTX 5050 laptop: `WASD` movement, left mouse/trackpad click-drag look, and `Esc` exit. It reported `RayTracingPipeline` and successful RT swapchain presentation at 984 x 661 on 2026-07-10.
- A separate 9,402-triangle Meshy biped with 11 named clips is staged at `assets/models/enemies/meshy/skeleton_biped_merged_animations_v01.glb`. It is not runtime-integrated, and the sword remains separate from its `RightHand` joint.

## Tested phone results

- Device tested: Samsung `SM-S948B`.
- Android debug builds repeatedly completed with `.\gradlew.bat assembleDebug installDebug --console=plain`.
- Launch command used: `adb shell am start -n com.samfa12.hordelanternrt/.MainActivity`.
- RT-present success log repeatedly observed: `RT frame reached Android swapchain presentation.`
- User confirmed the scene loads, is performant on the phone, manual movement works, and the ray-query path-tracing look is promising.
- The failed recursive experiment logged: `Failed to initialise presentable RT scene: Failed to create RT pipeline.`
- Resolution: keep phone path at ray pipeline recursion depth 1 and use ray queries for path-tracing-like secondary visibility/bounce work.

## Important files

- Android UI and touch controls: `android/app/src/main/java/com/samfa12/hordelanternrt/MainActivity.java`.
- JNI/native Android rendering bridge: `android/app/src/main/cpp/android_probe_bridge.cpp`.
- Shared RT scene implementation: `src/vulkan/raytracing/PresentableTinyRtScene.cpp`.
- RT scene interface: `src/vulkan/raytracing/PresentableTinyRtScene.h`.
- Windows diagnostic renderer: `src/platform/windows/DiagnosticWindow.cpp`.
- Shader sources: `shaders/raytracing/minimal.rgen`, `minimal.rmiss`, `minimal.rchit`.
- Embedded raygen shader include: `src/vulkan/raytracing/MinimalRayGenShader.inc`.
- Staged sword metadata/license gate: `assets/models/weapons/meshy/gothic_arming_sword_rh_v01.METADATA.md` and `ASSET_LICENSES.md`.
- Staged skeleton metadata/license gate: `assets/models/enemies/meshy/skeleton_biped_merged_animations_v01.METADATA.md` and `ASSET_LICENSES.md`.
- Capability/report model: `src/vulkan/DeviceCapabilities.h`, `src/vulkan/RtCapabilityReport.cpp`.
- Larger decisions doc: `PROJECT_DECISIONS.md`.

## Known limitations

- This is not yet a full playable game loop.
- Android has basic manual movement, 360 look, corridor bounds, and arch-post collision, but not a full physics/collision system.
- There is no enemy AI, attacks, block, dodge, animation, audio, or asset pipeline integration yet.
- Current runtime geometry is hand-authored simple triangles/quads. A Meshy sword is staged only and needs remesh/LOD plus a GLB/PBR import and right-hand attachment path before it can render.
- Current materials are procedural/stylized placeholders. They prove mood, shadows, reflections, and bounce direction, but are not final PBR textures.
- The visible handheld torch is a shader overlay/light source, not yet real world geometry that casts its own mesh shadow.

## Completed Phase 1B / early Phase 1C work

1. Real presentable RT path built for Android and Windows scaffolds.
2. Minimal triangle proof replaced with a gothic corridor scene.
3. Android build installed and run on phone with RT frame reaching swapchain presentation.
4. Visible handheld medieval torch added.
5. Manual mobile controls added: left drag movement, right drag 360 look.
6. Reflective objects, puddle response, second-room sun shaft, and shadow/bounce ray-query work added.
7. Recursive path-tracing attempt tested and rejected by phone pipeline creation; ray-query path became the stable route.
8. Phase 1C source adds left-hand torch placement, basic corridor/arch collision, and stronger procedural gothic materials/reflections; target-phone validation is pending.
9. Generated and staged a Meshy-6 PBR right-hand sword. It has 2K PBR maps but needs explicit remesh approval because the delivered 49,439 triangles are too costly for the Android RT target.
10. Added a verified interactive Windows RTX 5050 RT corridor build with keyboard and mouse/trackpad controls.
11. Staged the merged-animation skeleton, verified its 11 clip names, and kept the sword separate.

## Next-step sequence: Phase 1C Gothic material and collision proof

1. Build, install, and re-test the new collision/material source on the target phone before adding more scope.
2. Add a narrow GLB animation/PBR import path for the staged skeleton; then animate one enemy while keeping the sword separate.
3. Measure the skeleton's mobile RT cost before scaling enemies; its 9,402 triangles make it a viable first enemy candidate.
4. Decide whether to remesh the staged sword to roughly 10–15k triangles. Do not put the 49k-triangle source mesh into the Android TLAS/BLAS path.
5. Add a right-hand attachment path only after the remeshed weapon is approved; measure the phone RT cost before expanding asset scope.
6. Add open-source commercial-safe PBR environment textures, preferably CC0 from Poly Haven or ambientCG, and record every imported asset/texture in `ASSET_LICENSES.md`.
7. Strengthen gothic castle feeling with moss patches, wet edges, old stone blocks, ruin silhouettes, and a stronger second-room composition.

## Validation breadcrumbs

- Android build/install command: `.\gradlew.bat assembleDebug installDebug --console=plain` from `android/`.
- Launch command: `adb shell am start -n com.samfa12.hordelanternrt/.MainActivity`.
- Useful log filter: `adb logcat -d -s HordeRtProbeBridge AndroidRuntime`.
- Expected RT success log: `RT frame reached Android swapchain presentation.`

## Do not regress

- Do not reintroduce fake fallback rendering as a success path.
- Do not make diagnostics the main user-facing screen again unless the device is unsupported.
- Do not mark RT presentation successful before `vkQueuePresentKHR` succeeds.
- Do not add gameplay features that bypass the native Vulkan RT frame producer.
- Do not reintroduce recursive ray pipeline depth on phone unless the device capability and pipeline creation are proven first.
- Do not import assets without license tracking.
