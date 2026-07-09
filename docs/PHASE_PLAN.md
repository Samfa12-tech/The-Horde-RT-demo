# Phase Plan

## Phase 0 - Vulkan RT capability proof: complete

- Android and Windows use a native Vulkan capability probe with clear unsupported-device diagnostics.
- The target Galaxy S26 Ultra reported the required RT extensions/features and runs the app in `RayTracingPipeline` mode.
- The Android renderer has presented an RT-produced frame through the swapchain. There is no raster or fake-RT success path.

## Phase 1A/1B - presentable hardware RT scene: complete

- The renderer builds BLAS/TLAS, an RT pipeline and SBT, dispatches `vkCmdTraceRaysKHR`, writes a storage image, and copies it to the swapchain.
- The stable phone path retains ray-pipeline recursion depth 1 and performs primary/shadow/first-bounce work through `rayQueryEXT` in raygen.
- The playable visual proof is a first-person gothic corridor with torch light, wet floor response, fog, silhouettes, and a second-room sun shaft.

## Current slice - Phase 1C: collision and gothic material proof

### Implemented in source

- Simple Android player collision now keeps the camera inside the corridor and pushes it away from the two low arch posts.
- The held torch is a low-poly handle/flame BLAS refit as a camera-following second TLAS instance; its emissive flame is the direct-light and reflection source.
- Wall inserts are high-reflectivity deterministic ray-query mirrors, and the shader compensates for the RGBA-storage/BGRA-swapchain raw copy so the orange flame presents with the correct channel order.
- The active raygen shader has a compact procedural material table: dry stone, wet stone/puddles, mossy stone, aged metal/bronze, and flame emissive.
- Wet materials and puddles use stronger reflected torch and bounce-light responses.
- The first RT cleanup removes deliberate grain and time-varying hemisphere jitter and uses a deterministic single ray-query bounce.
- A Meshy-6 PBR right-hand sword is staged as source art with embedded and 2K sidecar maps. It is intentionally not in the runtime because the delivered 49,439 triangles are not phone-safe for the RT acceleration structure.
- The Windows RT scene is an interactive laptop build: `WASD` movement, left mouse/trackpad click-drag look, and `Esc` exit. On 2026-07-10, the RTX 5050 laptop reported `RayTracingPipeline` and a successful RT swapchain presentation at 984 x 661.
- The 9,402-triangle merged-animation skeleton is staged as source art with 11 correctly named clips. It is not loaded by the renderer, and the sword is intentionally separate.

### Still required to close Phase 1C

1. Build, install, and test this source slice on the target phone; confirm collision is comfortable and `RT frame reached Android swapchain presentation.` still appears.
2. Visually validate and tune the real torch mesh and its reflected flame proof on the phone; the RTX laptop proof is complete. Do not reintroduce a detached fake torch light or fullscreen overlay.
3. Add a narrow GLB animation/PBR loader for the staged skeleton and measure one animated enemy on laptop and phone; keep the sword separate.
4. Remesh/LOD the staged sword to a roughly 10–15k-triangle held-prop budget before adding it to the Android RT BLAS/TLAS. This needs a new credit approval.
5. Add a right-hand attachment only after that remesh is accepted; measure the mobile RT cost first.
6. Import a deliberately small set of commercial-safe environment PBR texture sets and record each one in `ASSET_LICENSES.md` before it ships.
7. Add one idle or moving horde proxy after the corridor feels bounded and materially convincing.

## Later milestones

1. Phase 2 - Torch corridor visual prototype expansion
2. Phase 3 - Movement and simple combat shell
3. Phase 4 - Asset upgrade
4. Phase 5 - Playable route
5. Phase 6 - Release package
