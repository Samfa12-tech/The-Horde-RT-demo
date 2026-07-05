# The Horde RT Demo / Horde Lantern RT

A public Samfa12 technology demo for proving native Vulkan hardware ray tracing on Android and Windows. The long-term creative target is a historical gothic scene: dark torch-lit tunnels, wet stone, puddles, fog, lanterns, and an eventual ruined corridor/courtyard horde encounter. The first job is not gameplay; it is proving real Vulkan RT support on target hardware.

## RT or nothing

This project is **RT or nothing**.

The app must use native Vulkan hardware ray tracing. It must query the actual Vulkan ray-tracing extensions and features exposed by the device. If RT support is unavailable, it must show a clear unsupported diagnostic state and write a capability report. It must not silently fall back to browser rendering, WebGPU, raster-only lighting, screen-space reflections, baked lighting, compute-only path tracing, or fake RT effects.

## Hardware targets

- Primary: Samsung Galaxy S26 Ultra.
- Secondary / equal target: Windows laptop with RTX 5050.

## Current status

Phase 0A real capability probe is implemented for Windows:

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
- Writes a plain text and JSON report.
- Prints diagnostics to console.

## Build and run (Phase 0A Windows executable)

```bash
cmake -S . -B build
cmake --build build --target horde_rt_capability_probe
```

```bash
.\build\horde_rt_capability_probe.exe
```

Reports are written to:

- `reports/vulkan_capability_report.txt`
- `reports/vulkan_capability_report.json`

## Planned build targets

- Android-first native Vulkan build path.
- Windows native Vulkan build path.

## First milestone: Phase 0 capability probe

This slice is the RT or nothing startup proof:

- Vulkan capability probe with real extension/feature diagnostics.
- JSON/text capability reporting.
- Unsupported output that explicitly explains missing requirements.
- No fake renderer fallback.

No claims should be made that RT rendering is complete until this probe is integrated into a native on-screen Android flow.

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
2. Main learning/reference: NVIDIA `nvpro-samples/vk_raytracing_tutorial_KHR`.
3. Focused reference snippets: Sascha Willems Vulkan examples.
4. Backup/reference only: Diligent Engine.
5. Deferred/not first base: The Forge and Unreal Engine.

If code is adapted later from permissive sources, preserve license notices and document the source.

## Repository map

```text
src/app/                  Application shell and high-level flow.
src/platform/android/     Android native Vulkan notes and future glue.
src/platform/windows/     Windows probe executable and future windowed entry.
src/vulkan/               Vulkan context, device capability, and report code.
src/vulkan/raytracing/    RT extension/feature requirement evaluation.
src/ui/                   Diagnostic overlay text/data model.
shaders/                  Future raygen/miss/hit/ray-query shaders.
assets/                   Future commercial-safe assets only.
tools/capability_report/  Future helper scripts/tools for capability reports.
```
