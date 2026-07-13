# Phase Plan

## Phase 0 - Vulkan RT capability proof: complete

- Android and Windows use a native Vulkan capability probe with clear unsupported-device diagnostics.
- The target Galaxy S26 Ultra reported the required RT extensions/features and runs the app in `RayTracingPipeline` mode.
- The Android renderer has presented an RT-produced frame through the swapchain. There is no raster or fake-RT success path.

## Phase 1A/1B - presentable hardware RT scene: complete

- The renderer builds BLAS/TLAS, an RT pipeline and SBT, dispatches `vkCmdTraceRaysKHR`, writes a storage image, and copies it to the swapchain.
- The stable phone path retains ray-pipeline recursion depth 1 and performs primary/shadow/first-bounce work through `rayQueryEXT` in raygen.
- The playable visual proof is a first-person gothic corridor with torch light, wet floor response, fog, silhouettes, and moonlight through a physical roof breach.

## Phase 1C - collision and gothic material proof: complete

### Implemented in source

- Simple Android player collision now keeps the camera inside the corridor and pushes it away from the two low arch posts.
- The held torch is a low-poly handle/flame BLAS refit as a camera-following second TLAS instance; its emissive flame is the direct-light and reflection source.
- Wall inserts are high-reflectivity deterministic ray-query mirrors, and the shader compensates for the RGBA-storage/BGRA-swapchain raw copy so the orange flame presents with the correct channel order.
- The active raygen shader has a compact procedural material table: dry stone, wet stone/puddles, mossy stone, aged metal/bronze, and flame emissive.
- Wet materials and puddles use stronger reflected torch and bounce-light responses.
- The first RT cleanup removes deliberate grain and time-varying hemisphere jitter and uses a deterministic single ray-query bounce.
- A Meshy-6 PBR right-hand sword is staged as source art with embedded and 2K sidecar maps. It is intentionally not in the runtime because the delivered 49,439 triangles are not phone-safe for the RT acceleration structure.
- The Windows RT scene is an interactive laptop build: `WASD` movement, left mouse/trackpad click-drag look, and `Esc` exit. On 2026-07-10, the RTX 5050 laptop reported `RayTracingPipeline` and a successful RT swapchain presentation at 984 x 661.
- The 9,402-triangle merged-animation skeleton is loaded, CPU-skinned from `Idle_5`, and refit into a dynamic RT BLAS. It is intentionally unarmed; the sword belongs to the player view.

### Phone closeout - verified 2026-07-11

1. Commit `93e8818` rebuilt, installed, and cold-launched on `SM-S948B`; the fresh run logged `RT frame reached Android swapchain presentation.`
2. Phone captures verify the real warm-orange torch mesh, reflected flame silhouette, wet-floor response, fog, silhouettes, and deeper corridor lighting without a fake overlay.
3. Full-turn yaw, pitch limits, corridor boundaries, and repeated arch-area movement were exercised with Android touch events without a crash, trap, or observed tunnelling.
4. A 126-interval SurfaceFlinger sample measured 17.284 ms median and 17.908 ms p95 at the `1440x2812` RT surface. See `docs/PHASE_1C_PHONE_VALIDATION_2026-07-11.md`.

## Animated skeleton proof - complete

1. Keep the narrow skeleton GLB animation loader bounded to exactly one unarmed animated enemy on laptop and phone.
2. Measure the 9,402-triangle skeleton's mobile RT cost before adding another enemy or gameplay behavior.
3. Preserve the Phase 1C phone baseline and keep imported environment textures, weapon remeshing, attachment, and combat out of this slice.

### Presentation cleanup and player weapon proof - verified 2026-07-12

1. Corrected player/skeleton scale: approximately 1.53 m eye height and 1.0x skeleton scale.
2. Removed the coarse world-space bone hash that appeared as checkerboard lighting.
3. Kept the skeleton unarmed and added a lightweight procedural player sword to the right side of the first-person view; it shares the camera-held prop BLAS with the left-hand torch.
4. Produced and audited `gothic_arming_sword_rh_lod1.glb` at 12,358 triangles; keep it staged until static GLB/PBR upload is ready.
5. Rebuilt, installed, cold-launched, captured, and confirmed `RT frame reached Android swapchain presentation.` on the target Galaxy.
6. The corrected player-sword build sampled 25.000 ms median and 37.500 ms p95 over 45 SurfaceFlinger intervals (approximately 40 FPS). This is a short sanity sample; treat animated skinning/BLAS refit performance as the immediate technical constraint until a longer profile confirms it.
7. Phone composition evidence: `docs/validation/player_sword_phone_2026-07-12.png`.

## Imported PBR texture batch

The one-enemy performance gate closed on 2026-07-12 at 16.667 ms median and 20.833 ms p95 over 126 phone intervals. See `docs/SKELETON_PERFORMANCE_2026-07-12.md`.

1. Add the smallest reusable image upload/sampler/material path required for albedo, normal, roughness, metallic, and AO.
2. Import exact CC0 Poly Haven sets for dry stone, wet stone, moss, puddle/water, and aged metal, capped at 1K for the first Android proof.
3. Record each asset URL, author, license, downloaded resolution, and derived mobile files in `ASSET_LICENSES.md`.
4. Re-run the phone benchmark after each material step and keep a 50+ FPS median regression gate.
5. Measure phone memory and frame pacing before replacing the procedural player sword with its textured 12,358-triangle LOD.

### First PBR batch - complete 2026-07-12

The five-source CC0 Poly Haven batch, Vulkan array upload, world-space mapping, Android staging, license manifest, visual phone proof, and 60 FPS median regression test are complete. See `docs/PBR_MATERIAL_BATCH_2026-07-12.md`.

## Next slice - settle material art direction and compressed mobile assets

1. Tune scale, orientation, wetness, and normal strength from hands-on phone feedback.
2. Replace raw runtime arrays with a supported mobile GPU-compressed format.
3. Measure APK size, device memory, and the 126-interval frame gate again.
4. Keep the full textured sword LOD separate until this environment material path is stable.

### Compressed material and first combat slice - implementation complete 2026-07-13

- Android packages ASTC KTX2 arrays at 6x6 diffuse/ARM and 4x4 normal quality, with strict header and Vulkan format-capability checks. Windows retains RGBA8 raw fallback.
- The procedural sword is an independent held-prop BLAS/TLAS instance and drives a timed one-enemy hit/death/respawn loop.
- The skeleton reader is still narrow: exactly `Idle_5`, `Walking`, `Attack`, and `Dead` at a 30 Hz skin/refit cadence.
- Windows and all-ABI Android builds, APK contents, RTX smoke, and automated combat state transitions pass. Target-phone runtime and the 126-interval performance gate remain open. See `docs/COMBAT_ASTC_SLICE_2026-07-13.md`.

### RT lighting refinement - complete 2026-07-12

- Removed held-prop self-shadow artifacts from torch direct lighting.
- Replaced the perfect point-light shadow sample with an interleaved two-point flame-area sample.
- Added physical roof gaps and genuine directional-light ray-query visibility through them.
- Rejected and removed fake analytic/overlay shafts.
- Deferred true volumetric dust until a participating-media implementation is proven, likely desktop-first.

## Next technical slice

1. Fresh-install and cold-benchmark the exact combat/ASTC build on the target phone; preserve the 50+ FPS median gate.
2. Tune only the existing combat readability/input and Windows exposure from hands-on comparison.
3. Consider true participating-media dust on Windows RTX only after the compressed phone path and combat loop are stable.

The runtime-only Android asset task is complete and reduced the debug APK from 93,855,324 to 46,793,811 bytes.

## Later milestones

1. Phase 2 - Torch corridor visual prototype expansion
2. Phase 3 - Movement and simple combat shell
3. Phase 4 - Asset upgrade
4. Phase 5 - Playable route
5. Phase 6 - Release package
