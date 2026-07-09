# Technical Requirements

## Non-negotiable rendering rule

This project is **RT or nothing**.

The renderer must use native Vulkan hardware ray tracing. It must not use browser rendering, Godot/Unreal as the first renderer path, Three.js, Babylon.js, WebGPU, raster-only rendering, screen-space reflections, baked lighting, fake RT effects, or compute-only path tracing as a substitute for the core proof.

## Phase 0A + 0B + diagnostic surface progress

- Real Vulkan probe now runs on Windows and is reused through Android JNI in the same project.
- Creates Vulkan instance and enumerates physical devices.
- Enumerates required extensions.
- Queries feature-chain structs through `vkGetPhysicalDeviceFeatures2`.
- Evaluates `RayTracingPipeline`, `RayQuery`, or `Unsupported`.
- Uses plain-text and JSON report format.
- Android shell target can now display a native Vulkan diagnostic surface and persist the report on-device.

## Phase 1B progress

- Added tiny RT scene skeleton verification in `src/vulkan/raytracing/TinyRtScene.cpp`.
- The skeleton now:
  - validates required RT extensions/features before attempting setup,
  - creates a minimal logical device with RT pipeline/AS feature chain,
  - checks required RT entry points via `vkGetDeviceProcAddr`,
  - creates and submits a minimal command buffer as a dispatch-path sanity check.
- The skeleton is still probe-gated: no surface, no framebuffer presentation, no gameplay.

## Phase 0A required report fields

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

## Vulkan extension interpretation

- `VK_KHR_acceleration_structure` is required.
- `VK_KHR_ray_tracing_pipeline` is the preferred path.
- `VK_KHR_ray_query` is accepted only when it is a genuine hardware-ray-query path.
- `VK_KHR_buffer_device_address` is required for practical RT support.
- `VK_KHR_deferred_host_operations` is required for RT pipeline probing.

## Feature structs to query

For each physical device, this project now queries and evaluates:

- `VkPhysicalDeviceAccelerationStructureFeaturesKHR`
- `VkPhysicalDeviceRayTracingPipelineFeaturesKHR`
- `VkPhysicalDeviceRayQueryFeaturesKHR`
- `VkPhysicalDeviceBufferDeviceAddressFeatures`

## RT mode rules

### RayTracingPipeline

- `VK_KHR_acceleration_structure`
- `VK_KHR_ray_tracing_pipeline`
- `VK_KHR_buffer_device_address`
- `VK_KHR_deferred_host_operations`
- `VkPhysicalDeviceAccelerationStructureFeaturesKHR::accelerationStructure`
- `VkPhysicalDeviceRayTracingPipelineFeaturesKHR::rayTracingPipeline`
- `VkPhysicalDeviceBufferDeviceAddressFeatures::bufferDeviceAddress`

### RayQuery

- `VK_KHR_acceleration_structure`
- `VK_KHR_ray_query`
- `VK_KHR_buffer_device_address`
- `VkPhysicalDeviceAccelerationStructureFeaturesKHR::accelerationStructure`
- `VkPhysicalDeviceRayQueryFeaturesKHR::rayQuery`
- `VkPhysicalDeviceBufferDeviceAddressFeatures::bufferDeviceAddress`

### Unsupported

- Neither mode is fully satisfied by required extensions/features.

## Android implementation status

Phase 0B shell now plus minimal diagnostic surface:

- Android app module under `android/`.
- JNI bridge uses shared C++ probe code from `src/vulkan/*`.
- Native diagnostic surface rendering starts from `SurfaceView` with shared text report overlay.
- On-screen diagnostic TextView remains for detailed probe text.
- Reports under `files/reports` in app private storage.

## Unsupported-device behaviour

When unsupported:

- `backend` still reports Vulkan.
- `RT mode` is `Unsupported`.
- Missing extension/feature diagnostics list exact reasons.
- No fake RT fallback path is used.
