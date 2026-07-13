# Horde Lantern RT - Project Memory

Last updated: 2026-07-13

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
- The current phone scene is a small first-person gothic corridor/ruin scene with authored triangle geometry, torch panels, reflective objects, horde silhouettes, fog, wet-floor response, a visible handheld medieval flame torch, and moonlight through a physical roof breach.
- Controls are mobile FPS style: left side drag walks/strafs, right side drag gives 360 camera look. Pitch remains clamped to avoid flipping.
- The path-tracing experiment now uses `rayQueryEXT` in the raygen shader for primary hits, shadow rays, and one first-bounce sample against the same TLAS/BVH-style acceleration structure.
- A recursive closest-hit path-tracing attempt with `maxPipelineRayRecursionDepth = 2` compiled but failed on phone at pipeline creation, so the phone-safe path is ray-query path tracing inside raygen with recursion depth 1.
- Diagnostics are hidden behind the HUD tap so the app opens as a scene instead of a probe screen.
- Phase 1C is now phone-verified. The visible torch/light stays on the left, corridor and arch collision remains bounded under sustained touch input, and the procedural dry/wet/mossy stone, puddle, aged-metal, and flame materials render distinctly enough for the current proof.
- The RT cleanup removes synthetic grain and temporal random-bounce shimmer, dims the blue sky/cold indirect contribution, and adds a real emissive held-torch mesh as a second TLAS instance. Its camera-local flame mesh is the source used for direct-light sampling and reflection rays; the previous fullscreen torch art is removed. Both laptop and phone captures now show a warm flame and reflected flame silhouette.
- The RGBA RT storage image is raw-copied into common BGRA swapchains. `PresentableTinyRtScene` now passes the presentation format into a shader push constant and swaps red/blue before that copy when required; this fixes the cyan-torch presentation bug without changing the RT light transport.
- A textured Meshy sword is staged at `assets/models/weapons/meshy/gothic_arming_sword_rh_v01.glb`. It has embedded and sidecar 2K PBR maps, but it is not loaded by the renderer, and its 49,439 triangles exceed the Android held-prop RT budget.
- The Windows RT scene now has a verified interactive desktop path on the RTX 5050 laptop: `WASD` movement, left mouse/trackpad click-drag look, and `Esc` exit. It reported `RayTracingPipeline` and successful RT swapchain presentation at 984 x 661 on 2026-07-10.
- A 9,402-triangle Meshy biped with 11 named clips is live at `assets/models/enemies/meshy/skeleton_biped_merged_animations_v01.glb`; `Idle_5` is CPU-skinned and refit into one dynamic RT BLAS each frame.
- The first live enemy presentation pass uses a 1.0x skeleton scale and raises the camera from 0.87 m to about 1.53 m above the corridor floor. The old coarse world-space hash was removed because it read as checkerboard lighting.
- The skeleton is deliberately unarmed. A lightweight procedural player sword is held on the right side of the first-person view and shares the camera-held prop BLAS with the left-hand torch. The staged textured sword was separately reduced from 49,439 to 12,358 triangles as `gothic_arming_sword_rh_lod1.glb`; it is not uploaded until static GLB/PBR support exists.
- The animated-skeleton performance pass on 2026-07-12 removed an unconditional post-present 16 ms sleep, switched animation to elapsed time, updates the skeleton skin/BLAS at 30 Hz, skins 9,854 unique vertices before expanding to 28,206 RT vertices, and compiles the installable Android debug native library with `-O2`.
- Target-phone validation recovered 16.667 ms median and 20.833 ms p95 over 126 SurfaceFlinger intervals (60 FPS median). Warm internal telemetry showed roughly 0.4-0.6 ms CPU recording, with the previous GPU frame/fence wait now dominant. Full evidence: `docs/SKELETON_PERFORMANCE_2026-07-12.md`.
- The first imported environment PBR batch is live: five exact CC0 Poly Haven 1K sources are retained and packed into three 512 x 512 x 5-layer runtime arrays for diffuse, OpenGL normal, and AO/roughness/metal. World-space planar sampling textures dry stone, wet cobblestone, mossy stone, damp ground/puddles, and aged metal.
- The PBR phone regression gate passed at 16.667 ms median and 20.833 ms p95 over 126 intervals (60 FPS median). Evidence and exact assets: `docs/PBR_MATERIAL_BATCH_2026-07-12.md` and `ASSET_LICENSES.md`.
- The first PBR debug APK was 93,855,324 bytes. A filtered Gradle runtime-asset task now packages only the live skeleton and three 5,242,880-byte arrays, reducing the APK to 46,793,811 bytes. GPU compression remains the next asset-size task.
- RT lighting refinement separates held props into TLAS mask `0x02` and world/enemy geometry into `0x01`, so direct torch visibility no longer lets the torch or player sword cast distracting self-shadows. The flame uses a deterministic interleaved two-point area sample without adding another shadow ray per pixel.
- The 2026-07-13 lighting repair seals zero-thickness room joins with an eight-triangle hidden outer shell and launches secondary rays from geometric normals with millimetre-scale bias, removing the cold exterior seams at floor/wall/ceiling joins. Room two now uses four physical roof slabs around one irregular breach; one aligned moon direction drives both the visible disc and the existing ray-query visibility test. Moon diffuse respects albedo, wet/metal surfaces receive a cheap view-dependent highlight, and the final pass uses filmic tone mapping plus linear-to-sRGB output. No fake shaft or additional visibility ray was added. See `docs/RT_LIGHTING_SEAM_FIX_2026-07-13.md`.
- The 2026-07-13 codebase audit removed the disabled legacy raygen blob, abandoned NativeActivity/scaffold paths, and the misleading empty-command-buffer Tiny RT bootstrap. Android and Windows now share one collision helper and one native source manifest; the RT mode predicate requires the ray-query feature used by the actual renderer. The standalone probe leaves `rtScene` as `Not attempted` until a real platform swapchain presents it. Windows probe/window and Android all-ABI builds pass. See `docs/CODEBASE_AUDIT_2026-07-13.md`.
- The first combat/ASTC slice is implemented. The procedural sword has its own BLAS/fourth TLAS instance and swings independently of the torch; one skeleton walks toward the player, attacks, dies to a timed forward hit, and respawns using the exact `Walking`, `Attack`, and `Dead` clips. Android has a `SWING` control; Windows uses right mouse or Space. Android packages 2,494,080 bytes of capability-checked ASTC KTX2 texture payload instead of 15,728,640 raw bytes, while Windows retains the raw fallback. Windows exposure is 0.62 and Android remains 0.92. Full evidence: `docs/COMBAT_ASTC_SLICE_2026-07-13.md`.

## Tested phone results

- Device tested: Samsung `SM-S948B`.
- Android debug builds repeatedly completed with `.\gradlew.bat assembleDebug installDebug --console=plain`.
- Launch command used: `adb shell am start -n com.samfa12.hordelanternrt/.MainActivity`.
- RT-present success log repeatedly observed: `RT frame reached Android swapchain presentation.`
- User confirmed the scene loads, is performant on the phone, manual movement works, and the ray-query path-tracing look is promising.
- Phase 1C closeout on 2026-07-11 rebuilt and installed commit `93e8818`, cold-launched successfully, and again logged `RT frame reached Android swapchain presentation.` at a `1440x2812` RT dispatch.
- A 126-interval SurfaceFlinger sample measured 17.284 ms median, 17.908 ms p95, and approximately 57.9 FPS median. The in-app FPS field remains honestly `N/A` because engine timing instrumentation is not implemented.
- The 2026-07-13 seam/moon repair compiled and presented successfully on Windows RTX, and the Android debug APK rebuilt successfully for all four configured ABIs. The phone was not connected for install/runtime validation; that gate remains open.
- The combat/ASTC build also passed Windows compilation, RTX smoke, all-ABI Android assembly, strict APK asset audit, and the automated combat state test. No phone was connected, so fresh install, ASTC runtime selection, current-shader visual comparison, combat input, RT presentation, and the 126-interval performance gate remain open.
- Scripted phone touch tests exercised full-turn yaw, both pitch limits, forward/back movement, corridor side limits, and repeated arch-area strafe/forward input without a crash, trap, or observed wall tunnelling.
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
- There is one deliberately narrow approach/attack/death/respawn enemy loop and player sword hit test. There is still no horde AI, block, dodge, player-death loop, or audio.
- Current environment and held-prop geometry is hand-authored simple triangles/quads. A procedural player sword is visible in the right hand; the Meshy sword now has a 12,358-triangle LOD but still needs static GLB/PBR import before it can replace the proof.
- Five CC0 PBR material sets are live through ASTC KTX2 arrays on supported Android hardware and raw RGBA fallback on Windows. The new phone path still needs runtime and performance validation.
- The held torch uses a host-written TLAS transform, so Android and Windows intentionally run one frame in flight until the renderer has per-frame TLAS/instance-buffer ownership.

## Completed Phase 1B / early Phase 1C work

1. Real presentable RT path built for Android and Windows scaffolds.
2. Minimal triangle proof replaced with a gothic corridor scene.
3. Android build installed and run on phone with RT frame reaching swapchain presentation.
4. Visible handheld medieval torch added.
5. Manual mobile controls added: left drag movement, right drag 360 look.
6. Reflective objects, puddle response, physical roof-breach moonlight, and shadow/bounce ray-query work added.
7. Recursive path-tracing attempt tested and rejected by phone pipeline creation; ray-query path became the stable route.
8. Phase 1C adds left-hand torch placement, basic corridor/arch collision, and stronger procedural gothic materials/reflections; target-phone validation passed on 2026-07-11.
9. Generated and staged a Meshy-6 PBR right-hand sword. It has 2K PBR maps but needs explicit remesh approval because the delivered 49,439 triangles are too costly for the Android RT target.
10. Added a verified interactive Windows RTX 5050 RT corridor build with keyboard and mouse/trackpad controls.
11. Staged the merged-animation skeleton, verified its 11 clip names, and kept the sword separate.
12. Replaced the fake held-torch overlay/light relationship with a two-BLAS RT scene: static corridor plus a camera-following emissive torch mesh in the TLAS.

## Next-step sequence

1. Fresh-install the combat/ASTC build, require the ASTC selection plus RT-present logs, and visually compare the current ACES/sRGB shader on the phone.
2. Re-run the 126-interval cold phone benchmark and preserve the 50+ FPS median gate.
3. Hands-on tune the existing sword timing, enemy range/readability, Android attack control, and desktop exposure; do not add another enemy yet.
4. Replace the procedural player-right-hand sword proof only after static GLB/PBR upload exists and the 12,358-triangle LOD is measured on phone. Never put the 49k source mesh into Android RT.
5. Keep enemies unarmed unless a later design explicitly calls for weapons.

## Validation breadcrumbs

- Android build/install command: `.\gradlew.bat assembleDebug installDebug --console=plain` from `android/`.
- Launch command: `adb shell am start -n com.samfa12.hordelanternrt/.MainActivity`.
- Useful log filter: `adb logcat -d -s HordeRtProbeBridge AndroidRuntime`.
- Expected RT success log: `RT frame reached Android swapchain presentation.`
- Full Phase 1C phone evidence: `docs/PHASE_1C_PHONE_VALIDATION_2026-07-11.md`.

## Do not regress

- Do not reintroduce fake fallback rendering as a success path.
- Do not make diagnostics the main user-facing screen again unless the device is unsupported.
- Do not mark RT presentation successful before `vkQueuePresentKHR` succeeds.
- Do not add gameplay features that bypass the native Vulkan RT frame producer.
- Do not reintroduce recursive ray pipeline depth on phone unless the device capability and pipeline creation are proven first.
- Do not import assets without license tracking.
