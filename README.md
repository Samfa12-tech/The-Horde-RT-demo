# The Horde RT Demo / Horde Lantern RT

A public Samfa12 technology demo for proving native Vulkan hardware ray tracing on Android and Windows. The long-term creative target is a historical gothic scene: dark torch-lit tunnels, wet stone, puddles, fog, lanterns, and an eventual ruined corridor/courtyard horde encounter. The first job is not gameplay; it is proving real Vulkan RT support on target hardware.

## RT or nothing

This project is **RT or nothing**.

The app must use native Vulkan hardware ray tracing. It must query the actual Vulkan ray-tracing extensions and features exposed by the device. If RT support is unavailable, it must show a clear unsupported diagnostic state and write a capability report. It must not silently fall back to browser rendering, WebGPU, raster-only lighting, screen-space reflections, baked lighting, compute-only path tracing, or fake RT effects.

## Hardware targets

- Primary: Samsung Galaxy S26 Ultra.
- Secondary / equal target: Windows laptop with RTX 5050.

## Current status

Phase 0A/B/C established the capability probe and native diagnostic shells. The project has now moved into an early Phase 1 phone scene.

Current tested phone build includes:

- A native Vulkan Android app that builds, installs, and launches on Samsung `SM-S948B`.
- A real RT-produced frame path: BLAS/TLAS, RT pipeline/SBT, `vkCmdTraceRaysKHR`, storage image output, and swapchain presentation.
- A first-person gothic corridor/ruin scene with authored triangle geometry.
- Visible handheld medieval flame torch on the left side of the player view.
- Left-side touch movement/strafe and right-side 360 look.
- Ray-query path-tracing style shading: primary TLAS queries, shadow queries, and a first bounce sample.
- Reflective objects, wet-floor/puddle response, fog, horde silhouettes, and second-room sunlight.
- Diagnostics hidden behind the HUD tap instead of being the primary screen.

Phase 1C adds simple corridor/arch collision and a stronger procedural material table for dry stone, wet stone/puddles, mossy stone, old metal, and flame. The RT look removes synthetic grain and time-varying random bounce sampling, reduces cool sky/indirect influence, and places a real low-poly emissive held-torch mesh into a second TLAS instance. Its camera-local flame placement drives the direct-light sample and is visible to reflection rays; the old fullscreen torch overlay is gone. The renderer also compensates for the RGBA-storage-to-BGRA-swapchain raw copy, so the fire stays orange rather than cyan. The complete torch/reflection, collision, and material proof is now visually verified on both the RTX laptop and target phone.

Asset staging update: a Meshy-6 PBR gothic sword has been generated for the future right hand. The staged GLB has 2K maps but also 49,439 triangles, so it is not runtime-integrated; it needs remesh/LOD plus a GLB/PBR import and right-hand attachment path before it can enter the Android RT scene.

Character presentation update (2026-07-12): the camera now reads at approximately 1.53 m above the corridor floor, the skeleton renders unarmed at its authored 1.0 scale, and its coarse checker-like procedural colour variation has been replaced with a smooth bone tone. A lightweight player sword occupies the right side of the first-person view and shares the camera-held prop BLAS with the left-hand torch. The textured source sword has also been reduced to a verified 12,358-triangle LOD, but remains outside the runtime until static GLB/PBR upload is implemented.

Animated-enemy performance update (2026-07-12): the Android loop no longer adds a fixed 16 ms sleep after presentation, animation uses real elapsed time, skeleton skinning/BLAS refit runs at 30 Hz, and unique source vertices are skinned once before RT expansion. The installable debug native library uses `-O2`. A sustained target-phone sample measured 16.667 ms median and 20.833 ms p95 over 126 intervals (60 FPS median), with CPU frame recording reduced to roughly 0.4-0.6 ms. See `docs/SKELETON_PERFORMANCE_2026-07-12.md`.

Imported-material update (2026-07-12): five exact CC0 Poly Haven PBR sets now texture dry stone, wet cobblestone, mossy masonry, damp puddle edges, and aged metal through three five-layer Vulkan arrays. The runtime proof uses 512 x 512 derived layers while retaining the 1K JPG sources. Its phone regression test held 60 FPS median over 126 intervals. See `docs/PBR_MATERIAL_BATCH_2026-07-12.md` and `ASSET_LICENSES.md`.

Lighting refinement (updated 2026-07-13): held props remain excluded from direct torch-shadow occlusion while staying visible to primary/reflection rays. Secondary rays now use geometric-normal millimetre-scale bias, and a tiny hidden outer shell catches numerical misses at the zero-thickness room joins. Four physical roof slabs surround one irregular breach in room two; the aligned visible moon and directional light reach surfaces only when the existing ray query passes through that breach. Moon diffuse now respects albedo, wet/metal surfaces receive a cheap highlight, and the final pass uses filmic tone mapping plus linear-to-sRGB output. No artificial volumetric shaft ships. See `docs/RT_LIGHTING_REFINEMENT_2026-07-12.md` and `docs/RT_LIGHTING_SEAM_FIX_2026-07-13.md`.

Android packaging now uses a Gradle-generated runtime-only asset set. Retained source textures and unused model sources stay in the repo but no longer enter the APK; the debug artifact fell from 93.9 MB to 46.8 MB. The live raw PBR arrays still need a capability-checked GPU-compressed replacement.

A separate Meshy skeleton is live through a narrow animation import path. It has 11 correctly named clips and is a 9,402-triangle skinned prop; `Idle_5` is CPU-skinned and refit into a dynamic RT BLAS. The enemy is intentionally unarmed because the sword belongs to the player view.

Verified desktop interactive run (2026-07-10): NVIDIA GeForce RTX 5050 Laptop GPU reported `RayTracingPipeline` and presented the RT corridor at 984 x 661. Controls are `WASD` to move, left mouse/trackpad click-drag to look, and `Esc` to exit.

Important phone finding:

- Recursive closest-hit secondary tracing with pipeline recursion depth 2 failed on phone at RT pipeline creation.
- The stable mobile path uses `rayQueryEXT` in raygen for secondary visibility/bounce work while keeping RT pipeline recursion depth 1.

The earlier probe foundation includes:

- Windows CLI: `horde_rt_capability_probe`.
- Android native shell (`android/`) that runs the same probe core and displays diagnostics in-app via shared text and a native Vulkan surface.
- Android report persistence to app private storage.
- Windows native diagnostic window: `horde_rt_diagnostic_window` with native Vulkan swapchain/surface diagnostics.
- Real probe now includes a presentable RT scene path on Android and Windows scaffolds.

### Probe feature coverage

- Creates a Vulkan instance.
- Enumerates all physical devices.
- Logs each candidate device and gathers:
  - GPU name, vendor ID, device ID, driver version, Vulkan API version.
  - extension presence for:
    - `VK_KHR_acceleration_structure`
    - `VK_KHR_ray_tracing_pipeline`
    - `VK_KHR_ray_query`
    - `VK_KHR_buffer_device_address`
    - `VK_KHR_deferred_host_operations`
  - feature-chain state for:
    - `VkPhysicalDeviceAccelerationStructureFeaturesKHR`
    - `VkPhysicalDeviceRayTracingPipelineFeaturesKHR`
    - `VkPhysicalDeviceRayQueryFeaturesKHR`
    - `VkPhysicalDeviceBufferDeviceAddressFeatures`
- Evaluates RT mode as `RayTracingPipeline`, `RayQuery`, or `Unsupported`.
- Writes plain text and JSON reports.

## Build and run (Windows interactive RT scene)

```powershell
$cmake = "C:\Program Files (x86)\Microsoft Visual Studio\2022\BuildTools\Common7\IDE\CommonExtensions\Microsoft\CMake\CMake\bin\cmake.exe"
& $cmake --build build --config Debug --target horde_rt_diagnostic_window
.\build\Debug\horde_rt_diagnostic_window.exe
```

Controls: `WASD` move, left mouse/trackpad click-drag looks, `Esc` exits. On unsupported hardware the window keeps its diagnostic view rather than falling back to a fake renderer.

Reports:

- `reports/vulkan_capability_report.txt`
- `reports/vulkan_capability_report.json`

## Build and run (Phase 0B Android activity shell)

```powershell
cd android
./gradlew.bat assembleDebug
```

On Windows/CLI, install, launch, and verify reports:

```powershell
cd android
./gradlew.bat installDebug
adb shell monkey -p com.samfa12.hordelanternrt 1
```

Then collect reports from app private storage:

- `adb shell run-as com.samfa12.hordelanternrt ls files/reports`
- `adb shell run-as com.samfa12.hordelanternrt cat files/reports/vulkan_capability_report.txt`
- `adb shell run-as com.samfa12.hordelanternrt cat files/reports/vulkan_capability_report.json`

Verified on-device run (2026-07-05, Samsung Galaxy S26 Ultra, model `SM-S948B`, manufacturer `samsung`):

- `adb devices` → `R5GL219SZGK	device`
- `adb shell getprop ro.product.model` → `SM-S948B`
- `adb shell getprop ro.product.manufacturer` → `samsung`
- Retrieved both report files successfully from `files/reports`.
- RT mode: `RayTracingPipeline`
- GPU name: `Adreno (TM) 840`
- Vendor ID: `20803`
- Device ID: `1141180977`
- Driver version: `512.842.19`
- Vulkan API version: `1.4.295`
- `VK_KHR_acceleration_structure`: `yes`
- `VK_KHR_ray_tracing_pipeline`: `yes`
- `VK_KHR_ray_query`: `yes`
- `VK_KHR_buffer_device_address`: `yes`
- `VK_KHR_deferred_host_operations`: `yes`
- Features: `accelerationStructure=true`, `rayTracingPipeline=true`, `rayQuery=true`, `bufferDeviceAddress=true`

## Planned build targets

- Android native phone build (`android/` app module).
- Windows native Vulkan build path (capability probe + diagnostic surface swapchain).

## First milestone: Phase 0 capability probe

- Vulkan capability probe with real extension/feature diagnostics.
- JSON/text capability reporting.
- Unsupported output explicitly explains missing requirements.
- No fake renderer fallback.

Phase 0 is complete enough to support Phase 1 scene work. Native Vulkan diagnostic surfaces on Windows and Android now present shared probe results, and Android has a real RT scene path.

## Next milestone: one animated skeleton

Next slice:

- Add a narrow GLB animation/PBR import path for the staged 9,402-triangle skeleton.
- Display and animate exactly one unarmed skeleton on laptop and phone while keeping the sword in the player view.
- Measure its mobile RT cost before adding another enemy, weapon attachment, combat, or environment texture scope.
- Preserve the phone-safe ray-query path-tracing route and the verified Phase 1C baseline unless a stronger RT route is proven on-device.

Phase 1C phone evidence, controls exercised, presentation timing, and screenshots are recorded in `docs/PHASE_1C_PHONE_VALIDATION_2026-07-11.md`.

## First milestone report fields

```text
Backend: Vulkan
RT mode: RayTracingPipeline / RayQuery / Unsupported
GPU name
Vendor ID
Device ID
Driver version
Vulkan API version
VK_KHR_acceleration_structure: yes/no
VK_KHR_ray_tracing_pipeline: yes/no
VK_KHR_ray_query: yes/no
VK_KHR_buffer_device_address: yes/no
VK_KHR_deferred_host_operations: yes/no
Internal render resolution
FPS / frame time
```

## Reference policy

This repo should stay clean. Do not dump a giant third-party engine into the project.

Reference hierarchy:

1. Primary base/reference: KhronosGroup/Vulkan-Samples, especially `samples/extensions/ray_tracing_basic`.
2. Main learning/reference: NVIDIA `nvpro-samples/vk_ray_tracing_tutorial_KHR`.
3. Focused reference snippets: Sascha Willems Vulkan examples.
4. Backup/reference only: Diligent Engine.
5. Deferred/not first base: The Forge and Unreal Engine.

If code is adapted later from permissive sources, preserve license notices and document the source.
