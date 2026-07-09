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
- Capability/report model: `src/vulkan/DeviceCapabilities.h`, `src/vulkan/RtCapabilityReport.cpp`.
- Larger decisions doc: `PROJECT_DECISIONS.md`.

## Known limitations

- This is not yet a full playable game loop.
- Android has basic manual movement, 360 look, corridor bounds, and arch-post collision, but not a full physics/collision system.
- There is no enemy AI, attacks, block, dodge, animation, audio, or asset pipeline integration yet.
- Current geometry is hand-authored simple triangles/quads, not imported production assets.
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

## Next-step sequence: Phase 1C Gothic material and collision proof

1. Build, install, and re-test the new collision/material source on the target phone before adding more scope.
2. Add open-source commercial-safe PBR textures, preferably CC0 from Poly Haven or ambientCG, and record every imported asset/texture in `ASSET_LICENSES.md`.
3. Bind real texture data only after its mobile performance cost is understood; keep the new procedural material slots as art fallbacks, never rendering fallbacks.
4. Strengthen gothic castle feeling with moss patches, wet edges, old stone blocks, ruin silhouettes, and a stronger second-room composition.
5. After collision/materials feel good, add one horde silhouette/enemy proxy that moves or idles in the corridor.

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
