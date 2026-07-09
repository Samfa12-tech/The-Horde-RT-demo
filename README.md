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

The current Phase 1C source slice additionally adds simple corridor/arch collision and a stronger procedural material table for dry stone, wet stone/puddles, mossy stone, old metal, and flame. It must be rebuilt and re-tested on the target phone before being counted as a verified phone result.

Asset staging update: a Meshy-6 PBR gothic sword has been generated for the future right hand. The staged GLB has 2K maps but also 49,439 triangles, so it is not runtime-integrated; it needs remesh/LOD plus a GLB/PBR import and right-hand attachment path before it can enter the Android RT scene.

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

## Build and run (Windows probe)

```bash
cmake -S . -B build
cmake --build build --target horde_rt_capability_probe
cmake --build build --target horde_rt_diagnostic_window
```

```bash
./build/horde_rt_capability_probe
./build/horde_rt_diagnostic_window
```

```bash
# Windows
.\build\horde_rt_diagnostic_window.exe
```

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

## Next milestone: close Phase 1C on device

Next slice:

- Build/install the latest collision/material source and confirm the RT swapchain success log on the Galaxy.
- Check that movement respects corridor walls and the low arch posts without trapping the player.
- Remesh the staged 49k-triangle sword to a phone-safe budget before implementing GLB/PBR loading and right-hand attachment.
- Bring in a small number of commercial-safe open-source PBR texture sets, then measure their mobile cost.
- Preferred texture sources: Poly Haven and ambientCG, both checked per asset and recorded in `ASSET_LICENSES.md`.
- Preserve the phone-safe ray-query path-tracing route unless a better RT route is proven on device.

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
