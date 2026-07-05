# The Horde RT Demo / Horde Lantern RT

A public Samfa12 technology demo for proving native Vulkan hardware ray tracing on Android and Windows. The long-term creative target is a historical gothic scene: dark torch-lit tunnels, wet stone, puddles, fog, lanterns, moving shadows, and an eventual ruined courtyard/colosseum horde encounter. The first job is not gameplay; it is proving real Vulkan RT support on target hardware.

## RT or nothing

This project is **RT or nothing**.

The app must use native Vulkan hardware ray tracing. It must query the actual Vulkan ray-tracing extensions and features exposed by the device. If RT support is unavailable, it must show a clear unsupported diagnostic screen and write a capability report. It must not silently fall back to browser rendering, WebGPU, raster-only lighting, screen-space reflections, baked lighting, compute-only path tracing, or fake RT effects.

## Hardware targets

- Primary: Samsung Galaxy S26 Ultra.
- Secondary / equal target: Windows laptop with RTX 5050.

## Current status

Phase 0 repository scaffold is in progress. The repo now defines the documentation, directory layout, and skeletal C++ architecture for the capability probe, but the real Vulkan extension/feature detection is **not implemented yet**.

No claims should be made that RT is working until the app performs real Vulkan physical-device queries and reports the result on screen and in a capability report.

## Planned build targets

- Android-first native Vulkan build path.
- Windows native Vulkan build path.

## Recommended tool checklist

- Windows 11
- Git
- Visual Studio 2022 with Desktop development with C++
- CMake
- Ninja
- Vulkan SDK
- Android Studio
- Android SDK
- Android NDK
- Android CMake package
- RenderDoc
- NVIDIA Nsight Graphics
- Snapdragon Profiler, if useful on the target phone
- Vulkan Hardware Capability Viewer or equivalent Vulkan capability checker

See `docs/INSTALL_CHECKLIST.md` for the setup checklist.

## First milestone: Phase 0 capability probe

The first real milestone is:

- Android and Windows capability probe.
- On-screen diagnostic overlay.
- JSON/text capability report.
- Unsupported-device diagnostic screen.

Phase 0 must report:

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

## Next milestone: minimal torch-lit hardware RT room

After Phase 0 is genuinely complete, the next milestone is a tiny hardware RT scene: a minimal torch-lit room with wet stone/puddle material tests. This must still use native Vulkan RT and must keep the diagnostic overlay/reporting path visible.

## Reference policy

This repo should stay clean. Do not dump a giant third-party engine into the project.

Reference hierarchy:

1. Primary base/reference: KhronosGroup/Vulkan-Samples, especially `samples/extensions/ray_tracing_basic`.
2. Main learning/reference: NVIDIA `nvpro-samples/vk_raytracing_tutorial_KHR`.
3. Focused reference snippets: Sascha Willems Vulkan examples.
4. Backup/reference only: Diligent Engine.
5. Deferred/not first base: The Forge and Unreal Engine.

If code is later adapted from a permissive source, preserve license notices and document the source.

## Repository map

```text
src/app/                  Application shell and high-level Phase 0 flow.
src/platform/android/     Android native Vulkan notes and future glue.
src/platform/windows/     Windows native Vulkan notes and future Win32 entry.
src/vulkan/               Vulkan context, device capability, and report code.
src/vulkan/raytracing/    RT extension/feature requirement evaluation.
src/ui/                   Diagnostic overlay text/data model.
shaders/                  Future raygen/miss/hit/ray-query shaders.
assets/                   Future commercial-safe assets only.
tools/capability_report/  Future helper scripts/tools for capability reports.
```
