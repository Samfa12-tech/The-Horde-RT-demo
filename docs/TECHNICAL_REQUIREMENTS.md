# Technical Requirements

## Non-negotiable rendering rule

This project is **RT or nothing**.

The renderer must use native Vulkan hardware ray tracing. It must not use browser rendering, Godot/Unreal as the first renderer path, Three.js, Babylon.js, WebGPU, raster-only rendering, screen-space reflections, baked lighting, fake RT effects, or compute-only path tracing as a substitute for the core proof.

## Phase 0 output fields

Phase 0 must query and report:

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

## Required Vulkan extension interpretation

- `VK_KHR_acceleration_structure` is essential for Vulkan RT.
- `VK_KHR_ray_tracing_pipeline` is the preferred path.
- `VK_KHR_ray_query` is acceptable only if it genuinely uses Vulkan hardware ray traversal and is clearly labelled as `RayQuery` mode.
- `VK_KHR_buffer_device_address` is required for practical acceleration-structure/ray-tracing work.
- `VK_KHR_deferred_host_operations` is required by the ray-tracing pipeline extension path.

## Feature structs to check and enable later

The real implementation must query and enable relevant feature structs through the Vulkan feature chain, including:

```text
VkPhysicalDeviceAccelerationStructureFeaturesKHR
VkPhysicalDeviceRayTracingPipelineFeaturesKHR
VkPhysicalDeviceRayQueryFeaturesKHR
VkPhysicalDeviceBufferDeviceAddressFeatures
```

A device should not be labelled as RT-capable just because extension names exist. The relevant feature fields must also be checked.

## RT mode rules

### RayTracingPipeline

Use when the device exposes the acceleration-structure path and `VK_KHR_ray_tracing_pipeline`, with required features enabled. This is the preferred path for the demo.

### RayQuery

Use only when the device exposes `VK_KHR_acceleration_structure`, `VK_KHR_ray_query`, buffer device address support, and the required feature structs. Label this clearly as `RayQuery` and do not pretend it is the full ray-tracing-pipeline path.

### Unsupported

Use when required Vulkan RT support is unavailable. The app must show a clear unsupported diagnostic screen and write a report. It must not silently fall back to fake RT.

## Unsupported-device behaviour

Unsupported devices must show:

- Backend attempted: Vulkan.
- GPU name, vendor ID, device ID, driver version, and Vulkan API version where available.
- Extension yes/no values.
- Feature yes/no values where available.
- A clear reason that the RT path is unavailable.
- No fake rendered scene pretending to be RT.

## First technical proof

The first successful proof is:

> The Galaxy S26 Ultra launches a native Vulkan app, reports RT capability on screen, writes a capability report, and renders a tiny hardware RT scene.

The tiny scene belongs to the next milestone after the capability probe itself is real.
